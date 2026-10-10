# 08 — Extended-range compositing and tonemapping

**Verdict up front: do not do it.** JellyDazzle should not composite in extended
range and tonemap. The engine is not clipping. Measured on the real blend
population, **0.066% of composited pixels reach 255 on any channel**, and the
mean composite sits at **luma 87/255** — roughly 2.9x of headroom already unused.
Adding headroom to a pipeline that is using a third of the range it has cannot
make an accent more visible.

The foggy-clutter complaint is real, but its mechanism is **ordering and weight**,
not range. The fix is about twelve lines in `span_max` plus one ground multiply,
and it measures 1.8x better accent separation at the same frame brightness.

Everything below is measured, not asserted. Sources: `src/engine/compositor.c`,
`src/app/main.c`, `docs/contrast/spawn_telemetry.csv` (1,962 real spawns), and
`catalog/img/*.png` (634 rendered pattern stills, sampled against the telemetry's
actual slot/blend/peak assignments).

---

## 1. What clipping is actually costing today

### 1.1 The clipping blend is dead code

`pick_blend` (compositor.c:1618) can return exactly three values:

```
mode = B_MAX;                                        /* default            */
if (st->dark >= 215 && st->sat >= 25) mode = B_SCREEN;
if (g_mood == M_BLAZE && st->dark >= 180) mode = B_SCREEN;
if (st->delta_q8 < 512 && (r & 15) == 3) mode = B_DIFF;
```

`B_ADD` — the one blend whose own comment says *"where two lit layers overlap it
saturates to white rather than to a colour"* — is **never selected**. Confirmed
against telemetry:

| Blend | Spawns | Share | Can it clip? |
|---|---:|---:|---|
| `B_MIX` (`span_lerp`) | 888 | 45.3% | **No** — convex combination, `wa+wb = 256` exactly |
| `B_MAX` (`span_max`) | 742 | 37.8% | **No** — `max(dst, src*w>>8)`, both operands ≤ 255 |
| `B_SCREEN` (`span_screen`) | 261 | 13.3% | **No** — asymptotic; the clamp catches `>>8`-vs-`/255` rounding only |
| `B_DIFF` (`span_diff`) | 71 | 3.6% | **No** — convex blend of `d` and `\|d−s\|` |
| `B_ADD` (`span_add`) | **0** | **0.0%** | Yes — but it is never reached |

All 888 `B_MIX` spawns are grounds (slot 0 and slot 4 / `JD_SHADOW`); overlays
never get it. So **100% of the composite path in production is range-safe by
construction.** `span_add` is the only saturating kernel in the file and it is
unreachable through `pick_blend`.

### 1.2 The composite does not saturate

Simulated the real stack — a telemetry-drawn ground plus three telemetry-drawn
overlays at their recorded blends and peak weights, 300 stacks x 60,000 pixels,
against actual rendered pattern frames:

| Measurement | Value |
|---|---:|
| Mean composite luma | **87.3 / 255** |
| Pixels with any channel at 255, pre-gain | **0.066%** |
| Mean ground channel value (spawn-weighted, 857 ground spawns) | **78.4 / 255** |
| Spawn-weighted mean ground luma | **75.7 / 255** (median 68) |

Six pixels in ten thousand touch the ceiling. There is nothing there to recover.

Worth stating plainly because it contradicts the premise of the proposal: the
grounds are not bright. They average 30% of range. J's "bright ground" is bright
*relative to the accent sitting on it*, not bright in absolute terms — and that
is a ratio problem, which extra headroom does not touch.

### 1.3 Where the pipeline actually clips: `span_gain`

The only genuine hard clamp that fires is `span_gain` (compositor.c:1261), which
is a **post-process on the finished frame**, not a compositing operation. It is
called from three places, all in `jd_frame`:

- the dead-air guard, `g_gain` up to `JD_LUMA_MAXGAIN` = 384 (1.50x)
- the audio bloom, `bump` capped at 272 (1.06x) — fires on essentially every
  frame while music is playing
- (`span_scale` for the boot ramp is `w < 256`, so it cannot clip)

Measured clip incidence on the simulated composite:

| Gain | Pixels clipped | Chroma-vector distortion on clipped px |
|---:|---:|---:|
| 272 (1.06x, audio bloom, routine) | **0.60%** | 0.020 — negligible |
| 384 (1.50x, dead-air floor) | **13.52%** | substantial |

The 13.5% figure is J's own observation, already in the source:

```c
#define JD_LUMA_MAXGAIN 384   /* <= 1.5x: 2.7x clamped the
                               * highlights and washed the
                               * colour out (J caught it)   */
```

`span_gain` clamps **per channel**, so the wash is a hue shift, not just a
brightness loss. A pixel `(200,150,90)` at 1.5x becomes `(255,225,135)`: the
channel ratio goes from 2.22:1.67:1 to 1.89:1.67:1 — desaturated toward yellow.
The current mitigation is to cap the gain at 1.5x, which costs dead-air recovery
to protect the highlights. **This is the one place a tonemap curve earns its
keep, and it is a twenty-line change with no architectural consequence.**
See §4.

### 1.4 What is actually costing the accent

Two mechanisms, both measured, neither of them range.

**(a) `span_max` weights the source instead of the blend. This is a bug.**

```c
/* compositor.c:1288 — span_max */
uint32_t srb = (((s & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;
...
dst[i] = ... (dr > sr ? dr : sr) ...
```

The weight is applied to the *source amplitude* before the max. So an overlay at
peak weight `w` can only ever paint where the ground channel is below `w`,
regardless of how bright the overlay itself is. Telemetry average peaks:

| Slot | Blend | Spawns | Mean peak (Q8) | Ceiling as fraction of white |
|---|---|---:|---:|---:|
| 1 (mid) | MAX | 297 | 153.9 | 0.60 |
| 2 (accent) | MAX | 220 | 119.4 | **0.47** |
| 3 (spark) | MAX | 225 | 92.4 | **0.36** |

A spark layer at weight 92 is *incapable of exceeding 36% white*, even where it
draws pure white. On the measured ground population, `P(ground channel < 92) =
61.9%` — and that is only the ceiling; the spark's own value must also clear the
ground. Simulated on real ground/overlay pairings:

- **MAX overlays change only 10.2% of pixels.** Roughly nine tenths of every
  accent layer is discarded by the ordering.
- Mean lift where it does land: 17.4 codes.

**(b) `span_screen`'s `(1−d)` factor.** Screen's marginal response to the source
is `∂/∂s [d + s − ds] = 1 − d`. Measured over the ground population, `mean(1 − d)
= 0.693` — but that average hides the failure: on the bright regions J is
complaining about (`d = 0.86`), an accent delivers 14% of its amplitude. An
accent at `w = 102` (the measured slot-2 SCREEN mean) over `d = 220` yields
`220 + 101 − 87 = 234`, a **14-code** lift. Broad, weak, grey. That is precisely
"foggy clutter."

Simulated: SCREEN overlays touch 29.8% of pixels but lift only 14.3 codes where
they land.

### 1.5 Why tonemapping cannot fix either one

**MAX commutes with any monotone tonemap.** For any monotone increasing `T`:

```
T(max(a, b)) = max(T(a), T(b))
```

Compositing 69% of overlays in extended range and tonemapping at the end produces
**bit-identical output** (modulo rounding) to compositing them in 8-bit. Not
"a small improvement" — literally none. This is the decisive algebraic fact and
it applies to the largest blend population in the engine.

**For SCREEN, the tonemap makes it worse, and the algebra says so.** Work the
case J is complaining about — ground at `d = 0.86`, accent amplitude `a = 0.47`:

| Path | Ground out | Ground+accent out | **Accent delta** |
|---|---:|---:|---:|
| Today: 8-bit screen | 220 | 237 | **17 codes** |
| HDR add + Reinhard `x/(1+x)` | 118 | 146 | **28 codes** |
| Ground x 0.54 in 8-bit, then existing screen | 118 | 183 | **65 codes** |

Reinhard buys 28 codes of accent by darkening the ground 220 → 118. Darkening the
ground by the *same amount* with one multiply and keeping the existing screen
blend buys **65 codes** — 2.3x better than the tonemap, for one instruction.

The reason is structural, and it is the whole argument in one sentence: **a
tonemap's job is to compress highlights, and the accent is a highlight.** Its
derivative at the accent's operating point is `1/(1+x)² ≈ 0.29–0.42` — it
attenuates by 60–70% exactly the signal J wants amplified. Every benefit
attributed to the HDR path in the proposal comes from the ground getting darker,
and the curve then takes most of it back. You can darken the ground directly.
Tonemapping is the wrong-signed tool for "make the accent pop."

---

## 2. The options

Numbers are for 3456x2160 = **7,464,960 px**, the resolution `main.c` documents
as native (`MAX_PIX 8300000`, "103 fps at 3456x2160"). Per-buffer footprint:
4 B/px = **29.86 MB**; 8 B/px = **59.72 MB**.

### Current allocation

| Allocation | Count | Each | Total |
|---|---:|---:|---:|
| `g_buf[JD_NBUF]` layer buffers | 5 | 29.86 MB | 149.30 MB |
| `framebuffer` (main.c) | 1 | 29.86 MB | 29.86 MB |
| `warp` scratch (static in `jd_frame`) | 1 | 29.86 MB | 29.86 MB |
| `g_pal[JD_NBUF]` (`PAL_N*2*4`) | 5 | 0.26 MB | 1.31 MB |
| `g_blend`, `g_pal_from` | 2 | 0.13 MB | 0.26 MB |
| **App-side total** | | | **210.6 MB** |
| SDL streaming texture (driver) | 1 | 29.86 MB | ~240 MB with driver |

### Current composite bandwidth

Per frame, 3 overlays, two-ground handover, audio live:

| Pass | Traffic |
|---|---:|
| Ground `span_lerp` (2 reads + 1 write) | 89.58 MB |
| 3 x `blend_span` (src read + dst read-modify-write) | 268.74 MB |
| `span_gain` (read + write) | 59.72 MB |
| `SDL_UpdateTexture` (read + write) | 59.72 MB |
| **Total** | **477.8 MB/frame** |

= **28.7 GB/s at 60 Hz**, **49.2 GB/s at the measured 103 fps**. Every buffer is
29.86 MB, far outside any cache level on the part, so this is all DRAM traffic.
Against ~100 GB/s of realistically sustainable LPDDR5X bandwidth, **the
compositor is already roughly half bandwidth-bound.** That matters for every
option below.

### The options table

| Option | Δ memory | Δ composite bandwidth | Precision | Fixes J's complaint? | Verdict |
|---|---:|---:|---|---|---|
| **A. 16-bit everywhere** (all `g_buf` + fb + warp at 8 B/px) | **+209 MB** (210.6 → 419.6, +99%) | +100% | 16 b/ch | No | **Reject.** Layer buffers are written by asm/pattern routines that emit ARGB8888 — every source is LDR by construction. Widening them stores zeros in the high bits at a cost of 149 MB. |
| **B. Wide accumulator only** (one 8 B/px composite target, layers stay 8-bit) | **+59.7 MB** (+28%) | **+50%** (477.8 → 716.6 MB/f, 43.0 GB/s @60) | 16 b/ch | No — see §1.5 | **Reject.** The only sane 16-bit variant, and it still buys nothing: the composite uses 34% of the range it already has. It also eats ~1.7 ms/frame of a 9.7 ms budget, and `BUDGET_Q8` (10.5 ms) will respond by *dropping overlay slots* — which removes accent layers. Self-defeating. |
| **C. Q8.8 / packed integer extended range** (10:10:10:2 in the existing `uint32`, or two `uint32` per px) | 10:10:10 = **+0 MB**; two-word Q8.8 = +59.7 MB | 10:10:10 ≈ +0% | 10:10:10 loses **2 bits/ch** | No | **Reject as proposed.** 10:10:10 is free on memory but breaks the SWAR blend structure the whole file is built on: the `0xFF00FF`/`0x00FF00` two-field trick needs 8-bit lanes with 8 bits of multiply headroom. At 10-bit fields, `field x w` is 10+9 = 19 bits and overflows the neighbour in a 32-bit word. You would need 64-bit lanes, which is a full rewrite of five kernels to solve a non-problem. |
| **D. Tiled Q8.8 in registers** (composite a tile of all layers in NEON registers, tonemap, store 8-bit) | **+0 MB** (tile lives in L1) | **−50%** (477.8 → 238.9 MB/f, 14.3 GB/s @60) | Q8.8 intermediate, 8-bit stored | Partly | **The only architecturally interesting option**, and the only one that is *cheaper* than today. Also the largest rewrite. See §5. |
| **E. 8-bit with reserved headroom** (composite at 0.62x scale, expand at the end) | **+0 MB** | +0% (one extra `span_scale`, already written) | −0.7 bits; **needs dither** | **Yes — measured** | **Recommended, as the ground-duck half of the staged fix.** |
| **F. Cheap local-contrast operator** (unsharp on the composite luma) | +29.9 MB scratch | +180 MB/f for a separable blur | 8 bit | Sideways | **Reject.** A local operator raises accent-vs-neighbourhood contrast, but the engine's whole aesthetic is soft ramps and the halo artefact around every sprite is exactly the "clutter" being complained about. It also costs more bandwidth than option B and is far harder to keep stable under the strobe guards, which measure frame-to-frame delta (`g_lsig`, `JUMP` at `dq8 > 4096`) — a contrast operator changes that delta nonlinearly and would trip `L->frozen` unpredictably. Do not put a nonlinearity the strobe guard cannot model in front of the strobe guard. |

---

## 3. Recommendation

**Against extended-range compositing.** Against A, B, C, F. Do E now, keep D on
the shelf.

The measured comparison, same simulation as §1.2 — 300 stacks of a real ground
plus three real overlays at their telemetry blends and peaks. "px lit" is the
fraction of the frame the overlay stack visibly changes; "lift" is the mean
per-pixel change where it lands:

| Configuration | px lit | lift (codes) | frame luma | luma sd |
|---|---:|---:|---:|---:|
| **A. Today** | 50.8% | 24.0 | 87.3 | 25.5 |
| **B. Weighted-lerp MAX only** | 60.0% | 28.7 | 91.1 | 27.2 |
| **C. Ground duck 0.62x only** | 58.1% | 29.1 | 60.6 | 19.5 |
| **D. Duck 0.62x + lerp MAX** | 67.0% | 33.3 | 65.0 | 22.4 |
| **E. Duck + lerp MAX + 1.30x expand w/ soft knee** | **67.0%** | **43.3** | **84.2** | **29.1** |

Row E: **the same frame brightness as today (84 vs 87), 1.32x the frame area
touched by the overlay stack, and 1.8x the accent separation**, with global
contrast up (sd 25.5 → 29.1). Total cost: one changed kernel, one multiply, and
a 1 KB lookup table. No new buffers, no new passes over memory beyond one that
already exists (`span_gain`/`span_scale` are already called most frames — fold
the expand into the gain the frame already pays for).

The HDR path, at its theoretical best, delivers less than this (§1.5 table:
28 codes vs 65 on the worked case) for +60 MB, +50% bandwidth, and a rewrite of
five blend kernels and the `layer_warp` path.

---

## 4. The curve, since you asked for it

Even though the recommendation is against HDR compositing, **a soft knee still
belongs in `span_gain`** — that is the one place §1.3 measured real clipping
(13.5% of pixels at gain 384) and where J already saw the colour wash.

### 4.1 The algebra

Rational soft knee: identity below the knee `K`, C¹ at `K`, asymptote at the
white point `T`. Let `T = 255`:

```
y(x) = x                              for x <= K
y(x) = T - (T-K)^2 / (x - 2K + T)     for x >  K
```

Checks:
- **Continuity:** `y(K) = T − (T−K)²/(K − 2K + T) = T − (T−K)²/(T−K) = T − (T−K) = K`. ✓
- **C¹:** `y'(x) = (T−K)²/(x − 2K + T)²`; at `x = K` that is `(T−K)²/(T−K)² = 1`,
  matching the identity segment's slope exactly. No visible break at the knee. ✓
- **Monotone:** `y'(x) > 0` everywhere above `K`. ✓ (Monotonicity matters: the
  strobe guard at compositor.c:1851 measures `|Δ|` per channel and a
  non-monotone curve would manufacture jumps that trip `L->frozen`.)
- **Bounded:** `y → T` as `x → ∞`. Never clips, for any gain. ✓

There is no free lunch and it should be stated: any C¹ compressive curve with
slope 1 at `K` necessarily maps `255 → below 255`. Pure white gets slightly grey.
That is the price of the roll-off, and it is what a filmic curve does on purpose.
Pick `K` by how much white you are willing to trade:

| Input (Q8, 256 = 1.0) | `K = 192` | `K = 224` | `K = 240` |
|---:|---:|---:|---:|
| 192 | 192 | 192 | 192 |
| 224 | 213 | 224 | 224 |
| **256 (pure white)** | **224** | **240** | **248** |
| 288 (1.13x) | 230 | 245 | 250 |
| 320 (1.25x) | 234 | 247 | 252 |
| 384 (1.50x — `JD_LUMA_MAXGAIN`) | 240 | 250 | 253 |
| 512 (2.0x) | 245 | 252 | 254 |
| 1023 (4.0x) | 251 | 254 | 255 |

**Recommend `K = 224`** — identity for 88% of the range, rolls only the top 31
codes, costs pure white 15 codes (6%). `K = 240` if even that reads as dull.

### 4.2 Hue preservation

Per-channel compression *shifts hue* — that is the existing bug in `span_gain`,
just with a curve instead of a clamp. Compress the **max channel** and scale all
three by the same ratio. Chroma direction is then exactly preserved and only
saturation-vs-luminance trades off, which is what the eye forgives.

### 4.3 The code

House style: integer, table-driven, no divides in the loop, no floats.

```c
/* ---- soft-knee gain table -------------------------------------------
 * span_gain's per-channel clamp is the ONE place this engine actually
 * saturates: measured 13.5% of pixels at g_gain 384, and because the
 * clamp is per channel it is a HUE shift, not a brightness loss —
 * (200,150,90) at 1.5x becomes (255,225,135), ratio 2.22:1.67:1 pulled
 * to 1.89:1.67:1, desaturated toward yellow.  That is the wash J caught,
 * and it is why JD_LUMA_MAXGAIN is capped at 1.5x today: the cap is
 * protecting the highlights at the cost of dead-air recovery.
 *
 * Replace the clamp with a soft knee on the MAX CHANNEL, then scale all
 * three by the same ratio, so the chroma direction survives exactly and
 * only saturation-vs-luminance trades off.
 *
 *   y(x) = x                            x <= K
 *   y(x) = T - (T-K)^2 / (x - 2K + T)   x >  K
 *
 * C1 at K (both segments have slope 1 there), monotone everywhere, and
 * asymptotic to T so no gain can ever clip.  Monotone matters: the JUMP
 * strobe guard measures per-channel |delta| and a non-monotone curve
 * would manufacture jumps that freeze layers for no reason.
 *
 * Stored as a SCALE, not an output: g_knee[m] = (y(m) << 8) / m, so the
 * inner loop is three multiplies and a shift with no divide.  1024 entries
 * covers 4x gain; anything above that is asymptote anyway.  2 KB, L1
 * resident, built once. */
#define JD_KNEE_K   224                 /* identity below this (Q8, 256=1.0) */
#define JD_KNEE_T   255                 /* asymptote / white point           */
#define JD_KNEE_N   1024                /* covers up to 4x                   */

static uint16_t g_knee[JD_KNEE_N];      /* Q8 scale factor, indexed by max ch */

static void knee_build(void)
{
    const int K = JD_KNEE_K, T = JD_KNEE_T;
    const int A = (T - K) * (T - K);     /* (T-K)^2, = 961 at K=224          */
    g_knee[0] = 256;
    for (int x = 1; x < JD_KNEE_N; x++) {
        int y;
        if (x <= K) y = x;
        else {
            int den = x - 2 * K + T;     /* > 0 for all x > K since T > K    */
            y = T - (A + (den >> 1)) / den;          /* rounded             */
        }
        if (y > T) y = T;
        /* scale that takes x to y, Q8, rounded */
        g_knee[x] = (uint16_t)(((y << 8) + (x >> 1)) / x);
    }
}

/* Drop-in replacement for span_gain.  Same signature, same call sites.
 * w <= 256 still delegates to span_scale (attenuation cannot clip). */
static void span_gain_knee(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    if (w <= 256) { span_scale(dst, src, n, w); return; }
    for (int i = 0; i < n; i++) {
        uint32_t c = src[i];
        uint32_t r = (((c >> 16) & 255) * w) >> 8;
        uint32_t g = (((c >>  8) & 255) * w) >> 8;
        uint32_t b = ( (c        & 255) * w) >> 8;
        uint32_t m = r > g ? (r > b ? r : b) : (g > b ? g : b);
        if (m > JD_KNEE_K) {                        /* the minority path    */
            uint32_t s = g_knee[m < JD_KNEE_N ? m : JD_KNEE_N - 1];
            r = (r * s) >> 8; g = (g * s) >> 8; b = (b * s) >> 8;
            /* s is built so that (m*s)>>8 <= T; the others are <= m, so the
             * clamps below are belt-and-braces against rounding only. */
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
        }
        dst[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}
```

Call `knee_build()` from `engine_init`. Once this is in, `JD_LUMA_MAXGAIN` can go
back up — the 1.5x cap exists only because of the clamp it replaces. 512 (2.0x)
is safe with `K = 224`; the knee holds the top at 252.

---

## 5. The staged path

Ordered by payoff per line changed. Stop when it looks right — the measurements
say stage 2 is very likely enough.

### Stage 0 — fix the `span_max` weight semantics *(≈12 lines; the single biggest win)*

Weight should control **how far toward the max you go**, not **how bright the
source is**. Measured: pixels lit 10.2% → 15.9%, mean lift 3.0 → 4.7 codes.

```c
/* MAX, weighted correctly.
 *
 * The old form was max(dst, (src*w)>>8) — the weight scaled the SOURCE
 * AMPLITUDE, which means an overlay could only ever paint where the ground
 * was darker than its own peak weight.  Telemetry: the accent slot averages
 * peak 119 and the spark slot 92, so a spark drawing pure white was
 * incapable of exceeding 36% of white ANYWHERE, and measured on real
 * ground/overlay pairings it touched 10.2% of the frame — nine tenths of
 * every accent layer thrown away by the ordering, not by clipping.  That is
 * the "barely visible accent".
 *
 * A weighted max is a LERP TOWARD THE MAX: the blend fraction is the weight,
 * the operand is the unweighted max.  On a dark ground it behaves as before
 * (max(30,255) scaled by 119/256 = 135 vs the old 118); on a bright ground
 * it now delivers something instead of nothing (max(200,255) lerped 119/256
 * = 226, where the old form gave max(200,119) = 200 — zero).
 *
 * Same SWAR structure, one extra multiply-add per lane. */
static void span_max(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    uint32_t iw = 256 - w;
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        /* per-channel unweighted max, still branch-light */
        uint32_t dr = d & 0x00FF0000u, dg = d & 0x0000FF00u, db = d & 0xFFu;
        uint32_t sr = s & 0x00FF0000u, sg = s & 0x0000FF00u, sb = s & 0xFFu;
        uint32_t mr = (dr > sr ? dr : sr) >> 16;
        uint32_t mg = (dg > sg ? dg : sg) >>  8;
        uint32_t mb =  db > sb ? db : sb;
        /* lerp dst -> max by w.  Both operands are <= 255 and iw+w == 256
         * exactly, so the result is <= 255 by construction: no clamp needed,
         * and this stays a range-safe blend. */
        uint32_t r = (((dr >> 16) * iw) + (mr * w)) >> 8;
        uint32_t g = (((dg >>  8) * iw) + (mg * w)) >> 8;
        uint32_t b = ((  db       * iw) + (mb * w)) >> 8;
        dst[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}
```

Note this **preserves the property the original comment cared about** —
"wherever the overlay is darker than the ground, the ground survives bit-exact":
if `s <= d` per channel then `m == d` and the lerp returns `d` exactly. The hard
edges are still there; they are just now reachable on bright ground.

### Stage 1 — soft-knee `span_gain` *(§4.3, ≈30 lines incl. the table)*

Removes the only real clipping in the pipeline. Unlocks raising
`JD_LUMA_MAXGAIN` past 384, which recovers dead-air response that was traded away
to protect highlights.

### Stage 2 — ground duck + expand *(≈8 lines; this is option E)*

Attenuate the ground by the overlay stack's aggregate weight, then expand the
whole composite back through the Stage-1 knee. Measured: **1.8x accent
separation at unchanged frame brightness**.

```c
/* ---- GROUND DUCK ------------------------------------------------------
 * The accent is not clipping — the composite averages luma 87/255 with
 * 2.9x of range unused.  It is losing to the ground on RATIO: screen's
 * marginal response to the source is (1-d), so on a ground at d=0.86 an
 * accent delivers 14% of its amplitude, a 14-code lift.  Make room by
 * lowering d, then put the brightness back globally at the end where the
 * knee can hold it.  This is exactly what an HDR pipeline's tonemap does
 * to the ground, minus the 60 MB accumulator and minus the curve taking
 * 60-70% of the accent back on the way out.
 *
 * Duck depth follows the stack: no overlays, no duck.  Capped so a busy
 * frame never drops the ground below 0.55, which is where the palette's
 * darker legs start to band. */
{
    uint32_t wsum = 0;
    for (int k = 0; k < no; k++) wsum += g_L[ov[k]].w_now;   /* 0..~500 */
    uint32_t duck = 256 - (wsum >> 2);                       /* 1.0 .. 0.55 */
    if (duck < 141) duck = 141;
    if (duck < 256) span_scale(fb, fb, npix, duck);
    g_expand = (256u * 256u) / duck;      /* Q8 inverse, folded into g_gain */
}
```

Then fold `g_expand` into the existing `g_gain` term rather than adding a pass —
`jd_frame` already calls `span_gain` on most frames for the audio bloom, so the
expand is free bandwidth. With `duck = 159` the expand is 412, which the `K=224`
knee handles without clipping (measured: 0.00% clipped, vs 0.07% with a hard
clamp at the same expand).

**Dither, if banding appears.** Compositing at 0.62x costs 0.68 bits — 159 levels
instead of 256 — and this engine paints large smooth ramps, which is the worst
case for banding. Insurance is a 4x4 ordered dither of ±0.5 LSB applied at the
expand, ~4 instructions per pixel:

```c
/* Bayer 4x4, scaled to +/- half an output LSB.  Applied at expand time, so
 * the quantisation the duck introduced is spread as noise below the
 * threshold of vision instead of contouring across a gradient. */
static const uint8_t JD_BAYER[16] = {
     0,  8,  2, 10,  12,  4, 14,  6,
     3, 11,  1,  9,  15,  7, 13,  5
};
/* in the expand loop, for pixel (x,y):
 *   int t = (int)JD_BAYER[((y & 3) << 2) | (x & 3)] - 8;   // -8..+7
 *   v = (v * g_expand + (t << 4)) >> 8;                     // ~+/- 0.5 LSB
 */
```

Add it only if a build with the duck shows contouring on a slow ground. It may
well not — the palette walk is already moving the ramp every frame, which
self-dithers temporally.

### Stage 3 — tiled Q8.8 composite *(only if 0–2 are not enough)*

If, after Stage 2, J still wants more separation, **this** is the extended-range
answer for this codebase — not a wide framebuffer.

- Composite a tile (say 128x64 px = 32 KB in Q8.8, L1-resident) through the whole
  layer stack in NEON registers: `vmovl_u8` up to `uint16x8_t`, blend in Q8.8,
  knee, `vqmovn` back to 8 bit, store once.
- **Memory cost: zero.** The extended range lives only in registers.
- **Bandwidth: 477.8 → 238.9 MB/frame, 28.7 → 14.3 GB/s at 60 Hz.** Each layer is
  read once instead of a read-modify-write pass per layer. Given the composite is
  already ~half bandwidth-bound at the native resolution, this is likely a real
  frame-rate *win*, not a cost.
- Fits the project — it is an ARM64 assembly engine, and this is the one place
  NEON would pay for itself in the C compositor.

**What it costs, honestly:**
- `layer_warp` must be re-seeded per tile. Tractable — the transform is already
  closed-form 16.16 stepping with an analytic origin (`sx0`, `sy0`, `dxx`…), so a
  tile origin is `sx0 + dxx*x0 + dyx*y0`. But it is the fiddliest part.
- The strobe guards, dark-ground guard, dead-air guard, `AUDITION` and `DIM` all
  sparse-sample `fb` and layer buffers **after** the composite. Those samplers
  must move or be recomputed from tile statistics accumulated during the pass.
  This is where the risk is — those guards are load-bearing safety, they encode
  hard-won tuning ("that single line accounted for most of the dark frames in the
  battery"), and reorganising the frame around them is how you break them
  quietly.
- Five blend kernels rewritten and re-verified against the current output.

**Do not start Stage 3 without a frame-level regression harness** — capture N
frames from a fixed seed before and after and diff, because none of these guards
will announce that they have stopped working; they will just produce a slightly
worse show that takes a week to notice.

---

## 6. Summary

- **Do not composite in extended range.** The engine clips 0.066% of pixels and
  runs at 34% of the range it has. The premise — "every layer immediately clips
  at 255" — is not true of this codebase: `pick_blend` never returns the one
  saturating blend, and the three it does return are all range-safe by
  construction.
- **`max()` commutes with any monotone tonemap.** For 69% of overlays an HDR
  pipeline would produce identical output. That is not a small win; it is zero.
- **A tonemap compresses highlights, and the accent is a highlight.** Its
  derivative at the accent's operating point is ~0.3–0.4. It attenuates exactly
  the signal J wants amplified. Every gain the HDR path shows comes from the
  ground getting darker, and you can do that with one multiply — and get 2.3x
  more accent separation than the curve leaves you.
- **The real bug is `span_max` scaling the source instead of the blend fraction.**
  A spark at peak weight 92 cannot exceed 36% white anywhere on the screen. Nine
  tenths of every MAX accent layer is discarded by ordering.
- **The real clipping is in `span_gain`**, a post-process, at 13.5% of pixels at
  gain 384, per channel, hue-shifting. That is worth a soft knee — and that is
  the entire legitimate tonemapping story here, twenty lines, no architecture.
- **Stages 0–2 measure 1.8x accent separation at unchanged frame brightness** for
  ~50 lines, zero new buffers, and zero added passes over memory. Do that first.
  If it is not enough, Stage 3 (tiled Q8.8 in NEON registers) is the version of
  "extended range" that is memory-free and bandwidth-*negative* — but it is a
  real rewrite that puts the strobe and dead-air guards at risk, and it should
  not be attempted without a frame-diff harness.

The exciting answer here is the wrong one. A well-chosen weight fix and one
ground multiply beat it, measurably, for two orders of magnitude less work.
