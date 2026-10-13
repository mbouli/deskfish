# The model for the board: `deskfish_model.bin`

From `teacher_s.pt` (size s, 29,952 numbers), scored on 1202 unseen situations from `teacher.txt`.

| Stored as | File size | Goal | Mood | Both | Valid |
|---|---|---|---|---|---|
| as trained (32-bit) | 142,544 bytes | 90.02% | 89.52% | 85.86% | 100.00% |
| 8-bit | 30,522 bytes | 89.77% | 89.27% | 85.52% | 100.00% |
| 4-bit | 15,546 bytes | 84.28% | 84.11% | 79.62% | 100.00% |

Written at **8 bits**: 30,522 bytes.

```
On 1202 situations the student never saw:
  goal matches the teacher    89.77%
  mood matches the teacher    89.27%
  both match                  85.52%
  answers that are valid     100.00%

Which goals it gives (across) for each of the teacher's (down):
                 wander seek_foo     rest   school investig     flee     hide  invalid
  wander            108        4        5        4        1        1        4        0
  seek_food           4      125        4        5        1        1        2        0
  rest                5        4      272        3        2        8        8        0
  school              6        9        4       52        2        1        0        0
  investigate         0        2        3        1       87        2        0        0
  flee                0        0        7        0        2      311        2        0
  hide                2        3        6        0        0        5      124        0
```
