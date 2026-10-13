"""The stage 2 teacher: a language model, asked through Ollama's local HTTP interface."""
import json
import time
import urllib.error
import urllib.request

import characters
import dataset
import schema


class TeacherError(Exception):
    """The teacher could not be reached, or stopped answering."""


class OllamaTeacher:
    def __init__(self, model="qwen3.5:4b", host="http://localhost:11434", temperature=0.0, timeout=120):
        self.model = model
        self.host = host.rstrip("/")
        self.temperature = temperature
        self.timeout = timeout
        self.seed = 1  # change it to get a different roll of the dice at temperatures above 0

    def ask(self, messages, answer_format):
        """Send one question; return the reply's text."""
        body = {
            "model": self.model,
            "messages": messages,
            "format": answer_format,   # the reply must have exactly this shape
            "think": False,            # with thinking on, one answer takes minutes
            "stream": False,
            "options": {"temperature": self.temperature, "seed": self.seed},
        }
        request = urllib.request.Request(f"{self.host}/api/chat", data=json.dumps(body).encode(),
                                         headers={"Content-Type": "application/json"})
        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as reply:
                return json.loads(reply.read())["message"]["content"]
        except (urllib.error.URLError, TimeoutError, OSError, KeyError, json.JSONDecodeError) as problem:
            raise TeacherError(f"no usable answer from {self.model} at {self.host}: {problem}. "
                               "Is Ollama running, and is the model pulled?") from None


def parse_answer(text, goals):
    """(goal, mood) if the reply is a goal on offer and a real mood; otherwise None."""
    try:
        reply = json.loads(text)
    except (json.JSONDecodeError, TypeError):
        return None
    if not isinstance(reply, dict):
        return None
    goal, mood = reply.get("goal"), reply.get("mood")
    if not isinstance(goal, str) or not isinstance(mood, str) or goal not in goals or mood not in schema.MOODS:
        return None
    return goal, mood


def label_one(state, teacher, tries=3):
    """Ask about one situation. None if the teacher gave nothing usable in `tries` goes."""
    goals = characters.available_goals(schema.parse_state(state))
    messages = characters.build_messages(state)
    answer_format = characters.answer_format(goals)
    for _ in range(tries):
        answer = parse_answer(teacher.ask(messages, answer_format), goals)
        if answer is not None:
            return answer
    return None


def label_many(states, path, teacher, log=None):
    """Label situations into a file, one line each, written as each answer arrives.

    Situations already in the file are skipped, so a stopped run carries on
    where it left off and no situation is ever labelled twice.
    """
    done = {state for state, _, _ in dataset.read_examples(path)} if path.exists() else set()
    summary = {"labelled": 0, "skipped": 0, "dropped": 0, "seconds": 0.0}
    began = time.time()
    with path.open("a") as out:
        for number, state in enumerate(states, start=1):
            if state in done:
                summary["skipped"] += 1
                continue
            answer = label_one(state, teacher)  # a TeacherError stops the run; the file keeps what it has
            if answer is None:
                summary["dropped"] += 1
            else:
                out.write(schema.format_example(state, *answer) + "\n")
                out.flush()
                done.add(state)
                summary["labelled"] += 1
            summary["seconds"] = time.time() - began
            if log and (number % 25 == 0 or number == len(states)):
                asked = summary["labelled"] + summary["dropped"]
                rate = asked / summary["seconds"] if summary["seconds"] > 0 else 0.0
                left = (len(states) - number) / rate / 60 if rate > 0 else 0.0
                log(f"{number}/{len(states)}  labelled {summary['labelled']}  skipped {summary['skipped']}  "
                    f"dropped {summary['dropped']}  {rate * 3600:.0f} an hour  about {left:.0f} min left")
    summary["seconds"] = time.time() - began
    return summary
