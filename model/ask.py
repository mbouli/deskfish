"""Ask a trained student one question.

    python3 model/ask.py model/out/rules_s.pt "species clownfish hunger 6 energy 7 stress 0 bold 8 social 7 time day startled no doing wander food near friend mid finger none cover far"
"""
import sys
from pathlib import Path

import schema
import student
import teacher_rules
from tokenizer import Tokenizer


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    model, _ = student.load(Path(sys.argv[1]))
    state = sys.argv[2]
    schema.parse_state(state)  # a clear error if the line is not a situation
    goal, mood = student.answer(model, Tokenizer(), state)
    print(f"student: {goal} {mood}")
    print(f"rules:   {teacher_rules.label(state)[0]} calm")


if __name__ == "__main__":
    main()
