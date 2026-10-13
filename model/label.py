"""The long run: have the teacher label situations for the student to learn from.

    python3 model/label.py [--count 12000] [--model qwen3.5:4b]

Safe to stop and start again: it carries on where it left off.
"""
import argparse
import random
from collections import Counter

import dataset
import schema
import teacher_llm

OUT = schema.MODEL_DIR / "out"


def pick_states(count, seed):
    """`count` different situations: recorded play and random ones, half and half, in a fixed order."""
    rng = random.Random(seed)
    traced = [state for state, _, _ in dataset.dedupe(dataset.read_examples(OUT / "traces.txt"))]
    rng.shuffle(traced)
    picked = dict.fromkeys(traced[:count // 2])
    while len(picked) < count:
        picked.setdefault(dataset.sample_state(rng))
    states = list(picked)
    rng.shuffle(states)
    return states


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--count", type=int, default=12000)
    parser.add_argument("--model", default="qwen3.5:4b")
    parser.add_argument("--temperature", type=float, default=0.0)
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()

    states = pick_states(args.count, args.seed)
    path = OUT / "teacher.txt"
    teacher = teacher_llm.OllamaTeacher(model=args.model, temperature=args.temperature)
    summary = teacher_llm.label_many(states, path, teacher, log=print)
    examples = dataset.read_examples(path)
    print(f"\nlabelled {summary['labelled']} this run, {summary['skipped']} were already done, "
          f"{summary['dropped']} dropped; {len(examples)} in {path}")
    for goal, n in Counter(goal for _, goal, _ in examples).most_common():
        print(f"  {goal:12} {n:6}  {100 * n / len(examples):4.1f}%")
    for mood, n in Counter(mood for _, _, mood in examples).most_common():
        print(f"  {mood:12} {n:6}  {100 * n / len(examples):4.1f}%")


if __name__ == "__main__":
    main()
