import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import schema

EXAMPLE = ("species clownfish hunger 6 energy 7 stress 0 bold 8 social 7 time day startled no "
           "doing wander food near friend mid finger none cover far")


class SchemaTest(unittest.TestCase):
    def test_vocab_file_matches_the_schema(self):
        words = schema.VOCAB_PATH.read_text().split()
        self.assertEqual(words, schema.expected_vocab())
        self.assertEqual(len(words), 54)
        self.assertEqual(len(set(words)), 54)

    def test_parse_the_documented_example(self):
        fields = schema.parse_state(EXAMPLE)
        self.assertEqual(fields["species"], "clownfish")
        self.assertEqual(fields["hunger"], 6)
        self.assertEqual(fields["bold"], 8)
        self.assertEqual(fields["time"], "day")
        self.assertEqual(fields["startled"], "no")
        self.assertEqual(fields["doing"], "wander")
        self.assertEqual(fields["cover"], "far")
        self.assertEqual(list(fields), schema.FIELDS)

    def test_format_is_the_inverse_of_parse(self):
        self.assertEqual(schema.format_state(schema.parse_state(EXAMPLE)), EXAMPLE)
        self.assertEqual(len(EXAMPLE.split()), schema.STATE_WORDS)

    def test_examples(self):
        state, goal, mood = schema.parse_example(EXAMPLE + " -> seek_food eager")
        self.assertEqual((state, goal, mood), (EXAMPLE, "seek_food", "eager"))
        self.assertEqual(schema.format_example(state, goal, mood), EXAMPLE + " -> seek_food eager")
        self.assertEqual(schema.EXAMPLE_TOKENS, 1 + schema.STATE_WORDS + 1 + 2 + 1)

    def test_rejects_bad_lines(self):
        bad = {
            "a field missing": EXAMPLE.replace(" cover far", ""),
            "an extra word": EXAMPLE + " far",
            "fields out of order": EXAMPLE.replace("hunger 6 energy 7", "energy 7 hunger 6"),
            "a number too big": EXAMPLE.replace("hunger 6", "hunger 12"),
            "a negative number": EXAMPLE.replace("stress 0", "stress -1"),
            "a word where a number belongs": EXAMPLE.replace("bold 8", "bold lots"),
            "an unknown species": EXAMPLE.replace("clownfish", "shark"),
            "an unknown goal": EXAMPLE.replace("doing wander", "doing dance"),
            "an unknown distance": EXAMPLE.replace("food near", "food close"),
            "a bad time": EXAMPLE.replace("time day", "time noon"),
            "upper case": EXAMPLE.replace("clownfish", "Clownfish"),
            "empty": "",
        }
        for what, line in bad.items():
            with self.assertRaises(ValueError, msg=what):
                schema.parse_state(line)

        for what, line in {
            "no separator": EXAMPLE + " seek_food eager",
            "no mood": EXAMPLE + " -> seek_food",
            "an unknown mood": EXAMPLE + " -> seek_food hungry",
            "an unknown goal": EXAMPLE + " -> dance calm",
            "mood and goal swapped": EXAMPLE + " -> eager seek_food",
            "three answer words": EXAMPLE + " -> seek_food eager calm",
            "a bad state": EXAMPLE.replace("clownfish", "shark") + " -> rest calm",
        }.items():
            with self.assertRaises(ValueError, msg=what):
                schema.parse_example(line)

    def test_error_messages_name_the_problem(self):
        with self.assertRaises(ValueError) as caught:
            schema.parse_state(EXAMPLE.replace("clownfish", "shark"))
        self.assertIn("shark", str(caught.exception))
        self.assertIn("species", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
