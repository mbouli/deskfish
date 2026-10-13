import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import schema
from tokenizer import Tokenizer

EXAMPLE = ("species clownfish hunger 6 energy 7 stress 0 bold 8 social 7 time day startled no "
           "doing wander food near friend mid finger none cover far")


class TokenizerTest(unittest.TestCase):
    def setUp(self):
        self.tok = Tokenizer()

    def test_size_and_markers(self):
        self.assertEqual(self.tok.size, 54)
        self.assertEqual(self.tok.words[self.tok.bos], "<s>")
        self.assertEqual(self.tok.words[self.tok.eos], "</s>")
        self.assertEqual(self.tok.words[self.tok.sep], "->")
        self.assertEqual((self.tok.bos, self.tok.eos, self.tok.sep), (0, 1, 2))

    def test_round_trip(self):
        ids = self.tok.encode(EXAMPLE)
        self.assertEqual(len(ids), schema.STATE_WORDS)
        self.assertEqual(self.tok.decode(ids), EXAMPLE)
        self.assertTrue(all(0 <= i < 54 for i in ids))
        self.assertEqual(self.tok.decode(self.tok.encode("seek_food eager")), "seek_food eager")

    def test_every_word_has_its_own_number(self):
        for position, word in enumerate(self.tok.words):
            self.assertEqual(self.tok.encode(word), [position])

    def test_examples(self):
        ids = self.tok.encode_example(EXAMPLE, "seek_food", "eager")
        self.assertEqual(len(ids), schema.EXAMPLE_TOKENS)
        self.assertEqual(ids[0], self.tok.bos)
        self.assertEqual(ids[-1], self.tok.eos)
        self.assertEqual(ids[27], self.tok.sep)
        self.assertEqual(self.tok.decode(ids[28:30]), "seek_food eager")
        self.assertEqual(self.tok.decode(ids), "<s> " + EXAMPLE + " -> seek_food eager </s>")

    def test_unknown_words_are_refused(self):
        for text, culprit in [("species shark", "shark"), ("Species clownfish", "Species"), ("hunger 10", "10"),
                              ("seek-food", "seek-food"), ("clownfish,", "clownfish,")]:
            with self.assertRaises(ValueError, msg=text) as caught:
                self.tok.encode(text)
            self.assertIn(culprit, str(caught.exception))  # the message names the word at fault
        with self.assertRaises(ValueError):
            self.tok.decode([54])
        with self.assertRaises(ValueError):
            self.tok.decode([-1])

    def test_blank_text(self):
        self.assertEqual(self.tok.encode(""), [])
        self.assertEqual(self.tok.encode("   "), [])
        self.assertEqual(self.tok.decode([]), "")


if __name__ == "__main__":
    unittest.main()
