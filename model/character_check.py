"""Does the student act differently as different fish?

    python3 model/character_check.py model/out/teacher_s.pt

Takes situations from the test part, changes only the species (and boldness
and sociability to match), and compares the student's answers.
"""
import argparse
import random
from collections import Counter
from pathlib import Path

import dataset
import schema
import student
from tokenizer import Tokenizer


def swap_species(state, species):
    """The same situation as a different fish, with that species' usual traits."""
    fields = schema.parse_state(state)
    fields["species"] = species
    fields["bold"], fields["social"] = schema.SPECIES[species]
    return schema.format_state(fields)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("--data", type=Path, default=schema.MODEL_DIR / "out" / "teacher.txt")
    parser.add_argument("--count", type=int, default=400)
    parser.add_argument("--report", type=Path, default=schema.MODEL_DIR / "out" / "character_check.md")
    args = parser.parse_args()

    model, _ = student.load(args.checkpoint)
    tok = Tokenizer()
    test = dataset.split(dataset.read_examples(args.data))["test"]
    states = [state for state, _, _ in random.Random(1).sample(test, min(args.count, len(test)))]

    goals, moods = Counter(), Counter()
    distinct_goals = distinct_answers = 0
    examples = []
    for state in states:
        answers = {species: student.answer(model, tok, swap_species(state, species)) for species in schema.SPECIES}
        for species, (goal, mood) in answers.items():
            goals[(species, goal)] += 1
            moods[(species, mood)] += 1
        distinct_goals += len({goal for goal, _ in answers.values()}) > 1
        distinct_answers += len(set(answers.values())) > 1
        if len(set(answers.values())) >= 4 and len(examples) < 8:
            examples.append((state, answers))

    def table(columns, counts):
        rows = ["| | " + " | ".join(columns) + " |", "|---|" + "---|" * len(columns)]
        for species in schema.SPECIES:
            rows.append(f"| {species} | " + " | ".join(str(counts.get((species, c), 0)) for c in columns) + " |")
        return rows

    lines = [
        f"# Character check: `{args.checkpoint.name}`", "",
        f"{len(states)} unseen situations, each put to the student as all seven species.", "",
        f"- The **goal** changed with the species in **{100 * distinct_goals / len(states):.1f}%** of situations.",
        f"- The goal or the mood changed in **{100 * distinct_answers / len(states):.1f}%**.", "",
        "Goals, by species:", "", *table(schema.GOALS, goals), "",
        "Moods, by species:", "", *table(schema.MOODS, moods), "",
        "## Situations where the fish disagreed most", "",
    ]
    for state, answers in examples:
        lines += [f"`{' '.join(state.split()[2:])}`", ""]
        lines += [f"- {species}: `{goal} {mood}`" for species, (goal, mood) in answers.items()] + [""]
    args.report.write_text("\n".join(lines))
    print("\n".join(lines[:6]))
    print(f"written to {args.report}")


if __name__ == "__main__":
    main()
