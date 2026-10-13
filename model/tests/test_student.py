import sys
import tempfile
import unittest
from pathlib import Path

import torch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import schema
import student
from tokenizer import Tokenizer

EXAMPLE = ("species clownfish hunger 6 energy 7 stress 0 bold 8 social 7 time day startled no "
           "doing wander food near friend mid finger none cover far")


def small():
    torch.manual_seed(0)
    return student.Student(student.Config(vocab_size=54, block_size=32, n_layer=2, n_head=2, n_embd=16))


class StudentTest(unittest.TestCase):
    def test_output_shape(self):
        model = small()
        tokens = torch.randint(0, 54, (5, 30))
        self.assertEqual(model(tokens).shape, (5, 30, 54))
        self.assertEqual(model(tokens[:, :1]).shape, (5, 1, 54))
        with self.assertRaises(ValueError):
            model(torch.randint(0, 54, (1, 33)))  # longer than the model can hold

    def test_it_cannot_see_the_future(self):
        model = small().eval()
        a = torch.randint(0, 54, (1, 20))
        b = a.clone()
        b[0, 12:] = torch.randint(0, 54, (8,))  # change everything from position 12 on
        with torch.no_grad():
            out_a, out_b = model(a), model(b)
        self.assertTrue(torch.allclose(out_a[0, :12], out_b[0, :12], atol=1e-6))
        self.assertFalse(torch.allclose(out_a[0, 12:], out_b[0, 12:], atol=1e-6))

    def test_word_order_matters(self):
        model = small().eval()
        a = torch.tensor([[5, 6, 7, 8]])
        b = torch.tensor([[6, 5, 7, 8]])
        with torch.no_grad():
            self.assertFalse(torch.allclose(model(a)[0, -1], model(b)[0, -1], atol=1e-6))

    def test_sizes_grow(self):
        counts = [student.count_parameters(student.Student(student.Config(vocab_size=54, **student.SIZES[name])))
                  for name in ["xs", "s", "m", "l", "xl"]]
        self.assertEqual(counts, sorted(counts))
        self.assertLess(counts[0], 10_000)
        self.assertGreater(counts[-1], 500_000)
        self.assertLess(counts[-1], 1_500_000)
        for name, size in student.SIZES.items():
            self.assertEqual(size["n_embd"] % size["n_head"], 0, name)

    def test_bad_configs_are_refused(self):
        with self.assertRaises(ValueError):
            student.Student(student.Config(vocab_size=54, n_layer=1, n_head=3, n_embd=16))

    def test_answer_is_two_words(self):
        model = small().eval()
        goal, mood = student.answer(model, Tokenizer(), EXAMPLE)
        self.assertIn(goal, Tokenizer().words)
        self.assertIn(mood, Tokenizer().words)
        self.assertEqual(student.answer(model, Tokenizer(), EXAMPLE), (goal, mood))  # no randomness

    def test_save_and_load(self):
        model = small().eval()
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "model.pt"
            student.save(model, path, {"note": "hello", "schema": schema.SCHEMA_VERSION})
            loaded, extra = student.load(path)
        self.assertEqual(extra["note"], "hello")
        tokens = torch.randint(0, 54, (3, 30))
        with torch.no_grad():
            self.assertTrue(torch.equal(model(tokens), loaded(tokens)))


if __name__ == "__main__":
    unittest.main()
