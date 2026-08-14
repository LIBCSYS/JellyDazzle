# Entropy QC — v2.1 scheduler

**Date:** 2026-08-14 · **Snapshot under test:** `bridge.c` md5 `fec8813e87cc1c49e97010760965587c`,
`registry.c` = 201 patterns (225 routines total = 24 asm + 201), `palette.bin` = 120 schemes.

> The repo was being edited by other agents throughout this QC. All numbers below are pinned to the
> snapshot above (copied to `/tmp/dz_qc`). `bridge.c` moved to `6324d285…` while the report was being
> written — re-run the harness before trusting these figures against a newer tree.

---

## 1. Verdict up front

| Guarantee J asked for | Status |
|---|---|
| No routine repeats until the pool is exhausted | **MET** — structurally, by the shuffled bags |
| Doubled routine library actually reaches the screen | **NOT MET** — 11 routines never play; accent/spark die after ~40 min |
| Doubled/varied palettes | **MET** — and comfortably |
| Layered composition (1, +3 s, +1 s, building up) | **NOT MET in steady state** — 1 layer 74.5 % of the time |
| Layer combinations essentially never recur | **MET** — 0 recurrences in 11.4 h |
| Launch seed randomises the starting point | **MET** (fixed during this QC by the new `g_run`) |
| Relaxing / no strobe (mean frame delta < 8) | **MET** — mean 0.76, one frame at 8.78 in 11 400 |

The bags do exactly what `scheduler.md` promised. The **compositor does not**, and the cause is a
single defect in `bag_draw` (finding **E-1**) that also silently strands 11 routines.

---

## 2. Method

Three independent measurements, cross-validated:

1. **`/tmp/jd_stats.c`** — `#include "bridge.c"` so its statics are reachable, then run the real
   startup probe against the real 225-routine library at the real framebuffer size (1280×960) and
   dump the measured `jd_stat` table + `g_pfeat` palette features as CSV.
2. **`/tmp/jd_sched_sim.c`** — same include trick, but drives the **real** `sched_tick` /
   `bag_draw` / `admissible` with a frozen stat table (`JD_STATTBL=`) so it is deterministic. This
   is the ground truth.
3. **`/tmp/jd_entropy.py`** — a hand port of the scheduling logic, used for the long simulations.

**Port fidelity:** the Python port reproduces the C scheduler **exactly** — 2000/2000 spawn events
identical in frame, slot and routine, over 2 462 055 frames (11.4 h of playback). Two porting
mismatches were found and fixed against the C during this process, both of which turned out to be
real behaviours of `bridge.c` worth knowing about (`g_gpostponed`, and the `g_exclude` leak, E-4).

Simulated horizon: **2000 spawn events = 11.4 h of playback**, versus v2.0's baseline of 300
segments = 2.8 h. Where a like-for-like number is needed, the **base layer** is the comparable
quantity: v2.0 showed exactly one routine at a time, so "v2.0 segment" ↔ "v2.1 base change".

---

## 3. Comparison table

| Metric | v2.0 baseline | v2.1 as-built | Change |
|---|---|---|---|
| Routine pool | 124 | 225 | +81 % |
| Palette schemes | 30 | 120 | +300 % |
| **Selection method** | `mix32(seg) % 124` (with replacement) | shuffled bag per role, no replacement | — |
| Distinct routines in 300 scheduling events | 110 of 124 (89 %) | 100 of 106 base-eligible (94 %) | better |
| Usage spread over 300 events | 1 … 8 (8× imbalance) | 1 … 4 (4× imbalance) | better |
| **Events before the first repeat** | **13** | **85** | **6.5×** |
| **Min gap between repeats** | **2 segments = 1.1 min** | **12 base changes = 5.2 min** | **4.7×** |
| Median gap between repeats | — | 40.4 min | — |
| Repeats inside a rolling 11.4-min window | 39 / 300 = 13.0 % | 9.5 % of base changes | better |
| Palette scheme usage spread | 6 … 16 (2.67× ratio) | 19 … 21 (1.11× ratio) over 2400 legs | **much better** |
| Palette schemes never used | — | 0 of 120 | — |
| Palette A/B pairs distinct | — | 2193 of 2386 (91.7 % used exactly once) | — |
| Adjacent schemes below perceptual threshold | — | 1 of 2386 legs | — |
| Full-spectrum schemes ("all rainbows") | 14 of 30 = **47 %** | 14 of 120 = **12 %** | **fixed** |
| Restrained schemes (≤ 6 hue bins) | — | 78 of 120 = 65 % | — |
| Cross-scheme brightness spread | 0.08 | **0.600** (sd 0.127) | 7.5× |
| Cross-scheme saturation spread | 0.15 | **0.749** (sd 0.169) | 5× |
| Layer combinations (base+mid+accent+spark) | n/a (single layer) | 9 states, 9 distinct, **0 recurrences** | — |
| base/mid/accent triples | n/a | 39 states, 35 distinct, 4 recurrences (10.3 %) | — |
| **Time showing only one layer** | 100 % | **74.5 %** | goal was ~0 % |
| Routines that never reach the screen in 11.4 h | 14 of 124 | **11 of 225** (6 FIGURE, 5 SPARK) | — |

---

## 4. Findings

### E-1 — `bag_draw` never refills a bag whose remaining tail is inadmissible *(critical)*

`bag_draw` advances `head` only on a **successful** draw, and `bag_refill` only runs when
`head >= n`. When every routine left in the current cycle fails admission, the function returns
`0xFFFF` with `head` unchanged — so the bag is pinned on that tail **forever**.

Measured bag state after 11.4 h:

| bag | n | head | left | stuck tail cost | whole-bag median cost |
|---|---|---|---|---|---|
| GROUND | 81 | 64 | 17 | 0.1 – 24.0 ms (median 13.3) | 3.3 ms |
| FIELD | 25 | 21 | 4 | 2.4 – 17.7 ms (median 13.4) | 2.8 ms |
| **FIGURE** | 66 | 60 | **6** | **10.4 – 21.5 ms** | 4.1 ms |
| **SPARK** | 53 | 48 | **5** | **7.7 – 15.4 ms** | 4.0 ms |

The budget is 10.5 ms *total*, so those tails can never be admitted to an overlay slot.
GROUND and FIELD drain their tails eventually because the base slot's `g_force` override
waives admission; **FIGURE and SPARK are only ever hosted by overlay slots, which always have a
live ground under them, so their tails never drain.**

Consequence, measured hour by hour:

```
hour  meanLayers   spawns: base shadow mid accent spark
   0       2.02         1    142   57     50    25
   1       1.17         0    144   19      0     0
   2       1.29         0    139   33      0     0
  ...
  10       1.31         0    141   35      0     0
```

**The accent and spark slots spawn only in the first hour and never again.** The 4-layer cascade
J asked for works for ~40 minutes and then the engine is a ground plus an occasional mid, forever.
6 FIGURE and 5 SPARK routines never render at all.

**Fix (one line, verified):** when the scan exhausts the cycle without an admit, set
`b->head = b->n` before returning `0xFFFF`, so the next draw refills. Refused routines simply sit
out that cycle — which is what "refused" already means. Measured effect over the same 11.4 h:

| | as-built | with fix |
|---|---|---|
| 1 layer live | 74.5 % | **0.7 %** |
| 2 layers | 23.7 % | 16.3 % |
| **3 layers** | 1.7 % | **67.4 %** |
| **4 layers** | 0.1 % | **15.5 %** |
| accent on screen | 2.9 % | **65.3 %** |
| spark on screen | 1.3 % | **53.9 %** |
| 4-layer stacks | 9 states, 0 recurrences | 1096 states, 1090 distinct, 6 recurrences (0.55 %) |

Two side effects to check before merging:

- Base repeat gap tightens (min 12 changes → 2 changes = 0.6 min) because the GROUND bag now
  recycles far more often. The `JD_SEAM` anti-seam guard protects only the first `min(n/2,16)`
  entries of a refilled cycle against the last 16 **drawn**; when a cycle ends via the new
  `head = n` path most entries were never drawn, so `hist` is stale. Worth widening.
- Distinct routines seen in 11.4 h drops 214 → 183: the expensive tail is now skipped **every**
  cycle instead of blocking. That is E-2, below — the two need fixing together.

### E-2 — 12 % of the library is too expensive to ever be composited

Cost profile of the 225 routines at 1280×960, against `BUDGET_Q8` = 10.5 ms:

| cost band | routines |
|---|---|
| 0 – 2 ms | 38 (17 %) |
| 2 – 4 ms | 85 (38 %) |
| 4 – 6 ms | 48 (21 %) |
| 6 – 10 ms | 26 (12 %) |
| **> 10 ms** | **28 (12 %)** — cannot coexist with anything |

- Absolute ceiling for an overlay (thinnest possible stack): **9.46 ms** → 10 of 119 FIGURE+SPARK
  routines can *never* be admitted to an overlay slot.
- Absolute ceiling for a ground on an empty stack: **10.03 ms** → 18 of 106 GROUND+FIELD routines
  can only enter via the `g_force` override.
- Three layers at the *median* routine cost is 13.0 ms — already over budget. The median stack is
  structurally 2 layers.

Admission rejection reasons, per candidate test:

| slot | cost | motion | dark<70 | admitted |
|---|---|---|---|---|
| 0 (base) | 68.0 % | 5.0 % | — | 0.1 % |
| 1 (mid) | 98.6 % | 0.7 % | 0.3 % | 0.3 % |
| 2 (accent) | 99.2 % | 0.7 % | — | 0.1 % |
| 3 (spark) | 99.2 % | 0.8 % | — | 0.0 % |

**Cost is the only gate that matters.** The motion cap (`DCAP_Q8`) is essentially never binding —
which is good news for the relaxing rule, and means there is room to trade cost for layers.

Note that raising `BUDGET_Q8` alone does **not** help (13.5 ms budget → 3 layers still only 8 %):
the base slot is tested first and simply claims the extra budget for a more expensive ground.
A per-slot reservation (cap the ground at ~40–45 % of budget) is needed alongside any budget rise.

### E-3 — the startup cost probe is wall-clock dependent, so the schedule is not reproducible

`probe_routine` measures milliseconds. Three back-to-back probe runs on the same binary:

| run | library total cost | median | mean | max |
|---|---|---|---|---|
| 1 | 787.8 ms | 3.49 | 3.50 | 12.89 |
| 2 | 914.6 ms | 3.57 | 4.06 | 19.12 |
| 3 | 1013.3 ms | 3.60 | 4.50 | 24.00 |

Per-routine spread across runs: median 0.42 ms, **max 19.88 ms** (one routine hit the 24 ms clamp
in a loaded run). Roles and motion deltas are perfectly stable across runs (0 differences) because
they are image-derived — only cost moves.

Combined with E-1 this is nasty: **which** routines end up in the permanently-stuck tail depends on
how loaded the Mac was at launch. One probe run during this QC produced a schedule that issued
**146 spawns in 60 000 000 frames** — i.e. the picture froze on one ground for the rest of the run.

Recommendation: probe cost as *work per pixel* (cycle count or a fixed-iteration proxy), or take
a min-of-N rather than a single timing, and let the runtime EWMA correct it afterwards.

### E-4 — `g_exclude` leaks after a failed spawn

In `try_spawn`, the `v == 0xFFFF` early return skips the `g_exclude = -1;` reset, which only
happens on the successful path. After a failed **shadow** spawn, `g_exclude` stays `0`, so every
subsequent `admissible()` call — for other slots, on later frames — omits the ground's cost and
motion from the stack totals until the next successful spawn. It makes overlays *too* permissive
rather than too strict, so it is not causing the current symptoms, but it is a real state leak and
it made the Python port disagree with C until it was replicated.

### E-5 — real frame time is already at the 60 Hz limit ~10 % of the time

Headless steady-state render, 11 400 frames from frame 500 000 (probe window skipped):

```
delta     mean 0.762   peak 8.783   frames>8: 1
ms/frame  p50 9.95     p90 16.22    p99 23.15   max 38.26
```

That is with the stack at ~1.25 mean layers. The motion budget is comfortably met (house rule:
mean < 8), but p90 already exceeds the 16.7 ms vsync budget. If E-1 is fixed and the stack really
runs 3–4 deep, frame time must be re-measured — the cost model does not account for blend kernels,
the per-layer palette rebuild, or the SDL blit.

---

## 5. Launch-seed randomisation — **verified, and fixed during this QC**

At the start of this QC the routine bags were seeded from the constant
`mix32(0x51ED0000 + role)`, so the opening sequence was near-deterministic: over 120 random launch
frames, the first spawned routine took only **3 distinct values**, one of which appeared 37 % of the
time, and the opening 4-spawn sequence repeated 11 times.

The snapshot under test adds `g_run = mix32(frame * 2654435761 + 0x1D0F1E55)` in `engine_init`,
fed into the bag seeds (`bags_init`), the palette epoch shuffle (`pal_shuffle_raw`) and the probe
seed. Re-measured over 150 random launch frames (as `main.c` does, `rand() & 0x3FFFFF`):

| | before | after |
|---|---|---|
| distinct 1st routines | 3 | **69** (most common 4.0 %, chance floor 1.2 %) |
| distinct first-4 sequences | 35 of 120 | **150 of 150** |
| distinct 1st palette schemes | 77 of 120 | 84 of 120 (max repeat 5) |

**Launch randomisation is working.** Both the routine walk and the palette walk start somewhere new.

---

## 6. Palette variety — J's "they are all basically similar"

Hue-bin coverage across the 120 schemes (bins carrying > 2 % of total chroma, out of 12):

```
 1 bin  :   5      7 bins :   5
 2 bins :  15      8 bins :   5
 3 bins :  22      9 bins :   9
 4 bins :  17     10 bins :   9
 5 bins :  11     11 bins :   7
 6 bins :   8     12 bins :   7
```

- Full-spectrum (11–12 bins): **14 of 120 = 12 %**, down from 47 % in v2.0.
- Restrained (≤ 6 bins): **78 of 120 = 65 %** — the "stark" end J asked for now exists.
- Brightness spread **0.600** (was 0.08); saturation spread **0.749** (was 0.15).
- Pairwise scheme distance: min 0.162, median 1.195, max 2.657.
- Walk quality: every scheme used exactly 19–21 times per 2400 legs, 91.7 % of adjacent pairs occur
  exactly once, and only 1 leg in 2386 puts two perceptually-close schemes next to each other.

This half of the v2.1 brief is done and measurably done.

---

## 7. What to do next, in order

1. **Fix `bag_draw`** (E-1) — one line, restores the 3–4 layer cascade from 1.8 % to 83 %.
2. **Widen the anti-seam guard** so the faster bag recycling in (1) does not reintroduce short
   repeat gaps on the base layer.
3. **Give overlays reserved budget** (E-2) — cap the ground at ~40–45 % of `BUDGET_Q8` instead of
   letting it claim all of it; otherwise the expensive third of the library stays unplayable.
4. **Make the cost probe load-independent** (E-3) — min-of-N, or measure work rather than time.
5. **Reset `g_exclude` on the failure path** (E-4).
6. **Re-measure frame time** (E-5) once the stack actually runs 3–4 deep.

Harnesses used, all reusable:
`/tmp/jd_stats.c`, `/tmp/jd_sched_sim.c` (both `#include "bridge.c"`), `/tmp/jd_entropy.py`,
`/tmp/jd_entropy2.py`.
