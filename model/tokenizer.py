"""Words to token numbers and back.

Every word the model will ever read or write is in vocab.txt, one per line. A
word's token number is its position in that file.
"""
import schema


class Tokenizer:
    def __init__(self, path=schema.VOCAB_PATH):
        self.words = path.read_text().split()
        self.size = len(self.words)
        self._number = {word: i for i, word in enumerate(self.words)}
        if len(self._number) != self.size:
            raise ValueError(f"{path} has a repeated word")
        self.bos = self._number[schema.BOS]
        self.eos = self._number[schema.EOS]
        self.sep = self._number[schema.SEPARATOR]

    def encode(self, text):
        """'species clownfish' -> [3, 29]. An unknown word is an error, never a guess."""
        ids = []
        for word in text.split():
            if word not in self._number:
                raise ValueError(f"{word!r} is not in the vocabulary")
            ids.append(self._number[word])
        return ids

    def decode(self, ids):
        for i in ids:
            if not 0 <= i < self.size:
                raise ValueError(f"{i} is not a token number (there are {self.size})")
        return " ".join(self.words[i] for i in ids)

    def encode_example(self, state, goal, mood):
        """<s> situation -> goal mood </s>, always EXAMPLE_TOKENS long."""
        ids = [self.bos] + self.encode(state) + [self.sep] + self.encode(f"{goal} {mood}") + [self.eos]
        if len(ids) != schema.EXAMPLE_TOKENS:
            raise ValueError(f"an example is {schema.EXAMPLE_TOKENS} tokens, this is {len(ids)}")
        return ids
