"""The student: a small decoder-only transformer, written from scratch.

It reads a line of tokens and, for every position, scores each word in the
vocabulary as the next one. That is all a language model is.
"""
import math
from dataclasses import asdict, dataclass

import torch
import torch.nn as nn
import torch.nn.functional as F

import schema


@dataclass
class Config:
    vocab_size: int = 54
    block_size: int = 32  # the longest line it can hold; an example is 31 tokens
    n_layer: int = 2      # how many blocks are stacked
    n_head: int = 2       # how many separate lookups each attention does
    n_embd: int = 32      # how many numbers describe each token


# The sizes compared. Each is roughly four times the one before.
SIZES = {
    "xs": dict(n_layer=1, n_head=2, n_embd=16),
    "s": dict(n_layer=2, n_head=2, n_embd=32),
    "m": dict(n_layer=2, n_head=4, n_embd=64),
    "l": dict(n_layer=4, n_head=4, n_embd=96),
    "xl": dict(n_layer=4, n_head=8, n_embd=128),
}


class SelfAttention(nn.Module):
    """Each position gathers information from itself and earlier positions.

    For every position the layer makes a query ("what am I looking for?"), and
    for every position a key ("what do I offer?") and a value ("what I pass
    on"). How well a query matches a key decides how much of that value is
    taken. Several heads do this side by side, looking for different things.
    """

    def __init__(self, config):
        super().__init__()
        self.n_head = config.n_head
        self.query_key_value = nn.Linear(config.n_embd, 3 * config.n_embd)
        self.out = nn.Linear(config.n_embd, config.n_embd)
        # 1 where looking is allowed: at yourself and to the left.
        allowed = torch.tril(torch.ones(config.block_size, config.block_size))
        self.register_buffer("allowed", allowed.view(1, 1, config.block_size, config.block_size))

    def forward(self, x):
        batch, time, width = x.shape
        head_width = width // self.n_head
        q, k, v = self.query_key_value(x).split(width, dim=2)
        # (batch, time, width) -> (batch, heads, time, head_width)
        q = q.view(batch, time, self.n_head, head_width).transpose(1, 2)
        k = k.view(batch, time, self.n_head, head_width).transpose(1, 2)
        v = v.view(batch, time, self.n_head, head_width).transpose(1, 2)

        match = (q @ k.transpose(-2, -1)) / math.sqrt(head_width)       # every query against every key
        match = match.masked_fill(self.allowed[:, :, :time, :time] == 0, float("-inf"))  # no looking ahead
        weights = F.softmax(match, dim=-1)                               # shares that add up to 1
        gathered = weights @ v                                           # a blend of the values
        gathered = gathered.transpose(1, 2).contiguous().view(batch, time, width)
        return self.out(gathered)


class Block(nn.Module):
    """Gather (attention), then digest (a small ordinary network)."""

    def __init__(self, config):
        super().__init__()
        self.norm1 = nn.LayerNorm(config.n_embd)
        self.attention = SelfAttention(config)
        self.norm2 = nn.LayerNorm(config.n_embd)
        self.digest = nn.Sequential(
            nn.Linear(config.n_embd, 4 * config.n_embd),
            nn.GELU(),
            nn.Linear(4 * config.n_embd, config.n_embd),
        )

    def forward(self, x):
        # Each step adds to what was there, so information can pass straight through.
        x = x + self.attention(self.norm1(x))
        x = x + self.digest(self.norm2(x))
        return x


class Student(nn.Module):
    def __init__(self, config):
        super().__init__()
        if config.n_embd % config.n_head != 0:
            raise ValueError(f"n_embd ({config.n_embd}) must divide evenly among n_head ({config.n_head})")
        self.config = config
        self.token_embedding = nn.Embedding(config.vocab_size, config.n_embd)     # what each word is
        self.position_embedding = nn.Embedding(config.block_size, config.n_embd)  # where each word sits
        self.blocks = nn.ModuleList(Block(config) for _ in range(config.n_layer))
        self.norm = nn.LayerNorm(config.n_embd)
        self.to_vocab = nn.Linear(config.n_embd, config.vocab_size, bias=False)   # a score for every word

    def forward(self, tokens):
        """tokens: (batch, time) of token numbers -> (batch, time, vocab) scores for the next token."""
        time = tokens.shape[1]
        if time > self.config.block_size:
            raise ValueError(f"a line of {time} tokens is longer than the model holds ({self.config.block_size})")
        positions = torch.arange(time, device=tokens.device)
        x = self.token_embedding(tokens) + self.position_embedding(positions)
        for block in self.blocks:
            x = block(x)
        return self.to_vocab(self.norm(x))


def count_parameters(model):
    return sum(p.numel() for p in model.parameters())


@torch.no_grad()
def answer(model, tok, state):
    """Give the model a situation; it writes two words, the likeliest each time."""
    model.eval()
    device = next(model.parameters()).device
    tokens = torch.tensor([[tok.bos] + tok.encode(state) + [tok.sep]], device=device)
    written = []
    for _ in range(2):
        next_token = model(tokens)[0, -1].argmax().item()
        written.append(next_token)
        tokens = torch.cat([tokens, torch.tensor([[next_token]], device=device)], dim=1)
    return tok.words[written[0]], tok.words[written[1]]


def save(model, path, extra=None):
    torch.save({"config": asdict(model.config), "weights": model.state_dict(),
                "schema": schema.SCHEMA_VERSION, "extra": extra or {}}, path)


def load(path):
    saved = torch.load(path, map_location="cpu")
    if saved["schema"] != schema.SCHEMA_VERSION:
        raise ValueError(f"{path} was trained for schema v{saved['schema']}, this is v{schema.SCHEMA_VERSION}")
    model = Student(Config(**saved["config"]))
    model.load_state_dict(saved["weights"])
    return model.eval(), saved["extra"]
