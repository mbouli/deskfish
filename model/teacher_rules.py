"""The stage 1 teacher: the seven hand-written rules, as in common/brain.c.

The simulator labels its own lines with the C rules. This copy labels lines
sampled in Python, and a test checks the two agree on every simulator line.
"""
import schema


def choose(f):
    """f is a parsed situation. First matching rule wins."""
    if f["finger"] == "near" and (f["startled"] == "yes" or f["bold"] <= 3):
        return "flee"
    if f["stress"] >= 6 and f["cover"] != "none":
        return "hide"
    if f["hunger"] >= 5 and f["food"] != "none":
        return "seek_food"
    if f["finger"] != "none" and f["bold"] >= 6 and f["stress"] <= 4:
        return "investigate"
    if f["time"] == "night" or f["energy"] <= 2 or (f["doing"] == "rest" and f["energy"] < 6):
        return "rest"
    if f["social"] >= 5 and f["friend"] in ("mid", "far"):
        return "school"
    return "wander"


def label(state):
    """The rules have no moods: every answer is calm."""
    return choose(schema.parse_state(state)), "calm"
