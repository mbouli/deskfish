import random
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import dataset
import schema


class DatasetTest(unittest.TestCase):
    def test_sampled_states_are_valid_and_varied(self):
        rng = random.Random(1)
        states = [dataset.sample_state(rng) for _ in range(5000)]
        seen = {field: set() for field in schema.FIELDS}
        for state in states:
            fields = schema.parse_state(state)  # raises if invalid
            bold, social = schema.SPECIES[fields["species"]]
            self.assertLessEqual(abs(fields["bold"] - bold), 1)
            self.assertLessEqual(abs(fields["social"] - social), 1)
            for field, value in fields.items():
                seen[field].add(value)
        self.assertEqual(seen["species"], set(schema.SPECIES))
        self.assertEqual(seen["doing"], set(schema.GOALS))
        for field in ["food", "friend", "finger", "cover"]:
            self.assertEqual(seen[field], set(schema.DISTS))
        for field in ["hunger", "energy", "stress"]:
            self.assertEqual(seen[field], set(range(10)))
        self.assertEqual(seen["time"], {"day", "night"})
        self.assertEqual(seen["startled"], {"yes", "no"})
        self.assertGreater(len(set(states)), 4900)

    def test_sampling_is_repeatable(self):
        a = [dataset.sample_state(random.Random(7)) for _ in range(3)]
        b = [dataset.sample_state(random.Random(7)) for _ in range(3)]
        self.assertEqual(a, b)

    def test_split_is_disjoint_and_stable(self):
        rng = random.Random(2)
        examples = [(dataset.sample_state(rng), "rest", "calm") for _ in range(6000)]
        parts = dataset.split(examples)
        self.assertEqual(set(parts), {"train", "val", "test"})
        states = {name: {e[0] for e in part} for name, part in parts.items()}
        self.assertFalse(states["train"] & states["val"])
        self.assertFalse(states["train"] & states["test"])
        self.assertFalse(states["val"] & states["test"])
        self.assertEqual(sum(len(p) for p in parts.values()), len(examples))
        share = {name: len(part) / len(examples) for name, part in parts.items()}
        self.assertAlmostEqual(share["train"], 0.8, delta=0.03)
        self.assertAlmostEqual(share["val"], 0.1, delta=0.02)
        self.assertAlmostEqual(share["test"], 0.1, delta=0.02)

        # A situation's part depends only on the situation: not on its label,
        # its neighbours, or the order of the file.
        again = dataset.split(list(reversed(examples[:1000])))
        for name, part in again.items():
            for state, _, _ in part:
                self.assertEqual(dataset.part_of(state), name)
                self.assertIn(state, states[name])

    def test_dedupe(self):
        a = "species bass hunger 0 energy 0 stress 0 bold 7 social 2 time day startled no doing rest food none friend none finger none cover none"
        b = a.replace("hunger 0", "hunger 1")
        examples = [(a, "rest", "calm"), (b, "rest", "calm"), (a, "rest", "calm")]
        self.assertEqual(dataset.dedupe(examples), [(a, "rest", "calm"), (b, "rest", "calm")])
        # The same situation with two different labels is a contradiction.
        with self.assertRaises(ValueError):
            dataset.dedupe([(a, "rest", "calm"), (a, "wander", "calm")])

    def test_files_round_trip(self):
        rng = random.Random(3)
        examples = [(dataset.sample_state(rng), "hide", "wary") for _ in range(50)]
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "examples.txt"
            dataset.write_examples(path, examples)
            self.assertEqual(dataset.read_examples(path), examples)
            self.assertEqual(len(path.read_text().splitlines()), 50)

            path.write_text(path.read_text() + "\n\nspecies shark -> rest calm\n")
            with self.assertRaises(ValueError) as caught:
                dataset.read_examples(path)
            self.assertIn("53", str(caught.exception))  # the line number


if __name__ == "__main__":
    unittest.main()
