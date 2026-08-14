### BEFORE — probe order = rotation (as-built) — 12 launches x 3600 frames (60 s @ 1280x960)

| launch (start frame) | distinct routines | spawns | repeats | distinct palettes | probe done (frame) | opening routine | library span | families |
|---|---|---|---|---|---|---|---|---|
| 123456 | 7 | 8 | 1 | 5 | 2837 | 54 | 127 | 4 |
| 417022 | 6 | 6 | 0 | 4 | NOT DONE | 11 | 50 | 2 |
| 1039284 | 9 | 9 | 0 | 5 | NOT DONE | 69 | 126 | 4 |
| 1583421 | 7 | 8 | 1 | 4 | NOT DONE | 117 | 103 | 5 |
| 1996488 | 6 | 6 | 0 | 5 | NOT DONE | 66 | 110 | 3 |
| 2244532 | 6 | 6 | 0 | 5 | NOT DONE | 68 | 131 | 2 |
| 2699421 | 6 | 6 | 0 | 4 | NOT DONE | 101 | 29 | 3 |
| 2938103 | 6 | 6 | 0 | 4 | NOT DONE | 21 | 6 | 1 |
| 3157482 | 6 | 6 | 0 | 4 | NOT DONE | 117 | 41 | 3 |
| 3506611 | 4 | 5 | 1 | 4 | NOT DONE | 2 | 41 | 2 |
| 3812004 | 7 | 7 | 0 | 5 | NOT DONE | 186 | 48 | 3 |
| 4109337 | 8 | 8 | 0 | 4 | NOT DONE | 105 | 123 | 4 |

- distinct routines per launch: min 4 mean 6.5 max 9
- launches that repeat a routine inside 60 s: 3/12 (total repeats 3)
- library span of a launch's material: mean 78 of 201 patterns (min 6, max 131)
- generator families per launch (20-pattern batches, max 11): mean 3.0 (min 1, max 5)

**ROUTINE overlap across all 66 launch pairs**
- mean shared: 0.27   max shared: 2   pairs with 0 shared: 52/66
- mean Jaccard: 0.023   max Jaccard: 0.200
- worst pair: launch 417022 vs 2699421 — 2 shared, [6, 11]

**FAMILY overlap across all 66 launch pairs**
- mean shared: 0.91   max shared: 3   pairs with 0 shared: 24/66
- mean Jaccard: 0.197   max Jaccard: 0.750
- worst pair: launch 1583421 vs 3157482 — 3 shared, [4, 5, 6]

**PALETTE overlap across all 66 launch pairs**
- mean shared: 0.20   max shared: 2   pairs with 0 shared: 54/66
- mean Jaccard: 0.025   max Jaccard: 0.250
- worst pair: launch 1039284 vs 1996488 — 2 shared, [85, 92]

**Opening 15 s (first 900 frames) — the part a friend sees before deciding it's the same app twice**
| launch | routines in first 900 frames |
|---|---|
| 123456 | [54, 55, 62, 80] |
| 417022 | [8, 11, 198] |
| 1039284 | [69, 70, 116, 123] |
| 1583421 | [117, 118, 131] |
| 1996488 | [66, 74, 82] |
| 2244532 | [68, 77] |
| 2699421 | [101, 102, 114] |
| 2938103 | [15, 21, 164, 169] |
| 3157482 | [117, 123, 132] |
| 3506611 | [2, 173] |
| 3812004 | [156, 164, 186] |
| 4109337 | [105, 113, 123] |

- opening-set mean shared: 0.08, max 1, identical openings: 0
- distinct opening routines across all launches: 33
