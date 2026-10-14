"""Schema v1: the frozen format of a situation line and its answer.

This is model/schema.md for code. Everything else in model/ gets its lists of
words and its parsing from here.
"""
from pathlib import Path

SCHEMA_VERSION = 1

MODEL_DIR = Path(__file__).resolve().parent
VOCAB_PATH = MODEL_DIR / "vocab.txt"

# Species, with their usual (bold, social). A fish's own values are within one
# of these. Must match common/species.c.
SPECIES = {
    "angelfish": (5, 4),
    "bass": (7, 2),
    "catfish": (4, 3),
    "clownfish": (8, 6),
    "goldfish": (6, 8),
    "pufferfish": (2, 3),
    "surgeonfish": (6, 7),
}
GOALS = ["wander", "seek_food", "rest", "school", "investigate", "flee", "hide"]
DISTS = ["none", "near", "mid", "far"]
MOODS = ["calm", "eager", "wary", "playful", "sleepy", "sulky"]

# The fields of a situation line, in order.
FIELDS = ["species", "hunger", "energy", "stress", "bold", "social", "time", "startled", "doing",
          "food", "friend", "finger", "cover"]
DIGIT_FIELDS = ["hunger", "energy", "stress", "bold", "social"]
ALLOWED = {
    "species": list(SPECIES),
    "time": ["day", "night"],
    "startled": ["yes", "no"],
    "doing": GOALS,
    "food": DISTS,
    "friend": DISTS,
    "finger": DISTS,
    "cover": DISTS,
}

BOS = "<s>"
EOS = "</s>"
SEPARATOR = "->"  # between the situation and the answer

STATE_WORDS = 2 * len(FIELDS)                 # 26
EXAMPLE_TOKENS = 1 + STATE_WORDS + 1 + 2 + 1  # 31: <s>, the line, ->, goal, mood, </s>


def expected_vocab():
    """Every token, in order. vocab.txt must match this exactly."""
    return ([BOS, EOS, SEPARATOR] + FIELDS + [str(d) for d in range(10)] + list(SPECIES) +
            ["day", "night", "yes", "no"] + GOALS + DISTS + MOODS)


def parse_state(line):
    """A situation line as a dict, field -> value, in field order. Numbers become ints."""
    words = line.split(" ") if line else []
    if len(words) != STATE_WORDS:
        raise ValueError(f"a situation has {STATE_WORDS} words, this has {len(words)}: {line!r}")
    fields = {}
    for i, field in enumerate(FIELDS):
        name, value = words[2 * i], words[2 * i + 1]
        if name != field:
            raise ValueError(f"expected the field {field!r} at word {2 * i + 1}, found {name!r}: {line!r}")
        if field in DIGIT_FIELDS:
            if value not in "0123456789" or len(value) != 1:
                raise ValueError(f"{field} must be a digit 0-9, found {value!r}: {line!r}")
            fields[field] = int(value)
        else:
            if value not in ALLOWED[field]:
                raise ValueError(f"{value!r} is not a valid {field}: {line!r}")
            fields[field] = value
    return fields


def format_state(fields):
    """The inverse of parse_state."""
    line = " ".join(f"{field} {fields[field]}" for field in FIELDS)
    parse_state(line)  # refuse to write a line we would not read
    return line


def parse_example(line):
    """'<situation> -> <goal> <mood>' as (situation, goal, mood)."""
    marker = f" {SEPARATOR} "
    if line.count(marker) != 1:
        raise ValueError(f"an example has exactly one {SEPARATOR!r}: {line!r}")
    state, answer = line.split(marker)
    parse_state(state)
    words = answer.split(" ")
    if len(words) != 2:
        raise ValueError(f"an answer is a goal and a mood, found {answer!r}: {line!r}")
    goal, mood = words
    if goal not in GOALS:
        raise ValueError(f"{goal!r} is not a goal: {line!r}")
    if mood not in MOODS:
        raise ValueError(f"{mood!r} is not a mood: {line!r}")
    return state, goal, mood


def format_example(state, goal, mood):
    line = f"{state} {SEPARATOR} {goal} {mood}"
    parse_example(line)
    return line
