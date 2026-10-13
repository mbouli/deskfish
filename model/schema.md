# Situation and answer schema — v1

**Frozen once labelled data exists.** Changing the field order, a word, or a
value range means regenerating every example and retraining. `schema.py` is
this document for code; `vocab.txt` is the vocabulary in token order.

## The situation line (what the model reads)

One line of lower-case words separated by single spaces, fields in this order:

```
species <species> hunger <0-9> energy <0-9> stress <0-9> bold <0-9> social <0-9>
time day|night startled yes|no doing <goal>
food <dist> friend <dist> finger <dist> cover <dist>
```

(Line breaks are for reading; the real line has none.) 26 words. Example:

```
species clownfish hunger 6 energy 7 stress 0 bold 8 social 7 time day startled no doing wander food near friend mid finger none cover far
```

| Field | Meaning |
|---|---|
| `species` | which fish this is |
| `hunger` `energy` `stress` | its needs, 0 (none) to 9 (most) |
| `bold` `social` | its traits, 0 to 9; within one of its species' usual values |
| `time` | `night` once the sun is 6 degrees below the horizon |
| `startled` | `yes` only for the decision a finger landing nearby has just forced |
| `doing` | its current goal |
| `food` `friend` `finger` `cover` | how far the nearest pellet, other fish, held finger and hiding place are |

Distances: `near` is under 48 pixels, `mid` under 120, `far` beyond that,
`none` if there is no such thing.

## The answer (what the model writes)

Two words: `<goal> <mood>`. Example: `seek_food eager`.

## Values

| Slot | Values |
|---|---|
| `<species>` | `angelfish` `bass` `catfish` `clownfish` `goldfish` `pufferfish` `surgeonfish` |
| `<goal>` | `wander` `seek_food` `rest` `school` `investigate` `flee` `hide` |
| `<dist>` | `none` `near` `mid` `far` |
| `<mood>` | `calm` `eager` `wary` `playful` `sleepy` `sulky` |

## Tokens

One token per word, 54 in all. For training, an example is

```
<s> <the 26 words of the line> -> <goal> <mood> </s>
```

which is 31 tokens. At run time the model is given everything up to and
including `->` and writes two words.

## Files of examples

One example per line, as text: `<line> -> <goal> <mood>`.
