# 02 — Blend selection policy: ground-driven, not dice-driven

**Status:** design. Nothing here is committed to `compositor.c` yet.
**Target:** `src/engine/compositor.c` — `pick_blend()` (line 1618), `blend_span()` (1412), the weights loop (2436), the overlay composite loop (2604).
**Evidence:** `docs/contrast/spawn_telemetry.csv`, 1,962 rows, 1,074 overlay spawns.

---

## 1. What governs the choice today

### 1.1 The function, exactly

`pick_blend(st, r, &peak, slot)` — `st` is the *overlay routine's* probe stats, `r` a hash of `frame*2654435761 + slot*7919`, `slot` the destination slot.

- **Default is `B_MAX`.** Nothing has to happen for MAX to win; it wins by falling through.
- **`B_SCREEN` if `st->dark >= 215 && st->sat >= 25`** — the overlay routine is ≥84% near-black and coloured.
- **`B_SCREEN` if `g_mood == M_BLAZE && st->dark >= 180`.**
- **`B_DIFF` if `st->delta_q8 < 512 && (r & 15) == 3`** — 1-in-16 of *slow* routines. The comment calls it "seasoning". Peak clamped to 90.
- **Peak** `p` is uniform in `[PEAK_LO[slot], PEAK_HI[slot]]`, drawn from `r >> 16`.
- **One ground-aware term exists, and only for SCREEN:** `p` is capped against `g_st[g_L[0].routine].luma`, the ground routine's *probe-time mean luma*.

### 1.2 What that means

- **Every input except one is a property of the overlay, the mood, or the dice.** `st->dark`, `st->sat`, `st->delta_q8` describe the layer being spawned. Nothing describes what it will land on.
- **The one ground term is a stale scalar and it does not touch the mode.** `g_st[rt].luma` was measured once, at probe resolution, under the probe's palette. The live ground's luma moves continuously — the palette walk drags the index band through light and dark stretches of the ramp, which is the entire reason the DIM guard at line 2563 exists. `pick_blend` reads a number that the engine itself does not trust enough to use for the dim check.
- **It only sees slot 0.** An accent in slot 2 composites over ground **plus** slot 1, and slot 1 can be sitting at `w_now` 180 covering the whole frame. That layer is invisible to the policy.
- **Two of five blends are unreachable for overlays.** `pick_blend` can return only `B_MAX`, `B_SCREEN`, `B_DIFF`. `B_ADD` is live code in `blend_span` that nothing ever selects; `B_MIX` is ground-only.

### 1.3 What the telemetry actually shows

Overlay spawns only (slots 1/2/3; slots 0 and 4 are always `B_MIX`):

| slot | n | MAX | SCREEN | DIFF | ADD | MIX |
|---|---|---|---|---|---|---|
| 1 mid | 360 | 82.5% | 10.3% | 7.2% | 0 | 0 |
| 2 accent | 362 | 60.8% | 31.5% | 7.7% | 0 | 0 |
| 3 spark | 352 | 63.9% | 31.3% | 4.8% | 0 | 0 |

Cross-tabulated against mood, which is the finding that matters:

| mood | n | MAX | SCREEN | DIFF |
|---|---|---|---|---|
| M_STARK | 264 | 94.7% | 0.0% | 5.3% |
| M_RICH | 408 | 92.6% | 0.5% | 6.9% |
| M_BLAZE | 402 | 28.4% | 64.4% | 7.2% |

- **The `dark>=215 && sat>=25` rule fired twice in 672 non-blaze overlay spawns.** It is dead in practice.
- **SCREEN is not a blend decision, it is a mood readout.** 259 of 261 SCREEN picks were M_BLAZE. The engine's actual policy is: *MAX, unless the mood is BLAZE, in which case SCREEN; plus 3.6% DIFF sprinkled on slow routines.*
- Mean peak by mode: MAX 125.0, SCREEN 96.2, DIFF 89.1.

### 1.4 Why this produces the reported symptom

The complaint — *"foggy clutter where the accent is barely visible"* — is the two failure modes of this table, one per mood family:

- **MAX over a bright ground is a no-op.** `span_max` keeps the ground bit-exact wherever ground > overlay. On a ground at luma 160 an accent at `w_peak` 140 has been pre-multiplied down to a peak of ~140/255 of its own values before the comparison — it wins on almost no pixels. 92% of STARK/RICH overlay spawns are MAX. The accent is *there* and *invisible*.
- **SCREEN over anything that is not dark is fog.** `span_screen` is inverse-multiply: it can only lighten, and it compresses everything toward white, which destroys local contrast rather than adding it. 64% of BLAZE overlay spawns are SCREEN. That is the fog.

Both are the same root cause: **the mode is chosen without knowledge of the ground, so it is right by luck.**

---

## 2. The signal

### 2.1 Is `g_gluma` the right one? No — right instinct, wrong tap point

`g_gluma` (declared 1456, updated 2563):

```c
g_gluma = (g_gluma * 31 + la) >> 5;   /* ~0.5 s EWMA */
```

Four problems, in order of severity:

- **It is gated off exactly when it is needed.** The update is inside `if (g_L[0].live && !g_L[JD_SHADOW].live && g_L[0].sl > 60)`. During a ground **handover** — the 180-frame window in which the ground's luma changes the most — the shadow slot is live, so `g_gluma` freezes at its last value. And whenever `sl <= 60` it is *slammed* to the constant 60. It is a dim-detector, and it is correct for that job; it is not a contrast signal.
- **Wrong scope.** It samples `g_L[0].buf` — the ground's own buffer, pre-envelope, pre-crossfade. What an accent lands on is `fb`: the ground *lerp*, the boot ramp, the dead-air gain, and every overlay beneath it.
- **Mean only.** Mean luma cannot distinguish the three cases that need different blends:
  - a uniform grey field at luma 120 (flat, mid) — MAX has nowhere to bite, OVER is right;
  - a sparse bright figure on black, also mean 120 (70% near-black) — MAX and SCREEN both work beautifully;
  - a dense woven moire, also mean 120 — everything additive turns to mud.
- **It aliases.** All three existing samplers walk a fixed grid, `buf[q * st]` with `st = npix/512`. Over a periodic pattern — and this library is 40% string-art and radial mandalas — a fixed grid can lock onto the period and report a moire ground as flat. Latent bug in the dead-air guard too.

**Recommendation: leave `g_gluma` alone, it does its job.** Add a purpose-built probe.

### 2.2 What to measure instead

Four numbers, all from one stratified walk over `fb`:

| field | definition | what it answers |
|---|---|---|
| `luma` | mean luma of samples | overall level |
| `hi` | fraction of samples > 200, /255 | **SCREEN headroom** — how much of the frame cannot be lightened at all |
| `lo` | fraction of samples < 32, /255 | **MAX real estate** — how much dark ground an overlay can land in |
| `busy` | mean \|Δluma\| between samples 4 px apart | **high-frequency energy** — mud risk |

`hi` and `lo` are the two that mean luma cannot give you, and they are what separate "sparse bright figure on black" from "grey field". `busy` is the direct measurement of the *clutter* half of the complaint.

```c
typedef struct {
    uint8_t luma;  /* mean luma of the ground under this layer, 0..255       */
    uint8_t hi;    /* frac of samples > 200, /255 — SCREEN headroom          */
    uint8_t lo;    /* frac of samples <  32, /255 — MAX real estate          */
    uint8_t busy;  /* mean |adjacent-pixel luma delta|, clamped 0..255       */
} jd_ground;

/* One stratified pass, 1024 scattered reads, no allocation, no branch in the
 * hot part.  The stride JITTER is not decoration: the existing samplers walk
 * a fixed grid (buf[q*st]), and a fixed grid over a periodic pattern aliases
 * — a moire ground can read as flat when it is the opposite of flat. */
static void ground_probe(const uint32_t *fb, int npix, int frame, jd_ground *G)
{
    int st = npix / 1024; if (st < 8) st = 8;
    uint32_t sum = 0, nhi = 0, nlo = 0, hf = 0;
    for (int q = 0; q < 1024; q++) {
        int i = q * st + (int)(mix32((uint32_t)q ^ (uint32_t)frame) % (uint32_t)st);
        if (i + 4 >= npix) i = npix - 5;
        uint32_t c  = fb[i], c2 = fb[i + 4];      /* 4 px apart == local HF */
        uint32_t l  = (((c  >> 16) & 255) * 77 + ((c  >> 8) & 255) * 151 + (c  & 255) * 28) >> 8;
        uint32_t l2 = (((c2 >> 16) & 255) * 77 + ((c2 >> 8) & 255) * 151 + (c2 & 255) * 28) >> 8;
        sum += l;  nhi += (l > 200);  nlo += (l < 32);
        hf  += (l > l2 ? l - l2 : l2 - l);
    }
    G->luma = (uint8_t)(sum >> 10);
    G->hi   = (uint8_t)((nhi * 255) >> 10);
    G->lo   = (uint8_t)((nlo * 255) >> 10);
    uint32_t b = hf >> 10;  G->busy = (uint8_t)(b > 255 ? 255 : b);
}
```

**Where to sample.** Two calls per frame, in the composite loop at 2604:

1. immediately after the ground is laid down (the `memcpy` / `span_lerp` at 2586) → this is what slot 1 lands on;
2. immediately after slot 1's `blend_span` → this is what slots 2 and 3 land on.

That is one extra sparse pass over the frame versus reusing the dead-air walk. 2,048 scattered reads against a 1.2 M-pixel composite is noise; it should still be timed rather than assumed.

**Cheaper fallback if it does not measure free:** piggyback the histogram and the adjacent-delta accumulation onto the dead-air guard's existing 512-sample walk (line ~2622) and use frame N−1's reading on frame N. Zero extra passes. The one-frame lag is invisible against a signal that is rate-limited to 1/256 of a mode per frame (§4).

---

## 3. The decision rule

### 3.1 First, the physics — what each blend needs from the ground

| mode | direction | needs from ground | fails when |
|---|---|---|---|
| `B_ADD` | adds much light | `lo` high (dark real estate) | ground bright → white mush |
| `B_SCREEN` | adds moderate light | `luma` low, `hi` ≈ 0 | ground mid/bright → **fog** |
| `B_MAX` | adds light only where brighter | `lo` high | ground bright → **no-op** |
| `B_OVER`* | replaces, ground-independent | nothing | overlay's own black would print (solved by luma keying, §3.4) |
| `B_DIFF` | **subtracts** | anything; the only mode that works on a blown-out ground | inverts hue — reads as a palette change, not a blend |

\* `B_OVER` does not exist yet. It is proposed in §3.4 and it is the missing tool.

The ordering above is not arbitrary: **the five modes are monotone in how much light they add to the ground.** That is the axis the whole policy runs on.

### 3.2 Ground classes

| class | test |
|---|---|
| DARK | `luma < 70` |
| MID | `70 ≤ luma ≤ 150` |
| BRIGHT | `luma > 150` or `hi > 90` |
| FLAT | `busy < 6` |
| BUSY | `busy > 18` |

FLAT/BUSY is an orthogonal axis that modulates the brightness axis, not a fifth class.

### 3.3 The rule

Weights are the target share among the admissible set for that cell. **A blank cell is a hard ban, not a low weight.**

| ground | mid (slot 1) | accent (slot 2) | spark (slot 3) | why |
|---|---|---|---|---|
| **DARK flat** | SCREEN 45, MAX 35, ADD 20 | SCREEN 40, ADD 35, MAX 25 | **ADD 55**, SCREEN 30, MAX 15 | maximum dark real estate; light-adding modes are at their best and cannot fog. Sparks get ADD because they are small and short — the one place a hard clamp to white is a highlight, not a blowout. |
| **DARK busy** | MAX 45, SCREEN 35, ADD 20 | ADD 40, MAX 30, SCREEN 30 | ADD 60, MAX 25, SCREEN 15 | still dark, but SCREEN over texture fills the gaps between strokes and turns weave into haze. MAX preserves the ground's own structure bit-exact, which is what keeps the stack readable. |
| **MID flat** | MAX 45, SCREEN 30, OVER 15, DIFF 10 | MAX 40, ADD 25, OVER 20, DIFF 15 | ADD 45, MAX 30, OVER 15, DIFF 10 | the only cell where today's default is genuinely right. SCREEN is already marginal here and gets a reduced peak. |
| **MID busy** | MAX 50, OVER 25, DIFF 15, SCREEN 10 | **OVER 35**, MAX 30, DIFF 20, ADD 15 | ADD 40, OVER 30, MAX 20, DIFF 10 | mud risk dominates. OVER is the only mode that reads as a distinct object rather than as more texture. |
| **BRIGHT flat** | **OVER 40**, DIFF 30, MAX 20, SCREEN 10 | **DIFF 40**, OVER 35, MAX 25 | DIFF 40, OVER 30, ADD 20, MAX 10 | MAX is a no-op and SCREEN is fog. Only replacing or subtracting can be seen. MAX kept at low weight because a *bright-flat* ground with a high `lo` (sparse figure) still gives it somewhere to land. |
| **BRIGHT busy** | OVER 50, DIFF 30, MAX 20 | **OVER 45**, DIFF 40, MAX 15 | OVER 45, DIFF 35, ADD 20 | the hardest cell and the one J is describing. Nothing additive survives here. **ADD/SCREEN banned outright.** |

**Hard bans, stated as rules rather than weights:**

- `SCREEN` is refused when `luma > 150 || hi > 90`. This is the fog rule and it is the single highest-value line in the design.
- `ADD` is refused when `luma > 130 || lo < 40`.
- `MAX` is *permitted but pointless* when `lo < 25 && luma > 170` — treat as refused, it is a wasted layer.
- `DIFF` is capped (§5, risk 1) so it does not become the new MAX.

### 3.4 The missing mode: `B_OVER`, luma-keyed

The table leans on OVER in five of six cells and it does not exist. It is worth adding on its own merits: **it is the only blend whose visibility is independent of the ground.** It cannot fog (adds no light) and it cannot no-op (does not compare against the ground).

`B_MIX` almost is this, and is ground-only for a good reason: `span_lerp` prints the overlay's *black* over the ground — the "you can see the box" artefact that `layer_warp`'s comment describes. Keying the coverage off the source's own luma fixes exactly that.

```c
/* OVER, keyed by the SOURCE's own luma.  The overlay's black is transparent,
 * which is the one thing B_MIX gets wrong and the reason MIX has never been
 * allowed in an overlay slot.  Neither adds light (cannot fog) nor compares
 * against the ground (cannot no-op) — which is why it is the answer on a
 * bright or busy ground.
 *
 * SAFETY: this would replace the ground outright under a routine whose canvas
 * is not black.  Two things currently prevent that and BOTH must stay true:
 *   - admissible() rule 2 keeps asm modes (incl. the WHITE-canvas accumulators
 *     17 and 20) out of every overlay slot;
 *   - admissible() rule 6 requires st->dark >= 70 for an overlay.
 * Do not relax rule 6 and add this mode in the same change. */
static void span_over(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t sr = (s >> 16) & 255, sg = (s >> 8) & 255, sb = s & 255;
        uint32_t a  = (sr * 77 + sg * 151 + sb * 28) >> 8;   /* coverage */
        a = (a * w) >> 8;                                    /* envelope */
        uint32_t ia = 256 - a;
        uint32_t dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
        dst[i] = 0xFF000000u
               | (((dr * ia + sr * a) >> 8) << 16)
               | (((dg * ia + sg * a) >> 8) <<  8)
               |  ((db * ia + sb * a) >> 8);
    }
}
```

Cost: 3 multiply-adds plus the luma dot product per pixel, versus `span_max`'s two masked multiplies. Expect ~1.3–1.6× `span_lerp`, which is already run once per frame for the ground handover. **Measure before shipping**; the `admissible()` blend allowance is 120 Q8 (0.47 ms) per layer and OVER may need that raised.

---

## 4. The timing problem

### 4.1 The scale of it

- Overlay hold is `HOLD_LO..HOLD_HI` scaled by `g_tempo` (0.70–1.33): **500–1,600 frames, 8–27 s.**
- Ground hold is 1,080–1,640 frames plus a 180-frame handover — **a mid layer routinely outlives one entire ground.**
- Independently, `JD_LEG` is 1,024 frames: the palette walk moves the ground's index band through the ramp *within* a tenancy. The DIM guard exists because a ground's own luma can walk to near-black mid-life. **The ground changes brightness class several times per layer lifetime even with no handover at all.**

So a blend chosen at spawn is provably wrong for part of every layer's life. This is not a corner case.

### 4.2 Three options, assessed

**A — Re-evaluate periodically, hard switch.** Cost: nil. **Reject.** Flipping MAX→SCREEN on a layer at `w_now` 180 changes every pixel in one frame. This codebase has already measured that class of event: the `draw.s` palette-walk cut is described as *"a full-frame cut at 100% weight, measured 43.3 / 50.8 / 62.0 / 108.9 on four of the thirty sampled starts"* — and that was considered serious enough to justify the whole `g_force` escape hatch. A blend flip is the same event. It violates the no-strobe law outright.

**B — Cross-fade between two modes.** Composite both modes and lerp the results by a morph parameter. Visually safe; the only correct way to do a discrete switch. Cost: two blend passes plus a lerp instead of one, for the duration of the fade. **Viable, but it needs a bound on how often it happens** or a stack of four morphing overlays triples the composite cost.

The naive shortcut — running mode A at `w*(1−bx)` and mode B at `w*bx` — is **not** the same thing. Blends are non-linear and non-commutative; MAX-then-SCREEN over the same ground is not the average of the two. It degrades tolerably for adjacent modes and badly for distant ones. Do it properly.

**C — Mode as a function evaluated per frame.** Naive form is the worst option: the policy will chatter on a class boundary, and chatter *is* strobe. But there is a version of C that is the right answer, and it is the recommendation.

### 4.3 Recommendation: a continuous axis, not a discrete choice

**Do not switch between modes. Move along an axis whose endpoints are modes.**

Order the five modes by how much light they add. Adjacent pairs then differ by one term, which is what makes interpolating between them visually safe:

```c
/* THE LIGHT AXIS.  Ordered by how much light the mode adds to the ground:
 * ADD (+much) -> SCREEN (+moderate) -> MAX (+where brighter) -> OVER (neutral,
 * replaces) -> DIFF (-, subtracts).  Adjacent entries are visually close, so
 * a lerp between neighbours is a shading, not a switch.  u is Q8 along this
 * axis: 0 = pure ADD, 1024 = pure DIFF. */
enum { U_ADD = 0, U_SCREEN, U_MAX, U_OVER, U_DIFF, U_N };
static const uint8_t U_MODE[U_N] = { B_ADD, B_SCREEN, B_MAX, B_OVER, B_DIFF };
#define U_MAXPOS ((U_N - 1) << 8)          /* 1024 */
```

The §3.3 table then falls out of one monotone function of the ground:

```c
/* Ground -> position on the light axis, Q8.  This function IS the policy;
 * everything after it is smoothing and variety. */
static uint32_t u_target(const jd_ground *G)
{
    /* LEVEL is the primary term, near-linear.  The knee (230) is set so that
     * SCREEN's band (128..383) stops being reachable above luma ~120, which
     * is the fog line measured in 01. */
    int32_t u = ((int32_t)G->luma * 1024) / 230;

    /* HEADROOM.  A ground already 35% near-white cannot be lightened at all,
     * whatever its mean says.  This is the term that catches a dark frame
     * with a blown-out figure in it. */
    u += (int32_t)G->hi;

    /* REAL ESTATE.  A sparse bright figure on black measures luma 120 but is
     * 70% near-black — MAX and SCREEN both work there.  Mean luma alone
     * cannot tell that apart from a grey field; this is why lo is measured. */
    u -= ((int32_t)G->lo * 384) >> 8;

    /* BUSY.  High local contrast is the case where adding light reads as mud
     * regardless of level.  Push toward the modes that REPLACE or SUBTRACT,
     * which are the only ones still distinguishable from the texture.
     * Capped so a busy ground alone cannot saturate the axis into DIFF. */
    if (G->busy > 8) {
        int32_t b = ((int32_t)G->busy - 8) * 20;
        u += b > 256 ? 256 : b;
    }

    /* DIFF RATION.  DIFF inverts hue: at scale it reads as a palette change,
     * not as a blend, and it will fight the mood system.  Reserve it for a
     * ground that is genuinely blown out; everything else tops out at OVER. */
    if (G->hi < 120 && u > 896) u = 896;

    if (u < 0) u = 0;
    if (u > U_MAXPOS) u = U_MAXPOS;
    return (uint32_t)u;
}
```

**Band readout:** 0–127 ADD · 128–383 SCREEN · 384–639 MAX · 640–895 OVER · 896–1024 DIFF.

Worked examples, before the per-slot and per-layer offsets:

| ground | luma | hi | lo | busy | u | lands on |
|---|---|---|---|---|---|---|
| dark flat | 45 | 0 | 180 | 4 | 0 | ADD |
| dark busy | 40 | 0 | 170 | 25 | 263 | SCREEN |
| mid flat | 110 | 10 | 60 | 5 | 400 | MAX |
| mid busy | 115 | 12 | 55 | 24 | 705 | OVER |
| bright flat | 180 | 100 | 20 | 5 | 871 | OVER→DIFF |
| bright busy | 170 | 90 | 30 | 26 | 896 | capped at OVER/DIFF edge |

That reproduces the §3.3 table's intent as a continuum.

**The role term is a fixed offset on the same axis:**

```c
/* PER-SLOT BIAS.  A SPARK is small and short-lived: it is allowed to add
 * light where a MID — which covers the whole frame for twenty seconds — is
 * not.  This is the term that stops the whole stack going milky at once, and
 * it is the reason the axis is per-layer rather than global. */
static const int16_t U_SLOT[JD_NSLOT] = { 0, +288, +96, -160 };
/*                                     gnd   mid  accent spark */
```

### 4.4 Why this cannot pop — as a bound, not as an opinion

```c
/* SMOOTHING IS THE SAFETY ARGUMENT, so it is stated as a bound.
 *
 * u_slow is an EWMA of the target (tau ~1 s) and is then RATE-LIMITED to one
 * Q8 step per frame.  Because the two straddling modes are lerped by frac(u),
 * a one-step move changes any output pixel by at most
 *
 *     |A(p) - B(p)| / 256
 *
 * and for ADJACENT modes on the light axis |A-B| is bounded by 255 per
 * channel, so the worst case is ONE LSB per channel per frame.  A full mode
 * traverse takes 256 frames (4.3 s), which is slower than the fastest
 * envelope the engine already runs.  Nothing here can strobe; it is not a
 * matter of taste.
 *
 * The DEADBAND is the second half of the safety argument: without it, a
 * target sitting exactly on a band edge dithers the EWMA back and forth
 * forever.  See risk 3 — the feedback loop is real. */
static uint32_t g_u_slow = 512;                 /* open at MAX */

static void ground_axis_tick(const jd_ground *G)
{
    int32_t t = (int32_t)u_target(G);
    int32_t s = (int32_t)g_u_slow;
    int32_t d = t - s;
    if (d > -48 && d < 48) return;              /* deadband: ignore small moves */
    s += d / 64;                                /* EWMA, tau ~1 s */
    if (s > (int32_t)g_u_slow + 1) s = (int32_t)g_u_slow + 1;   /* rate limit */
    if (s < (int32_t)g_u_slow - 1) s = (int32_t)g_u_slow - 1;
    g_u_slow = (uint32_t)(s < 0 ? 0 : s > U_MAXPOS ? U_MAXPOS : s);
}
```

**Resolving one layer, per frame, in the weights loop at 2436:**

```c
/* Turn the axis position into (mode, mode2, morph) for this frame.
 * SNAP: within 3% of a pure mode use the single kernel — the morph costs a
 * second blend per pixel and there is nothing to see.  In steady state the
 * axis is parked and every layer is snapped, so the two-pass cost is paid
 * only while the ground is actually changing character. */
static void layer_blend_now(jd_layer *L, int sidx)
{
    int32_t u = (int32_t)g_u_slow + U_SLOT[sidx] + L->u_off;
    if (u < 0) u = 0;
    if (u > U_MAXPOS) u = U_MAXPOS;
    int      i = u >> 8;
    uint32_t f = (uint32_t)(u & 255);
    if (i >= U_N - 1) { i = U_N - 2; f = 256; }
    if      (f <   8) { L->blend = U_MODE[i];     L->blend2 = L->blend; L->bx = 0; }
    else if (f > 248) { L->blend = U_MODE[i + 1]; L->blend2 = L->blend; L->bx = 0; }
    else              { L->blend = U_MODE[i]; L->blend2 = U_MODE[i + 1]; L->bx = (uint16_t)f; }
}
```

New `jd_layer` fields: `uint8_t blend2; uint16_t bx; int16_t u_off;`.

**The morph kernel — one pass, no npix scratch:**

```c
/* Morph between two modes in ONE pass over the data, through a 256-pixel
 * stack tile.  No allocation, and the working set stays in L1 — which matters
 * more than the instruction count at 1280x960, where a full-frame scratch is
 * 4.9 MB and blows L2 twice per layer.
 *
 * Correctness note: this lerps the two RESULTS.  Running mode A at w*(1-bx)
 * and mode B at w*bx would be cheaper and is WRONG — the blends are
 * non-linear and non-commutative, so that is not a crossfade, it is a third
 * blend that nobody designed. */
static void span_morph(uint32_t *dst, const uint32_t *src, int n,
                       uint32_t w, int a_mode, int b_mode, uint32_t bx)
{
    uint32_t ta[256], tb[256];
    for (int off = 0; off < n; off += 256) {
        int m = n - off < 256 ? n - off : 256;
        memcpy(ta, dst + off, (size_t)m * 4);
        memcpy(tb, dst + off, (size_t)m * 4);
        blend_span(ta, src + off, m, w, a_mode);
        blend_span(tb, src + off, m, w, b_mode);
        span_lerp(dst + off, ta, tb, m, bx);
    }
}
```

Call site at 2610 becomes:

```c
if (L->bx) span_morph(fb, srcp, npix, L->w_now, L->blend, L->blend2, L->bx);
else       blend_span(fb, srcp, npix, L->w_now, L->blend);
```

**Cost control.** Budget one morphing overlay at a time with a token taken in the weights loop; a layer that cannot get the token holds its current snapped mode until the token frees. Since `g_u_slow` moves at 1 Q8 step/frame and the per-slot offsets are 448 apart at their widest, simultaneous morphs are rare anyway — but the token makes the worst case bounded rather than merely unlikely.

### 4.5 Where B still applies

The axis handles gradual ground change, which is the common case. Two events are genuinely discontinuous and want option B explicitly:

- **A forced re-pick** (e.g. a future `jd_req_shape`-style live control that re-rolls `u_off`).
- **The `bx` snap thresholds** — crossing 8 or 248 changes which pair is active. This is safe by construction (the pair changes at frac ≈ 0 or 1, where the outgoing mode's contribution is already under 3%), but it is the one place worth a trace line during bring-up.

---

## 5. Guarding against monotony

A purely deterministic policy trades *"always MAX"* for *"always whatever this ground implies"* — the same complaint one level up, and J will find it in a week.

**The variety mechanism is a per-tenancy offset on the axis, not a second lottery over modes.** This is deliberately the same knob as the policy: the layer still *tracks* the ground for its whole life, only its distance along the axis is personal. Two layers on the same ground get different, and both admissible, blends.

```c
/* VARIETY.  Each tenancy gets a fixed offset on the light axis, drawn from a
 * TRIANGULAR distribution (sum of two uniforms) so the tails — the picks most
 * likely to be wrong for the ground — are rare rather than uniform.
 *
 * ANTI-REPEAT is the same shape as the routine bag's cross-launch memory: an
 * offset within 96 Q8 of either of the last two tenancies in this slot is
 * refused.  Bounded retries, and the last attempt is accepted unconditionally
 * so this can never starve a spawn (the same guarantee admissible() rule 0
 * gets from its bag-size test). */
static int16_t g_uhist[JD_NSLOT][2];

static int16_t draw_u_off(int slot, uint32_t r)
{
    int16_t v = 0;
    for (int t = 0; t < 6; t++) {
        uint32_t a = mix32(r + (uint32_t)t * 0x9E37u);
        v = (int16_t)((int32_t)(a % 449u) + (int32_t)((a >> 16) % 449u) - 448);
        int ok = 1;
        for (int i = 0; i < 2; i++) {
            int32_t d = (int32_t)v - g_uhist[slot][i];
            if (d < 96 && d > -96) ok = 0;
        }
        if (ok) break;
    }
    g_uhist[slot][1] = g_uhist[slot][0];
    g_uhist[slot][0] = v;
    return v;
}
```

**Why this beats weighted-random-over-admissible-modes:**

- A discrete weighted draw over `{MAX, OVER, DIFF}` re-introduces the problem the axis solves: the drawn mode is fixed at spawn and goes stale.
- The offset is fixed at spawn but the *mode it produces* still follows the ground. Variety and adaptivity come from the same mechanism instead of fighting each other.
- The ±448 range spans 1.75 modes, so a slot's three consecutive tenancies over the same ground land on genuinely different modes — verifiable directly from the telemetry as a spread statistic on `u`, not just as a mode histogram.

**Two smaller variety terms worth having:**

- **Mood as a small axis bias, not a mode switch.** M_BLAZE currently *is* the SCREEN rule; replace it with `U_MOOD[M_STARK]=+64, [M_RICH]=0, [M_BLAZE]=-96` — blaze leans additive, stark leans toward subtract/replace. Same intent, one twelfth the authority, and the ground still overrules it.
- **Ban DIFF in M_STARK.** Stark is already a narrow palette slice; hue inversion inside a near-monochrome window reads as a bug.

---

## 6. Interaction with `admissible()` and the role machinery

**Keep them separate.** `admissible()` decides *which routine* may take a slot; the blend policy decides *how it composites*. Mixing them would make the admission test depend on a per-frame signal, and admission is already the most load-bearing function in the file.

Three real couplings:

- **Rule 6 (`st->dark < 70` refusal) is what makes `B_OVER` safe.** It guarantees every overlay is ≥27% near-black, so luma-keyed coverage is genuinely sparse. There is a tempting follow-on — *"with OVER available we can relax rule 6 and open up the library"* — and it must not be done in the same change as adding OVER. Relaxing rule 6 is a separate, separately-measured step.
- **Rule 2 (asm modes are grounds only)** is the other guarantee: the `C_CANVAS` accumulators 17 and 20 paint **white** canvases, and a white-canvas routine under OVER has coverage 1.0 everywhere and would replace the ground outright. It cannot reach an overlay slot today. If that ever changes, `span_over` needs a canvas-floor subtraction in its coverage term.
- **`pick_blend()`'s peak logic moves out.** The SCREEN peak cap was never about SCREEN — it was about *a light-adding mode over a ground with no headroom must be quiet*. Express that directly, and split it: `w_peak` stays drawn once at spawn (the envelope machinery depends on it), and the ground correction becomes a per-frame **trim**, alongside the audio surge that already exists at line 2443.

```c
/* Ground trim, applied per frame right next to g_surge — the engine already
 * has a per-frame weight multiplier hook, so the envelope machinery does not
 * change at all.
 *
 * It only ever REDUCES.  Raising a layer because the ground went dark is how
 * you build a pump, and a pump is the slow cousin of a strobe. */
static uint32_t w_trim_q8(uint32_t u, const jd_ground *G)
{
    if (u >= (U_MAX << 8)) return 256;          /* MAX/OVER/DIFF: nothing to trim */
    uint32_t k = 150u + ((uint32_t)G->lo * 106u) / 255u;   /* 0.59 .. 1.00 */
    return k;
}
/* ...and EWMA'd per layer at 1/32 before use, or it is a per-frame gain wobble. */
```

`pick_blend()` itself shrinks to: draw `w_peak` from `PEAK_LO/HI` as today, call `draw_u_off()`, store `L->u_off`. The mode is no longer chosen at spawn at all — which is the point.

`SLOT_ROLE`, `pick_role`, the bags, the shape-family separation and the cross-launch ring are all untouched.

---

## 7. Risks, honestly

1. **DIFF becomes the new MAX.** Today DIFF is 3.6% of spawns at peak ≤90. This policy makes it the answer on bright grounds, potentially 20–30% of the time. DIFF inverts hue, so a lot of it will read as *a different palette*, not a different blend, and will fight the mood system that a fresh ground just set. Mitigations already in the code above: the `hi < 120` ration in `u_target`, the DIFF peak cap, the M_STARK ban. **This still needs an A/B before ship.** It is the most likely thing to be wrong.

2. **`B_OVER` changes the engine's look more than the policy does.** It is a new kernel doing a thing the compositor has never done in an overlay slot. Its cost is unmeasured and may exceed the 120 Q8 blend allowance `admissible()` budgets. Land it behind an env knob (`JD_OVER=0`) and measure before it becomes load-bearing in five of six table cells.

3. **The feedback loop is real, and it is the largest risk here.** The overlays brighten the composite → `G.luma` rises → the axis moves toward DIFF → the overlays stop adding light → `G.luma` falls → the axis moves back. With tau ≈ 1 s and a 1-step rate limit this would be a slow breathing over ~4–8 s, not a strobe — but slow breathing across the whole frame is exactly the kind of thing J notices and cannot name. Mitigations, in order of preference:
   - take the **primary** reading before the overlay loop (ground only) and weight the after-slot-1 reading at 50%;
   - the ±48 deadband in `ground_axis_tick`, which is there for this and not for chatter alone;
   - if it still oscillates, drive `u_target` from the ground buffers only and use the composite reading purely for `busy`.
   **This must be instrumented and watched, not assumed away.**

4. **Monotony, inverted.** If the ground spends most of its time at mid luma, `u` parks near MAX and the whole change is theatre. This is checkable *before* implementing: log ground state per frame and see how the existing library's grounds actually distribute across the classes. If they cluster in MID, the class thresholds need retuning against the real distribution rather than against my round numbers.

5. **Ground handover.** During the 180-frame ground crossfade the composite can change character faster than the rate limit tracks, so layers spend ~2–3 s composited for the outgoing ground. Accept it: wrong for two seconds beats popping. Do **not** be tempted to unclamp the rate limit during a handover — that is precisely the frame where a pop would be most visible.

6. **All constants in §4.3 are reasoned, not measured.** The knee at 230, the `lo` weight of 384, the busy scale of 20, `U_SLOT`, the ±448 offset range. Every one of them is a guess that happens to reproduce the §3.3 table on the six worked examples. They are the first thing to fit against real telemetry.

---

## 8. Verification plan

- **Extend the telemetry row** with `gluma,ghi,glo,gbusy,u,uoff` and add a periodic `ground` event every 60 frames, so the ground-class distribution can be measured before any of this is committed. That measurement alone settles risk 4 and calibrates every constant in §4.3.
- **Ship the primary success metric.** *Accent visibility* = mean absolute per-pixel difference between the composite with the layer and without it, over the 1024 probe points, normalised by `w_now`. This is directly J's complaint expressed as a number: today, MAX over a bright ground scores near zero, and that is the bug. Compute it in the composite loop by sampling `fb` at the same indices before and after each `blend_span`. Cost: one extra 1024-sample read per overlay, and it can be compiled out.
- **Acceptance targets:** no blend mode above 45% share in any slot; accent-visibility median up materially against the 3.0.4 baseline; zero frames with a composite delta above the JUMP threshold (4096 Q8) attributable to a blend change; the `u` trace shows no oscillation with period under 30 s.
- **Staging.** (1) probe + telemetry only, no behaviour change, measure. (2) axis driving the existing four modes with `B_ADD` re-enabled, no OVER. (3) `B_OVER`. (4) revisit `admissible()` rule 6.
