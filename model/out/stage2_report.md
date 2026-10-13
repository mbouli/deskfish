# Student sizes compared: teacher

Data: `model/out/teacher.txt`. 9588 situations to learn from, 1210 to watch during training, 1202 held back for these scores. 30 passes over the data.

Agreement with the teacher on situations the student never saw:

| Size | Numbers | Goal | Mood | Both | Valid | Training time |
|---|---|---|---|---|---|---|
| xs | 5,552 | 84.86% | 83.44% | 78.62% | 100.00% | 14 s |
| s (chosen) | 29,952 | 90.02% | 89.52% | 85.86% | 100.00% | 33 s |
| m | 109,056 | 89.68% | 90.52% | 86.77% | 100.00% | 63 s |
| l | 460,992 | 90.77% | 90.60% | 87.77% | 99.92% | 91 s |

Chosen: **s**, the smallest size within two points of the best goal agreement.

### Size xs

```
On 1202 situations the student never saw:
  goal matches the teacher    84.86%
  mood matches the teacher    83.44%
  both match                  78.62%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander            102        4       10        7        0        1        3        0
  seek_food          13       91        8       21        4        2        3        0
  rest                1       10      272        2        1        8        8        0
  school              7       20        5       40        2        0        0        0
  investigate         0        2        3        5       83        1        1        0
  flee                0        0        5        0        3      308        6        0
  hide                1        3        8        0        1        3      124        0
```

### Size s

```
On 1202 situations the student never saw:
  goal matches the teacher    90.02%
  mood matches the teacher    89.52%
  both match                  85.86%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander            109        4        5        3        1        1        4        0
  seek_food           4      124        4        5        2        1        2        0
  rest                5        3      273        3        2        8        8        0
  school              6        9        4       52        2        1        0        0
  investigate         0        1        3        1       88        2        0        0
  flee                0        0        6        0        2      312        2        0
  hide                2        3        6        0        0        5      124        0
```

### Size m

```
On 1202 situations the student never saw:
  goal matches the teacher    89.68%
  mood matches the teacher    90.52%
  both match                  86.77%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander            109        6        6        1        1        1        3        0
  seek_food           8      122        2        6        1        1        2        0
  rest                7        4      273        3        2        4        9        0
  school              6        4        4       59        1        0        0        0
  investigate         1        3        4        1       85        1        0        0
  flee                0        3        4        0        3      309        3        0
  hide                2        5        4        0        0        8      121        0
```

### Size l

```
On 1202 situations the student never saw:
  goal matches the teacher    90.77%
  mood matches the teacher    90.60%
  both match                  87.77%
  answers that are valid      99.92%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander            109        6        4        2        1        2        3        0
  seek_food           4      123        3        8        1        1        1        1
  rest                6        3      275        3        0        8        7        0
  school              2        5        3       63        1        0        0        0
  investigate         1        3        5        0       84        2        0        0
  flee                1        1        4        1        1      312        2        0
  hide                1        7        5        0        0        2      125        0
```
