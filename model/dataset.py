"""Making, reading, de-duplicating and splitting examples.

An example is (situation, goal, mood). On disk it is one line of text.
"""
import hashlib

import schema


def sample_state(rng):
    """One situation, every field picked at random, for coverage of rare cases.

    bold and social stay within one of the species' usual values, as they do
    in the tank.
    """
    species = rng.choice(list(schema.SPECIES))
    bold, social = schema.SPECIES[species]
    fields = {
        "species": species,
        "hunger": rng.randint(0, 9),
        "energy": rng.randint(0, 9),
        "stress": rng.randint(0, 9),
        "bold": min(9, max(0, bold + rng.randint(-1, 1))),
        "social": min(9, max(0, social + rng.randint(-1, 1))),
        "time": rng.choice(["day", "night"]),
        "startled": rng.choice(["yes", "no"]),
        "doing": rng.choice(schema.GOALS),
        "food": rng.choice(schema.DISTS),
        "friend": rng.choice(schema.DISTS),
        "finger": rng.choice(schema.DISTS),
        "cover": rng.choice(schema.DISTS),
    }
    return schema.format_state(fields)


def read_examples(path):
    """Every line of a file as (situation, goal, mood). Blank lines are skipped."""
    examples = []
    for number, line in enumerate(path.read_text().splitlines(), start=1):
        if not line.strip():
            continue
        try:
            examples.append(schema.parse_example(line))
        except ValueError as problem:
            raise ValueError(f"{path}, line {number}: {problem}") from None
    return examples


def write_examples(path, examples):
    path.write_text("".join(schema.format_example(*example) + "\n" for example in examples))


def dedupe(examples):
    """Each situation once, in first-seen order. Two labels for one situation is an error."""
    seen = {}
    for state, goal, mood in examples:
        if state in seen and seen[state] != (goal, mood):
            raise ValueError(f"two different answers for one situation: {state!r}: "
                             f"{seen[state]} and {(goal, mood)}")
        seen.setdefault(state, (goal, mood))
    return [(state, goal, mood) for state, (goal, mood) in seen.items()]


def part_of(state):
    """Which part a situation belongs to: 80% train, 10% val, 10% test.

    Decided by a hash of the situation alone, so it is the same every time and
    one situation can never be in two parts.
    """
    bucket = int(hashlib.md5(state.encode()).hexdigest(), 16) % 100
    return "train" if bucket < 80 else "val" if bucket < 90 else "test"


def split(examples):
    parts = {"train": [], "val": [], "test": []}
    for example in examples:
        parts[part_of(example[0])].append(example)
    return parts
