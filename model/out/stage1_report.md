# Student sizes compared: rules

Data: `model/out/rules.txt`. 74941 situations to learn from, 9338 to watch during training, 9348 held back for these scores. 20 passes over the data.

Agreement with the teacher on situations the student never saw:

| Size | Numbers | Goal | Mood | Both | Valid | Training time |
|---|---|---|---|---|---|---|
| xs (chosen) | 5,552 | 100.00% | 100.00% | 100.00% | 100.00% | 80 s |
| s | 29,952 | 100.00% | 100.00% | 100.00% | 100.00% | 187 s |
| m | 109,056 | 99.97% | 100.00% | 99.97% | 100.00% | 357 s |
| l | 460,992 | 100.00% | 100.00% | 100.00% | 100.00% | 589 s |
| xl | 811,264 | 100.00% | 100.00% | 100.00% | 100.00% | 706 s |

Chosen: **xs**, the smallest size within two points of the best goal agreement.

### Size xs

```
On 9348 situations the student never saw:
  goal matches the teacher   100.00%
  mood matches the teacher   100.00%
  both match                 100.00%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander           1844        0        0        0        0        0        0        0
  seek_food           0     1074        0        0        0        0        0        0
  rest                0        0     1573        0        0        0        0        0
  school              0        0        0      459        0        0        0        0
  investigate         0        0        0        0     1533        0        0        0
  flee                0        0        0        0        0      905        0        0
  hide                0        0        0        0        0        0     1960        0
```

### Size s

```
On 9348 situations the student never saw:
  goal matches the teacher   100.00%
  mood matches the teacher   100.00%
  both match                 100.00%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander           1844        0        0        0        0        0        0        0
  seek_food           0     1074        0        0        0        0        0        0
  rest                0        0     1573        0        0        0        0        0
  school              0        0        0      459        0        0        0        0
  investigate         0        0        0        0     1533        0        0        0
  flee                0        0        0        0        0      905        0        0
  hide                0        0        0        0        0        0     1960        0
```

### Size m

```
On 9348 situations the student never saw:
  goal matches the teacher    99.97%
  mood matches the teacher   100.00%
  both match                  99.97%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander           1842        0        0        2        0        0        0        0
  seek_food           0     1074        0        0        0        0        0        0
  rest                0        0     1573        0        0        0        0        0
  school              1        0        0      458        0        0        0        0
  investigate         0        0        0        0     1533        0        0        0
  flee                0        0        0        0        0      905        0        0
  hide                0        0        0        0        0        0     1960        0
```

### Size l

```
On 9348 situations the student never saw:
  goal matches the teacher   100.00%
  mood matches the teacher   100.00%
  both match                 100.00%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander           1844        0        0        0        0        0        0        0
  seek_food           0     1074        0        0        0        0        0        0
  rest                0        0     1573        0        0        0        0        0
  school              0        0        0      459        0        0        0        0
  investigate         0        0        0        0     1533        0        0        0
  flee                0        0        0        0        0      905        0        0
  hide                0        0        0        0        0        0     1960        0
```

### Size xl

```
On 9348 situations the student never saw:
  goal matches the teacher   100.00%
  mood matches the teacher   100.00%
  both match                 100.00%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander           1844        0        0        0        0        0        0        0
  seek_food           0     1074        0        0        0        0        0        0
  rest                0        0     1573        0        0        0        0        0
  school              0        0        0      459        0        0        0        0
  investigate         0        0        0        0     1533        0        0        0
  flee                0        0        0        0        0      905        0        0
  hide                0        0        0        0        0        0     1960        0
```
