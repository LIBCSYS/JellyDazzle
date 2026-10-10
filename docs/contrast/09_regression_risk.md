# 09 — Regression Risk Register: the contrast/compositing proposals

Scope: `src/engine/compositor.c` @ 3.0.4 (2859 lines). Evidence: the source, its
comments, and `docs/contrast/spawn_telemetry.csv` (1,962 spawns).

Proposals under review, as four classes:

- **P1** — new blend modes (multiply / darken / overlay-type kernels)
- **P2** — a ground-luma attenuation controller (dim bright grounds)
- **P3** — changed layer weights (`PEAK_LO` / `PEAK_HI`)
- **P4** — ground-dependent blend selection, incl. switching a live layer's blend

---

## 0. The finding that reframes the whole job

**The bug is not missing capability. It is a lerp-era weight cap applied to a
max-era blend kernel. Three of the guards named below are already causing it.**

### 0.1 `span_max` scales the overlay *before* the max, so weight is brightness, not opacity

```c
/* MAX: wherever the overlay is darker than the ground, the ground survives
 * bit-exact.  That is what keeps a stack readable instead of grey. */
static void span_max(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
        uint32_t srb = (((s & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;
        ...
        dst[i] = ... (dr > sr ? dr : sr) ...
}
```

The overlay is multiplied by `w` and *then* maxed. So an overlay pixel contributes
only when `(s*w)>>8 > d`, i.e. `s > d*256/w`. Since `s <= 255`:

> **An overlay under B_MAX is mathematically incapable of marking any ground
> channel value above its own peak weight.**

| slot | role | `PEAK_LO..HI` | telemetry mean peak | highest ground channel it can ever mark |
|---|---|---|---|---|
| 1 mid | FIELD/FIGURE | 128..180 | 143.8 | 179 |
| 2 accent | FIGURE/SPARK | 100..140 | 111.5 | **139** |
| 3 spark | SPARK/FIGURE | 72..115 | 91.1 | **114** |

That is the reported bug exactly, and it is slot-selective in the way the report
is: the mid slot survives a bright ground, the accent and spark slots do not.
Against a ground sitting at channel 200, an accent needs a source value of
`200*256/111 = 461`. There is no such pixel.

The cap's own justification is written for a blend overlays never receive:

```c
/* peak weight per slot, Q8 — an overlay that reaches 256 has replaced the
 * ground instead of enriching it */
```

True under `B_MIX` (a lerp). Untrue under `B_MAX`, where 256 means
`max(d,s)` — the ground still survives everywhere it is brighter. **Overlays are
never assigned `B_MIX`**: `try_spawn` gives `B_MIX` only to `sidx == 0`, and
`pick_blend` returns only `B_MAX`/`B_SCREEN`/`B_DIFF`. Telemetry confirms
1,074/1,074 overlay spawns:

- `B_MAX` 742 (69.1%), `B_SCREEN` 261 (24.3%), `B_DIFF` 71 (6.6%), `B_ADD` 0.

**93.4% of overlays composite through a lighten-only kernel, and lighten-only
kernels have vanishing contrast against a bright ground.** `B_SCREEN` is no better:
at `d=200, s=150, w=111`, `s' = 65`, result `200 + 65 - 51 = 214` — a 14-unit
lift, sub-threshold at that adaptation level. `B_ADD` clips. `B_DIFF` is the only
kernel that can darken and it is gated to `st->delta_q8 < 512 && (r & 15) == 3`.

### 0.2 The dead-air gain is applied TWICE per frame whenever audio is live

`g_gain` is written and consumed by the dead-air block, then written and consumed
*again* by the audio-bloom block in the same frame:

```c
/* dead-air guard */
    g_gain = want > g_gain ? g_gain + ((want - g_gain + 31) >> 5) : ...;
    if (g_gain > 260) span_gain(fb, fb, npix, g_gain);
...
/* AUDIO: bloom on the beat, capped at ~1.18x */
    if (bump > g_gain) g_gain += (bump - g_gain) >> 3; else g_gain -= (g_gain - bump) >> 5;
    if (g_gain > 260) span_gain(fb, fb, npix, g_gain);
```

`span_gain` mutates `fb` in place, so the compounded factor is `(g_gain/256)^2`:

- **Ordinary music, ordinary frame**: `g_gain` equilibrates ~264–272 → **1.13x**,
  against a comment that promises "brightness barely moves — 1.06x at most".
- **Dark frame with audio live**: `want` saturates at `JD_LUMA_MAXGAIN = 384`;
  `g_gain ≈ 380` → 1.484x, then the audio block re-applies ~1.473x → **≈ 2.19x**.
  The header says `JD_LUMA_MAXGAIN 384 /* <= 1.5x: 2.7x clamped the highlights
  and washed the colour out (J caught it) */`. **The clamp J won is being
  defeated by a second call site.**

Consequence for contrast: a bright ground at channel 220 is shipped at 249, where
`span_add` and `span_screen` have ~6 units of headroom and `span_max`'s threshold
`s > 249*256/111 = 574` is even further out of reach.

### 0.3 The palette reshape expands ground highlights on purpose

`layer_pal_build` maps `o = v + (6 + t*245 - v) * k`, `k = 1 - span/24000`, capped
0.85. Grounds run `SPAN[mood][0]` = 14000 / 20000 / 32768, plus the flat-routine
boost capped at 17000 — **all six slot-0 rows in telemetry carry `span=17000`**,
i.e. `k = 0.29`. A ground window `lo=40, hi=180` is remapped `40→26, 110→118,
180→210`. Mean luma barely moves; the **top of the ground's range moves up ~30
units**, precisely into the band where accents die.

**Framing to take to J: the fix is a correction to three existing
overcorrections, not a fifth control layer.**

---

## 1. Ranked risk table

Likelihood and severity 1–5; rank = product. "Applies to" names the proposal.

| # | Risk | Applies to | L | S | Rank |
|---|---|---|---|---|---|
| R1 | Darkening kernel breaks the "black is transparent" invariant that the `dark >= 70` admission rule is built on — blacks out ≥27.5% of the frame | P1 | 5 | 5 | **25** |
| R2 | Darkening kernel routes overlay motion into `fb`, `g_motion` crosses 5.5, CALM trims the accent 7/8 every 0.5 s until it is invisible again, then retires it | P1, P4 | 5 | 4 | **20** |
| R3 | New per-slot state not added to the ground-promotion swap block → the documented "frozen at sl==235, replaced 240 frames later, for ever" loop | P2, P4 | 5 | 4 | **20** |
| R4 | Attenuation written into `L->buf` — compounds on accumulators, trips the JUMP detector, and **persists a poisoned `delta_q8` to the on-disk probe cache**, blacklisting the routine across launches | P2 | 4 | 5 | **20** |
| R9 | Mid-layer blend switch: a single-frame full-region cut that **neither strobe detector can see** | P4 | 4 | 4 | **16** |
| R8 | Raised `PEAK_HI` inflates the `admissible()` motion charge (×`PEAK_HI`×`amp_q8`) → accents refused on `DCAP_Q8`; also saturates the audio-surge clamp | P3 | 5 | 3 | **15** |
| R7 | New kernel / measurement pass raises `g_ewma_ms` past 13.5 → `g_hot` blocks spawns for **slots 2 and 3 — the accent and spark slots** | P1, P2 | 3 | 5 | **15** |
| R5 | Attenuation silently cancelled by the dead-air gain (up to 1.5x) below composite luma 30; hard floor at 20 | P2 | 5 | 3 | **15** |
| R10 | Ground-dependent selection reads a stale luma — `st->luma` is probe-time and palette-blind; `g_gluma` carries the *previous* ground for ~70 frames after promotion | P4 | 4 | 3 | **12** |
| R11 | Attenuation implemented in the palette → gated by the `g_blend_key` short-circuit and the frozen `lo_s/hi_s/lut`, so it lands in 5–15-frame steps: the break the code already measured | P2 | 3 | 4 | **12** |
| R6 | Attenuation drives `g_gluma` under 20 → forced handover churn, ground lifetime cut ~85% | P2 | 3 | 4 | **12** |
| R13 | A weight change that gates ground presence on `w16` → `!ng` blank frame + boot-ramp restart (measured composite luma 0.0 against a ground buffer at 131) | P3 | 2 | 5 | **10** |
| R12 | A stricter ground admission rule drives `g_gpostponed` to 300 → `g_force = 1` waives budget, motion, probed-ness *and* the overlay dark rule | P2, P4 | 3 | 3 | **9** |
| R14 | Three-way fight between attenuator, boot ramp and dead-air gain in the first 2 s — the most-watched 2 s of the app | P2 | 3 | 3 | **9** |
| R15 | Buffer attenuation burns the two-shot AUDITION budget (`lb < 18 && la > 3*lb`, `g_audition < 2`) | P2 | 2 | 3 | **6** |
| R16 | Peaks lowered under `FREEZE_W = 38` → the layer stops rendering the instant it starts falling | P3 | 2 | 2 | **4** |

---

## 2. Per-risk detail

### R1 — a darkening kernel inverts the meaning of the overlay admission rule

```c
    /* 6. an overlay must have somewhere to be seen through: it has to be at
     *    least a third near-black, or it just paints over the ground */
    if (slot_i != 0 && st->dark < 70) return 0;
```

`dark` is the fraction of pixels with luma < 16, scaled /255 — so **every admitted
overlay is ≥ 27.5% near-black by construction**, and the whole overlay library was
selected on that basis. Under MAX/SCREEN/ADD/DIFF, a black source pixel is a
no-op: `max(d,0)=d`, `d+0-0=d`, `d+0=d`, `(d*iw + |d-0|*w)>>8 = d`. The engine
states the invariant itself, in the `layer_warp` comment:

```c
             * is the "you can see the box" artefact: ... It only showed on
             * B_MIX, because black is a no-op under MAX/SCREEN/ADD
             * - which is why it looked intermittent.
```

**Under multiply, `0 * d = 0`.** The 27.5%-minimum near-black region stops being
transparent and starts being an opaque black stencil punched through the ground.
On the sparsest admitted material (`dark` near 255) that is ~95% of the frame
going black at the overlay's weight.

Compounding: `admissible()` cannot know the blend. `bag_draw` runs first;
`pick_blend` runs afterwards inside `try_spawn` on the already-drawn candidate. So
a per-blend admission rule is not expressible where the rules currently live.

**Requirement if P1 ships:** any darkening kernel needs its own admission
predicate (`dark` *low*, not high), which means blend selection must move ahead of
`bag_draw` or `admissible` must take a candidate blend. Do not ship a darkening
kernel behind the existing `dark >= 70` gate.

### R2 — the composite-motion loop is the trap that undoes the fix

`motion_probe` samples **`fb`**, the composite:

```c
    g_motion = g_motion * 0.9 + d * 0.1;
...
    if (g_motion > 5.5) {
        if (++g_jitter > 30) {
```

Under B_MAX over a bright ground, an invisible accent contributes **exactly zero**
composite motion — the ground survives bit-exact, so the overlay's animation is
fully suppressed. That suppression is why the budgets are set where they are:
admission allows `DCAP_Q8 = 6.5` weighted motion units, while CALM fires at 5.5 on
the realised composite. The 1.0-unit inversion is only survivable because MAX
throws motion away.

A multiply kernel is a product: `Δ(d·s/255) = (d·Δs + s·Δd)/255`. At `d = 200`
the overlay's own motion reaches the composite at ~0.78x. **Removing the
suppression moves realised composite motion toward the admitted 6.5 — above the
5.5 trigger, permanently.** Then:

- `g_jitter` reaches 30 every 30 frames → **one corrective action every 0.5 s**.
- Trim path: `L->w_peak = L->w_peak * 7 / 8`. From 140: 122, 107, 94, 82, 71, 63,
  55, 48, 42, 37. **The accent is back below its original visibility in ~4 s and
  under `FREEZE_W` in ~5 s.**
- Once `w_peak <= 40` the trim branch is skipped and CALM retires the layer.

**Worse — the blame is misattributed.** CALM ranks by `mdel * w_now`, and `mdel`
is measured from `L->buf`, which the blend never touches:

```c
                uint32_t c = ((uint32_t)g_L[s].mdel * g_L[s].w_now) >> 8;
                if (c > wd) { wd = c; worst = s; }
```

The multiply accent's *buffer* motion is unchanged, so CALM will repeatedly dim
and retire whichever layer has the highest buffer motion — an innocent bystander —
while the actual source of the composite motion runs untouched. Expect: "the
accents show up now but the mid layer keeps disappearing."

**Requirement:** if any kernel passes ground content through multiplicatively,
`mdel` must be measured post-blend (per-layer composite contribution) or the CALM
blame heuristic must be re-derived. Re-validate `DCAP_Q8` against 5.5 after any
kernel change.

### R3 — the promotion swap is a checklist, and it is already documented

```c
        /* The motion signature is per SLOT, not per layer, so it has to
         * travel with the tenant.  Leaving it behind makes the promoted
         * ground's first measured step a comparison against the OUTGOING
         * ground's pixels — a whole-frame difference, which the strobe
         * detector below reads as a JUMP and answers by freezing the fresh
         * ground and bringing its handover forward.  That self-perpetuates:
         * measured before this line existed, every ground was frozen at
         * sl==235 and replaced 240 frames later, for ever. */
```

`sched_tick` swaps `g_L[0] <-> g_L[JD_SHADOW]`, plus `g_pal`, `g_buf`, `g_lsig`,
`g_lsig_ok`. **Anything a new controller keeps per-slot outside `jd_layer` and
outside that block — an attenuation EWMA, a ground-luma history, a per-slot blend
state — reproduces the forever-loop verbatim.** This is the single most likely
implementer omission in the set.

Note `g_gluma` is *already* in this category: it is per-engine, is not reset on
promotion (only `g_gpostponed` and `g_audition` are), and its
`else if (... sl <= 60) g_gluma = 60;` branch cannot fire after a promotion
because the shadow ran for `g_lead = 90..180` frames first. **For ~32–70 frames
after every handover, `g_gluma` describes the ground that just left.** Any
controller reading `g_gluma` inherits that.

**Prefer:** put new per-tenancy state as fields inside `jd_layer` — the struct is
swapped whole — never in a parallel `[JD_NBUF]` array.

### R4 — attenuating `L->buf` is three separate failures at once

1. **It compounds.** `C_CANVAS` layers (asm 15..23) accumulate into their own
   buffer and are explicitly primed to do so (`canvas_prime`), and feedback-class
   patterns read their previous frame. Multiplying that buffer by `a` each frame
   is `a^n`: at `a = 0.8`, 30 frames to 0.001. The engine then reads
   `g_gluma < 20` and retires the ground, or `!ng` blanks. And this hits precisely
   the modes the fix targets — the `canvas_prime` comment records that modes 17
   and 20 paint **white** canvases.
2. **It trips the JUMP detector on the frame it is switched on.** The detector
   samples `L->buf` against `g_lsig[s]` and fires at `dq8 > 4096` (mean channel
   delta 16.0). A one-shot scale to `a = 0.7` of a ground at channel mean 150
   yields `dq8 = 45*256 = 11520` — **2.8x over threshold**. A ground survives one
   strike (`if (sidx != 0 || L->strikes++) L->frozen = 1;`) but still gets
   `t_out = frame + 60`; a second event freezes it.
3. **It poisons the persisted stats.** The JUMP handler writes the measurement
   back: `g_st[L->routine].delta_q8 = (uint16_t)(dq8 > 65000 ? 65000 : dq8);`.
   `admissible()` rule 5 caps the weighted sum at `DCAP_Q8 = 1664`; a `delta_q8`
   of 11520 refuses that routine everywhere, for the rest of the run. And
   `recent_note()` calls `probe_cache_save()` **on every spawn**, which writes
   `g_st` wholesale to
   `~/Library/Application Support/JellyDazzle/probe-<stamp>.bin`. The stamp is
   `mix32(magic ^ g_nr<<8 ^ g_ns ^ sizeof(jd_stat)<<24)` — deliberately *not*
   version-keyed — so **the poisoned motion figure survives every subsequent
   launch of every build with the same library size.** A routine can be retired
   from the library permanently by one attenuation transient.

**Requirement:** ground attenuation must be applied to `fb` between the ground
composite and the overlay loop, never to `L->buf`. If a gradual ramp into `L->buf`
is ever unavoidable, it must spread over ≥ 30 frames to stay a quarter under the
4096 trip with the routine's own motion on top.

### R5 — attenuation vs. the dead-air gain: silent cancellation, not oscillation

The dead-air gain is **feed-forward, not feedback**: `luma` is sampled from `fb`
*before* `span_gain` runs, and `fb` is rebuilt from layer buffers next frame. The
gain never sees its own output. **So two controllers on the same scalar will not
oscillate — they will silently cancel.**

```c
        uint32_t luma = sum / 512, want = 256;
        if (luma < JD_LUMA_FLOOR) {
            want = (JD_LUMA_FLOOR * 256) / (luma < 4 ? 4 : luma);
            if (want > JD_LUMA_MAXGAIN) want = JD_LUMA_MAXGAIN;
        }
```

With attenuation `a` on a composite of true mean luma `L`:

| `a·L` | dead-air response | net shipped luma | attenuation actually delivered |
|---|---|---|---|
| ≥ 30 | none | `a·L` | full |
| 20 – 30 | `want = 30/(a·L)` | **30** | **zero — fully cancelled** |
| < 20 | gain saturates at 1.5x | `1.5·a·L` | 1.5x of it clawed back |

Worked: a legitimately moody frame at `L = 45`, attenuated `a = 0.6` → 27 → gain
1.11x → **ships at 30, exactly as if the attenuator did not exist**. There is a
hard floor of composite mean 20 that nothing downstream can go below.

**Ordering rules, in priority order:**

1. Attenuate **before** the dead-air measurement, or the gain is computed for a
   frame that is not the one shipped.
2. Never attenuate **after** `span_gain` — that reintroduces the exact dead air
   `JD_LUMA_FLOOR` exists to prevent.
3. **Merge, do not stack.** The attenuator and the dead-air gain are one
   controller with opposite signs on one measured quantity. Compute a single
   output factor in `[a_min, JD_LUMA_MAXGAIN]` from one measurement and apply
   `span_gain` **once**. The current double-`span_gain` (§0.2) is the existing
   proof of what stacking costs.

### R6 — the dark-ground churn loop is real but placement-dependent

```c
        g_gluma = (g_gluma * 31 + la) >> 5;                  /* ~0.5 s EWMA */
        if (g_gluma < 20 && g_L[0].t_out > frame + 240) {
            g_L[0].t_out = frame + 60; g_L[0].t_end = g_L[0].t_out + FADE_OUT[0];
```

`la` is sampled from **`g_L[0].buf`** — the ground's own buffer. So:

- **Composite-time attenuation is invisible to this guard.** No churn. (This is
  the second reason to apply at `fb`, after R4.)
- **Buffer-time attenuation is fully visible.** Trip condition is `a·G < 20`:

| ground luma `G` | attenuation that trips the guard |
|---|---|
| 200 | `a < 0.10` |
| 120 | `a < 0.167` |
| 60 (the reset default) | `a < 0.33` |
| 40 | `a < 0.50` |

A *ground-luma-dependent* attenuator that only dims bright grounds is therefore
safe by construction; a **flat** attenuator is not — it takes a legitimate luma-40
ground under at `a = 0.5`, which is well inside the range being discussed.

Churn cost when it does fire: `g_gluma` is pinned to 60 for a ground's first 60
frames, then converges with a 32-frame time constant — `32·ln(45/5) ≈ 70` frames
to cross 20 from a start of 60. So the guard fires ~130 frames (2.2 s) in, then
`t_out = frame + 60`, `t_end = +180`. **Ground retired ~4 s into a `HOLD_LO..HI`
tenancy of 1080–1640 frames (18–27 s): an ~85% cut, i.e. a handover every 4–5 s
instead of every 20–27 s.** Every one of those handovers also resets the mood, the
tempo (`g_tempo`), `g_lead`, and gates all overlay entry while it runs —
the whole show's rhythm collapses.

### R7 — cost is a closed loop with the accent slots, and it points the wrong way

```c
    if (!g_hot && g_ewma_ms > 13.5) {
        g_hot = 1; g_cool = 0;
        if (no) {                                  /* half-rate the top layer */
            g_L[ov[no - 1]].half = 1;
```
```c
        if (handover || (g_hot && s >= 2) || frame - g_last_change < g_gap) continue;
```

`g_hot` blocks spawning **slots 2 and 3 — accent and spark — the exact layers whose
invisibility is the complaint.** A fix that costs frame time makes the engine stop
producing the thing being fixed.

Budget arithmetic:

- `BUDGET_Q8 = 10.5 ms` (admission) vs `g_hot` at 13.5 ms — 3 ms of margin.
- Admission charges a flat `cost += n * 120;` = **0.469 ms per blend**. A kernel
  costing 0.9 ms under-reads by 0.43 ms per layer; at 4 layers the engine admits a
  stack it believes costs 10.5 ms that actually costs 12.2 ms — inside the
  `g_hot` boundary's noise.
- A **full-frame** luma measurement pass is ~1.23 Mpx at 1280x960, comparable to a
  whole blend pass. Every existing guard deliberately samples **512 of 1.23 M
  pixels (0.04%)** — `int st = npix / 512;` appears four times in the file. Match
  that convention; do not add a full-frame measurement.
- Also on the critical path: `palette_update` rebuilds 32,768 entries for the
  shared ramp **plus every live layer's window and reshape** whenever `t8` steps.
  Over a 1024-frame leg `t8` walks 0→256, and smootherstep's mid-leg derivative is
  1.875x average, so at the leg midpoint that is a ~164 k-pixel rebuild **every
  ~2 frames**. Anything that forces extra rebuilds multiplies the engine's largest
  existing cost spike.

**Second-order:** when `g_hot` half-rates the top overlay, a multiply accent
updates every other frame, so its per-frame composite delta on the frames it does
change **doubles** — feeding R2. Load shedding and the motion governor amplify
each other.

**Requirement if P1/P2 ship:** update the `n * 120` constant to the measured cost
of the new kernel, and re-measure `g_ewma_ms` on a 4-layer stack at 1280x960
before and after.

### R8 — changed peaks touch four systems, not one

`PEAK_LO`/`PEAK_HI` are read in four places:

1. **The weight itself** — `layer_w_q16` returns `(e * L->w_peak) >> 8`.
2. **The admission motion charge**, and this is the one that bites:
   ```c
       uint32_t d = ((uint32_t)st->delta_q8 * PEAK_HI[slot_i]) >> 8;
       d = (d * amp_q8(slot_i)) >> 8;
       ...
       if (d > (uint32_t)DCAP_Q8) return 0;
   ```
   For the accent slot in `M_RICH`, `SPAN[M_RICH][2] = 8000` → `k = 0.667` →
   `amp_q8 = 427` (1.67x). At `PEAK_HI = 140` a candidate is charged
   `delta * 0.911`. **Raise it to 200 and the charge rises 43%**, against a fixed
   `DCAP_Q8 = 1664` that the live ground (at `w_peak = 256`, full delta) is
   already consuming. Net: **raising the accent's peak makes accents harder to
   admit.** The fix again removes the layer it wanted to show.
3. **The trim/retire fork** in both the JUMP handler and CALM: `L->w_peak > 40`
   chooses "turn it down" over "retire it".
4. **The audio surge clamp**:
   ```c
            uint32_t v = ((uint32_t)L->w_now * g_surge) >> 8;
            L->w_now = (uint16_t)(v > 256 ? 256 : v);
   ```
   `g_surge` reaches 410 (1.6x) on a drop. At the mid slot's existing `PEAK_HI =
   180` the surge saturates at 1.42x and the top 11% of the audio range is already
   dead. **Push the accent peak to 200 and its surge saturates at 1.28x — the
   accent stops responding to the music**, which is a quality regression traded
   for a contrast gain that R2 will claw back anyway.

### R9 — a mid-layer blend switch is the one break neither detector can catch

Nothing in 3.0.4 changes `L->blend` after spawn. Every visual change in this engine
is enveloped; `layer_w_q16`'s comment states the principle: *"Every entry, exit and
handover in the engine runs through here, which is why nothing in the picture ever
cuts."*

Magnitude of a switch at fixed weight: accent `w = 120` over ground `d = 200`,
source `s = 150`. B_MAX contributes `(150*120)>>8 = 70 < 200` → **nothing**;
ground survives bit-exact. Multiply gives `200*150/255 = 118`. **A per-pixel step
of 82 channel units, in one frame, across the accent's whole coverage.**

Neither detector sees it:

- The **JUMP detector** samples `L->buf`, which the blend never touches. Blind by
  construction.
- The **composite detector** would see one frame of `d ≈ 0.3 * 82 = 24.6` (30%
  coverage) → `g_motion += 2.46`, decaying at 0.9/frame. It needs `g_motion > 5.5`
  on **30 consecutive frames** to act. A single-frame spike decays back under
  threshold in ~3 frames. Blind in practice.

So a blend switch is a hard cut that no guard reports and a viewer sees
immediately — the worst possible combination in a codebase whose stated law is
"nothing strobes". The `shape_kick` comment already draws the correct line:
*"A hard cut of the whole frame is the worst break this engine can make, and a key
press is not a licence for it."*

**Requirement:** do not switch a live layer's blend. If ground-dependent blending
is wanted, either (a) choose at spawn and hold, or (b) cross-fade by compositing
both kernels into a scratch and lerping between them over ≥ `FADE_IN[sidx]`
frames — which doubles that layer's blend cost and feeds R7.

### R10 — no accurate ground-luma reading is available where P4 needs one

Both candidate inputs are wrong in different ways.

**`st->luma`** — what `pick_blend` already uses for its SCREEN cap:
```c
        uint32_t gl = g_L[0].live ? g_st[g_L[0].routine].luma : 100;
        uint32_t cap = gl >= 160 ? 154u : gl <= 40 ? 64u : 64u + (gl - 40) * 90u / 120u;
```
This is the **probe** measurement: 320x240, rendered against `jd_palette` (scheme
0), before the layer's palette window, before the reshape stretch, before `wild`,
before `hrot`, before `layer_warp`, and before the mood was chosen. Telemetry
shows 96.5% of grounds (857/888) are palette-driven patterns, so `st->luma`
describes a colouring the ground will never actually wear. **The existing SCREEN
cap is already keyed to a stale proxy; P4 must not inherit it.**

**`g_gluma`** — live, but only conditionally maintained:
```c
    if (g_L[0].live && !g_L[JD_SHADOW].live && g_L[0].sl > 60) { ... }
    else if (g_L[0].live && g_L[0].sl <= 60) g_gluma = 60;
```
- Frozen for the entire duration of every handover (`g_L[JD_SHADOW].live`).
- Carries the previous ground's value for ~32–70 frames after promotion (R3).
- 32-frame EWMA, so it lags any ground whose palette leg is walking.

Overlays are gated from spawning during a handover
(`if (handover || ...) continue;`), so the first overlays after a promotion spawn
into exactly the window where `g_gluma` is still the old ground's. **A blend
chosen from `g_gluma` at spawn time will systematically mis-read the first
overlays under every new ground.**

**Requirement:** if P4 ships, sample the ground plate at composite time (512
samples of `fb` immediately after the ground `memcpy`/lerp, before overlays), keep
it in a variable that is reset on promotion, and make the *blend response*
continuous in that value rather than a threshold — a threshold on a lagging EWMA
is a scheduled pop (R9).

### R11 — do not implement attenuation in the palette

Two guards make `layer_pal_build` the wrong home:

```c
    if (key == g_blend_key && !g_cfade) return;
```
The per-frame path is short-circuited whenever the leg has not moved, and the
`for (s) layer_pal_build(s)` loop sits **after** that return. A palette-level
attenuator would therefore apply only on rebuild frames — 2 to 15 frames apart.
The file already records that failure by name:

```c
        /* Measure the window ONCE, at spawn, and freeze the transfer curve.
         * Re-measuring on every rebuild was itself a rough break: rebuilds
         * are 5-15 frames apart, so each re-smoothing step moved the whole
         * layer's colour at once.  A fixed monotone curve maps a smoothly
         * drifting ramp to a smoothly drifting image, by construction. */
```

Attenuating the ground palette is exactly "re-measuring on every rebuild" with a
different name.

### R12 — a stricter ground rule triggers its own total bypass

```c
            /* `frame >= t_out + 300` can never fire: the postponement below
             * pushes t_out 30 frames per 30 frames elapsed, so the deadline
             * runs away from the clock at exactly the speed of the clock.
             * Measured: 3727 consecutive frames of refusals with the ground
             * pinned live.  Count the deferral instead of chasing t_out. */
            if (clear_soon || g_gpostponed >= 300) {
                g_force = 1;
```

Any contrast fix expressed as an admission rule ("don't schedule a ground brighter
than X") raises the refusal rate for the ground bag. At 300 accumulated deferral
frames — 5 seconds — `g_force = 1`. Under force, `admissible()` returns 1 after
only rules 0–3; **the budget cap, the `DCAP_Q8` motion cap, the probed-ness check
and the overlay `dark >= 70` rule are all waived**. The new brightness rule waives
itself along with every other taste rule. `try_spawn`'s force block also scans
*every* bag, so a FIGURE routine can land on the ground slot.

**Requirement:** express contrast policy as a *rendering* decision (weight,
kernel, gain), never as an admission rule. Admission refusals in this engine
terminate in `g_force`.

### R13 / R14 — the boot ramp and the `!ng` blank path

```c
    /* A GROUND counts as present whenever it is LIVE, even on the frames its
     * eased envelope still rounds to zero.  It used to be gated on w16 != 0
     * like an overlay, and smootherstep underflows to 0 for the first ~3
     * frames of a 180-frame fade — so every fresh ground opened with a
     * two-or-three-frame window in which the engine believed it had NO
     * ground, took the !ng branch, blanked the frame and RESTARTED the boot
     * ramp. ... measured, composite luma 0.0 against a ground buffer at luma 131. */
        if (s == 0 || s == JD_SHADOW) { if (ng < 2) gnd[ng++] = s; }
        else if (L->w16 && no < JD_NSLOT) ov[no++] = s;
```

**Hard rule for P3: ground presence must stay unconditional on weight.** Any
"skip a fully-attenuated ground" or "ground weight can reach zero" change re-enters
`if (!ng) { g_frame0 = frame; memset(fb, 0, ...); return; }` — a blank frame *and*
a 120-frame boot-ramp restart, on a frame where the ground buffer holds a perfectly
good picture.

The boot ramp is itself already in a fight the attenuator would join:

```c
        uint32_t bw = 48 + ((e * 208) >> 16);
        if (bw < 256) span_scale(fb, fb, npix, bw);
```

At frame 0, `bw = 48` → the frame ships at 18.75%. The dead-air guard runs
**after** the boot ramp and measures the dimmed frame: a ground at true luma 120
measures 22.5, under `JD_LUMA_FLOOR = 30`, so `want = 341` (1.33x). **The dead-air
gain already partially undoes the boot fade for the first ~40 frames of every
run.** Adding a third actor makes the opening two seconds — the most-watched two
seconds of the app — the product of three uncoordinated controllers.

### R15 — the AUDITION budget is two shots and buffer attenuation spends them

```c
    if (ng == 2 && g_L[JD_SHADOW].live && g_L[JD_SHADOW].sl == 6 && g_audition < 2) {
        ...
        if (lb < 18 && la > 3 * lb) { ... g_L[JD_SHADOW].live = 0; ... g_audition++; }
```

Reads `L->buf`, fires on **exactly** `sl == 6` (one frame), and is limited to two
uses per handover (`g_audition` is reset only at promotion). Buffer-time
attenuation at `a = 0.5` takes a legitimate `lb = 30` shadow to 15 → under 18 →
killed and redrawn. Two such kills exhaust the budget and the **third** candidate
is installed unexamined — the guard's protection is spent on grounds the
attenuator darkened rather than on grounds that were dark. It also returns the
scheduler to the `g_gpostponed` path (R12). Another consequence of R4's placement
error; composite-time attenuation is immune.

### R16 — `FREEZE_W` is a floor under any peak reduction

```c
        int falling = frame > L->t_out;
        if (falling && L->w_now < FREEZE_W && L->cls != C_CANVAS) continue;
```

`FREEZE_W = 38`. A peak at or below 38 means the layer stops being rendered the
instant it starts falling and dissolves as a still image. Note the existing trim
chain can already reach there — `PEAK_LO[3] = 72`, one JUMP trim (5/8) → 45, one
CALM trim (7/8) → 39, another → 34 — so **do not lower `PEAK_LO` further** without
re-deriving `FREEZE_W`.

---

## 3. What actually fixes it, at the lowest risk

Ranked by risk-adjusted value. The first item makes the other three optional.

### F1 — reorder `span_max` so weight means opacity (one line, no new state)

```c
/* current: scale-then-max — weight is the overlay's BRIGHTNESS */
uint32_t srb = (((s & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;
dst[i] = max(d_channels, s_scaled_channels);

/* proposed: max-then-weight — weight is the overlay's OPACITY */
/* per channel: out = d + (((max(d,s) - d) * w) >> 8) */
```

What it preserves, exactly:

- **"Black is transparent" survives.** `max(d,0) = d` → contribution 0. R1 is not
  triggered; the `dark >= 70` admission rule keeps its meaning.
- **"The ground survives where the overlay is darker" survives.** `max(d,s) = d`
  wherever `s < d` → contribution 0, bit-exact. The comment's stated intent is
  unchanged.
- **No new mode, no new controller, no new per-slot state.** R3, R4, R6, R10, R11,
  R12, R15 do not apply.
- **No admission change.** R8 and R12 do not apply.
- **No blend switching.** R9 does not apply.
- **Cost:** one subtract, one multiply, one add per channel — comparable to
  `span_screen`, which the engine already runs on 24% of overlays. R7 is bounded
  and measurable.

What changes: an accent at `w = 111` over a ground at 200 now contributes
`(255-200)*111/256 = 24` where its source is at 255 — visible, and **bounded by
`w`**, which is what the peak caps were written to bound in the first place.

The one risk it does carry is **R2**: overlay motion now reaches the composite over
bright grounds, where MAX previously suppressed it to zero. But the increase is
bounded by `w/256` (≤ 55% for the mid slot, ≤ 43% for accents) rather than the
~78% a multiply passes through, and `g_motion`/CALM is already the correct
governor for it. **Measure `g_motion` on a 4-layer bright-ground stack before and
after; if it sits above 5.5, the correct response is to re-derive `DCAP_Q8`, not to
re-cap the peaks.**

### F2 — fold the audio bloom into a single gain application

`span_gain` is currently called twice per frame on the same `fb` with the same
`g_gain` (§0.2), delivering ~1.13x on ordinary music and ~2.19x on dark frames —
past the 2.7x J already caught and clamped. Compute one `want` from the max of the
dead-air target and the audio bump, ease it once, apply `span_gain` once. This
recovers ~13% of highlight headroom on every audio-live frame, which is headroom
the accent needs under `span_screen` and `span_add`.

### F3 — only if F1 and F2 are insufficient: attenuate at the composite, merged

If a ground attenuator is still wanted after F1/F2:

- Apply to `fb`, after the ground `memcpy`/lerp, **before** the overlay loop.
  Never to `L->buf` (R4), never to the palette (R11).
- **Merge it with the dead-air gain into one controller** producing one factor in
  `[a_min, JD_LUMA_MAXGAIN]` from one 512-sample measurement, applied once (R5).
- Make the response continuous in ground luma, and make it a no-op below ground
  luma ~90 — where accents are already visible and where `g_gluma`'s 20-threshold
  and `JD_LUMA_FLOOR`'s 30-threshold both live.
- Sample at 512 points, matching every other guard in the file (R7).

### F4 — do not ship, in this order

- **A multiply/darken kernel behind the existing `dark >= 70` gate** (R1). If one
  ships at all, it needs its own inverted admission predicate, which means moving
  blend selection ahead of `bag_draw`.
- **Mid-layer blend switching** (R9) — a cut no guard can see.
- **Raised `PEAK_HI`** (R8) — it reduces accent admissions and kills the audio
  surge response. F1 delivers the same visibility without touching the caps.
- **Contrast policy expressed as an admission rule** (R12) — it waives itself via
  `g_force` five seconds later.

---

## 4. Mandatory pre-merge checks for any of P1–P4

1. `g_motion` on a 4-layer stack over a luma-180+ ground, 2000 frames — must not
   hold above 5.5 for 30 consecutive frames. Count `TRIM` and `CALM` lines with
   `JD_DEBUG=1`; a rate above ~1 per 10 s means R2 has landed.
2. `g_ewma_ms` before/after at 1280x960 with 4 layers — must stay under 12.0 to
   keep the 13.5 `g_hot` margin. Confirm slots 2 and 3 still spawn.
3. Grep every new `static ... [JD_NBUF]` against the promotion swap block in
   `sched_tick`. Prefer fields inside `jd_layer` (R3).
4. `JD_DEBUG=1` for 5 minutes: count `DIM` lines (ground-luma handovers) and
   ground spawn intervals. Ground tenancy must stay in the 18–27 s band, not fall
   to 4–5 s (R6).
5. Diff the probe cache before and after a run
   (`~/Library/Application Support/JellyDazzle/probe-*.bin`): no routine's
   `delta_q8` should have moved by more than its own honest motion (R4).
6. Frame-by-frame capture of the first 200 frames, checking for the boot ramp
   restarting and for the emblem overlay (frames 0–194) landing on a dimmed
   ground (R13, R14).
7. `JD_TELEMETRY=` a new run; confirm the blend histogram and per-slot peak means
   still match the design intent and that no slot's mean peak has drifted below
   `FREEZE_W = 38` (R16).
