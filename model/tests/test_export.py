import random
import sys
import tempfile
import unittest
from pathlib import Path

import torch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dataset
import evaluate
import export
import student
import teacher_rules
import train
from tokenizer import Tokenizer


def trained():
    rng = random.Random(1)
    examples = [(s, *teacher_rules.label(s)) for s in (dataset.sample_state(rng) for _ in range(1500))]
    tok = Tokenizer()
    tokens = train.to_tensor(examples, tok)
    model, _ = train.train(tokens, tokens, student.Config(vocab_size=tok.size, **student.SIZES["s"]),
                           epochs=25, batch_size=64, lr=3e-3, seed=0, device="cpu", log=None)
    return model, tok, examples


class ExportTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model, cls.tok, cls.examples = trained()

    def test_round_trip_8_bit(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "model.bin"
            export.write(self.model, self.tok, path, bits=8)
            size = path.stat().st_size
            loaded, words = export.read(path)
            again, _ = export.read(path)
        self.assertEqual(words, self.tok.words)
        self.assertEqual(loaded.config, self.model.config)
        # About one byte per number, plus a small header.
        numbers = student.count_parameters(self.model)
        self.assertLess(size, numbers + 2000)
        self.assertGreater(size, numbers)
        # Every number comes back within half a step of where it started.
        for name in export.tensor_names(self.model):
            original, restored = self.model.state_dict()[name], loaded.state_dict()[name]
            step = original.abs().max().item() / 127
            self.assertLessEqual((original - restored).abs().max().item(), step * 0.51 + 1e-9, name)
        # Reading the same file twice gives the same model.
        tokens = torch.randint(0, 54, (4, 30))
        with torch.no_grad():
            self.assertTrue(torch.equal(loaded(tokens), again(tokens)))

    def test_shrinking_costs_little(self):
        before = evaluate.evaluate(self.model, self.tok, self.examples)
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "model.bin"
            export.write(self.model, self.tok, path, bits=8)
            eight, _ = export.read(path)
            size8 = path.stat().st_size
            export.write(self.model, self.tok, path, bits=4)
            four, _ = export.read(path)
            size4 = path.stat().st_size
        after8 = evaluate.evaluate(eight, self.tok, self.examples)
        after4 = evaluate.evaluate(four, self.tok, self.examples)
        self.assertGreater(before["goal"], 0.95)
        self.assertGreater(after8["goal"], before["goal"] - 0.01)  # 8 bits: under a point lost
        self.assertEqual(after8["valid"], 1.0)
        self.assertLess(size4, size8 * 0.6)                        # 4 bits: about half the size again
        self.assertGreater(after4["goal"], 0.5)                    # and it still mostly works

    def test_tensor_order_is_fixed(self):
        names = export.tensor_names(self.model)
        self.assertEqual(names[0], "token_embedding.weight")
        self.assertEqual(names[1], "position_embedding.weight")
        self.assertEqual(names[-1], "to_vocab.weight")
        self.assertFalse(any("allowed" in name for name in names))  # the look-back mask is not a learned number
        self.assertEqual(sum(self.model.state_dict()[n].numel() for n in names),
                         student.count_parameters(self.model))
        other = student.Student(self.model.config)
        self.assertEqual(export.tensor_names(other), names)

    def test_bad_arguments(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(ValueError):
                export.write(self.model, self.tok, Path(folder) / "m.bin", bits=16)

    def test_bad_files_are_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "model.bin"
            export.write(self.model, self.tok, path, bits=8)
            good = path.read_bytes()

            for what, data in {
                "not ours": b"NOPE" + good[4:],
                "cut short in the header": good[:10],
                "cut short in the vocabulary": good[:40],
                "cut short in the numbers": good[:len(good) // 2],
                "one byte missing": good[:-1],
                "extra bytes on the end": good + b"\x00\x00",
                "empty": b"",
            }.items():
                path.write_bytes(data)
                with self.assertRaises(export.ExportError, msg=what):
                    export.read(path)

            # A file for a different schema version is refused, not misread.
            wrong_version = bytearray(good)
            wrong_version[4] = 99
            path.write_bytes(bytes(wrong_version))
            with self.assertRaises(export.ExportError) as caught:
                export.read(path)
            self.assertIn("99", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
