"""Train every size on the same data and compare them.

    python3 model/sweep.py --data model/out/rules.txt --prefix rules --report model/out/stage1_report.md
"""
import argparse
import time
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
    parser.add_argument("--prefix", required=True, help="checkpoints are model/out/<prefix>_<size>.pt")
    parser.add_argument("--sizes", nargs="+", default=list(student.SIZES), choices=list(student.SIZES))
    parser.add_argument("--epochs", type=int, default=20)
    parser.add_argument("--batch-size", type=int, default=128)
    parser.add_argument("--lr", type=float, default=3e-3)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()

    tok = Tokenizer()
    parts = dataset.split(dataset.read_examples(args.data))
    train_tokens, val_tokens = train.to_tensor(parts["train"], tok), train.to_tensor(parts["val"], tok)
    rows, details = [], []
    for size in args.sizes:
        config = student.Config(vocab_size=tok.size, **student.SIZES[size])
        device = train.pick_device("auto", config)
        numbers = student.count_parameters(student.Student(config))
        print(f"\n=== size {size}: {numbers:,} numbers, on {device} ===")
        began = time.time()
        model, history = train.train(train_tokens, val_tokens, config, args.epochs, args.batch_size, args.lr,
                                     args.seed, device)
        seconds = time.time() - began
        model = model.cpu()
        student.save(model, schema.MODEL_DIR / "out" / f"{args.prefix}_{size}.pt",
                     {"size": size, "data": str(args.data), "history": history, "seconds": round(seconds, 1)})
        results = evaluate.evaluate(model, tok, parts["test"])
        rows.append((size, numbers, results, seconds))
        details.append(f"### Size {size}\n\n```\n{evaluate.report(results)}\n```\n")

    best = max(results["goal"] for _, _, results, _ in rows)
    chosen = next(size for size, _, results, _ in rows if results["goal"] >= best - 0.02)
    table = ["| Size | Numbers | Goal | Mood | Both | Valid | Training time |", "|---|---|---|---|---|---|---|"]
    for size, numbers, results, seconds in rows:
        table.append(f"| {size}{' (chosen)' if size == chosen else ''} | {numbers:,} | {100 * results['goal']:.2f}% | "
                     f"{100 * results['mood']:.2f}% | {100 * results['both']:.2f}% | {100 * results['valid']:.2f}% | "
                     f"{seconds:.0f} s |")
    text = "\n".join([
        f"# Student sizes compared: {args.prefix}",
        "",
        f"Data: `{args.data}`. {len(parts['train'])} situations to learn from, {len(parts['val'])} to watch during "
        f"training, {len(parts['test'])} held back for these scores. {args.epochs} passes over the data.",
        "",
        "Agreement with the teacher on situations the student never saw:",
        "",
        *table,
        "",
        f"Chosen: **{chosen}**, the smallest size within two points of the best goal agreement.",
        "",
        *details,
    ])
    args.report.write_text(text)
    print("\n" + "\n".join(table))
    print(f"\nchosen: {chosen}; report written to {args.report}")


if __name__ == "__main__":
    main()
