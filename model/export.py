"""Write a trained student as one small file for the board, and read it back.

    python3 model/export.py model/out/teacher_s.pt --data model/out/teacher.txt

The file, all little-endian:

    "DFM1"
    7 x u32   schema version, vocab_size, block_size, n_layer, n_head, n_embd, bits
    vocab     for each word: one length byte, then the word
    tensors   in tensor_names() order, for each:
                u32 count, f32 scale, then the values:
                  8 bits: one signed byte each
                  4 bits: two per byte, low half first, each stored as value + 8
              a value v stands for the number v * scale
"""
import argparse
import struct
from pathlib import Path

import torch

import dataset
import evaluate
import schema
import student
from tokenizer import Tokenizer

MAGIC = b"DFM1"


class ExportError(Exception):
    """The file is not a model file of ours, or is damaged."""


def tensor_names(model):
    """Every learned grid of numbers, in the order they are written. The attention mask is not learned."""
    return [name for name, _ in model.named_parameters()]


def _quantise(tensor, bits):
    """Whole numbers in -limit..limit, and the scale that turns them back."""
    limit = 2 ** (bits - 1) - 1  # 127 or 7
    largest = tensor.abs().max().item()
    scale = largest / limit if largest > 0 else 1.0
    values = torch.clamp(torch.round(tensor.flatten() / scale), -limit, limit).to(torch.int64).tolist()
    return values, scale


def write(model, tok, path, bits=8):
    if bits not in (4, 8):
        raise ValueError("bits must be 4 or 8")
    config = model.config
    out = bytearray(MAGIC)
    out += struct.pack("<7I", schema.SCHEMA_VERSION, config.vocab_size, config.block_size, config.n_layer,
                       config.n_head, config.n_embd, bits)
    for word in tok.words:
        encoded = word.encode()
        out += struct.pack("<B", len(encoded)) + encoded
    weights = model.state_dict()
    for name in tensor_names(model):
        values, scale = _quantise(weights[name], bits)
        out += struct.pack("<If", len(values), scale)
        if bits == 8:
            out += struct.pack(f"<{len(values)}b", *values)
        else:
            if len(values) % 2:
                values = values + [0]
            out += bytes((values[i] + 8) | ((values[i + 1] + 8) << 4) for i in range(0, len(values), 2))
    path.write_bytes(bytes(out))


class _Reader:
    def __init__(self, data, path):
        self.data, self.at, self.path = data, 0, path

    def take(self, count, what):
        if self.at + count > len(self.data):
            raise ExportError(f"{self.path} ends in the middle of {what}")
        piece = self.data[self.at:self.at + count]
        self.at += count
        return piece


def read(path):
    """The model and its vocabulary, exactly as the board will see them."""
    reader = _Reader(path.read_bytes(), path)
    if reader.take(4, "the header") != MAGIC:
        raise ExportError(f"{path} is not a deskfish model file")
    version, vocab_size, block_size, n_layer, n_head, n_embd, bits = struct.unpack("<7I", reader.take(28, "the header"))
    if version != schema.SCHEMA_VERSION:
        raise ExportError(f"{path} is for schema v{version}; this code is v{schema.SCHEMA_VERSION}")
    if bits not in (4, 8) or not 0 < vocab_size < 4096 or n_head == 0 or n_embd % n_head:
        raise ExportError(f"{path} has a nonsensical header")
    words = []
    for _ in range(vocab_size):
        length = reader.take(1, "the vocabulary")[0]
        words.append(reader.take(length, "the vocabulary").decode())

    model = student.Student(student.Config(vocab_size=vocab_size, block_size=block_size, n_layer=n_layer,
                                           n_head=n_head, n_embd=n_embd))
    weights = model.state_dict()
    limit = 2 ** (bits - 1) - 1
    for name in tensor_names(model):
        count, scale = struct.unpack("<If", reader.take(8, name))
        if count != weights[name].numel():
            raise ExportError(f"{path}: {name} has {count} numbers, expected {weights[name].numel()}")
        if bits == 8:
            values = list(struct.unpack(f"<{count}b", reader.take(count, name)))
        else:
            packed = reader.take((count + 1) // 2, name)
            values = [half - 8 for byte in packed for half in (byte & 0x0F, byte >> 4)][:count]
        if any(abs(v) > limit for v in values):
            raise ExportError(f"{path}: {name} holds a value outside -{limit}..{limit}")
        weights[name] = (torch.tensor(values, dtype=torch.float32) * scale).view_as(weights[name])
    if reader.at != len(reader.data):
        raise ExportError(f"{path} has {len(reader.data) - reader.at} unexpected bytes after the last tensor")
    model.load_state_dict(weights)
    return model.eval(), words


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--out", type=Path, default=schema.MODEL_DIR / "out" / "deskfish_model.bin")
    parser.add_argument("--report", type=Path, default=schema.MODEL_DIR / "out" / "export_report.md")
    args = parser.parse_args()

    tok = Tokenizer()
    model, extra = student.load(args.checkpoint)
    test = dataset.split(dataset.read_examples(args.data))["test"]
    rows = [("as trained (32-bit)", args.checkpoint.stat().st_size, evaluate.evaluate(model, tok, test))]
    scratch = args.out.with_suffix(".tmp")
    for bits in (8, 4):
        write(model, tok, scratch, bits)
        shrunk, _ = read(scratch)
        rows.append((f"{bits}-bit", scratch.stat().st_size, evaluate.evaluate(shrunk, tok, test)))
    scratch.unlink()

    full = rows[0][2]["goal"]
    # 8 bits unless 4 bits costs under a point.
    chosen = 4 if rows[2][2]["goal"] >= full - 0.01 and rows[2][2]["valid"] == 1.0 else 8
    write(model, tok, args.out, chosen)

    lines = [
        f"# The model for the board: `{args.out.name}`", "",
        f"From `{args.checkpoint.name}` (size {extra.get('size', '?')}, {student.count_parameters(model):,} numbers), "
        f"scored on {len(test)} unseen situations from `{args.data.name}`.", "",
        "| Stored as | File size | Goal | Mood | Both | Valid |", "|---|---|---|---|---|---|",
    ]
    for label, size, results in rows:
        lines.append(f"| {label} | {size:,} bytes | {100 * results['goal']:.2f}% | {100 * results['mood']:.2f}% | "
                     f"{100 * results['both']:.2f}% | {100 * results['valid']:.2f}% |")
    lines += ["", f"Written at **{chosen} bits**: {args.out.stat().st_size:,} bytes.", "",
              "```", evaluate.report(rows[1 if chosen == 8 else 2][2]), "```", ""]
    args.report.write_text("\n".join(lines))
    print("\n".join(lines[:9]))
    print(f"\nwritten {args.out} ({args.out.stat().st_size:,} bytes) at {chosen} bits; report in {args.report}")


if __name__ == "__main__":
    main()
