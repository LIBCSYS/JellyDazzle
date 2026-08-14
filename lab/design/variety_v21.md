# JellyDazzle v2.1 — LAUNCH VARIETY

**Date:** 2026-08-14 · **Owner:** launch-variety agent
**Snapshot under test:** `bridge.c` md5 `cf81569944c48b18f49c84a683b14bdc`, 225 routines
(24 asm + 201 patterns), `palette.bin` = 120 schemes, measured at **1280×960**.

> `bridge.c` was being rewritten by the darkness agent throughout this work — it moved
> `cf815699…` → `6324d285…` → `7925463f…` while these numbers were being taken. **The file was
> therefore treated as locked and was not edited.** Everything below was measured on a pinned
> copy of the snapshot (`/tmp/jdvar`, and the patched arm in `/tmp/jdvar2`). The patch in §5
> was verified to apply cleanly against the live tree at md5 `7925463f…` — the block it touches
> is byte-identical there.

---

## 1. Verdict up front

| Question the brief asked | Answer |
|---|---|
| Do 12 independent launches show different routines in the first 60 s? | **YES** — 52 of 66 launch pairs share **zero** routines, mean 0.27 shared, **0 identical openings** |
| Do they show different palettes? | **YES** — 54 of 66 pairs share zero schemes, mean 0.20 shared |
| Does any launch repeat a routine within 60 s? | **3 of 12 launches, 1 repeat each** — see §3.3, it is benign |
| How many distinct routines per launch? | **4 – 9, mean 6.5** — structurally capped by layer lifetimes, not by variety (§3.2) |
| Is the g_run probe rotation enough? | **No.** It fixed the symptom, not the disease — §4 |

**The claim in the brief holds.** Launches do not open alike. The `g_run` probe rotation
did its job at the level of *which* routines appear.

**What it did not fix** is how those routines relate to each other *inside one launch*: a
rotation still hands every launch a **contiguous block of the library**, and neighbours in this
library are siblings — `lab/_gen_p31_p50.py` made 031–050, `_gen_p51_p70.py` made 051–070. Worst
measured launch drew its entire 60 seconds from **6 adjacent patterns of 201**. §5 is a 12-line
patch that turns the rotation into a coprime-stride permutation; measured effect in §6.

---

## 2. Method

`lab/qa/variety.c` `#includes bridge.c` rather than linking against it, so the rig reads the
engine's file-static state (`g_L`, `g_st`, `g_bag`, `g_run`, `g_probe_i`, `scheme_at`) **without a
single edit to bridge.c**. Same technique `lab/design/entropy_qc.md` used, and the reason it was
chosen here: the file is locked.

A *launch* is `jd_frame()` from a start frame drawn from the same domain `main.c` uses
(`rand() & 0x3FFFFF`), run 3600 frames at 1280×960 — exactly 60 s at the 60 fps target. Twelve
fixed start frames, so the battery is reproducible:

```
123456 417022 1039284 1583421 1996488 2244532 2699421 2938103 3157482 3506611 3812004 4109337
```

```sh
clang -O2 -I. -DJD_NS=$(expr $(stat -f%z palette.bin) / 131072) \
      lab/qa/variety.c patterns_c/pattern_*.c patterns_c/registry.c draw.s -o /tmp/variety -lm
lab/qa/variety_ab.sh /tmp/variety /tmp/variety_after /tmp/jdvar/ab   # paired A/B
python3 lab/qa/variety_report.py /tmp/jdvar/ab/after
```

Two things the rig had to get right, both of which changed the answer:

- **A tenancy is `(routine, t_in)`, not `(routine, slot)`.** When the ground retires,
  `sched_tick` *swaps* `g_L[0]` with `g_L[JD_SHADOW]`, so one layer reappears under a different
  slot index. Counting that as a second spawn reported a phantom repeat in **12 of 12** launches.
  With the correct key it is 3 of 12.
- **Runs are paired, not batched.** `probe_step()` spends a *wall-clock* budget, so an arm that
  happens to run on a quieter machine probes more of the library and looks more varied for
  reasons unrelated to the change. `variety_ab.sh` runs before/after concurrently per seed.

---

## 3. Baseline — as built

### 3.1 Per launch

| launch (start frame) | distinct routines | spawns | repeats | distinct palettes | probe done (frame) | opening routine | library span | families |
|---|---|---|---|---|---|---|---|---|
| 123456 | 7 | 8 | 1 | 5 | 2837 | 54 | 127 | 4 |
| 417022 | 6 | 6 | 0 | 4 | **NOT DONE** | 11 | 50 | 2 |
| 1039284 | 9 | 9 | 0 | 5 | **NOT DONE** | 69 | 126 | 4 |
| 1583421 | 7 | 8 | 1 | 4 | **NOT DONE** | 117 | 103 | 5 |
| 1996488 | 6 | 6 | 0 | 5 | **NOT DONE** | 66 | 110 | 3 |
| 2244532 | 6 | 6 | 0 | 5 | **NOT DONE** | 68 | 131 | 2 |
| 2699421 | 6 | 6 | 0 | 4 | **NOT DONE** | 101 | 29 | 3 |
| 2938103 | 6 | 6 | 0 | 4 | **NOT DONE** | 21 | **6** | **1** |
| 3157482 | 6 | 6 | 0 | 4 | **NOT DONE** | 117 | 41 | 3 |
| 3506611 | 4 | 5 | 1 | 4 | **NOT DONE** | 2 | 41 | 2 |
| 3812004 | 7 | 7 | 0 | 5 | **NOT DONE** | 186 | 48 | 3 |
| 4109337 | 8 | 8 | 0 | 4 | **NOT DONE** | 105 | 123 | 4 |

*library span* = how wide a slice of the 201-pattern library that launch's material came from.
*families* = distinct 20-pattern generator batches touched (max 11).

### 3.2 Overlap between launches — the brief's headline question

| | routines | palettes |
|---|---|---|
| mean shared, over all 66 pairs | **0.27** | **0.20** |
| max shared | 2 | 2 |
| pairs sharing nothing | **52 / 66** | **54 / 66** |
| mean Jaccard | 0.023 | 0.025 |
| **identical openings (first 15 s)** | **0 / 66** | — |

Distinct routines opening the first 15 s across the 12 launches: **33**. The launches are not
alike. Note the ceiling this sits under: at 4 slots with 13–31 s lifetimes a launch only *has*
4–9 tenancies in 60 s, so ~6 routines per launch is the schedule working as designed, not a
variety failure. Uniform draws from 201 would give an expected 0.2 shared per pair; measured 0.27.
**Selection is essentially as random as the pool allows.**

### 3.3 The repeats are benign

3 of 12 launches replay one routine inside the minute (1 each). All three are the ground handing
over to a new ground while an overlay that had already retired comes back — 20+ seconds apart,
in a different slot, under a different palette leg and mood. The `JD_SEAM` anti-repeat window is
16 draws deep and a launch only makes 6–9 draws, so this is not the bag misbehaving; it is a
routine legitimately winning its slot's role bag twice when very few routines are eligible. The
eligibility squeeze (§4.2) is the cause; fix that and these thin out.

---

## 4. What is still wrong

### 4.1 V-1 — the probe order is a rotation, so every launch is one family *(fixed, §5)*

`probe_step()` walks the library as `(g_probe_i + rot) % np` with `rot = g_run % np`. That
randomises **where** the sweep starts and nothing else — the routines it reaches first are always
a **contiguous run**. Because `admissible()` rule 4 refuses anything unprobed, *whatever the sweep
has reached is the entire library for the opening minute*.

Deterministic, no timing involved — the span of the first 25 patterns the probe measures:

| launch | g_run | first 25 probed (BEFORE) | span | stride (AFTER) | first 25 probed (AFTER) | span |
|---|---|---|---|---|---|---|
| 123456 | 2783766615 | 30..54 | **25** | 136 | 1,7,30,36,42,48… | **196** |
| 417022 | 1195275215 | 173..197 | **25** | 52 | 0,7,14,24,31,38… | **195** |
| 1039284 | 1145870292 | 45..69 | **25** | 142 | 10,12,23,34,36,45… | **191** |
| 1583421 | 3458516842 | 91..115 | **25** | 133 | 2,5,8,11,14,17… | **155** |
| 1996488 | 1154168766 | 33..57 | **25** | 100 | 21,22,23,24,25,26… | **113** |
| 2244532 | 2553297614 | 41..65 | **25** | 145 | 5,15,18,28,38,41… | **192** |
| 2699421 | 1200871156 | 73..97 | **25** | 70 | 5,12,21,30,39,48… | **193** |
| 2938103 | 2490590530 | 133..157 | **25** | 130 | 2,14,26,37,38,49… | **191** |
| 3157482 | 69974214 | 84..108 | **25** | 92 | 6,16,23,33,40,50… | **195** |
| 3506611 | 3752195989 | 148..172 | **25** | 106 | 2,13,24,35,46,53… | **191** |
| 3812004 | 1898238906 | 132..156 | **25** | 43 | 2,16,17,30,31,45… | **188** |
| 4109337 | 3620747145 | 78..102 | **25** | 85 | 0,8,16,23,31,39… | **187** |
| | | **mean span** | **25 / 201** | | **mean span** | **182 / 201** |

Neighbours in this library are siblings, generated in batches by one script. So the opening pool
is not merely small — it is *thematically uniform*. Launch 2938103 spent the whole minute inside
**one** 20-pattern family.

### 4.2 V-2 — the probe sweep does not finish inside the first minute *(not fixed — belongs with perf)*

The sweep completed within 3600 frames in only **1 of 12** launches (frame 2837), and not at all
in the other 11. Consequences, all of which suppress variety for the whole opening minute:

- `admissible()` rule 4 refuses every unprobed routine, so the pool stays partial.
- `!g_probe_done` also halves the motion cap (`DCAP_Q8 / 2`), thinning the stack.
- Unprobed patterns default to `R_FIGURE`, so the `GROUND` and `FIELD` bags are starved and the
  ground bag falls back on the 24 asm modes.

Root cause is the budget: `probe_begin` spends 250 ms up front, then each frame donates
`12.0 - g_ewma_ms` clamped to `[0.5, 6.0]` ms. At the real 1280×960 stack cost the EWMA sits near
the ceiling, so the donation is pinned at the **0.5 ms floor** — 225 routines × ~10 ms ≈ 2.25 s of
probe work needs ~4500 frames at that rate. This is also **wall-clock dependent** (`entropy_qc.md`
E-3), so a slower machine than this one gets *less* variety, not the same amount later.

Recommendation, owned by whoever holds the frame budget: probe cost as work-per-pixel rather than
milliseconds, or raise the per-frame floor from 0.5 ms to ~1.5 ms while fewer than 3 layers are
live (the opening is exactly when there is slack). Not measured here — it trades against fps and
must not be landed without the fps agent's numbers.

### 4.3 V-3 — palette variety in 60 s is structural, and is fine

5 schemes maximum per launch, because `JD_LEG` is 1024 frames and 3600 frames is 3.5 legs. The
selection itself is healthy: 54 of 66 pairs share no scheme at all, and `entropy_qc.md` shows
usage spread 19–21 over 2400 legs with 0 of 120 schemes unused. **No action.**

---

## 5. The patch

Applies to `bridge.c` at `probe_step()`, plus one helper next to `mix32()`. Verified to apply
cleanly at live md5 `7925463f…`. **Not applied — the darkness agent holds the file.**

### 5.1 Helper, immediately after `mix32()` (bridge.c ~line 96)

```c
static int jd_gcd(int a, int b) { while (b) { int t = a % b; a = b; b = t; } return a; }
```

### 5.2 In `probe_step()`, replace the rotation block

Replace:

```c
            int np = g_nr - JD_NASM;
            /* Rotate the probe order by a per-run offset.  Without this the
             * library is always measured in the same order, so the same few
             * routines become eligible first and every launch opened with
             * the same material (J: "same start ... with 200 patterns"). */
            int rot = (int)(g_run % (uint32_t)(np > 0 ? np : 1));
            probe_open(g_probe_i < np
                       ? JD_NASM + (g_probe_i + rot) % np
                       : g_probe_i - np,
                       g_pa, g_pb);
```

with:

```c
            int np = g_nr - JD_NASM;
            int m  = np > 0 ? np : 1;
            /* Probe order decides LAUNCH VARIETY, because rule 4 of
             * admissible() refuses anything not yet measured and the sweep
             * takes longer than the first minute (measured: finished inside
             * 3600 frames in 1 of 12 launches).  So whatever the probe has
             * reached IS the library for the opening minute.
             *
             * Rotating that order by g_run fixed the "every launch opens
             * with 001-006" symptom but not the disease: a rotation still
             * hands each launch a CONTIGUOUS block, and neighbours in this
             * library are siblings — patterns 031..050 came out of one
             * generator script, 051..070 out of the next.  Measured over 12
             * launches: the first 25 routines probed spanned 25 of 201
             * every time, and one launch spent its whole 60 s inside a
             * single 20-pattern family.
             *
             * Walk a coprime STRIDE instead.  Still a permutation — every
             * routine is probed exactly once and the sweep still terminates
             * in g_nr steps — but successive probes land far apart, so the
             * pool is WIDE after the first 250 ms rather than merely
             * differently placed: first-25 span 25 -> 182 of 201. */
            int rot = (int)(g_run % (uint32_t)m);
            int stp = 1;
            if (m >= 12) {
                /* keep the stride out of the +-1/+-2 band, where a "stride"
                 * is just a rotation again (measured: 2.4% of launches drew
                 * stride 2, 199 or 200 and collapsed back to neighbours) */
                int lo = m / 6 + 1, rng = m - 2 * lo; if (rng < 1) rng = 1;
                stp = lo + (int)(mix32(g_run ^ 0x7A17E55u) % (uint32_t)rng);
                for (int g = 0; g < m && jd_gcd(stp, m) != 1; g++)
                    stp = (stp + 1 <= m - lo) ? stp + 1 : lo;
                if (stp < 2) stp = 2;
            }
            probe_open(g_probe_i < np
                       ? JD_NASM + (int)(((long)g_probe_i * stp + rot) % m)
                       : g_probe_i - np,
                       g_pa, g_pb);
```

### 5.3 Why it is safe

- **Still a permutation.** `gcd(stp, m) == 1` ⇒ `i·stp + rot (mod m)` visits every index exactly
  once. Checked exhaustively over 421 sampled launch seeds at `m` = 201, and over `m` ∈
  {201, 225, 120, 50, 13, 11, 3, 1}: **0 failures**. Small libraries (`m < 12`) fall back to
  `stp = 1`, i.e. exactly today's behaviour.
- **The sweep still terminates in `g_nr` steps.** The loop bound, the resumability, the asm-mode
  tail (`g_probe_i - np`) and the probe arithmetic are untouched.
- **No new per-frame cost.** The stride is a handful of integer ops evaluated once per routine
  opened, not per frame.
- **It changes only the ORDER of measurement.** No admission rule, no role rule, no envelope, no
  blend, no budget is touched — which is why the relaxing contract cannot be affected by
  construction, and is confirmed by measurement in §6.2.

---

## 6. Before / after

### 6.1 Variety, 12 paired launches × 3600 frames @ 1280×960

| metric | BEFORE (rotation) | AFTER (stride) | |
|---|---|---|---|
| **library span of one launch's material, mean** | 78 / 201 | **113 / 201** | +45 % |
| **library span, worst launch** | **6 / 201** | **72 / 201** | 12× |
| **generator families per launch, mean** | 3.0 | **3.8** | +27 % |
| **generator families, worst launch** | **1** | **2** | — |
| span of first 25 probed, mean (deterministic) | 25 / 201 | **182 / 201** | 7.3× |
| distinct routines per launch, mean | 6.5 | 6.2 | flat (slot-count bound) |
| launches repeating a routine in 60 s | 3 / 12 | 3 / 12 | flat |
| **routine pairs sharing nothing** | 52 / 66 | 50 / 66 | flat |
| routine mean shared per pair | 0.27 | 0.26 | flat |
| **identical openings** | **0 / 66** | **0 / 66** | held |
| palette pairs sharing nothing | 54 / 66 | 54 / 66 | held |
| probe sweep finished inside 60 s | 1 / 12 | 4 / 12 | incidental |

**Read this honestly.** Cross-launch *routine* overlap was already good and did not move — it
could not, it was already at the floor the pool allows. What moved is the metric that matches
what the eye actually complains about: **a launch is no longer confined to one corner of the
library.** The worst case went from 6 adjacent patterns for a whole minute to 72 spread across
the library.

One number goes the "wrong" way and should not be misread: *family* overlap between launches
rises (mean 0.91 → 1.65, pairs sharing no family 24/66 → 4/66). That is arithmetic, not
regression — before, each launch occupied 1–3 families so two launches rarely collided; now each
launch spans 3–6 families so they overlap more, while each individual launch is far more varied.
Trading "two launches are cleanly different but each is monotonous" for "each launch is a mixed
bag and the mixes differ" is the right trade for the client's eye. The hard metric — shared
routine *identities* — did not regress.

### 6.2 House rules — no regression

Whole-engine QA battery (`/tmp/jd21/qa.c`), 40 samples, same snapshot, same machine:

| | BEFORE | AFTER | rule |
|---|---|---|---|
| **strobe samples (delta ≥ 8)** | **0 / 40** | **0 / 40** | must stay 0 — **held** |
| mean frame-to-frame delta | 0.70 | 0.71 | < 8 — held |
| worst frame-to-frame delta | 2.76 | **2.21** | < 8 — held, improved |
| mean luma | 51.5 | **60.6** | +18 % |
| samples under luma 40 | 19 / 40 | **15 / 40** | improved |
| max luma | 127.2 | **183.1** | improved |
| min luma | 0.1 | 0.3 | **unchanged — still a black frame, see below** |
| fps @ 800×600 | 11051 | 11511 | no cost |

The luma improvement is a side effect worth naming: with a wide probe pool the ground is chosen
from the whole library instead of one neighbourhood, so bright grounds are available from the
first seconds. **It does not fix the black-frame defect** — min luma is still ~0 in both arms.
That remains the darkness agent's, and this patch neither helps nor hinders it.

---

## 7. Files

| Path | What |
|---|---|
| `lab/qa/variety.c` | the rig — one launch, `#include "bridge.c"`, machine-readable records |
| `lab/qa/variety_run.sh` | 12 sequential launches into an output dir |
| `lab/qa/variety_ab.sh` | paired A/B battery, both arms concurrent per seed |
| `lab/qa/variety_report.py` | the tables in §3 and §6.1 from an output dir |

Re-run in full:

```sh
cd /Users/exeter/dev/m5/assembly/dzzle1
NS=$(expr $(stat -f%z palette.bin) / 131072)
clang -O2 -I. -DJD_NS=$NS lab/qa/variety.c $(ls patterns_c/*.c | grep -v harness) \
      draw.s -o /tmp/variety -lm
lab/qa/variety_run.sh /tmp/out
python3 lab/qa/variety_report.py /tmp/out
```
