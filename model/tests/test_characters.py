import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import characters
import schema

QUIET = dict(species="catfish", hunger=1, energy=8, stress=0, bold=4, social=3, time="day",
             startled="no", doing="wander", food="none", friend="none", finger="none", cover="none")


def state(**changes):
    return schema.format_state({**QUIET, **changes})


class CharactersTest(unittest.TestCase):
    def test_every_species_has_a_sheet(self):
        self.assertEqual(set(characters.CHARACTERS), set(schema.SPECIES))
        for species, sheet in characters.CHARACTERS.items():
            self.assertIn(species, sheet)
            self.assertGreater(len(sheet), 150, species)
        self.assertEqual(set(characters.GOAL_MEANINGS), set(schema.GOALS))
        self.assertEqual(set(characters.MOOD_MEANINGS), set(schema.MOODS))
        # No two fish are described the same way.
        self.assertEqual(len(set(characters.CHARACTERS.values())), 7)

    def test_available_goals(self):
        self.assertEqual(characters.available_goals(QUIET), ["wander", "rest"])
        self.assertEqual(characters.available_goals({**QUIET, "food": "far"}), ["wander", "seek_food", "rest"])
        self.assertEqual(characters.available_goals({**QUIET, "friend": "near"}), ["wander", "rest", "school"])
        self.assertEqual(characters.available_goals({**QUIET, "finger": "mid"}),
                         ["wander", "rest", "investigate", "flee"])
        self.assertEqual(characters.available_goals({**QUIET, "cover": "far"}), ["wander", "rest", "hide"])
        everything = {**QUIET, "food": "near", "friend": "mid", "finger": "near", "cover": "far"}
        self.assertEqual(characters.available_goals(everything), schema.GOALS)  # all seven, in schema order

    def test_description_mentions_what_matters(self):
        text = characters.describe({**QUIET, "hunger": 8, "food": "near", "finger": "near", "startled": "yes",
                                    "time": "night", "doing": "rest", "cover": "mid", "friend": "far"})
        for needle in ["8 out of 9", "night", "resting", "finger", "fright", "pellet", "Hiding place"]:
            self.assertIn(needle, text)
        quiet = characters.describe(QUIET)
        self.assertIn("no food", quiet)
        self.assertIn("not in the water", quiet)
        self.assertNotIn("fright", quiet)
        # It reads as English, not as the raw line.
        self.assertNotIn("startled no", quiet)
        self.assertNotIn("seek_food", characters.describe({**QUIET, "doing": "seek_food"}))

    def test_description_differs_when_the_situation_does(self):
        seen = set()
        for field, values in [("hunger", range(10)), ("energy", range(10)), ("stress", range(10)),
                              ("food", schema.DISTS), ("friend", schema.DISTS), ("finger", schema.DISTS),
                              ("cover", schema.DISTS), ("time", ["day", "night"]), ("doing", schema.GOALS),
                              ("startled", ["yes", "no"]), ("bold", range(10)), ("social", range(10))]:
            texts = {characters.describe({**QUIET, "finger": "near", field: value}) if field == "startled"
                     else characters.describe({**QUIET, field: value}) for value in values}
            self.assertEqual(len(texts), len(list(values)), field)
            seen |= texts
        self.assertGreater(len(seen), 60)

    def test_messages(self):
        messages = characters.build_messages(state(species="pufferfish", bold=2, finger="near", cover="far"))
        self.assertEqual([m["role"] for m in messages], ["system", "user"])
        system, user = messages[0]["content"], messages[1]["content"]
        self.assertIn(characters.CHARACTERS["pufferfish"], system)
        self.assertNotIn(characters.CHARACTERS["clownfish"], system)
        for mood in schema.MOODS:
            self.assertIn(mood, system)
        # Only the goals on offer are named as choices.
        for goal in ["wander", "rest", "investigate", "flee", "hide"]:
            self.assertIn(goal, user)
        for goal in ["seek_food", "school"]:
            self.assertNotIn(goal, user)
        with self.assertRaises(ValueError):
            characters.build_messages("species shark")

    def test_answer_format(self):
        fmt = characters.answer_format(["wander", "rest"])
        self.assertEqual(fmt["properties"]["goal"]["enum"], ["wander", "rest"])
        self.assertEqual(fmt["properties"]["mood"]["enum"], schema.MOODS)
        self.assertEqual(fmt["required"], ["goal", "mood"])


if __name__ == "__main__":
    unittest.main()
