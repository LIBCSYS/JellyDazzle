### AFTER — probe order = coprime stride — 12 launches x 3600 frames (60 s @ 1280x960)

| launch (start frame) | distinct routines | spawns | repeats | distinct palettes | probe done (frame) | opening routine | library span | families |
|---|---|---|---|---|---|---|---|---|
| 123456 | 6 | 7 | 1 | 5 | 3024 | 66 | 143 | 4 |
| 417022 | 8 | 8 | 0 | 4 | NOT DONE | 48 | 112 | 3 |
| 1039284 | 8 | 8 | 0 | 5 | 3478 | 211 | 108 | 5 |
| 1583421 | 6 | 6 | 0 | 4 | NOT DONE | 115 | 78 | 4 |
| 1996488 | 5 | 5 | 0 | 5 | NOT DONE | 163 | 109 | 2 |
| 2244532 | 5 | 6 | 1 | 5 | NOT DONE | 65 | 96 | 3 |
| 2699421 | 7 | 7 | 0 | 4 | NOT DONE | 8 | 86 | 3 |
| 2938103 | 6 | 6 | 0 | 4 | NOT DONE | 86 | 72 | 3 |
| 3157482 | 4 | 5 | 1 | 4 | NOT DONE | 108 | 131 | 4 |
| 3506611 | 4 | 4 | 0 | 4 | NOT DONE | 183 | 121 | 3 |
| 3812004 | 8 | 8 | 0 | 5 | 3410 | 84 | 184 | 6 |
| 4109337 | 8 | 8 | 0 | 4 | 3255 | 40 | 117 | 6 |

- distinct routines per launch: min 4 mean 6.2 max 8
- launches that repeat a routine inside 60 s: 3/12 (total repeats 3)
- library span of a launch's material: mean 113 of 201 patterns (min 72, max 184)
- generator families per launch (20-pattern batches, max 11): mean 3.8 (min 2, max 6)

**ROUTINE overlap across all 66 launch pairs**
- mean shared: 0.26   max shared: 2   pairs with 0 shared: 50/66
- mean Jaccard: 0.023   max Jaccard: 0.182
- worst pair: launch 2699421 vs 2938103 — 2 shared, [18, 22]

**FAMILY overlap across all 66 launch pairs**
- mean shared: 1.65   max shared: 3   pairs with 0 shared: 4/66
- mean Jaccard: 0.302   max Jaccard: 0.750
- worst pair: launch 123456 vs 2244532 — 3 shared, [1, 2, 6]

**PALETTE overlap across all 66 launch pairs**
- mean shared: 0.20   max shared: 2   pairs with 0 shared: 54/66
- mean Jaccard: 0.025   max Jaccard: 0.250
- worst pair: launch 1039284 vs 1996488 — 2 shared, [85, 92]

**Opening 15 s (first 900 frames) — the part a friend sees before deciding it's the same app twice**
| launch | routines in first 900 frames |
|---|---|
| 123456 | [54, 66, 190] |
| 417022 | [48, 55, 159] |
| 1039284 | [119, 211, 224] |
| 1583421 | [115, 153, 174] |
| 1996488 | [55, 163] |
| 2244532 | [62, 65, 154] |
| 2699421 | [8, 63] |
| 2938103 | [86, 133] |
| 3157482 | [91, 108, 129] |
| 3506611 | [183] |
| 3812004 | [84, 98, 156, 198] |
| 4109337 | [40, 55, 109] |

- opening-set mean shared: 0.05, max 1, identical openings: 0
- distinct opening routines across all launches: 30
