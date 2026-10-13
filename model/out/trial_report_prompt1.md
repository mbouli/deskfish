# Teacher trial: qwen3.5:4b

210 situations, 30 of each species, each asked twice at temperature 0.3, thinking off.

## Speed

- 1812 answers an hour (1.99 seconds each).
- 12,000 labels would take about 6.6 hours.
- Dropped after three invalid answers: 0.

## Does it agree with itself?

On the 210 situations answered both times:

- same goal: **83.8%**
- same mood: **91.4%**
- same goal and mood: **81.9%**

This is the ceiling: a student cannot agree with the teacher more often than the teacher agrees with itself.

## Is there character?

The teacher picked the same goal as the seven rules **32.4%** of the time. Near 100% would mean it adds nothing; very low would mean it has stopped being sensible.

Goals chosen, by species (first pass):

| | wander | seek_food | rest | school | investigate | flee | hide |
|---|---|---|---|---|---|---|---|
| angelfish | 4 | 0 | 20 | 1 | 0 | 1 | 4 |
| bass | 2 | 3 | 19 | 2 | 0 | 1 | 3 |
| catfish | 0 | 0 | 29 | 0 | 0 | 1 | 0 |
| clownfish | 2 | 11 | 10 | 0 | 7 | 0 | 0 |
| goldfish | 0 | 13 | 9 | 6 | 0 | 0 | 2 |
| pufferfish | 3 | 2 | 3 | 2 | 0 | 10 | 10 |
| surgeonfish | 0 | 9 | 13 | 2 | 1 | 2 | 3 |

Moods chosen, by species (first pass):

| | calm | eager | wary | playful | sleepy | sulky |
|---|---|---|---|---|---|---|
| angelfish | 9 | 0 | 10 | 0 | 11 | 0 |
| bass | 8 | 2 | 10 | 0 | 10 | 0 |
| catfish | 10 | 0 | 8 | 0 | 9 | 3 |
| clownfish | 1 | 15 | 2 | 1 | 10 | 1 |
| goldfish | 1 | 12 | 9 | 1 | 7 | 0 |
| pufferfish | 0 | 0 | 17 | 0 | 1 | 12 |
| surgeonfish | 0 | 1 | 6 | 10 | 11 | 2 |

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

Teacher: `rest sleepy`; asked again: `rest sleepy`

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

Teacher: `rest wary`; asked again: `rest wary`

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

Teacher: `rest sulky`; asked again: `rest sulky`

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

Teacher: `rest sleepy`; asked again: `rest sleepy`

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

Teacher: `seek_food playful`; asked again: `seek_food playful`

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

Teacher: `rest calm`; asked again: `rest calm`

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

Teacher: `flee wary`; asked again: `flee wary`

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

Teacher: `seek_food eager`; asked again: `school playful`

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

Teacher: `rest sleepy`; asked again: `rest sleepy`

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

Teacher: `rest sulky`; asked again: `rest sulky`

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

Teacher: `rest wary`; asked again: `rest wary`

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

Teacher: `flee sulky`; asked again: `hide sulky`

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

Teacher: `rest sleepy`; asked again: `rest sleepy`

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

Teacher: `wander calm`; asked again: `rest calm`

**goldfish** (rules would say `hide`)

- Hunger: 6 out of 9 (hungry).
- Energy: 6 out of 9 (fine).
- Stress: 7 out of 9 (frightened).
- Your boldness is 5 out of 9 and your sociability is 8 out of 9.
- It is daytime.
- At the moment you are resting.
- Food: there is a pellet far away.
- Other fish: the nearest is far away.
- The keeper's finger: it is in the water, a little way off.
- Hiding place: the nearest is far away.

Teacher: `rest wary`; asked again: `seek_food wary`

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

Teacher: `investigate eager`; asked again: `investigate eager`

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

Teacher: `investigate wary`; asked again: `investigate wary`

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

Teacher: `seek_food playful`; asked again: `seek_food playful`

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

Teacher: `rest calm`; asked again: `rest calm`

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

Teacher: `school wary`; asked again: `wander wary`

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

Teacher: `investigate playful`; asked again: `investigate wary`
