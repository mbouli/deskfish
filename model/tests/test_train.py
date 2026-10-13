import random
import sys
import unittest
from pathlib import Path

import torch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dataset
import evaluate
import student
import teacher_rules
import train
from tokenizer import Tokenizer


def rule_examples(count, seed):
    rng = random.Random(seed)
    states = [dataset.sample_state(rng) for _ in range(count)]
    return [(state, *teacher_rules.label(state)) for state in states]


class TrainTest(unittest.TestCase):
    def setUp(self):
        self.tok = Tokenizer()

    def test_tensor_layout(self):
        examples = rule_examples(8, 1)
        tokens = train.to_tensor(examples, self.tok)
        self.assertEqual(tokens.shape, (8, 31))
        self.assertTrue((tokens[:, 0] == self.tok.bos).all())
        self.assertTrue((tokens[:, 27] == self.tok.sep).all())
        self.assertTrue((tokens[:, 30] == self.tok.eos).all())
        self.assertEqual(train.ANSWER_AT, 27)
        for row, (_, goal, mood) in zip(tokens, examples):
            self.assertEqual(self.tok.decode(row[28:30].tolist()), f"{goal} {mood}")

    def test_only_the_answer_is_scored(self):
        torch.manual_seed(0)
        logits = torch.randn(4, 30, 54)
        targets = torch.randint(0, 54, (4, 30))
        loss = train.answer_loss(logits, targets)
        changed = logits.clone()
        changed[:, :27] += 5.0   # everything before the answer
        changed[:, 29:] -= 5.0   # and after it
        self.assertAlmostEqual(train.answer_loss(changed, targets).item(), loss.item(), places=5)
        wrong = targets.clone()
        wrong[:, 5] = (wrong[:, 5] + 1) % 54
        self.assertAlmostEqual(train.answer_loss(logits, wrong).item(), loss.item(), places=5)
        wrong = targets.clone()
        wrong[:, 27] = (wrong[:, 27] + 1) % 54  # the goal
        self.assertNotAlmostEqual(train.answer_loss(logits, wrong).item(), loss.item(), places=3)

    def test_it_can_memorise_ten_examples(self):
        """If the model cannot even memorise ten examples, the training loop is broken."""
        examples = rule_examples(10, 2)
        tokens = train.to_tensor(examples, self.tok)
        config = student.Config(vocab_size=self.tok.size, **student.SIZES["s"])
        model, history = train.train(tokens, tokens, config, epochs=200, batch_size=10, lr=3e-3, seed=0,
                                     device="cpu", log=None)
        self.assertLess(history[-1]["loss"], history[0]["loss"] / 20)
        self.assertEqual(train.agreement(model, tokens)["both"], 1.0)

    def test_it_learns_something_general(self):
        """A few thousand examples, a couple of minutes at most: well above guessing on unseen ones."""
        parts = dataset.split(rule_examples(4000, 3))
        config = student.Config(vocab_size=self.tok.size, **student.SIZES["s"])
        model, _ = train.train(train.to_tensor(parts["train"], self.tok), train.to_tensor(parts["val"], self.tok),
                               config, epochs=12, batch_size=64, lr=3e-3, seed=0, device="cpu", log=None)
        results = evaluate.evaluate(model, self.tok, parts["test"])
        self.assertEqual(results["count"], len(parts["test"]))
        self.assertGreater(results["goal"], 0.85)
        self.assertGreater(results["mood"], 0.99)  # always calm: easy
        self.assertEqual(results["valid"], 1.0)

    def test_training_is_repeatable(self):
        tokens = train.to_tensor(rule_examples(200, 4), self.tok)
        config = student.Config(vocab_size=self.tok.size, **student.SIZES["xs"])
        runs = [train.train(tokens, tokens, config, epochs=2, batch_size=32, lr=3e-3, seed=5, device="cpu", log=None)
                for _ in range(2)]
        self.assertEqual(runs[0][1], runs[1][1])

    def test_invalid_answers_are_counted(self):
        """A model that writes nonsense must be scored as wrong, not crash the scorer."""
        torch.manual_seed(1)
        untrained = student.Student(student.Config(vocab_size=self.tok.size, **student.SIZES["xs"])).eval()
        examples = rule_examples(300, 6)
        results = evaluate.evaluate(untrained, self.tok, examples)
        self.assertLess(results["valid"], 0.9)
        self.assertLessEqual(results["both"], results["goal"])
        self.assertLessEqual(results["both"], results["mood"])
        self.assertLessEqual(results["goal"], results["valid"] + 1e-9)
        self.assertEqual(sum(sum(row.values()) for row in results["confusion"].values()), 300)
        text = evaluate.report(results)
        self.assertIn("goal", text)
        self.assertIn("invalid", text)

    def test_confusion_table(self):
        examples = rule_examples(500, 7)
        model, _ = train.train(train.to_tensor(examples, self.tok), train.to_tensor(examples, self.tok),
                               student.Config(vocab_size=self.tok.size, **student.SIZES["s"]),
                               epochs=30, batch_size=50, lr=3e-3, seed=0, device="cpu", log=None)
        results = evaluate.evaluate(model, self.tok, examples)
        diagonal = sum(results["confusion"][goal].get(goal, 0) for goal in results["confusion"])
        self.assertAlmostEqual(diagonal / 500, results["goal"], places=6)


if __name__ == "__main__":
    unittest.main()
