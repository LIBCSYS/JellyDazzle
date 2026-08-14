# JellyDazzle v2.1 — TRANSITIONS

Spec for layered composition, crossfades, the palette walk, accumulator retirement,
and fixed-point easing. Every claim about current behaviour below was read out of
`bridge.c` / `draw.s` and measured, not assumed.

---

## 0. Measured baseline (v2.0)

Verified by reading the source and running the numbers.

| Claim | Verdict | Evidence |
|---|---|---|
| Routine pick is random-with-replacement over 124 | **Confirmed** | `bridge.c:31` `m = mix32(seg) % total`, `total = 24 + jd_pattern_count` = 124 |
| Only 110/124 routines appear in 300 segments | **Confirmed** | simulated the exact expression: 110 distinct, one routine hit 8× |
| 39 repeats inside a 20-segment window | **Confirmed exactly** | 39 of 300 segments repeat a routine seen in the prior 19 |
| Palette A-usage ranges 5..16 over 300 segments | **Confirmed exactly** | min 5, max 16 |
| Cross-scheme brightness spread 0.08, saturation 0.15 | **Confirmed** (these are *stdev*) | stdev 0.080 / 0.154 over the 30 schemes; min–max range is 0.37 / 0.61 |
| Half the schemes are full-spectrum | **Confirmed, and worse** | 19 of 30 schemes cover ≥11 of 12 hue bins, not 14 |

Two things the brief did not have, both found by reading the source:

**A. `draw.s` already chains its palette walk. `bridge.c` does not.**

`draw.s:161-207` picks `A = mix(p) % 30`, `B = mix(p+1) % 30` where `p = frame >> 10`.
Because `B` of leg *p* is literally `mix(p+1)` — the same expression as `A` of leg *p+1* —
the walk is continuous by construction. The comment even says so. Measured: **0 of 599 leg
boundaries jump.**

`bridge.c:37-39` picks `A = mix32(seg*K+1) % 30`, `B = mix32(seg*K+2) % 30` — two
*independent* draws per segment. Next segment re-rolls both. Measured: **286 of 299 segment
boundaries are discontinuous, mean per-channel jump 73.4/255.** So the C patterns take a
hard colour cut every 34 s while the asm modes never do. That is a bug against the engine's
own stated intent, and it is likely a large part of what J is seeing.

**B. The fade parameter `t` is linear in both engines.**

`draw.s:207` `ubfx w25, w3, #2, #8` → `t = (frame>>2) & 255`, period 1024 = exactly one leg.
`bridge.c:40` `t = (sl>>3) & 255`, period 2048 = exactly one segment.

Linear means colour velocity is at maximum the instant a leg begins and stops dead at the
seam. Even with A chained to B, there is a **velocity discontinuity** at every seam. This is
a real "rough break" that survives the chaining, and fixing it is the single cheapest win in
this document (§3.2).

**C. Accumulators hard-blank.** Six sites in `draw.s` (`L5start`, `L7start`, `L8start`,
`L10start`, `L11start`, `L12start`) all do `and w22, w9, #2047` / `cbnz w22, L…draw`, then
fill the entire framebuffer with `0xFF0A0A12`. Instant blank, one frame. The C accumulators
(`pattern_041/071/072/078`, …) follow the same `sl == 0` convention.

**D. Every segment boundary is a hard cut.** `bridge.c` has no crossfade between routines at
all. When `m < 24` it returns early from `draw_frame` without ever touching the blended
palette, so asm→C transitions also re-roll colour.

---

## 1. Architecture: the layer stack

J: *"modular. use 1, then 3 seconds later, another, then 1 second another, building layer
upon layer."*

Replace "one routine per 2048-frame segment" with a **stack of up to 4 independently-scheduled
layers**, each with its own lifecycle and its own persistent canvas.

```c
#define JD_LAYERS   4
#define JD_W        1280
#define JD_H        960

enum { JD_MIX = 0, JD_SCREEN, JD_MULT };

typedef struct {
    int       live;        /* slot occupied */
    int       routine;     /* 0..23 = asm mode, 24.. = lab pattern */
    int       t_in, t_full, t_out, t_end;   /* global frame marks */
    uint32_t  seed;        /* stable for the layer's whole life */
    uint8_t   blend;       /* JD_MIX for slot 0, MIX/SCREEN above */
    uint8_t   style;       /* JD_DISSOLVE | JD_BLOOM | JD_COLLAPSE */
    uint32_t *buf;         /* private canvas, allocated once, never freed */
} jd_layer;

static jd_layer  g_layer[JD_LAYERS];
static uint32_t *g_buf[JD_LAYERS];   /* 4 × 1280×960×4 = 19.7 MB, one-time */
```

**Why private buffers are non-negotiable:** accumulator routines build an image over
thousands of frames. They cannot share a scratch framebuffer with a co-resident layer. One
buffer per slot, allocated at startup, reused by successive tenants.

### 1.1 Layer lifecycle

```
  t_in ──fade in──▶ t_full ──────hold──────▶ t_out ──fade out──▶ t_end
    w=0            w=1                       w=1                w=0
```

| Phase | Duration | Notes |
|---|---|---|
| fade in | **180 frames (3.0 s)** | never below 120; see §4.3 for the derivation |
| hold | 900–3600 frames (15–60 s) | dice per layer |
| fade out | **240 frames (4.0 s)** | deliberately longer than the fade in — a slow exit reads as *dissolving*, a fast one reads as *cut* |

### 1.2 Scene scheduling

A **scene** is a co-resident group of layers. Scenes overlap so there is never a frame on
which the whole composition changes.

```
layers per scene:   1 → 25%   2 → 45%   3 → 25%   4 → 5%
entry stagger:      +150..+300 frames (2.5–5.0 s) after the previous layer   ← J's "3 seconds… 1 second"
scene length:       2700..5400 frames (45–90 s)
exit stagger:       180..420 frames (3–7 s) apart, reverse-ish order
scene overlap:      the last layer of scene K fades out THROUGH the first
                    layer of scene K+1 fading in
```

**Hard invariants**, assert them in a debug build:

1. `sum(w_i) > 0` on every frame — the screen is never empty.
2. At most one layer changes `live` state per 120 frames — no simultaneous swaps.
3. No two live slots hold the same `routine`.
4. Slot 0 (the ground) is always `JD_MIX`.

---

## 2. Compositing

### 2.1 Weight envelope

```c
/* Q16 weight of a layer at global frame f. Returns 0..65536. */
static uint32_t layer_weight(const jd_layer *L, int f)
{
    if (f <= L->t_in  || f >= L->t_end) return 0;
    if (f >= L->t_full && f <= L->t_out) return 65536;
    if (f < L->t_full) {                      /* rising */
        uint32_t x = (uint32_t)((int64_t)(f - L->t_in) * 65536 / (L->t_full - L->t_in));
        return ease_ss(x);
    }
    /* falling */
    uint32_t x = (uint32_t)((int64_t)(L->t_end - f) * 65536 / (L->t_end - L->t_out));
    return ease_ss(x);
}
```

### 2.2 Normalized dissolve — the default, and the reason nothing flashes

Naive `sum(w_i * c_i)` blows the brightness out as layers stack. A **normalized weighted
average** conserves total brightness, so the screen does not pulse brighter or darker as a
layer enters or leaves. That property *is* "no rough breaks".

The divide must not be per-pixel. Normalize the weights **once per frame** so they sum to
exactly 256 (Q8), then the inner loop is multiply-accumulate only:

```c
/* once per frame: gather live layers, normalize weights to Q8 summing to 256.
   NOTE nw is uint16_t, NOT uint8_t — a dominant layer legitimately reaches 256. */
static int composite_prepare(int f, int *idx, uint16_t *nw)
{
    uint32_t w[JD_LAYERS]; uint64_t W = 0; int n = 0;
    for (int i = 0; i < JD_LAYERS; i++) {
        if (!g_layer[i].live) continue;
        uint32_t wi = layer_weight(&g_layer[i], f);
        if (!wi) continue;
        idx[n] = i; w[n] = wi; W += wi; n++;
    }
    if (!n) return 0;
    int sum = 0, big = 0;
    for (int k = 0; k < n; k++) {
        nw[k] = (uint16_t)((uint64_t)w[k] * 256 / W);
        sum += nw[k];
        if (w[k] > w[big]) big = k;
    }
    nw[big] = (uint16_t)(nw[big] + (256 - sum));   /* rounding remainder to the dominant layer */
    return n;
}
```

`nw[]` now sums to exactly 256, so no per-pixel divide and no brightness drift.

> **`uint8_t` here is a bug.** With weights like `{65535, 1}` the floors are `{255, 0}`, the
> remainder is 1, and the dominant layer lands on **256 — which does not fit in `uint8_t`** and
> wraps to 0, blanking the frame. Verified over 400 000 randomized 2/3/4-layer trials with
> `uint16_t`: sum is exactly 256 in every case, and compositing identical inputs returns them
> unchanged (brightness conservation holds). Overflow headroom in the `n>=3` path is fine:
> max channel accumulate is `255 × 256 × 4 = 261 120`, well inside `uint32_t`.

### 2.3 The inner loop, specialized by layer count

```c
void jd_composite(uint32_t *fb, int npix, int n, const int *idx, const uint16_t *nw)
{
    if (n == 1) return;                       /* layer rendered straight into fb — free */

    if (n == 2) {                             /* the common case: one lerp */
        const uint32_t *a = g_layer[idx[0]].buf, *b = g_layer[idx[1]].buf;
        uint32_t tb = nw[1], ta = 256 - tb;
        for (int i = 0; i < npix; i++) {
            uint32_t ca = a[i], cb = b[i];
            uint32_t rb = (((ca & 0xFF00FFu) * ta + (cb & 0xFF00FFu) * tb) >> 8) & 0xFF00FFu;
            uint32_t g  = (((ca & 0x00FF00u) * ta + (cb & 0x00FF00u) * tb) >> 8) & 0x00FF00u;
            fb[i] = 0xFF000000u | rb | g;
        }
        return;
    }

    /* n >= 3: per-channel accumulate. 255*256*4 = 261120, fits u32 with room. */
    for (int i = 0; i < npix; i++) {
        uint32_t r = 0, g = 0, b = 0;
        for (int k = 0; k < n; k++) {
            uint32_t c = g_layer[idx[k]].buf[i], m = nw[k];
            r += ((c >> 16) & 255) * m;
            g += ((c >>  8) & 255) * m;
            b += ( c        & 255) * m;
        }
        fb[i] = 0xFF000000u | ((r >> 8) << 16) | ((g >> 8) << 8) | (b >> 8);
    }
}
```

The `n == 1` path costs exactly what v2.0 costs today. Under the §1.2 schedule the `n >= 3`
path is live roughly 15% of wall time.

### 2.4 Blend modes above the ground

Slot 0 is always the normalized ground. Upper slots may declare `JD_SCREEN` for
additive-feeling material (sparks, particles, ray fans) — it never darkens and never hard
clips:

```c
/* out = lerp(a, screen(a,b), w) per channel, w in Q8 */
static inline uint32_t ch_screen(uint32_t a, uint32_t b, uint32_t w) {
    uint32_t s = a + b - ((a * b) >> 8);      /* screen */
    return (a * (256 - w) + s * w) >> 8;
}
```

**Cap screen layers at `w ≤ 0.60`** (`39322` Q16). Uncapped screen layers wash the frame to
white and wreck the motion budget. The cap is applied to the envelope, not the composite, so
the ease shape is preserved.

### 2.5 Transition styles: dissolve, bloom, collapse

J said "no rough breaks *if possible*". A hard-edged wipe is a break. A **heavily feathered
radial reveal** is not — it reads as an organic bloom. Spec both, ban the hard edge.

```
JD_DISSOLVE  70%   uniform weight across the frame (§2.2)
JD_BLOOM     20%   radial reveal outward from centre
JD_COLLAPSE  10%   radial reveal inward from the edges
```

For bloom/collapse the layer's weight becomes **per-pixel**:

```c
/* built once per resolution: distance from centre, in 1/4 pixel units */
static uint16_t *g_radlut;        /* w*h */
static uint8_t   g_easelut[256];  /* ease_ss sampled to 8 bits */

/* FEATHER must be >= 25% of the screen diagonal — this is what makes it a
   bloom and not a wipe line. At any instant a quarter of the frame is
   mid-blend. */
#define FEATHER_NUM 1
#define FEATHER_DEN 4

/* R sweeps -FEATHER .. diag+FEATHER over the fade, driven by the SAME
   eased global weight, so bloom and dissolve share one timing law. */
static inline uint32_t bloom_w(int i, int32_t R, int32_t feather, uint32_t wglob)
{
    int32_t d = (int32_t)g_radlut[i];
    int32_t u = ((R - d) << 8) / feather;
    if (u <= 0)   return 0;
    if (u >= 256) return wglob;
    return (g_easelut[u] * wglob) >> 8;       /* local reveal × global envelope */
}
```

Multiplying by `wglob` matters: the reveal edge itself also fades up, so even the leading
edge of the bloom never appears as a sharp arc.

---

## 3. The palette walk

### 3.1 One walk, globally owned

Today there are **two** independent walks (asm and C) and they disagree. Collapse to one,
owned by `bridge.c`, consumed by both — `draw.s` reads it through globals the same way it
already reads `g_mode`.

Layers **share the palette**. This is the quiet hero of the whole spec: two routines
crossfading in the *same* colours read as one image changing shape. Two routines crossfading
in *different* colours read as two images fighting. Shared palette is what makes a plain
dissolve look intentional.

### 3.2 Chained legs with eased traversal

```c
#define JD_NS      60          /* schemes after "double the palettes" */
#define JD_LEG   1024          /* frames per leg ≈ 17.07 s */

static void jd_palette_walk(int frame, int *A, int *B, uint32_t *t_q16)
{
    uint32_t leg = (uint32_t)frame >> 10;
    *A = scheme_at(leg);
    *B = scheme_at(leg + 1);                   /* chained: this leg ENDS where the next BEGINS */
    uint32_t x = ((uint32_t)frame & (JD_LEG - 1)) << 6;   /* 0..65472, Q16 */
    *t_q16 = ease_ss(x);                       /* <-- the fix in (B) */
}
```

Two properties, both required:

- **Chained** (`B(leg) == A(leg+1)`): kills the 73.4/255 colour jump. `draw.s` already has
  this; `bridge.c` does not. Measured on the proposed expression: **0 discontinuities.**
- **Eased** `t`: kills the *velocity* kink at the seam that both engines have today. Colour
  eases out of A and into B with zero velocity at both ends, so the seam is not merely
  continuous but C²-continuous. Costs one `ease_ss` per frame.

### 3.3 Scheme selection: shuffled bag, not hash-mod

`mix32(x) % NS` is sampling with replacement — that is the direct cause of the 5..16 usage
range. Replace with a **deterministic shuffled bag**: every scheme appears exactly once per
epoch.

```c
static uint8_t  g_bag[JD_NS];
static uint32_t g_bag_epoch = 0xFFFFFFFFu;

static void build_bag(uint32_t epoch)
{
    uint8_t prev_last = g_bag_epoch != 0xFFFFFFFFu ? g_bag[JD_NS - 1] : 0xFF;
    for (int i = 0; i < JD_NS; i++) g_bag[i] = (uint8_t)i;
    uint32_t r = mix32(epoch ^ 0x5BF03635u);
    for (int i = JD_NS - 1; i > 0; i--) {      /* Fisher-Yates */
        r = mix32(r);
        int j = (int)(r % (uint32_t)(i + 1));
        uint8_t s = g_bag[i]; g_bag[i] = g_bag[j]; g_bag[j] = s;
    }
    /* epoch seam guard: never repeat across the bag boundary */
    if (g_bag[0] == prev_last) { uint8_t s = g_bag[0]; g_bag[0] = g_bag[1]; g_bag[1] = s; }
    g_bag_epoch = epoch;
}

static int scheme_at(uint32_t leg)
{
    uint32_t epoch = leg / JD_NS, pos = leg % JD_NS;
    if (epoch != g_bag_epoch) build_bag(epoch);
    return g_bag[pos];
}
```

With `JD_NS = 60`, one epoch is 60 legs ≈ **17 minutes** — every palette is seen before any
repeats. A-usage range collapses from 5..16 to exactly 1 per epoch.

> **Note — `scheme_at` is called for both `leg` and `leg+1`.** At a bag boundary that means
> two different epochs in one call. Either keep two cached bags, or (simpler) precompute the
> full 2·NS-entry window. Do not let `build_bag` thrash once per frame at the seam.

### 3.4 Same bag for routines

Identical mechanism, applied to layer starts instead of segments. This is the fix for
measured problem 1:

- draw the routine for a new layer from a shuffled bag over `0..total-1`
- reject a draw if that routine is already live in another slot (advance the bag)
- with `total = 248` after doubling, the bag guarantees all 248 before any repeat

Projected against the measured baseline: distinct-over-300 goes 110/124 → 124/124, and
in-window repeats go 39/300 → 0.

### 3.5 Rebuild the blended palette only when it changes

`bridge.c:41-48` rebuilds all **32 768** palette entries **every frame** — 2.0 M blends/s —
even though the current `t = (sl>>3)&255` only changes once every 8 frames. Pure waste.

```c
static uint32_t g_blend[32768];
static int      g_blend_key = -1;

static void palette_update(int A, int B, uint32_t t_q16)
{
    uint32_t t8  = t_q16 >> 8;                       /* Q16 -> 0..255 */
    int key = (A << 16) | (B << 8) | (int)t8;
    if (key == g_blend_key) return;                  /* <-- 8x fewer rebuilds */
    g_blend_key = key;
    const uint32_t *pa = jd_palette + (size_t)A * 32768;
    const uint32_t *pb = jd_palette + (size_t)B * 32768;
    uint32_t it = 256 - t8;
    for (int i = 0; i < 32768; i++) { /* ... existing RB/G lerp ... */ }
}
```

Quantizing the eased Q16 `t` to 8 bits also removes the banding a raw Q16 `t` would produce.
The inner loop is trivially NEON-able (`vmull_u8` + `vshrn`, 8 px/iter) for another ~4×.

---

## 4. Easing in fixed point

### 4.1 The curves

Both are written in **factored (Horner) form with round-to-nearest**. This is not cosmetic —
see the warning below.

```c
/* smoothstep, Q16 in -> Q16 out.  x^2 * (3 - 2x).  f'(0) = f'(1) = 0 */
static inline uint32_t ease_s(uint32_t x)
{
    uint64_t X  = x;
    uint64_t x2 = (X * X + 32768) >> 16;
    int64_t  in = 3 * 65536 - 2 * (int64_t)X;             /* Q16 */
    int64_t  r  = ((int64_t)x2 * in + 32768) >> 16;
    return (uint32_t)(r < 0 ? 0 : (r > 65536 ? 65536 : r));
}

/* smootherstep, Q16 in -> Q16 out.  x^3 * (10 - 15x + 6x^2).
   f'(0)=f'(1)=0 AND f''(0)=f''(1)=0 — no onset, no offset, no kink. */
static inline uint32_t ease_ss(uint32_t x)
{
    uint64_t X  = x;
    uint64_t x2 = (X * X + 32768) >> 16;
    uint64_t x3 = (x2 * X + 32768) >> 16;
    int64_t  in = 10 * 65536 - 15 * (int64_t)X + 6 * (int64_t)x2;   /* Q16 */
    int64_t  r  = ((int64_t)x3 * in + 32768) >> 16;
    return (uint32_t)(r < 0 ? 0 : (r > 65536 ? 65536 : r));
}
```

> **Do not use the naive `6*x5 - 15*x4 + 10*x3` expansion.** It was in the first draft of this
> spec and it is broken: independently truncating `x2,x3,x4,x5` lets the `-15·x4` term
> dominate near the top of the range, and the result goes **backwards**. Measured on a 180-frame
> fade it is non-monotonic at f=178 — i.e. the layer briefly *un-fades* one frame before it
> lands. That is precisely the kind of one-frame hitch J has been rejecting builds for.

Verified on the factored forms above:

| Check | `ease_s` | `ease_ss` |
|---|---|---|
| endpoints `0 / 32768 / 65536` | 0 / 32768 / 65536 | 0 / 32768 / 65536 |
| quarter points vs float | exact | 6784 / 58752 — exact |
| max abs error vs float, full range | 1.97 LSB | 5.12 LSB (of 65536) |
| worst backward step, full range | 1 LSB | 8 LSB |
| monotonic when sampled at D=180 | yes | yes, 0 violations |

The residual LSB wobble is rounding noise at the flat ends: 8/65536 = **0.012 % of opacity**,
and consecutive frames are ≥546 LSB apart at D=120, so no frame pair ever lands inside a
wobble. Harmless — but stated rather than hidden.

### 4.2 Which curve where

| Use | Curve | Why |
|---|---|---|
| layer weight envelope | `ease_ss` | it is the visible one; C² continuity is worth the cost |
| palette leg traversal `t` | `ease_ss` | removes the seam velocity kink (§3.2) |
| bloom/collapse reveal edge | `ease_ss` via 256-entry LUT | per-pixel, must be table-driven |
| radius / orbit / interior modulation | `ease_s` | cheaper, kink is not visible on geometry |

### 4.3 Fade duration derived from the motion budget

House rule: mean per-channel frame-to-frame delta **< 8**.

For `c = (1-w)a + w·b`:

```
dc/df  ≈  |dw/df|·|b-a|  +  (1-w)·|da/df|  +  w·|db/df|
             ^^^^^^^^^^^^^^^^^^^ the term the crossfade ADDS
```

Peak `|dw/df|` for smootherstep is `30x²(1-x)²` at `x=0.5` = `1.875 / D` where D is the fade
length in frames. With a typical inter-layer mean absolute difference of ~90/255:

| D (frames) | D (s) | added delta |
|---|---|---|
| 120 | 2.0 | **1.41** |
| 180 | 3.0 | **0.94** |
| 240 | 4.0 | **0.70** |

All far inside the budget of 8. **Floor D at 120 frames** and the crossfade can never be the
thing that makes it jerky.

> **Honest tradeoff:** linear easing has peak `|dw/df| = 1/D`, so smootherstep costs 1.9× the
> peak fade delta (0.94 vs 0.50 at D=180). We spend it deliberately — linear's derivative
> discontinuity at the endpoints is exactly the "pop" the eye catches, and the budget has
> room to spare.

---

## 5. Accumulator retirement

> *"what happens when an accumulator ends — fade its canvas out, don't blank it"*

**This solves itself under the layer model, with zero pattern edits.** That is the point of
the design.

### 5.1 The mechanism

1. An accumulator owns `layer->buf` exclusively.
2. `sl` becomes **layer-local**: `sl = frame - L->t_in`, not `frame & 2047`.
3. The pattern still clears at `sl == 0` — but that frame is now `t_in`, where **`w == 0`**.
   The blank happens off-screen. Nobody sees it.
4. At the end, the envelope eases `w → 0` over 240 frames. The composite dissolves the
   accumulated canvas away. **The pattern is never asked to blank; it just stops being
   weighted.**
5. The next tenant of that slot clears the same buffer on *its* frame 0 — again at `w == 0`.

The plug-in contract change is one sentence, and no existing pattern needs touching:

> `sl` is frames since **this layer** started. A pattern must clear its canvas when
> `sl == 0` and **must never clear at any other time.**

### 5.2 `draw.s` changes — six mechanical sites

Add globals next to `g_mode`, set by `bridge.c` before each `draw_frame` call:

```c
uint32_t g_mode;      /* existing */
uint32_t g_sl;        /* NEW: layer-local frame */
uint32_t g_seed;      /* NEW: per-layer stable hash */
```

Then in each of `L5start L7start L8start L10start L11start L12start`, replace

```asm
    and     w22, w9, #2047              // segment-local frame
```

with a load from `_g_sl`. `L5start` additionally has `lsr w23, w9, #11` / `mul w23, w23, w10`
building a segment hash — replace that with a load from `_g_seed`. Six sites, identical edit
at five of them.

Also: `draw_frame` must render into `L->buf`, not the shared `fb`. It already takes the
target pointer as `x0`, so this is a call-site change in `bridge.c` only — no asm change.

### 5.3 Freeze optimization

An accumulator must keep *running* while it fades out, or you get a frozen still sliding to
black — which reads as a stall, not a transition. But below ~15% opacity nobody can tell:

```c
#define JD_FREEZE_W 9830          /* 0.15 in Q16 */

/* in the render loop, for a FALLING layer only: */
if (falling && w < JD_FREEZE_W) {
    /* skip the pattern call; reuse buf as-is, keep decaying w */
} else {
    render_layer(L, f);
}
```

Saves the tail of every fade-out at zero visual cost. Do **not** apply it to rising layers —
a frozen entrance is very visible.

---

## 6. Optimization summary

| # | Change | Win |
|---|---|---|
| 1 | Rebuild blended palette only on `t8` change (§3.5) | **8×** fewer palette rebuilds |
| 2 | NEON the palette blend loop | **~4×** on top of #1 |
| 3 | Normalize weights once per frame, not per pixel (§2.2) | removes a per-pixel divide entirely |
| 4 | Specialize composite by layer count; `n==1` writes straight to `fb` (§2.3) | 1-layer cost == v2.0 cost |
| 5 | Skip rendering any layer with `w == 0` | 4 slots, typically 1–2 rendering |
| 6 | Freeze falling layers below `w < 0.15` (§5.3) | ~25% of every fade-out |
| 7 | Layer buffers allocated once at startup | zero alloc churn |

Net: 3-layer overlap is only ~15% of wall time under the §1.2 schedule, and #1+#2 pay for a
large part of the extra render cost outright.

---

## 7. Verification

Non-negotiable — a compiling change that looks wrong is a failure.

**Motion budget.** Dump frames *around scheduled transitions*, not random frames — transitions
are the only place the budget is at risk.

```
clang -O2 -I. <harness using jd_frame> bridge.c patterns_c/pattern_*.c \
      patterns_c/registry.c draw.s -o /tmp/dump
# for each scheduled t_in / t_out, dump [t-30, t+D+30] and assert mean delta < 8
```

**Palette continuity.** Pure unit test, no rendering:

- `scheme_at(leg+1) == B(leg)` for 100 000 legs
- per-channel palette delta across a leg seam == **0** (today: 73.4 for the C path)
- `d(t)/df == 0` at both ends of every leg (eased traversal)

**Bag uniformity.** Over 10 000 draws: per-item count within ±1 of uniform; **zero** repeats
inside any window of `total`. Assert distinct-over-300 == 124 (or 248) and in-window repeats
== 0, against the measured baseline of 110 and 39.

**Easing regression** (both bugs above were caught here — keep these permanently):

- sample `ease_ss` at every frame of a D=120/180/240 fade; assert **no backward step**. The
  naive expansion fails this at f=178/180.
- assert endpoints exactly `0` and `65536`, and midpoint exactly `32768`.
- assert max abs error vs a double-precision reference < 8 LSB over the full range.

**Composite regression:**

- 400 000 randomized 2/3/4-layer weight sets: `sum(nw) == 256` exactly, and `nw` never
  exceeds 256 (this is the `uint8_t` overflow test).
- identity: compositing N layers that all hold the same colour must return that colour
  bit-exact — this is the brightness-conservation proof.

**Invariants** (debug build, every frame): `sum(w_i) > 0`; at most one `live` transition per
120 frames; no duplicate routine across live slots.

**Visual.** Watch a full scene cycle at 1× and confirm: no flash at any layer entry or exit,
no colour jump at any leg seam, no blank frame when an accumulator retires.

---

## 8. Build order

1. **`ease_s` / `ease_ss` + unit tests.** Standalone, zero risk.
2. **Palette walk fix** — chain + ease `t` in `bridge.c`, shuffled bag, cached rebuild.
   *Ship this alone first.* It is small, it is the largest measured defect (73.4/255 jump
   every 34 s), and it needs no layer work. Verify the app still builds and runs.
3. **Routine bag** in the existing single-routine dispatcher. Still no layers. Fixes
   problem 1 on its own.
4. **Layer stack**, `JD_LAYERS = 2`, dissolve only. `n==1` path must be byte-identical to
   today's output — that is the regression test.
5. **`g_sl` / `g_seed`** and the six `draw.s` sites; asm accumulators into layer buffers.
6. **`JD_LAYERS = 4`**, screen blend, bloom/collapse.
7. **NEON palette blend**, freeze optimization.

Steps 1–3 are independently shippable and each one strictly improves v2.0. The app builds and
runs at every step.
