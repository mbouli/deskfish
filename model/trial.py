"""Try the teacher out before trusting it with a long run.

    python3 model/trial.py [--count 210] [--model qwen3.5:4b] [--temperature 0.3]

Asks about each situation twice and writes model/out/trial_report.md:
speed, how often the teacher agrees with itself, how its choices differ by
species, and a sample to read.
"""
import argparse
import random
import time
from collections import Counter
from pathlib import Path

import characters
import dataset
import schema
import teacher_llm
import teacher_rules

OUT = schema.MODEL_DIR / "out"


def pick_states(count, seed):
    """`count` different situations, the same number of each species.

    Two in three come from recorded play, the rest are random, so both
    everyday and unusual situations are tried.
    """
    rng = random.Random(seed)
    per_species = count // len(schema.SPECIES)
    traces = OUT / "traces.txt"
    traced = {species: [] for species in schema.SPECIES}
    if traces.exists():
        for state, _, _ in dataset.dedupe(dataset.read_examples(traces)):
            traced[schema.parse_state(state)["species"]].append(state)
    picked = []
    for species in schema.SPECIES:
        mine = set()
        pool = traced[species]
        for state in rng.sample(pool, min(len(pool), per_species * 2 // 3)):
            mine.add(state)
        while len(mine) < per_species:
            state = dataset.sample_state(rng)
            if schema.parse_state(state)["species"] == species:
                mine.add(state)
        picked.extend(sorted(mine))
    rng.shuffle(picked)
    return picked


def self_agreement(first, second):
    """How often two passes over the same situations gave the same goal, mood, and both."""
    again = {state: (goal, mood) for state, goal, mood in second}
    pairs = [((goal, mood), again[state]) for state, goal, mood in first if state in again]
    count = len(pairs)
    if count == 0:
        return {"count": 0, "goal": 0.0, "mood": 0.0, "both": 0.0}
    return {
        "count": count,
        "goal": sum(a[0] == b[0] for a, b in pairs) / count,
        "mood": sum(a[1] == b[1] for a, b in pairs) / count,
        "both": sum(a == b for a, b in pairs) / count,
    }


def table(rows, columns, counts):
    lines = ["| | " + " | ".join(columns) + " |", "|---|" + "---|" * len(columns)]
    for row in rows:
        lines.append(f"| {row} | " + " | ".join(str(counts.get((row, column), 0)) for column in columns) + " |")
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--count", type=int, default=210, help="situations; a multiple of 7")
    parser.add_argument("--model", default="qwen3.5:4b")
    parser.add_argument("--temperature", type=float, default=0.3)
    parser.add_argument("--seed", type=int, default=7)
    parser.add_argument("--fresh", action="store_true", help="start again instead of resuming")
    args = parser.parse_args()

    states = pick_states(args.count, args.seed)
    paths = [OUT / "trial_a.txt", OUT / "trial_b.txt"]
    if args.fresh:
        for path in paths:
            path.unlink(missing_ok=True)

    teacher = teacher_llm.OllamaTeacher(model=args.model, temperature=args.temperature)
    summaries = []
    for name, path in zip(["first pass", "second pass"], paths):
        print(f"{name}: {len(states)} situations")
        # A different seed each pass, so the second is a fresh roll of the dice.
        teacher.seed = len(summaries) + 1
        summaries.append(teacher_llm.label_many(states, path, teacher, log=print))

    first, second = (dataset.read_examples(path) for path in paths)
    agreement = self_agreement(first, second)
    asked = sum(s["labelled"] + s["dropped"] for s in summaries)
    seconds = sum(s["seconds"] for s in summaries)
    per_hour = asked / seconds * 3600 if seconds > 0 else 0.0
    with_rules = sum(teacher_rules.label(state)[0] == goal for state, goal, _ in first) / max(1, len(first))

    species_goal = Counter((schema.parse_state(state)["species"], goal) for state, goal, _ in first)
    species_mood = Counter((schema.parse_state(state)["species"], mood) for state, _, mood in first)
    again = {state: (goal, mood) for state, goal, mood in second}

    report = [
        f"# Teacher trial: {args.model}",
        "",
        f"{len(states)} situations, {len(states) // len(schema.SPECIES)} of each species, each asked twice at "
        f"temperature {args.temperature}, "
        "thinking off.",
        "",
        "## Speed",
        "",
        f"- {per_hour:.0f} answers an hour ({seconds / max(1, asked):.2f} seconds each)." if asked else
        "- Nothing was asked this run (the files were already complete); speed not measured.",
        f"- 12,000 labels would take about {12000 / per_hour:.1f} hours." if per_hour > 0 else "",
        f"- Dropped after three invalid answers: {sum(s['dropped'] for s in summaries)}.",
        "",
        "## Does it agree with itself?",
        "",
        f"On the {agreement['count']} situations answered both times:",
        "",
        f"- same goal: **{100 * agreement['goal']:.1f}%**",
        f"- same mood: **{100 * agreement['mood']:.1f}%**",
        f"- same goal and mood: **{100 * agreement['both']:.1f}%**",
        "",
        "This is the ceiling: a student cannot agree with the teacher more often than the teacher agrees "
        "with itself.",
        "",
        "## Is there character?",
        "",
        f"The teacher picked the same goal as the seven rules **{100 * with_rules:.1f}%** of the time. "
        "Near 100% would mean it adds nothing; very low would mean it has stopped being sensible.",
        "",
        "Goals chosen, by species (first pass):",
        "",
        *table(list(schema.SPECIES), schema.GOALS, species_goal),
        "",
        "Moods chosen, by species (first pass):",
        "",
        *table(list(schema.SPECIES), schema.MOODS, species_mood),
        "",
        "## A sample to read",
        "",
    ]
    rng = random.Random(args.seed)
    for state, goal, mood in rng.sample(first, min(28, len(first))):
        fields = schema.parse_state(state)
        second_answer = " ".join(again.get(state, ("(none)", "")))
        report += [
            f"**{fields['species']}** (rules would say `{teacher_rules.label(state)[0]}`)",
            "",
            characters.describe(fields),
            "",
            f"Teacher: `{goal} {mood}`; asked again: `{second_answer.strip()}`",
            "",
        ]
    path = OUT / "trial_report.md"
    path.write_text("\n".join(report))
    print(f"\nself-agreement: goal {100 * agreement['goal']:.1f}%, mood {100 * agreement['mood']:.1f}%, "
          f"both {100 * agreement['both']:.1f}%")
    print(f"agrees with the rules: {100 * with_rules:.1f}%")
    print(f"speed: {per_hour:.0f} an hour" if asked else "speed: not measured (nothing new asked)")
    print(f"report written to {path}")


if __name__ == "__main__":
    began = time.time()
    main()
    print(f"({time.time() - began:.0f} s)")
