"""Score a trained student on the test part: situations it never saw.

    python3 model/evaluate.py model/out/rules_s.pt --data model/out/rules.txt
"""
import argparse
from pathlib import Path

import dataset
import schema
import student
import train
from tokenizer import Tokenizer

INVALID = "(invalid)"


def evaluate(model, tok, examples):
    """Agreement with the teacher on these examples.

    valid:     the model wrote a real goal followed by a real mood
    goal:      ...and the goal matched
    mood:      the mood matched
    both:      both matched
    confusion: teacher's goal -> the model's goal -> how many times
    """
    tokens = train.to_tensor(examples, tok)
    device = next(model.parameters()).device
    confusion = {goal: {} for goal in schema.GOALS}
    valid = right_goal = right_mood = right_both = 0
    for start in range(0, len(tokens), 2048):
        goal_ids, mood_ids = train.generate(model, tokens[start:start + 2048].to(device))
        for (_, goal, mood), goal_id, mood_id in zip(examples[start:start + 2048], goal_ids.tolist(), mood_ids.tolist()):
            said_goal, said_mood = tok.words[goal_id], tok.words[mood_id]
            is_valid = said_goal in schema.GOALS and said_mood in schema.MOODS
            valid += is_valid
            goal_ok = is_valid and said_goal == goal
            mood_ok = said_mood == mood
            right_goal += goal_ok
            right_mood += mood_ok
            right_both += goal_ok and mood_ok
            column = said_goal if said_goal in schema.GOALS else INVALID
            confusion[goal][column] = confusion[goal].get(column, 0) + 1
    count = max(1, len(examples))
    return {"count": len(examples), "valid": valid / count, "goal": right_goal / count,
            "mood": right_mood / count, "both": right_both / count, "confusion": confusion}


def report(results):
    lines = [
        f"On {results['count']} situations the student never saw:",
        f"  goal matches the teacher   {100 * results['goal']:6.2f}%",
        f"  mood matches the teacher   {100 * results['mood']:6.2f}%",
        f"  both match                 {100 * results['both']:6.2f}%",
        f"  answers that are valid     {100 * results['valid']:6.2f}%",
        "",
        "Which goals it gives (across) for each of the teacher's (down):",
        "  " + " " * 12 + "".join(f"{goal[:8]:>9}" for goal in schema.GOALS) + f"{'invalid':>9}",
    ]
    for goal in schema.GOALS:
        row = results["confusion"][goal]
        lines.append(f"  {goal:12}" + "".join(f"{row.get(column, 0):9}" for column in schema.GOALS + [INVALID]))
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--part", choices=["train", "val", "test"], default="test")
    args = parser.parse_args()

    model, extra = student.load(args.checkpoint)
    examples = dataset.split(dataset.read_examples(args.data))[args.part]
    print(f"{args.checkpoint.name}: size {extra.get('size', '?')}, {student.count_parameters(model):,} numbers")
    print(report(evaluate(model, Tokenizer(), examples)))


if __name__ == "__main__":
    main()
