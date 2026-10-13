import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dataset
import schema
import teacher_rules

ROOT = Path(__file__).resolve().parent.parent.parent
SIM = ROOT / "sim" / "deskfish_sim"

CALM = dict(species="angelfish", hunger=0, energy=9, stress=0, bold=5, social=0, time="day",
            startled="no", doing="wander", food="none", friend="none", finger="none", cover="none")


def choose(**changes):
    return teacher_rules.choose({**CALM, **changes})


class RulesTest(unittest.TestCase):
    """The same cases as tests/test_brain.c, so the two cannot drift apart unnoticed."""

    def test_default_is_wander(self):
        self.assertEqual(choose(), "wander")

    def test_flee(self):
        self.assertEqual(choose(finger="near", bold=3), "flee")
        self.assertNotEqual(choose(finger="near", bold=4), "flee")
        self.assertEqual(choose(finger="near", bold=9, startled="yes"), "flee")
        self.assertNotEqual(choose(finger="mid", bold=9, startled="yes"), "flee")

    def test_hide(self):
        self.assertEqual(choose(stress=6, cover="far"), "hide")
        self.assertNotEqual(choose(stress=5, cover="far"), "hide")
        self.assertNotEqual(choose(stress=9, cover="none"), "hide")

    def test_seek_food(self):
        self.assertEqual(choose(hunger=5, food="far"), "seek_food")
        self.assertNotEqual(choose(hunger=4, food="far"), "seek_food")
        self.assertNotEqual(choose(hunger=9, food="none"), "seek_food")

    def test_investigate(self):
        self.assertEqual(choose(finger="far", bold=6, stress=4), "investigate")
        self.assertNotEqual(choose(finger="far", bold=5, stress=4), "investigate")
        self.assertNotEqual(choose(finger="far", bold=6, stress=5), "investigate")
        self.assertNotEqual(choose(finger="none", bold=6, stress=0), "investigate")

    def test_rest(self):
        self.assertEqual(choose(time="night"), "rest")
        self.assertEqual(choose(energy=2), "rest")
        self.assertNotEqual(choose(energy=3), "rest")
        self.assertEqual(choose(doing="rest", energy=5), "rest")
        self.assertNotEqual(choose(doing="rest", energy=6), "rest")

    def test_school(self):
        self.assertEqual(choose(social=5, friend="far"), "school")
        self.assertEqual(choose(social=5, friend="mid"), "school")
        self.assertNotEqual(choose(social=5, friend="near"), "school")
        self.assertNotEqual(choose(social=4, friend="far"), "school")
        self.assertNotEqual(choose(social=9, friend="none"), "school")

    def test_priority_order(self):
        everything = dict(startled="yes", finger="near", bold=8, stress=7, cover="near", hunger=9,
                          food="near", time="night", social=9, friend="far")
        self.assertEqual(choose(**everything), "flee")
        everything["startled"] = "no"
        self.assertEqual(choose(**everything), "hide")
        everything["stress"] = 4
        self.assertEqual(choose(**everything), "seek_food")
        everything["food"] = "none"
        self.assertEqual(choose(**everything), "investigate")
        everything["finger"] = "none"
        self.assertEqual(choose(**everything), "rest")
        everything["time"] = "day"
        self.assertEqual(choose(**everything), "school")
        everything["friend"] = "none"
        self.assertEqual(choose(**everything), "wander")

    def test_label(self):
        state = schema.format_state({**CALM, "time": "night"})
        self.assertEqual(teacher_rules.label(state), ("rest", "calm"))

    @unittest.skipUnless(SIM.exists(), "build the simulator first: make -C sim")
    def test_agrees_with_the_c_rules_on_real_traces(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "traces.txt"
            subprocess.run([str(SIM), "--headless", "--traces", str(path), "--runs", "6", "--ticks", "3000",
                            "--seed", "11"], check=True, capture_output=True)
            examples = dataset.read_examples(path)  # every line parses and tokenizes
        self.assertGreater(len(examples), 3000)
        self.assertGreater(len({schema.parse_state(s)["species"] for s, _, _ in examples}), 5)
        self.assertGreaterEqual(len({goal for _, goal, _ in examples}), 5)
        wrong = [(s, goal) for s, goal, _ in examples if teacher_rules.label(s)[0] != goal]
        self.assertEqual(wrong, [])
        self.assertTrue(all(mood == "calm" for _, _, mood in examples))
        # Traits stay within one of the species' usual values, as the schema promises.
        for state, _, _ in examples:
            fields = schema.parse_state(state)
            bold, social = schema.SPECIES[fields["species"]]
            self.assertLessEqual(abs(fields["bold"] - bold), 1)
            self.assertLessEqual(abs(fields["social"] - social), 1)


if __name__ == "__main__":
    unittest.main()
