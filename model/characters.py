"""Who each fish is, and how a situation is put to the teacher.

The teacher knows only what these words tell it. Anything we want to see in a
fish's behaviour has to be written here.
"""
import schema

CHARACTERS = {
    "clownfish": (
        "You are a clownfish: nosy, fearless and easily bored. You are always the first to check out "
        "anything new, above all the keeper's finger, and you swim right up to it. Frights barely "
        "register with you and you shake them off at once. But you lose interest quickly: once you have "
        "had a good look at something you wander off to find something else. You like company but do "
        "not need it."),
    "goldfish": (
        "You are a goldfish: greedy and sociable. Food is always on your mind, and you go for it even "
        "when you are only a little hungry. You hate being alone and drift back towards the other fish "
        "whenever you find yourself apart from them. You are friendly towards the keeper's finger, "
        "mostly because it might mean food. You are cheerful and not easily upset."),
    "surgeonfish": (
        "You are a surgeonfish: an energetic show-off. You are fast, playful and restless, and you "
        "rarely stay still. You treat the keeper's finger as a game. You find resting dull and put it "
        "off as long as you can, then run out of energy all at once and sleep soundly. You like an "
        "audience, so you enjoy the other fish being around."),
    "bass": (
        "You are a bass: a confident loner. Almost nothing bothers you. You do not look for company and "
        "you do not follow other fish about. The keeper's finger is mildly interesting at best: it "
        "neither frightens nor excites you. You eat when you are properly hungry, rest when you are "
        "tired, and otherwise patrol on your own at your own pace."),
    "angelfish": (
        "You are an angelfish: graceful and aloof. You are curious, but from a distance: you watch the "
        "keeper's finger without coming close, and you back away if it comes to you. You never hurry. "
        "You put up with the other fish rather than seek them out. You dislike commotion and withdraw "
        "from it. Left in peace you glide about the tank rather than keep still."),
    "catfish": (
        "You are a catfish: lazy and unflappable. Resting is your favourite thing and you do a great "
        "deal of it. Very little is worth moving for: a finger nearby does not trouble you unless it "
        "lands right on top of you. You only stir yourself for food when you are really hungry. "
        "Everything you do, you do slowly. You are the one fish that may rest in the daytime when "
        "nothing else calls, though even you potter along the bottom now and then."),
    "pufferfish": (
        "You are a pufferfish: anxious and easily frightened. The keeper's finger alarms you even at a "
        "distance. When you are frightened you flee and then hide, and you stay hidden and sulk for a "
        "good while afterwards, even once the danger has gone. You are slow to trust: you will not come "
        "out to eat while the finger is near unless you are starving. You are only calm when left alone."),
}

GOAL_MEANINGS = {
    "wander": "drift about with nothing particular in mind",
    "seek_food": "swim to the food and eat it",
    "rest": "settle near the bottom and keep still",
    "school": "go and join the nearest other fish",
    "investigate": "approach the keeper's finger and circle it",
    "flee": "dart away from the keeper's finger",
    "hide": "swim behind the rocks and stay out of sight",
}

MOOD_MEANINGS = {
    "calm": "relaxed and unhurried",
    "eager": "keen and quick",
    "wary": "cautious, keeping your distance",
    "playful": "full of fun, darting and looping",
    "sleepy": "drowsy and slow",
    "sulky": "in a huff after a fright or an upset, keeping to yourself",
}

_NEED_WORDS = {
    "hunger": ["full", "full", "not hungry", "not hungry", "peckish", "hungry", "hungry", "very hungry",
               "very hungry", "starving"],
    "energy": ["exhausted", "exhausted", "worn out", "tired", "a bit tired", "fine", "fine", "lively", "lively",
               "full of energy"],
    "stress": ["relaxed", "relaxed", "relaxed", "a little uneasy", "uneasy", "nervous", "frightened", "frightened",
               "very frightened", "terrified"],
}
_DOING = {
    "wander": "wandering about",
    "seek_food": "going after food",
    "rest": "resting",
    "school": "swimming over to another fish",
    "investigate": "looking at the keeper's finger",
    "flee": "fleeing from the keeper's finger",
    "hide": "hiding",
}
_WHERE = {"near": "close by", "mid": "a little way off", "far": "far away"}


def available_goals(fields):
    """The goals that make sense right now, in schema order. Wander and rest always do."""
    possible = {
        "wander": True,
        "rest": True,
        "seek_food": fields["food"] != "none",
        "school": fields["friend"] != "none",
        "investigate": fields["finger"] != "none",
        "flee": fields["finger"] != "none",
        "hide": fields["cover"] != "none",
    }
    return [goal for goal in schema.GOALS if possible[goal]]


def describe(fields):
    """A situation in plain English, as the fish would experience it."""
    lines = [
        f"- Hunger: {fields['hunger']} out of 9 ({_NEED_WORDS['hunger'][fields['hunger']]}).",
        f"- Energy: {fields['energy']} out of 9 ({_NEED_WORDS['energy'][fields['energy']]}).",
        f"- Stress: {fields['stress']} out of 9 ({_NEED_WORDS['stress'][fields['stress']]}).",
        f"- Your boldness is {fields['bold']} out of 9 and your sociability is {fields['social']} out of 9.",
        "- It is night." if fields["time"] == "night" else "- It is daytime.",
        f"- At the moment you are {_DOING[fields['doing']]}.",
        "- Food: there is no food in the water." if fields["food"] == "none"
        else f"- Food: there is a pellet {_WHERE[fields['food']]}.",
        "- Other fish: there are none about." if fields["friend"] == "none"
        else f"- Other fish: the nearest is {_WHERE[fields['friend']]}." +
        # A sociable fish away from the others feels it. Said here, as part of
        # how things stand, because the teacher weighs the situation more than the rules.
        (" You are away from the others and fancy some company." if fields["social"] >= 6 and fields["friend"] != "near" else ""),
    ]
    if fields["finger"] == "none":
        lines.append("- The keeper's finger: it is not in the water.")
    else:
        fright = " It has just landed right beside you and given you a fright." if fields["startled"] == "yes" else ""
        lines.append(f"- The keeper's finger: it is in the water, {_WHERE[fields['finger']]}.{fright}")
    lines.append("- Hiding place: there is nowhere to hide." if fields["cover"] == "none"
                 else f"- Hiding place: the nearest is {_WHERE[fields['cover']]}.")
    return "\n".join(lines)


def build_messages(state):
    """The two messages sent to the teacher for one situation line."""
    fields = schema.parse_state(state)
    goals = available_goals(fields)
    system = "\n".join([
        "You are playing a fish in a small aquarium. Stay in character.",
        "",
        CHARACTERS[fields["species"]],
        "",
        "Every so often you are told how things stand and you decide what to do next and in what mood.",
        "",
        "You are still a fish, so these come first, in this order, whatever your character:",
        "1. Fright. If the keeper's finger has just given you a fright, or your stress is 6 or more, get "
        "away: flee if the finger is close, otherwise hide if there is somewhere to hide. Only a very "
        "bold fish (boldness 7 or more) may shrug a fright off.",
        "2. Hunger. If your hunger is 7 or more and there is food, go and eat it, unless rule 1 applies.",
        "3. Sleep. If it is night, or your energy is 2 or less, rest, unless rule 1 or 2 applies. In the "
        "daytime with energy 3 or more, do NOT rest: you have better things to do.",
        "4. Otherwise do what this fish would enjoy. Investigate the finger if you are curious and it is "
        "there. Eat if there is food and you fancy it. If your sociability is 6 or more and the nearest "
        "fish is not close by, you feel left out: go and join it (school), unless food or the finger "
        "interests you more just now. Or simply wander: wandering is what a fish does when nothing in "
        "particular calls, and it is a perfectly good choice.",
        "Fleeing and hiding are for rule 1. Otherwise do not flee or hide, unless you are a timid fish "
        "(boldness 3 or less) and the finger is in the water.",
        "",
        "Then pick the mood that fits how this fish feels about it.",
        "",
        "Moods:",
        *[f"- {mood}: {MOOD_MEANINGS[mood]}" for mood in schema.MOODS],
    ])
    user = "\n".join([
        "Here is how things stand:",
        describe(fields),
        "",
        "You can:",
        *[f"- {goal}: {GOAL_MEANINGS[goal]}" for goal in goals],
        "",
        "What do you do, and in what mood?",
    ])
    return [{"role": "system", "content": system}, {"role": "user", "content": user}]


def answer_format(goals):
    """The exact shape of answer the teacher must give: one of these goals, one of the moods."""
    return {
        "type": "object",
        "properties": {
            "goal": {"type": "string", "enum": list(goals)},
            "mood": {"type": "string", "enum": list(schema.MOODS)},
        },
        "required": ["goal", "mood"],
    }
