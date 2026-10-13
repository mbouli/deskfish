import sys
import unittest
from pathlib import Path

import torch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import schema
import student
from tokenizer import Tokenizer

STATE = ("species clownfish hunger 6 energy 7 stress 0 bold 8 social 7 time day startled no "
         "doing wander food near friend mid finger none cover far")


class ConstrainedAnswer(unittest.TestCase):
    def test_an_untrained_model_still_gives_a_goal_and_a_mood(self):
        tok = Tokenizer()
        for seed in range(20):
            torch.manual_seed(seed)
            model = student.Student(student.Config(**student.SIZES["xs"]))
            goal, mood = student.constrained_answer(model, tok, STATE)
            self.assertIn(goal, schema.GOALS)
            self.assertIn(mood, schema.MOODS)

    def test_it_matches_the_free_answer_when_that_is_valid(self):
        tok = Tokenizer()
        checkpoint = schema.MODEL_DIR / "out" / "teacher_s.pt"
        if not checkpoint.exists():
            self.skipTest("no trained student here")
        model, _ = student.load(checkpoint)
        free = student.answer(model, tok, STATE)
        self.assertIn(free[0], schema.GOALS)
        self.assertIn(free[1], schema.MOODS)
        self.assertEqual(student.constrained_answer(model, tok, STATE), free)


if __name__ == "__main__":
    unittest.main()
