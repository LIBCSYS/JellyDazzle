# 07 — Performance budget: what the contrast fixes actually cost

JellyDazzle 3.0.4 · Apple M5 (4P + 6E, 32 GB) · macOS 25.6.0 · clang -O2
Everything below is either **[M] measured on this machine** or **[E] estimated**, marked per line.

---

## 0. Bottom line

- **The blend mode is not the expensive part. The warp is.** Measured at 3456×2160, every overlay pays `layer_warp` at **4.2–12.8 ms**, versus **2.7–5.4 ms** for its blend. The warp is not in the cost model at all.
- **The composite kernels are ALU-bound, not bandwidth-bound** — the opposite of the working assumption. Shipping `span_max` runs at **3.1× the memory-bandwidth floor**; `span_screen` at **6.3×**. Hand-NEON `span_max` runs at **1.09× the floor**.
- **That flips the design rule.** In scalar C, a darkening blend costs +1.4 to +4.1 ms per overlay per frame. In NEON it costs **+0.20 to +1.59 ms**. Rewrite the kernels first and the blend-mode question stops being a performance question.
- **`JD_NSLOT` is not the ceiling.** Measured across eight runs, slot 2 (accent) was live **0.0 % of frames at 3456×2160** and slot 3 (spark) **0.0 % at every resolution**. `g_hot` latches on within seconds at full Retina and the `(g_hot && s >= 2)` gate in `sched_tick` then locks both slots out permanently. Raising `JD_NSLOT` to 6 adds two slots the scheduler will never fill.
- **There is no headroom at 4K today.** A ground with *zero* overlays measured **11.49 ms/frame** at 3456×2160 — 69 % of a 60 Hz frame gone before anything is composited. The `MAX_PIX` comment's "103 fps at 3456×2160" does not reproduce in 3.0.4.

---

## 1. How these numbers were made

| | |
|---|---|
| Kernel timings | Standalone harness, `clang -O2`, best-of-15 over 7.46 Mpx buffers, kernels copied verbatim from `compositor.c`. Best-of-N is contention-resistant; these are the load-bearing numbers. |
| In-engine profile | `compositor.c` copied to scratch and instrumented around `layer_warp`, `blend_span`, the ground composite and the render loop. 200 warm-up frames discarded, then 600–1200 frames. |
| Full-engine fps | `tools/gate_harness.c` — `gate battery W H START TOTAL EVERY`. Built ad hoc; there is no `gate` target in the Makefile. |

**Caveat, stated plainly:** the machine carried a load average of 8–17 during the in-engine runs (another agent's `gate_ct` at 99 % CPU, Zed, WindowServer). **Absolute fps figures are a lower bound.** The kernel microbenchmarks are best-of-15 and hit 126 GB/s on `memcpy`, i.e. hardware peak, so contention did not distort them. Ratios and per-stage splits are sound; treat single fps numbers as ±25 %.

**Second caveat:** the engine's schedule is a closed loop through `g_ewma_ms` → `g_hot` → spawn gating, so two runs from the same start frame diverge within seconds. Measured: same binary, same start frame, **46.6 fps and 52.8 fps**. Any A/B on the live engine needs ≥5 fixed seeds and a median.

---

## 2. Where the frame actually goes, 3456×2160

Instrumented, ms per frame, 600–1200 frames each after warm-up. **[M]**

| seed | frame | render | ground | warp | blend | other | overlays live |
|---|---|---|---|---|---|---|---|
| 300000 | 18.37 | 9.71 | 1.12 | 4.58 | 1.84 | 1.12 | 0.63 |
| 700000 | 29.11 | 15.32 | 1.53 | 9.25 | 2.91 | 0.09 | 0.72 |
| 1300000 | 43.89 | 31.45 | 1.76 | 7.09 | 3.26 | 0.32 | 0.60 |
| 1900000 | 16.91 | — | — | 0 | 0 | — | 0.00 |
| 300000 (b) | **11.49** | 10.91 | 0.56 | **0** | **0** | 0.03 | **0.00** |

Per-operation cost, same runs: **[M]**

| operation | ms each | count/frame |
|---|---|---|
| pattern render (one layer) | **4.87 – 17.24** | 1.5 – 1.8 |
| `layer_warp` (one overlay) | **4.21 – 12.77** | 0.56 – 0.72 |
| `blend_span` (one overlay) | **2.67 – 5.41** | 0.56 – 0.72 |
| ground composite (`memcpy` or `span_lerp`) | 0.56 – 1.76 | 1 |

Read the last row of the first table twice. **A solo ground, no overlays, no warp, no blend, cost 11.49 ms.** `BUDGET_Q8` is 10.5 ms for *render + blend combined*. The engine is over its own budget with an empty stack at full Retina.

---

## 3. The per-pixel composite loop today

### 3.1 Blend path — `blend_span` → `span_*`, one pass per overlay

Traffic per overlay at 7.46 Mpx: 28.5 MB source read + 28.5 MB destination read + 28.5 MB destination writeback = **85.5 MB**.

Operation counts are per pixel, scalar source; clang auto-vectorises the packed-integer forms. Cycles/px assume 4.3 GHz. **[M]** for ms, **[E]** for op counts and cycles.

| kernel | int ops/px | ld/st | ms @ 7.46 Mpx | cycles/px | × bandwidth floor |
|---|---|---|---|---|---|
| `memcpy` (ground copy) | 0 | 1+1 | **0.474** | 0.27 | 0.83 (1R+1W) |
| `span_scale` | ~6 | 1+1 | **1.206** | 0.70 | 2.1 |
| `span_lerp` (ground crossfade) | ~10 | 2+1 | **1.541** | 0.89 | 1.8 |
| `span_max` | ~23 | 2+1 | **2.080** | 1.20 | **3.1** |
| `span_diff` | ~28 | 2+1 | **3.524** | 2.03 | 5.2 |
| `span_screen` | ~30 | 2+1 | **4.296** | 2.48 | **6.3** |
| `span_gain` (dead-air lift, in place) | ~12 | 1+1 | **2.409** | 1.39 | 4.2 |

`span_max` is the cheap one because it works on packed 0x00FF00FF / 0x0000FF00 halves and never unpacks to bytes. `span_screen`, `span_diff` and everything proposed below *do* unpack per channel, and that is where the 2–3× goes.

### 3.2 Warp/resample path — `layer_warp`, one pass per *moving* overlay

- ~10 integer ops/px: 2 adds, 2 shifts, 2 bounds compares, one address multiply-add, one gather load, one store.
- **Fewer ops than any blend, and 2–5× the cost**, because the load is data-dependent with a stride the prefetcher cannot follow.
- **Every overlay is warped, always.** Both spawn branches (`compositor.c:1911` and `:1924`) set `L->moving = 1` unconditionally. The `moving == 0 → skip the resample` fast path is dead code.

Warp sweep, 3456×2160, best-of-8: **[M]**

| transform | shipping (mirror) | no bounds handling at all | out-of-bounds |
|---|---|---|---|
| identity `z=1.00 r=0.00` | **3.79** | 3.99 | 0.00 % |
| `z=1.00 r=0.005` | 4.18 | 4.29 | 0.28 % |
| `z=1.00 r=0.02` (≈1°) | **8.82** | 8.84 | 1.09 % |
| `z=1.20 r=0.15` (typical) | **9.39** | 9.33 | 7.32 % |
| `z=0.50 r=0.00` | 8.67 | — | 84 % |
| `z=1.30 r=0.79` (worst) | **11.26** | 8.00 | 23.7 % |

- **One degree of rotation costs +5.0 ms** — 2.3× the identity case.
- Stripping the `jd_mirror` modulo and the bounds branch entirely changes it by **0.02 ms**. It is not the arithmetic and it is not the branch misprediction. It is **locality**: a rotated walk of the source touches ~70 different source rows per output row, so every cache line is fetched for a handful of pixels of use.
- Overlay spawns draw `tz` from 0.40–2.50 and `tr` from 0–2π uniformly, so the typical case is the 9–11 ms column, not the 3.8 ms one.

---

## 4. The Apple Silicon angle: bandwidth vs ALU

Measured single-core streaming ceilings on this M5: **[M]**

| reference | ms @ 7.46 Mpx | effective GB/s |
|---|---|---|
| `memcpy` (1R + 1W, 57 MB) | 0.474 | **126.0** |
| read-only sum (1R, 28.5 MB) | 0.327 | 91.3 |
| write-only fill (1W, 28.5 MB) | 0.357 | 83.6 |

**Bandwidth floor for one blend pass** (85.5 MB at 126 GB/s) = **0.68 ms**.

### The hypothesis is half right, and the wrong half is the one that binds today

- **"Extra ALU in an existing pass is nearly free."** **False as shipped, true if rewritten.** Shipping `span_max` is 2.08 ms = 3.1× the floor; `span_screen` is 6.3× the floor. These kernels are ALU-bound, so extra arithmetic is charged in full. Hand-written NEON `span_max` measures **0.740 ms = 1.09× the floor** — at *that* point it is bandwidth-bound and the hypothesis holds exactly.

| kernel | scalar C (shipping style) | hand NEON | ratio |
|---|---|---|---|
| MAX | 2.080 | **0.740** | 2.8× |
| SCREEN | 4.296 | **1.351** | 3.2× |
| MULTIPLY | 3.521 | **0.940** | 3.7× |
| OVERLAY | 6.170 | **2.326** | 2.7× |
| MAX + per-pixel mask | 8.897 | **0.963** | 9.2× |
| full-frame luma | 1.539 | 1.350 | 1.1× |

- **"An extra pass over the frame is expensive."** **Unconditionally true**, and it is the strongest measured result here. A new full-frame pass costs ≥0.33 ms at hardware peak and 1.5–4.9 ms written as ordinary C. The proof: a regional-dim mask as a **separate pass costs 4.87 ms**; the identical visual result **fused into the NEON max costs 0.22 ms**. Twenty-two times cheaper.
- **The warp is the rule seen from the other side.** Ten ops per pixel, 5.4 cycles per pixel. On Apple Silicon you spend locality, not instructions.

**Design implication, in order:**
1. Any new work goes **inside an existing pass**, never in a new one. Regional dimming, ground-luma gathering, and gamma handling all admit fused forms.
2. **Write the `span_*` kernels in NEON before choosing a blend mode.** It recovers 1.3–3.0 ms per overlay per frame *and* it converts every subsequent blend-mode decision from a 1.4–4.1 ms decision into a 0.2–1.6 ms one. This is the highest-leverage change on the list and it changes nothing about the picture.
3. Prefer packed-channel formulations (`0x00FF00FF` / `0x0000FF00`) over per-channel unpacking. That difference alone is the 2.08 vs 4.30 gap between MAX and SCREEN.
4. **Never use a lookup table in a full-frame pass.** Measured below — every LUT variant was the slowest option in its class, because a byte gather defeats vectorisation completely.

---

## 5. Cost of each proposal

All figures per overlay per frame at 3456×2160 (7.46 Mpx). "Δ vs MAX" is against the current default blend in the same implementation style. **[M]** unless noted.

### 5.1 Darkening blend modes vs MAX

| proposal | scalar C | Δ vs MAX | hand NEON | Δ vs NEON MAX | derivation |
|---|---|---|---|---|---|
| MAX (today) | 2.080 | — | 0.740 | — | measured |
| SCREEN (today, dark+coloured) | 4.296 | +2.22 | 1.351 | +0.61 | measured |
| DIFF (today, seasoning) | 3.524 | +1.44 | — | — | measured |
| **MULTIPLY**, weight-limited | 3.521 | **+1.44** | 0.940 | **+0.20** | measured; `out = lerp(d, d·s/255, w)` |
| **OVERLAY**, branchy | 6.169 | **+4.09** | 2.326 | **+1.59** | measured |
| OVERLAY, branchless select | 6.170 | +4.09 | 2.326 | +1.59 | measured — **the branch is not the cost**; identical to branchy |
| **SOFT LIGHT** (Pegtop, 2 mults, no sqrt) | 5.946 | **+3.87** | ~2.2 **[E]** | ~+1.5 **[E]** | measured scalar; NEON estimated by the 2.7× ratio observed on OVERLAY |
| SOFT LIGHT via 64 KB LUT | 9.224 | +7.14 | — | — | measured — **worse than computing it**; the table is L2-resident and still loses |

**Read:** in scalar C, moving from MAX to multiply costs +1.44 ms/overlay/frame — about 9 % of a 60 Hz frame, per overlay, which the engine cannot afford today. In NEON it costs +0.20 ms — 1.2 % of a frame. **The blend-mode choice is affordable if and only if the kernels are rewritten first.** Multiply is much the cheapest darkening operator; overlay and soft light are 5–8× its incremental cost.

### 5.2 Per-frame ground luma measurement

Current: `g_gluma` in `jd_frame`, 512 strided samples of the ground buffer, 32-frame EWMA (`compositor.c:2563`).

| metric | cost @ 7.46 Mpx | what it gives |
|---|---|---|
| current: 512 samples, mean | **0.000** (below timer resolution; ~2 µs **[E]**) | one global mean |
| stride-8 read, 933 k samples | **0.325** | global mean + a real 256-bin histogram → any percentile |
| full-frame luma, scalar | 1.539 | mean, no better than stride-8 for this purpose |
| full-frame luma, NEON | 1.350 | as above |
| **16×16 tile map, 1-in-16 subsample** (29 k samples, 32 k tiles) | **0.316** | global percentiles **and** a local map |

**Is 512 samples enough? For a mean, yes. For this problem, no — and the problem is not the sample count.**

- Statistically the mean is fine: measured frame luma σ is 9–30 code values (the battery's contrast column), so the standard error on 512 samples is ~0.4–1.3 code values, and the 32-frame EWMA cuts that further.
- **The sample lattice is degenerate.** The stride is `npix/512` = 14 570, which at width 3456 is 4.216 rows — the 512 samples lie on a near-diagonal line across the frame, not on a lattice. Any pattern with vertical or diagonal periodicity aliases hard against it. This is a real risk in a library with 20 % radial mandalas and 20 % string-art weaves.
- **The mean is the wrong statistic.** An accent dies where the ground is *locally* bright. A frame averaging luma 60 can be half black and half 120, and the accent is invisible on the bright half while `g_gluma` reports it as dim. `pick_blend` already leans on `g_st[ground].luma` for its SCREEN cap and inherits the same flaw.
- **Recommendation:** replace the 512-sample mean with the **16×16 tile map at 0.316 ms**. It costs the same as a stride-8 read, it is the only option that yields a *local* metric, and — critically — it is the same structure the regional mask needs, so one pass serves both. Derive `g_gluma` as the p50 of the tile map and add a p90 for the "how bright does it get" question the current metric cannot answer.

### 5.3 Per-pixel local mask for regional ground dimming

| implementation | cost | derivation |
|---|---|---|
| build 16×16 tile map (1-in-16 subsample) | **0.316** | measured |
| apply as a **separate** dim pass, scalar | **4.867** | measured |
| fuse into `span_max`, scalar | 8.897 | measured — the per-pixel tile index kills auto-vectorisation |
| **fuse into `span_max`, NEON** (row-wise tile broadcast) | **0.963** | measured, vs 0.740 unmasked → **+0.223** |

- **Total for regional dimming, done right: 0.316 + 0.223 = 0.54 ms per frame per overlay.** That is affordable now and trivially affordable after the NEON rewrite.
- **Done wrong (separate pass, scalar): 5.18 ms.** Ten times the cost for the same picture.
- The NEON form broadcasts one tile value per 16-pixel run inside the row loop, so there is no gather. Interpolating between tiles would need a gather or a second broadcast lane — start with nearest-tile; a 16-px hard edge on a dim factor of ≤0.25 is below threshold **[E]**, verify with `gate contrast`.

### 5.4 Linear-light blending vs direct sRGB

| implementation | cost | Δ vs same-style MAX |
|---|---|---|
| sRGB MAX, scalar (today) | 2.080 | — |
| sRGB MAX, NEON | 0.740 | — |
| **sRGB→linear16 LUT + linear→sRGB LUT, scalar** | **8.889** | **+6.81** |
| **gamma-2.0 approximation in NEON float** (square, max, `vsqrtq_f32`) | **4.464** | **+3.76** |
| gamma-2.0 in NEON u16 fixed point with a rsqrt approximation | ~2.0–2.5 **[E]** | ~+1.3–1.8 **[E]** |

- **The LUT is the worst option, by a wide margin.** Nine 8-bit table lookups per pixel is nine byte gathers, and NEON `vtbl` only handles 16-byte tables, so the whole loop falls back to scalar. Measured 4.3× the shipping sRGB max.
- Even the good implementation costs 3.8 ms/overlay/frame — **five times the entire NEON MAX kernel**, and more than the entire proposed multiply + mask + tile-map package combined.
- **Recommendation: do not do linear-light blending.** It is the most expensive item on the list and the least certain to be visible. If the goal is "the accent's energy adds correctly against a bright ground", a weight-limited multiply in sRGB at 0.94 ms gets most of the perceptual result for a quarter of the cost. If linear light later proves necessary, do it as gamma-2.0 fixed point, never as a LUT.

---

## 6. Can the overlay ceiling go above `JD_NSLOT` 4?

**No, and raising it would change nothing on screen — because 4 is not the ceiling that binds.**

### 6.1 What is actually binding

`sched_tick`, `compositor.c:2314`:

```c
if (handover || (g_hot && s >= 2) || frame - g_last_change < g_gap) continue;
```

- `g_hot` latches when `g_ewma_ms > 13.5` and clears only after **120 consecutive frames** under 10.5 ms (`compositor.c:2796–2806`).
- Measured mean frame time at 3456×2160: **11.5 – 43.9 ms**. It latches in the first seconds and never clears.
- Measured `g_hot` duty cycle: **95.9 %, 87.5 %, 100 %, 100 %, 100 %** at 3456×2160; **83.4 % and 100 %** even at 1280×960.

Consequence, measured across eight instrumented runs: **[M]**

| resolution | mean overlays live | slot 1 | slot 2 (accent) | slot 3 (spark) |
|---|---|---|---|---|
| 3456×2160 | **0.00 – 1.00** | 60–72 % of frames | **0.0 %** | **0.0 %** |
| 1280×960 | 0.72 – 1.77 | — | 0 % or 84.8 % | **0.0 %** |

**The accent slot — the exact slot carrying the visibility problem this whole exercise is about — never spawns at full Retina.** J's "I see 2–3 overlays" is generous; the instrumented mean is under one. Slot 3 did not spawn once in any run measured.

### 6.2 What a 5th and 6th layer would cost if they did spawn

Per additional overlay at 3456×2160, from the in-engine profile: **[M]**

| component | measured range | note |
|---|---|---|
| pattern render | 4.87 – 17.24 ms | irreducible; the library's own cost |
| `layer_warp` | 4.21 – 12.77 ms | every overlay pays it |
| `blend_span` | 2.67 – 5.41 ms | scalar; 0.74–2.33 in NEON |
| **total, full rate** | **11.8 – 35.4 ms** | against a 16.67 ms frame |
| **total, half rate** (`L->half`) | **5.9 – 17.7 ms** | amortised |

- A 5th layer at full rate costs **more than an entire 60 Hz frame**, on top of a stack that already spends 11.5 ms on a solo ground.
- Even after every optimisation named in this document (NEON blends, cache-blocked warp), an extra layer is **≈10–15 ms** — the render dominates and cannot be optimised from the compositor.
- Buffer cost is trivial by comparison: `g_buf` is `JD_NBUF` = `JD_NSLOT + 1` allocations of `w·h·4` = **28.5 MB each**; two more slots is +57 MB out of 32 GB. **[M]**

### 6.3 More layers would make the visibility problem worse, not better

- `PEAK_LO`/`PEAK_HI` fall monotonically per slot: `{256, 128, 100, 72}` / `{256, 180, 140, 115}`. Continuing that ramp puts slot 4 at ~50–90 Q8 and slot 5 at ~35–70 Q8 — **14–35 % opacity**.
- Under MAX at 25 % weight, an accent pixel must be **4× brighter than the ground** to change the output at all. Against the bright grounds that motivated this work, a 6th layer is arithmetically invisible.
- `SPAN` narrows per slot too (`M_RICH`: 20000/14000/8000/4000). A 5th and 6th slot's palette window would be a near-monochrome slice of the ramp, reducing the chance the layer differs from the ground in *hue* as well as in luma.
- The `admissible()` rule `if (slot_i != 0 && st->dark < 70) return 0` already requires an overlay to be ≥27 % near-black. Thinner slots need thinner material still, and the bags run out.

### 6.4 What to do instead

- **Do not raise `JD_NSLOT`.** Make slots 2 and 3 actually spawn. That delivers exactly the "more than 2–3 overlays" J is asking for, with no new buffers and no new slots.
- Three concrete levers, in order of measured payoff:
  1. **Cut the warp cost** (4.2–12.8 ms/overlay). Cache-block the rotated gather, or skip the warp when `|tr| < ε` and `|tz − 1| < ε` (measured: identity is 3.79 ms vs 9.39 ms typical — a 5.6 ms saving on the frames where it applies). This alone can pull `g_ewma_ms` under the 13.5 ms `g_hot` threshold at 4K.
  2. **NEON the blends** (1.3–3.0 ms/overlay).
  3. **Reconsider the `g_hot && s >= 2` gate.** It is a cliff, not a ramp: one crossing of 13.5 ms removes half the layer stack for the rest of the run. A graduated response — half-rate slot 3 first, then slot 2, then refuse — would degrade instead of collapse.

---

## 7. Where the headroom actually is

**The `MAX_PIX` comment — "measured: 103 fps at 3456×2160 (7.5 Mpx)" — does not reproduce in 3.0.4 on this machine.** Measured across eight runs: **22.8 – 87.0 fps**, mean frame time 11.5 – 43.9 ms. Under contention, so treat the low end as pessimistic — but the *best* run measured, with **zero overlays on screen**, was 87 fps / 11.49 ms. That is the ceiling, not the operating point.

Honest budget at 60 Hz (16.67 ms) at 3456×2160:

| | ms |
|---|---|
| solo ground, render + composite **[M]** | 11.5 |
| **remaining for everything else** | **5.2** |
| cost of one overlay today (warp + blend, excl. render) **[M]** | 6.9 – 18.2 |

**There is no slack to spend. There is slack to *recover*.** Ranked by measured ms per frame at 3456×2160:

| recover | measured saving | risk |
|---|---|---|
| Skip / cache-block `layer_warp` | **4.2 – 12.8 ms per overlay** | medium — changes the resample, needs a visual check |
| NEON `span_max` / `span_screen` / `span_lerp` | **1.3 – 3.0 ms per overlay** | low — bit-identical output is achievable |
| Fuse any new mask into the blend rather than adding a pass | **4.6 ms** vs the naive form | low |
| Fix the probe-cache resolution bug (§8) | 0 ms directly | low — but it makes the budget test mean something |

- **Total recoverable, measured: ~6–15 ms/frame at 4K.**
- **How much can honestly be spent on the contrast work: about 1.5 ms per overlay per frame, and only after the NEON rewrite banks the first 1.3–3.0 ms.** That buys: multiply blend (+0.20), tile-map ground metric (+0.32 once per frame), fused regional mask (+0.22). Total **+0.74 ms/overlay/frame** — comfortably inside what the rewrite recovers.
- **It does not buy linear light (+3.76) and it does not buy a 5th layer (+10 to +15).** Pick one of those two later, from a measured position, not both and not now.

---

## 8. Two defects in the cost model, found while measuring

**8.1 The probe cache is not keyed by render resolution.**

- `cost_q8` is `fixed + slope · fullpix` evaluated at the framebuffer area of the run that measured it (`probe_advance`, `compositor.c:526–533`).
- `probe_stamp()` hashes routine count, scheme count and `sizeof(jd_stat)` — **not the area**.
- The resize path in `jd_frame` updates `g_probe_pix` but never rescales `g_st[].cost_q8`.
- **Measured on the live cache** (`~/Library/Application Support/JellyDazzle/probe-dec4f6e0.bin`, 634 routines, all probed): median `cost_q8` = **1.15 ms**, p90 = 5.78, max = 18.64. Measured in-engine render at 3456×2160: **4.87 – 17.24 ms per layer**. The cache is running ~6× low — consistent with having been probed at 1280×960 (6.07× fewer pixels).
- **Consequence:** at full Retina the `admissible()` budget test believes six layers fit inside `BUDGET_Q8` when two blow it. The budget is not protecting anything.
- **Fix:** store the probe area in the cache header; scale `cost_q8` by `new_area / cached_area` on load and on resize. The linear model is already there.

**8.2 `layer_warp` is not in the cost model at all.**

- `admissible()` charges `cost += n * 120` Q8 — **0.47 ms per layer** — as its entire composite allowance.
- Measured: blend alone is **2.67 – 5.41 ms**, warp is **4.21 – 12.77 ms**. The allowance is ~15× low.
- The warp is also untimed at runtime: `L->cost_ms` is measured around the pattern call only; the warp happens later in the overlay loop and is absorbed silently by `g_ewma_ms`.
- **Fix:** time the warp into `L->cost_ms`, and charge overlays a warp allowance scaled by area in `admissible()`.

**8.3 (minor)** Both spawn branches set `L->moving = 1`, so the documented `moving == 0 → skip the resample` fast path never fires. Grounds set it too, and grounds are never warped — the overlay loop is the only caller.

---

## 9. What to measure, before and after

### 9.1 Protocol

Use **five fixed start frames** and report medians. A single-run A/B on this engine is noise: measured 46.6 vs 52.8 fps from the same binary and the same start frame.

```
# throughput + strobe gate — the price
for S in 300000 700000 1300000 1900000 2500000; do
  gate battery 3456 2160 $S 3000 100
done
```

Take from each run:
- **`FPS`** — the throughput gate. Median across seeds. Regression threshold: any median drop is a cost that must be justified by a §9.2 gain.
- **`delta` column, max over samples** — the strobe gate. The engine's one law. Any change that raises max single-frame delta above baseline is a regression regardless of what it does to the picture.
- **`contrast` column (luma stddev)** — first cheap read on whether a darkening blend raised global contrast at all. Necessary, not sufficient.
- **`luma` column** — a darkening blend that drops mean luma below ~30 will start fighting the dead-air guard (`JD_LUMA_FLOOR`), which will lift the frame back with `span_gain` and undo the work. Watch for `g_gain > 260` firing more often than baseline.

```
# visibility gate — the point
for S in 300000 700000 1300000 1900000 2500000; do
  gate contrast 3456 2160 $S 3000 4 frames_$S.csv tenancies_$S.csv
done
```

Take: **V (p50 and vis_frac)**, **ground luma p10/p50/p90 and the >140 fraction**, and the **fog metric**. This is the number the whole exercise is trying to move; `battery` only says whether it was paid for.

### 9.2 What `battery` cannot tell you — add these

`gate battery` reports one aggregate FPS and cannot distinguish "the blend got more expensive" from "the scheduler gave me more layers". Both raise frame time. Instrument and record per run:

- **Per-stage split**: render / ground / warp / blend / other, ms per frame and ms per operation. (The scratch instrumentation used for this document wraps `layer_warp` and `blend_span` in `now_ms()` and totals into globals — worth landing behind a `JD_PROF` env in the real harness.)
- **`g_hot` duty cycle** — the single most diagnostic number in the engine. If a change pushes it from 88 % to 100 %, it removed slots 2 and 3 and the picture got *simpler*, whatever the fps says.
- **Per-slot live fraction, slots 1/2/3** — any "make the accents visible" change that drops slot 2's live fraction has made things worse in a way FPS will never show. Baseline at 3456×2160 is slot 2 = 0.0 %; the target is a number above zero.
- **Mean overlays live per frame** — baseline 0.00–1.00 at 4K.

### 9.3 Kernel-level, before landing anything

Benchmark the candidate `span_*` in isolation, best-of-15 over a 7.46 Mpx buffer, against these baselines. It is a two-minute test and it catches a 4 ms mistake before it reaches the engine:

| baseline | ms |
|---|---|
| `memcpy` (bandwidth peak) | 0.474 |
| bandwidth floor for one blend pass (85.5 MB) | 0.68 |
| `span_max` scalar (today) | 2.080 |
| `span_max` NEON (target) | 0.740 |

**Rule of thumb from these measurements:** a full-frame blend kernel above **1.5 ms** at 7.46 Mpx is ALU-bound and has a 2–3× NEON rewrite waiting in it. A kernel at **0.7–0.8 ms** is at the memory wall and is finished.
