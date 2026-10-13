"""Build the stage 1 data set: situations labelled by the seven rules.

    python3 model/make_rules_data.py [--traces FILE] [--sampled N] [--out FILE]

Situations come from the simulator's traces (realistic) and from random
sampling (covers rare cases). Run the simulator first:

    ./sim/deskfish_sim --headless --traces model/out/traces.txt --runs 40
"""
import argparse
import random
from collections import Counter
from pathlib import Path

import dataset
import schema
import teacher_rules

OUT = schema.MODEL_DIR / "out"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--traces", type=Path, default=OUT / "traces.txt")
    parser.add_argument("--sampled", type=int, default=40000, help="how many random situations to add")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out", type=Path, default=OUT / "rules.txt")
    args = parser.parse_args()

    traced = dataset.read_examples(args.traces)
    disagree = [(s, g) for s, g, _ in traced if teacher_rules.label(s)[0] != g]
    if disagree:
        raise SystemExit(f"the Python rules disagree with the simulator on {len(disagree)} lines, "
                         f"for example {disagree[0]}")

    rng = random.Random(args.seed)
    sampled = [(state, *teacher_rules.label(state)) for state in
               (dataset.sample_state(rng) for _ in range(args.sampled))]

    examples = dataset.dedupe(traced + sampled)
    dataset.write_examples(args.out, examples)

    parts = dataset.split(examples)
    print(f"{len(traced)} traced lines, {len(set(traced))} of them different")
    print(f"{len(sampled)} sampled lines")
    print(f"{len(examples)} different situations in all, written to {args.out}")
    print("parts: " + ", ".join(f"{name} {len(part)}" for name, part in parts.items()))
    print("goals:")
    for goal, count in Counter(goal for _, goal, _ in examples).most_common():
        print(f"  {goal:12} {count:6}  {100 * count / len(examples):4.1f}%")
    print("species:")
    for species, count in sorted(Counter(schema.parse_state(s)["species"] for s, _, _ in examples).items()):
        print(f"  {species:12} {count:6}")


if __name__ == "__main__":
    main()
