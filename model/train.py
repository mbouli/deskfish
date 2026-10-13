"""Train a student on a file of examples.

    python3 model/train.py --data model/out/rules.txt --size s --name rules_s

Writes model/out/<name>.pt.
"""
import argparse
import math
import time
from pathlib import Path

import torch
import torch.nn.functional as F

import dataset
import schema
import student
from tokenizer import Tokenizer

# An example is  <s> [26 words] -> goal mood </s>,  token positions 0..30.
# The model reads positions 0..29 and, at each one, predicts the next. So its
# output at position 27 (the "->") is its guess at the goal, and at position
# 28 (the goal) its guess at the mood.
ANSWER_AT = 27


def to_tensor(examples, tok):
    return torch.tensor([tok.encode_example(*example) for example in examples], dtype=torch.long)


def answer_loss(logits, targets):
    """How surprised the model was by the right goal and mood. Lower is better.

    Only those two positions count: the situation itself is not predictable
    and the model is not asked to predict it.
    """
    scored = logits[:, ANSWER_AT:ANSWER_AT + 2].reshape(-1, logits.shape[-1])
    return F.cross_entropy(scored, targets[:, ANSWER_AT:ANSWER_AT + 2].reshape(-1))


@torch.no_grad()
def generate(model, tokens):
    """Write the goal, then the mood having seen its own goal, for a whole batch."""
    model.eval()
    prompt = tokens[:, :ANSWER_AT + 1]                        # everything up to and including "->"
    goal = model(prompt)[:, -1].argmax(dim=-1)
    mood = model(torch.cat([prompt, goal[:, None]], dim=1))[:, -1].argmax(dim=-1)
    return goal, mood


@torch.no_grad()
def agreement(model, tokens, batch_size=2048):
    """The share of examples where the model's goal, mood, and both match the teacher's."""
    device = next(model.parameters()).device
    right_goal = right_mood = right_both = 0
    for start in range(0, len(tokens), batch_size):
        batch = tokens[start:start + batch_size].to(device)
        goal, mood = generate(model, batch)
        goal_ok = goal == batch[:, ANSWER_AT + 1]
        mood_ok = mood == batch[:, ANSWER_AT + 2]
        right_goal += goal_ok.sum().item()
        right_mood += mood_ok.sum().item()
        right_both += (goal_ok & mood_ok).sum().item()
    count = max(1, len(tokens))
    return {"goal": right_goal / count, "mood": right_mood / count, "both": right_both / count}


def train(train_tokens, val_tokens, config, epochs, batch_size, lr, seed, device, log=print):
    """Returns the trained model and one record per epoch."""
    torch.manual_seed(seed)
    model = student.Student(config).to(device)
    optimiser = torch.optim.AdamW(model.parameters(), lr=lr, weight_decay=0.01)
    shuffler = torch.Generator().manual_seed(seed)
    steps_per_epoch = math.ceil(len(train_tokens) / batch_size)
    total_steps = epochs * steps_per_epoch
    warmup = min(100, total_steps // 10)
    step = 0
    history = []

    for epoch in range(1, epochs + 1):
        model.train()
        order = torch.randperm(len(train_tokens), generator=shuffler)
        total_loss = 0.0
        for start in range(0, len(order), batch_size):
            batch = train_tokens[order[start:start + batch_size]].to(device)
            # A gentle start, then a slow glide down: big steps early, fine ones late.
            scale = (step + 1) / warmup if step < warmup else \
                0.5 * (1 + math.cos(math.pi * (step - warmup) / max(1, total_steps - warmup)))
            for group in optimiser.param_groups:
                group["lr"] = lr * scale

            logits = model(batch[:, :-1])            # read everything but the last token...
            loss = answer_loss(logits, batch[:, 1:])  # ...and be scored on predicting the next
            optimiser.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimiser.step()
            total_loss += loss.item() * len(batch)
            step += 1

        record = {"epoch": epoch, "loss": round(total_loss / len(train_tokens), 6)}
        record.update({f"val_{k}": round(v, 6) for k, v in agreement(model, val_tokens).items()})
        history.append(record)
        if log:
            log(f"epoch {epoch:3}  loss {record['loss']:.4f}  unseen: goal {100 * record['val_goal']:5.1f}%  "
                f"mood {100 * record['val_mood']:5.1f}%")
    return model.eval(), history


def pick_device(name, config):
    """Small models are quicker on the processor than on the graphics chip."""
    if name != "auto":
        return name
    return "mps" if torch.backends.mps.is_available() and config.n_embd >= 96 else "cpu"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--size", choices=list(student.SIZES), default="s")
    parser.add_argument("--name", required=True, help="the checkpoint is model/out/<name>.pt")
    parser.add_argument("--epochs", type=int, default=20)
    parser.add_argument("--batch-size", type=int, default=128)
    parser.add_argument("--lr", type=float, default=3e-3)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--device", default="auto")
    args = parser.parse_args()

    tok = Tokenizer()
    parts = dataset.split(dataset.read_examples(args.data))
    config = student.Config(vocab_size=tok.size, **student.SIZES[args.size])
    device = pick_device(args.device, config)
    print(f"size {args.size}: {student.count_parameters(student.Student(config)):,} numbers, on {device}; "
          f"{len(parts['train'])} examples to learn from, {len(parts['val'])} to watch")

    began = time.time()
    model, history = train(to_tensor(parts["train"], tok), to_tensor(parts["val"], tok), config,
                           args.epochs, args.batch_size, args.lr, args.seed, device)
    path = schema.MODEL_DIR / "out" / f"{args.name}.pt"
    student.save(model.cpu(), path, {"size": args.size, "data": str(args.data), "history": history,
                                     "seconds": round(time.time() - began, 1)})
    print(f"saved {path} after {time.time() - began:.0f} s")


if __name__ == "__main__":
    main()
