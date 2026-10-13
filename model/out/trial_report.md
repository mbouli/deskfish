# Teacher trial: qwen3.5:4b

210 situations, 30 of each species, each asked twice at temperature 0.3, thinking off.

## Speed

- 2115 answers an hour (1.70 seconds each).
- 12,000 labels would take about 5.7 hours.
- Dropped after three invalid answers: 0.

## Does it agree with itself?

On the 210 situations answered both times:

- same goal: **99.0%**
- same mood: **99.5%**
- same goal and mood: **99.0%**

This is the ceiling: a student cannot agree with the teacher more often than the teacher agrees with itself.

## Is there character?

The teacher picked the same goal as the seven rules **48.1%** of the time. Near 100% would mean it adds nothing; very low would mean it has stopped being sensible.

Goals chosen, by species (first pass):

| | wander | seek_food | rest | school | investigate | flee | hide |
|---|---|---|---|---|---|---|---|
| angelfish | 7 | 0 | 13 | 2 | 2 | 4 | 2 |
| bass | 2 | 3 | 14 | 0 | 3 | 5 | 3 |
| catfish | 3 | 1 | 17 | 0 | 0 | 3 | 6 |
| clownfish | 5 | 3 | 12 | 2 | 4 | 4 | 0 |
| goldfish | 0 | 6 | 7 | 5 | 2 | 9 | 1 |
| pufferfish | 7 | 1 | 3 | 0 | 0 | 11 | 8 |
| surgeonfish | 0 | 2 | 9 | 8 | 2 | 5 | 4 |

Moods chosen, by species (first pass):

| | calm | eager | wary | playful | sleepy | sulky |
|---|---|---|---|---|---|---|
| angelfish | 8 | 0 | 10 | 0 | 9 | 3 |
| bass | 6 | 0 | 10 | 0 | 14 | 0 |
| catfish | 6 | 1 | 10 | 0 | 13 | 0 |
| clownfish | 5 | 7 | 5 | 1 | 12 | 0 |
| goldfish | 5 | 7 | 10 | 1 | 7 | 0 |
| pufferfish | 3 | 0 | 6 | 0 | 1 | 20 |
| surgeonfish | 0 | 2 | 9 | 10 | 9 | 0 |

## A sample to read

**clownfish** (rules would say `rest`)

- Hunger: 3 out of 9 (not hungry).
- Energy: 7 out of 9 (lively).
- Stress: 0 out of 9 (relaxed).
- Your boldness is 9 out of 9 and your sociability is 6 out of 9.
- It is night.
- At the moment you are resting.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is not in the water.
- Hiding place: the nearest is close by.

Teacher: `rest sleepy`; asked again: `rest sleepy`

**catfish** (rules would say `hide`)

- Hunger: 4 out of 9 (peckish).
- Energy: 3 out of 9 (tired).
- Stress: 7 out of 9 (frightened).
- Your boldness is 5 out of 9 and your sociability is 4 out of 9.
- It is daytime.
- At the moment you are hiding.
- Food: there is a pellet far away.
- Other fish: the nearest is far away.
- The keeper's finger: it is in the water, far away.
- Hiding place: the nearest is close by.

Teacher: `hide wary`; asked again: `hide wary`

**surgeonfish** (rules would say `seek_food`)

- Hunger: 7 out of 9 (very hungry).
- Energy: 5 out of 9 (fine).
- Stress: 0 out of 9 (relaxed).
- Your boldness is 5 out of 9 and your sociability is 8 out of 9.
- It is daytime.
- At the moment you are looking at the keeper's finger.
- Food: there is a pellet far away.
- Other fish: the nearest is close by.
- The keeper's finger: it is not in the water.
- Hiding place: the nearest is a little way off.

Teacher: `seek_food eager`; asked again: `seek_food eager`

**bass** (rules would say `hide`)

- Hunger: 4 out of 9 (peckish).
- Energy: 1 out of 9 (exhausted).
- Stress: 8 out of 9 (very frightened).
- Your boldness is 6 out of 9 and your sociability is 3 out of 9.
- It is night.
- At the moment you are looking at the keeper's finger.
- Food: there is no food in the water.
- Other fish: the nearest is a little way off.
- The keeper's finger: it is in the water, far away.
- Hiding place: the nearest is a little way off.

Teacher: `flee wary`; asked again: `flee wary`

**catfish** (rules would say `hide`)

- Hunger: 3 out of 9 (not hungry).
- Energy: 4 out of 9 (a bit tired).
- Stress: 7 out of 9 (frightened).
- Your boldness is 3 out of 9 and your sociability is 4 out of 9.
- It is night.
- At the moment you are going after food.
- Food: there is a pellet close by.
- Other fish: the nearest is far away.
- The keeper's finger: it is in the water, a little way off. It has just landed right beside you and given you a fright.
- Hiding place: the nearest is a little way off.

Teacher: `flee wary`; asked again: `flee wary`

**goldfish** (rules would say `flee`)

- Hunger: 4 out of 9 (peckish).
- Energy: 0 out of 9 (exhausted).
- Stress: 9 out of 9 (terrified).
- Your boldness is 6 out of 9 and your sociability is 7 out of 9.
- It is night.
- At the moment you are resting.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, close by. It has just landed right beside you and given you a fright.
- Hiding place: the nearest is close by.

Teacher: `flee wary`; asked again: `flee wary`

**surgeonfish** (rules would say `wander`)

- Hunger: 4 out of 9 (peckish).
- Energy: 5 out of 9 (fine).
- Stress: 0 out of 9 (relaxed).
- Your boldness is 5 out of 9 and your sociability is 6 out of 9.
- It is daytime.
- At the moment you are wandering about.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, a little way off.
- Hiding place: the nearest is a little way off.

Teacher: `investigate playful`; asked again: `investigate playful`

**bass** (rules would say `investigate`)

- Hunger: 2 out of 9 (not hungry).
- Energy: 7 out of 9 (lively).
- Stress: 2 out of 9 (relaxed).
- Your boldness is 6 out of 9 and your sociability is 3 out of 9.
- It is night.
- At the moment you are resting.
- Food: there is a pellet close by.
- Other fish: the nearest is a little way off.
- The keeper's finger: it is in the water, a little way off.
- Hiding place: the nearest is close by.

Teacher: `rest sleepy`; asked again: `rest sleepy`

**pufferfish** (rules would say `flee`)

- Hunger: 8 out of 9 (very hungry).
- Energy: 7 out of 9 (lively).
- Stress: 9 out of 9 (terrified).
- Your boldness is 3 out of 9 and your sociability is 2 out of 9.
- It is night.
- At the moment you are swimming over to another fish.
- Food: there is a pellet far away.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, close by.
- Hiding place: the nearest is a little way off.

Teacher: `flee wary`; asked again: `flee wary`

**pufferfish** (rules would say `flee`)

- Hunger: 5 out of 9 (hungry).
- Energy: 8 out of 9 (lively).
- Stress: 2 out of 9 (relaxed).
- Your boldness is 2 out of 9 and your sociability is 3 out of 9.
- It is night.
- At the moment you are going after food.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, close by. It has just landed right beside you and given you a fright.
- Hiding place: there is nowhere to hide.

Teacher: `flee sulky`; asked again: `flee sulky`

**goldfish** (rules would say `wander`)

- Hunger: 4 out of 9 (peckish).
- Energy: 5 out of 9 (fine).
- Stress: 3 out of 9 (a little uneasy).
- Your boldness is 5 out of 9 and your sociability is 7 out of 9.
- It is daytime.
- At the moment you are swimming over to another fish.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is not in the water.
- Hiding place: the nearest is close by.

Teacher: `seek_food eager`; asked again: `seek_food eager`

**angelfish** (rules would say `rest`)

- Hunger: 1 out of 9 (full).
- Energy: 4 out of 9 (a bit tired).
- Stress: 4 out of 9 (uneasy).
- Your boldness is 4 out of 9 and your sociability is 5 out of 9.
- It is daytime.
- At the moment you are resting.
- Food: there is no food in the water.
- Other fish: the nearest is close by.
- The keeper's finger: it is not in the water.
- Hiding place: the nearest is close by.

Teacher: `rest calm`; asked again: `rest calm`

**pufferfish** (rules would say `rest`)

- Hunger: 4 out of 9 (peckish).
- Energy: 4 out of 9 (a bit tired).
- Stress: 0 out of 9 (relaxed).
- Your boldness is 2 out of 9 and your sociability is 4 out of 9.
- It is daytime.
- At the moment you are resting.
- Food: there is no food in the water.
- Other fish: the nearest is a little way off.
- The keeper's finger: it is not in the water.
- Hiding place: the nearest is close by.

Teacher: `rest sleepy`; asked again: `rest sleepy`

**angelfish** (rules would say `hide`)

- Hunger: 2 out of 9 (not hungry).
- Energy: 6 out of 9 (fine).
- Stress: 6 out of 9 (frightened).
- Your boldness is 4 out of 9 and your sociability is 5 out of 9.
- It is daytime.
- At the moment you are hiding.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is not in the water.
- Hiding place: the nearest is close by.

Teacher: `hide wary`; asked again: `hide wary`

**catfish** (rules would say `hide`)

- Hunger: 5 out of 9 (hungry).
- Energy: 5 out of 9 (fine).
- Stress: 7 out of 9 (frightened).
- Your boldness is 3 out of 9 and your sociability is 3 out of 9.
- It is daytime.
- At the moment you are hiding.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, far away.
- Hiding place: the nearest is close by.

Teacher: `hide wary`; asked again: `hide wary`

**pufferfish** (rules would say `flee`)

- Hunger: 4 out of 9 (peckish).
- Energy: 0 out of 9 (exhausted).
- Stress: 9 out of 9 (terrified).
- Your boldness is 1 out of 9 and your sociability is 2 out of 9.
- It is night.
- At the moment you are hiding.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, close by. It has just landed right beside you and given you a fright.
- Hiding place: the nearest is close by.

Teacher: `flee sulky`; asked again: `flee sulky`

**catfish** (rules would say `flee`)

- Hunger: 0 out of 9 (full).
- Energy: 6 out of 9 (fine).
- Stress: 9 out of 9 (terrified).
- Your boldness is 3 out of 9 and your sociability is 3 out of 9.
- It is night.
- At the moment you are hiding.
- Food: there is a pellet a little way off.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, close by.
- Hiding place: the nearest is close by.

Teacher: `hide wary`; asked again: `hide wary`

**angelfish** (rules would say `school`)

- Hunger: 0 out of 9 (full).
- Energy: 5 out of 9 (fine).
- Stress: 0 out of 9 (relaxed).
- Your boldness is 5 out of 9 and your sociability is 5 out of 9.
- It is daytime.
- At the moment you are wandering about.
- Food: there is a pellet close by.
- Other fish: the nearest is a little way off.
- The keeper's finger: it is in the water, far away.
- Hiding place: the nearest is close by.

Teacher: `wander calm`; asked again: `wander calm`

**goldfish** (rules would say `hide`)

- Hunger: 6 out of 9 (hungry).
- Energy: 6 out of 9 (fine).
- Stress: 7 out of 9 (frightened).
- Your boldness is 5 out of 9 and your sociability is 8 out of 9.
- It is daytime.
- At the moment you are resting.
- Food: there is a pellet far away.
- Other fish: the nearest is far away. You are away from the others and fancy some company.
- The keeper's finger: it is in the water, a little way off.
- Hiding place: the nearest is far away.

Teacher: `flee wary`; asked again: `flee wary`

**catfish** (rules would say `wander`)

- Hunger: 0 out of 9 (full).
- Energy: 3 out of 9 (tired).
- Stress: 5 out of 9 (nervous).
- Your boldness is 5 out of 9 and your sociability is 3 out of 9.
- It is daytime.
- At the moment you are wandering about.
- Food: there is a pellet a little way off.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, close by.
- Hiding place: the nearest is close by.

Teacher: `rest sleepy`; asked again: `rest sleepy`

**clownfish** (rules would say `investigate`)

- Hunger: 4 out of 9 (peckish).
- Energy: 6 out of 9 (fine).
- Stress: 0 out of 9 (relaxed).
- Your boldness is 7 out of 9 and your sociability is 5 out of 9.
- It is night.
- At the moment you are looking at the keeper's finger.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, a little way off.
- Hiding place: the nearest is a little way off.

Teacher: `investigate playful`; asked again: `investigate playful`

**surgeonfish** (rules would say `wander`)

- Hunger: 2 out of 9 (not hungry).
- Energy: 5 out of 9 (fine).
- Stress: 1 out of 9 (relaxed).
- Your boldness is 5 out of 9 and your sociability is 8 out of 9.
- It is daytime.
- At the moment you are swimming over to another fish.
- Food: there is a pellet far away.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, far away.
- Hiding place: the nearest is close by.

Teacher: `school playful`; asked again: `school playful`

**clownfish** (rules would say `investigate`)

- Hunger: 3 out of 9 (not hungry).
- Energy: 9 out of 9 (full of energy).
- Stress: 4 out of 9 (uneasy).
- Your boldness is 9 out of 9 and your sociability is 5 out of 9.
- It is daytime.
- At the moment you are fleeing from the keeper's finger.
- Food: there is a pellet close by.
- Other fish: there are none about.
- The keeper's finger: it is in the water, a little way off. It has just landed right beside you and given you a fright.
- Hiding place: there is nowhere to hide.

Teacher: `flee wary`; asked again: `flee wary`

**surgeonfish** (rules would say `investigate`)

- Hunger: 4 out of 9 (peckish).
- Energy: 3 out of 9 (tired).
- Stress: 0 out of 9 (relaxed).
- Your boldness is 7 out of 9 and your sociability is 7 out of 9.
- It is daytime.
- At the moment you are looking at the keeper's finger.
- Food: there is a pellet a little way off.
- Other fish: the nearest is close by.
- The keeper's finger: it is in the water, close by.
- Hiding place: the nearest is close by.

Teacher: `investigate playful`; asked again: `investigate playful`

**catfish** (rules would say `wander`)

- Hunger: 4 out of 9 (peckish).
- Energy: 7 out of 9 (lively).
- Stress: 1 out of 9 (relaxed).
- Your boldness is 4 out of 9 and your sociability is 2 out of 9.
- It is daytime.
- At the moment you are wandering about.
- Food: there is a pellet a little way off.
- Other fish: the nearest is a little way off.
- The keeper's finger: it is not in the water.
- Hiding place: the nearest is close by.

Teacher: `wander calm`; asked again: `wander calm`

**pufferfish** (rules would say `wander`)

- Hunger: 1 out of 9 (full).
- Energy: 3 out of 9 (tired).
- Stress: 3 out of 9 (a little uneasy).
- Your boldness is 2 out of 9 and your sociability is 4 out of 9.
- It is daytime.
- At the moment you are wandering about.
- Food: there is a pellet a little way off.
- Other fish: the nearest is a little way off.
- The keeper's finger: it is in the water, far away.
- Hiding place: the nearest is a little way off.

Teacher: `wander sulky`; asked again: `wander sulky`

**surgeonfish** (rules would say `hide`)

- Hunger: 2 out of 9 (not hungry).
- Energy: 0 out of 9 (exhausted).
- Stress: 6 out of 9 (frightened).
- Your boldness is 5 out of 9 and your sociability is 6 out of 9.
- It is night.
- At the moment you are hiding.
- Food: there is a pellet close by.
- Other fish: the nearest is close by.
- The keeper's finger: it is not in the water.
- Hiding place: the nearest is close by.

Teacher: `rest sleepy`; asked again: `rest sleepy`

**clownfish** (rules would say `flee`)

- Hunger: 4 out of 9 (peckish).
- Energy: 1 out of 9 (exhausted).
- Stress: 9 out of 9 (terrified).
- Your boldness is 9 out of 9 and your sociability is 5 out of 9.
- It is daytime.
- At the moment you are resting.
- Food: there is no food in the water.
- Other fish: the nearest is a little way off.
- The keeper's finger: it is in the water, close by. It has just landed right beside you and given you a fright.
- Hiding place: the nearest is a little way off.

Teacher: `flee wary`; asked again: `flee wary`
