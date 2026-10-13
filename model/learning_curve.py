"""How does agreement grow with the number of examples?

    python3 model/learning_curve.py --data model/out/rules.txt --size s

Trains the same size on growing slices of the training part and scores each on
the same test part. On the rules data this costs nothing and gives a lower
bound on what the teacher's data will need: the teacher is harder to imitate.
"""
import argparse
import random
from pathlib import Path

import dataset
import evaluate
import schema
import student
import train
from tokenizer import Tokenizer


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--size", choices=list(student.SIZES), default="s")
    parser.add_argument("--counts", type=int, nargs="+", default=[250, 500, 1000, 2000, 4000, 8000, 16000])
    parser.add_argument("--steps", type=int, default=4000, help="training steps for every slice, so they are comparable")
    parser.add_argument("--report", type=Path, default=schema.MODEL_DIR / "out" / "learning_curve.md")
    args = parser.parse_args()

    tok = Tokenizer()
    parts = dataset.split(dataset.read_examples(args.data))
    random.Random(1).shuffle(parts["train"])
    config = student.Config(vocab_size=tok.size, **student.SIZES[args.size])
    val, test = train.to_tensor(parts["val"][:2000], tok), parts["test"]
    rows = []
    for count in args.counts:
        if count > len(parts["train"]):
            break
        tokens = train.to_tensor(parts["train"][:count], tok)
        epochs = max(1, round(args.steps * 128 / count))  # the same amount of training whatever the slice
        model, _ = train.train(tokens, val, config, epochs, 128, 3e-3, 1, "cpu", log=None)
        results = evaluate.evaluate(model, tok, test)
        rows.append((count, results))
        print(f"{count:6} examples: goal {100 * results['goal']:6.2f}%  both {100 * results['both']:6.2f}%")

    lines = [f"# Learning curve: size {args.size} on `{args.data.name}`", "",
             f"Each row is the same model trained for the same number of steps on a larger slice of the data, "
             f"scored on the same {len(test)} unseen situations.", "",
             "| Examples | Goal | Mood | Both |", "|---|---|---|---|"]
    lines += [f"| {count:,} | {100 * r['goal']:.2f}% | {100 * r['mood']:.2f}% | {100 * r['both']:.2f}% |"
              for count, r in rows]
    args.report.write_text("\n".join(lines) + "\n")
    print(f"written to {args.report}")


if __name__ == "__main__":
    main()
