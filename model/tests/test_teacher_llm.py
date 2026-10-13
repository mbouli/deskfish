import random
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dataset
import schema
import teacher_llm
import trial


class FakeTeacher:
    """Stands in for Ollama. Gives the replies it was built with, in turn."""

    def __init__(self, replies):
        self.replies = list(replies)
        self.calls = 0
        self.seen_goals = []

    def ask(self, messages, answer_format):
        self.calls += 1
        self.seen_goals.append(answer_format["properties"]["goal"]["enum"])
        reply = self.replies.pop(0) if self.replies else '{"goal": "wander", "mood": "calm"}'
        if isinstance(reply, Exception):
            raise reply
        return reply


def states(count, seed=1):
    rng = random.Random(seed)
    out = []
    while len(out) < count:
        candidate = dataset.sample_state(rng)
        if candidate not in out:
            out.append(candidate)
    return out


class ParseTest(unittest.TestCase):
    def test_good_answers(self):
        goals = ["wander", "rest", "hide"]
        self.assertEqual(teacher_llm.parse_answer('{"goal": "hide", "mood": "wary"}', goals), ("hide", "wary"))
        self.assertEqual(teacher_llm.parse_answer('{\n  "mood": "sulky",\n  "goal": "rest"\n}', goals), ("rest", "sulky"))

    def test_bad_answers(self):
        goals = ["wander", "rest"]
        for reply in ['{"goal": "hide", "mood": "wary"}',        # a real goal, but not on offer
                      '{"goal": "dance", "mood": "calm"}',       # not a goal
                      '{"goal": "rest", "mood": "furious"}',     # not a mood
                      '{"goal": "rest"}',                        # no mood
                      '{"goal": ["rest"], "mood": "calm"}',      # wrong shape
                      '["rest", "calm"]', 'rest calm', '', '{', 'null', '42']:
            self.assertIsNone(teacher_llm.parse_answer(reply, goals), reply)


class LabelTest(unittest.TestCase):
    def test_one_good_answer(self):
        teacher = FakeTeacher(['{"goal": "rest", "mood": "sleepy"}'])
        self.assertEqual(teacher_llm.label_one(states(1)[0], teacher), ("rest", "sleepy"))
        self.assertEqual(teacher.calls, 1)

    def test_only_sensible_goals_are_offered(self):
        teacher = FakeTeacher([])
        quiet = schema.format_state(dict(species="bass", hunger=0, energy=9, stress=0, bold=7, social=2, time="day",
                                         startled="no", doing="wander", food="none", friend="none",
                                         finger="none", cover="none"))
        self.assertEqual(teacher_llm.label_one(quiet, teacher), ("wander", "calm"))
        self.assertEqual(teacher.seen_goals, [["wander", "rest"]])

    def test_bad_answers_are_retried_then_dropped(self):
        state = states(1)[0]
        teacher = FakeTeacher(["nonsense", '{"goal": "dance", "mood": "calm"}', '{"goal": "rest", "mood": "calm"}'])
        self.assertEqual(teacher_llm.label_one(state, teacher), ("rest", "calm"))
        self.assertEqual(teacher.calls, 3)

        teacher = FakeTeacher(["nonsense"] * 5)
        self.assertIsNone(teacher_llm.label_one(state, teacher))
        self.assertEqual(teacher.calls, 3)  # three tries, no more

    def test_label_many_writes_as_it_goes(self):
        todo = states(6)
        teacher = FakeTeacher(['{"goal": "rest", "mood": "calm"}'] * 2 + ["bad"] * 3)  # the third is dropped
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "labels.txt"
            summary = teacher_llm.label_many(todo, path, teacher)
            written = dataset.read_examples(path)
        self.assertEqual(summary["labelled"], 5)
        self.assertEqual(summary["dropped"], 1)
        self.assertEqual(summary["skipped"], 0)
        self.assertEqual([s for s, _, _ in written], todo[:2] + todo[3:])
        self.assertEqual(written[0][1:], ("rest", "calm"))

    def test_resume_skips_what_is_done(self):
        todo = states(5)
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "labels.txt"
            teacher_llm.label_many(todo[:3], path, FakeTeacher(['{"goal": "rest", "mood": "sulky"}'] * 3))
            again = FakeTeacher([])  # would answer "wander calm" if asked
            summary = teacher_llm.label_many(todo, path, again)
            written = dataset.read_examples(path)
        self.assertEqual(again.calls, 2)  # only the two that were missing
        self.assertEqual(summary["skipped"], 3)
        self.assertEqual(summary["labelled"], 2)
        self.assertEqual(len(written), 5)
        self.assertEqual(len({s for s, _, _ in written}), 5)  # no situation twice
        self.assertEqual(written[0][1:], ("rest", "sulky"))   # the earlier answers were kept

    def test_a_failing_teacher_keeps_what_it_has(self):
        todo = states(5)
        teacher = FakeTeacher(['{"goal": "rest", "mood": "calm"}'] * 2 + [teacher_llm.TeacherError("Ollama is not answering")])
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "labels.txt"
            with self.assertRaises(teacher_llm.TeacherError) as caught:
                teacher_llm.label_many(todo, path, teacher)
            self.assertIn("Ollama", str(caught.exception))
            self.assertEqual(len(dataset.read_examples(path)), 2)  # already safely on disk

    def test_real_teacher_reports_no_server_clearly(self):
        nobody_home = teacher_llm.OllamaTeacher(host="http://127.0.0.1:9", timeout=2)
        with self.assertRaises(teacher_llm.TeacherError) as caught:
            nobody_home.ask([{"role": "user", "content": "hello"}], {"type": "object"})
        self.assertIn("127.0.0.1:9", str(caught.exception))


class TrialTest(unittest.TestCase):
    def test_pick_states(self):
        picked = trial.pick_states(70, seed=3)
        self.assertEqual(len(picked), 70)
        self.assertEqual(len(set(picked)), 70)
        by_species = {}
        for state in picked:
            species = schema.parse_state(state)["species"]
            by_species[species] = by_species.get(species, 0) + 1
        self.assertEqual(set(by_species), set(schema.SPECIES))
        self.assertEqual(set(by_species.values()), {10})  # the same number of each
        self.assertEqual(trial.pick_states(70, seed=3), picked)
        self.assertNotEqual(trial.pick_states(70, seed=4), picked)

    def test_agreement(self):
        a = [("s1", "rest", "calm"), ("s2", "hide", "wary"), ("s3", "flee", "wary"), ("s4", "rest", "calm")]
        b = [("s1", "rest", "sleepy"), ("s2", "hide", "wary"), ("s3", "hide", "wary"), ("s5", "rest", "calm")]
        result = trial.self_agreement(a, b)
        self.assertEqual(result["count"], 3)  # only situations answered both times
        self.assertAlmostEqual(result["goal"], 2 / 3)
        self.assertAlmostEqual(result["mood"], 2 / 3)
        self.assertAlmostEqual(result["both"], 1 / 3)
        self.assertEqual(trial.self_agreement([], [])["count"], 0)


if __name__ == "__main__":
    unittest.main()
