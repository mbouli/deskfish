"""Write situations with the shipped model's answers, for the C code to reproduce.

    python3 model/make_agreement.py

Run it again whenever deskfish_model.bin is replaced.
"""
import argparse
from pathlib import Path

import dataset
import export
import schema
import student
from tokenizer import Tokenizer


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--model", type=Path, default=schema.MODEL_DIR / "out" / "deskfish_model.bin")
    parser.add_argument("--data", type=Path, default=schema.MODEL_DIR / "out" / "teacher.txt")
    parser.add_argument("--count", type=int, default=500)
    parser.add_argument("--out", type=Path, default=schema.MODEL_DIR.parent / "tests" / "model_agreement.txt")
    args = parser.parse_args()

    tok = Tokenizer()
    model, words = export.read(args.model)
    if words != tok.words:
        raise SystemExit(f"{args.model} was made with a different vocabulary")
    unseen = dataset.split(dataset.read_examples(args.data))["test"]
    # Evenly spread through the sorted list, so every species takes part.
    ordered = sorted(state for state, _, _ in unseen)
    count = min(args.count, len(ordered))
    states = [ordered[i * len(ordered) // count] for i in range(count)]
    lines = []
    for state in states:
        goal, mood = student.constrained_answer(model, tok, state)
        lines.append(f"{state} {schema.SEPARATOR} {goal} {mood}")
    args.out.write_text("\n".join(lines) + "\n")
    print(f"{len(lines)} situations written to {args.out}")


if __name__ == "__main__":
    main()
