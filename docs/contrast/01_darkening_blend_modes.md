# Darkening and contrast blend modes for the JellyDazzle compositor

Target: `src/engine/compositor.c` §4 (blend kernels), `blend_span`, `pick_blend`.
Basis: 1,962 spawns in `docs/contrast/spawn_telemetry.csv`; kernels as written at v3.0.4.

---

## 0. The measurement that decides everything

Telemetry, overlay slots only (slot 0 and slot 4 are ground/shadow and are always `B_MIX` at 256):

| mode | spawns | mean peak w | max peak w | direction |
|---|---|---|---|---|
| `B_MAX` | 742 | 125 (49%) | 180 | lighten |
| `B_SCREEN` | 261 | 96 (38%) | 139 | lighten |
| `B_DIFF` | 71 | 89 (35%) | 90 | contrast-ish |
| `B_MIX` | 888 | 256 | 256 | ground only |

- **69% of all overlays are `B_MAX` at a mean weight of 49%.** That is the single most-executed decision in the engine, and against a bright ground it is a no-op.
- Take a real bright ground, `d = 200`, at the mean overlay weight `w = 110` (43%). What each mode can actually put on screen:

| mode | source used | result over d=200 | Δ | local contrast \|Δ\|/d |
|---|---|---|---|---|
| `B_MAX` | any s ≤ 255 | 200 | **0** | **0%** |
| `B_SCREEN` | s = 255 (white) | 225 | +25 | 12.5% |
| `B_ADD` | s = 255 | 255 (clipped) | +55 | 27.5%, toward white |
| MULTIPLY | s = 0 | 114 | −86 | **43%** |
| MULTIPLY | s = 64 | 136 | −64 | 32% |
| DARKEN (min) | s = 0 | 146 | −54 | 27% |
| LINEAR BURN | s = 0 | 91 | −109 | **54.5%** |
| HARD LIGHT | s = 0 | 114 | −86 | 43% |
| COLOUR BURN | s = 64 | 129 | −71 | 35.5% |

- `B_MAX` needs a raw source of `s ≥ 466` to register at that weight. There is no such colour. The accent is arithmetically discarded — this is not a tuning problem, it is a `max()` returning its left operand 100% of the time.
- Multiply at the *same* weight delivers 3.4× the local contrast of screen, and it delivers it **without moving toward white**, which is the mechanism behind "foggy clutter."
- Note the asymmetry: darkening does not need higher weights than the engine already grants. A 43% multiply is a 43% luminance drop. A 43% screen is a 12.5% lift. The existing `PEAK_HI` table is already generous for a darkening mode; if anything it wants capping downward, not raising.

---

## 1. Weight semantics — the contract inverts, and the file already has the precedent

### The two existing contracts

The file already contains **two different meanings of `w`**, and nobody wrote it down:

- **Source-scaling contract** (`span_max`, `span_screen`, `span_add`): scale the source toward black, then combine. Works only because black is the identity element of `max`, `screen`, and `add`. `w = 0` ⇒ source is black ⇒ no-op. Cheap: the scale is one packed multiply.
- **Result-lerp contract** (`span_diff`): compute the blend at full strength, then `lerp(dst, blend, w)` per channel. `span_diff` does this inline with `iw = 256 - w`. `w = 0` ⇒ dst survives ⇒ no-op.

`span_diff` is the precedent. Every darkening mode must follow it.

### Why source-scaling is fatal for darkening

For multiply, `blend(d, s) = d·s/255`. Under the source-scaling contract:

```
s'  = s·w/256
out = d·s'/255 = d·s·w/(255·256)
```

At `w = 0` this is **pure black**, not identity. The weight ramp is inverted: fading a multiply layer *in* would fade the screen *out*. A layer's `w_now` envelope (0 → `w_peak` → 0, `FADE_IN`/`FADE_OUT`) would produce a black flash at both ends of every tenancy. Non-negotiable: darkening modes cannot use the source-scaling contract.

### The algebra: what the correct interpolation actually is

Start from result-lerp and simplify for multiply:

```
out = lerp(d, d·s/255, w)
    = [ d·(256−w) + (d·s/255)·w ] / 256
    = d · [ 255·(256−w) + s·w ] / (255·256)
    = d · lerp(255, s, w) / 255
```

**The result-lerp of a multiply is exactly a multiply by a source lifted toward *white*.** So the identity element of the darkening family is `0xFFFFFF`, not `0x000000`, and the source-scaling contract survives intact — you just change which corner you scale toward:

| family | "quiet" direction for the source | pre-scale |
|---|---|---|
| lighten (`MAX`, `SCREEN`, `ADD`) | toward black | `s' = (s·w) >> 8` |
| **darken (`MULT`, `MIN`, `LBURN`)** | **toward white** | `s' = 255 − (((255−s)·w) >> 8)` |

Verify the lift at the endpoints: `w = 256` ⇒ `s' = 255 − (255−s) = s` (full strength). `w = 0` ⇒ `s' = 255` (identity). Monotone in between.

This matters for cost: the white-lift is the same shape as the black-scale, so it is **packable with the same `0x00FF00FF` / `0x0000FF00` trick** — per-lane `255 − x` is `0x00FF00FF − (x & 0x00FF00FF)`, and no borrow can cross a lane because every lane byte is ≤ `0xFF`. Overflow envelope is identical to `span_max`: `0x00FF00FF · 256 = 0xFF00FF00`, which fits `uint32_t` exactly. `MULT` and `MIN` therefore cost the same weight-application as `MAX` does.

`HARD LIGHT`, `LINEAR BURN` and `COLOUR BURN` are not pure multiplies and do not admit the white-lift shortcut — they use the explicit inline `iw = 256 − w` lerp, exactly as `span_diff` does.

### What has to change in `blend_span`

**Structurally, nothing.** Each darkening kernel folds its own weighting, so:

```c
static void blend_span(uint32_t *dst, const uint32_t *src, int n,
                       uint32_t w, int mode)
{
    if (!w) return;                       /* still exact for both families */
    switch (mode) {
        case B_MAX:    span_max(dst, src, n, w);    break;
        case B_SCREEN: span_screen(dst, src, n, w); break;
        case B_ADD:    span_add(dst, src, n, w);    break;
        case B_DIFF:   span_diff(dst, src, n, w);   break;
        case B_MULT:   span_mult(dst, src, n, w);   break;   /* new */
        case B_MIN:    span_min(dst, src, n, w);    break;   /* new */
        case B_HARD:   span_hard(dst, src, n, w);   break;   /* new */
        case B_LBURN:  span_lburn(dst, src, n, w);  break;   /* new */
        case B_CBURN:  span_cburn(dst, src, n, w);  break;   /* optional */
        default:       span_lerp(dst, dst, src, n, w); break;
    }
}
```

The `if (!w) return;` guard stays correct — `w == 0` is identity in both contracts. Fades stay clean.

**The rejected alternative**, stated so it is on the record: a generic wrapper that calls the unweighted blend into a scratch buffer and then `span_lerp`s it. That is *four* passes over `npix` instead of two, plus a 29.9 MB scratch allocation at 3456×2160. See §8. Do not do this.

**Two real changes are needed elsewhere:**

1. **The enum needs a class marker**, because `pick_blend` must reason about direction:

```c
/* Ordered by class so direction is a range test, not a switch.
 * MIX is neutral; MAX..DIFF lighten or preserve; MULT..CBURN darken;
 * HARD is the only bidirectional mode and sits with the darkeners
 * because that is the half of its range we are buying it for. */
enum { B_MIX = 0,
       B_MAX, B_SCREEN, B_ADD, B_DIFF,       /* lighten / contrast */
       B_MULT, B_MIN, B_LBURN, B_HARD, B_CBURN };
#define JD_IS_DARK(m)  ((m) >= B_MULT)
```

2. **`pick_blend` needs the mirror of its own SCREEN cap.** The existing cap encodes "screen over a *dark* ground goes milky fast." The dual is exactly as true: multiply over a *dark* ground goes to mud fast, and worse, it drags composite luma under `JD_LUMA_FLOOR` and hands the frame to `g_gain`, which lifts everything and washes the colour back out. That is a feedback loop between two correction systems and it will look like the engine is breathing. Sketch:

```c
if (JD_IS_DARK(mode)) {
    uint32_t gl = g_L[0].live ? g_st[g_L[0].routine].luma : 100;
    /* Below ~90 there is nothing left to carve out of; above ~170 the
     * ground can absorb a full-strength darkener.  Linear between. */
    uint32_t cap = gl >= 170 ? 168u : gl <= 90 ? 0u : (gl - 90) * 168u / 80u;
    if (p > cap) p = cap;
    if (!cap) mode = B_MAX;              /* fall back, do not force it */
}
```

The `cap == 0` fallback matters: a darkening mode over a genuinely dark ground is not "subtle," it is invisible and it costs luma the frame does not have.

---

## 2. `B_MULT` — multiply. The one to build first.

**What it does.** `out = d·s/255`. Every channel of the destination is scaled by the corresponding source channel expressed as a fraction. The source acts as a transmission filter: white passes, black stops, a saturated colour subtracts its complement. It can never lighten and it can never clip.

**When it beats MAX/SCREEN.** Whenever the ground's luma is above roughly 150 — which is where every one of J's "barely visible accent" frames lives. `MAX` returns the ground bit-exact there. `SCREEN` returns the ground plus a desaturated lift, i.e. the fog. Multiply is the only mode in the set whose *effect size scales with how bright the ground is*: over `d = 200` a black source at 43% removes 86 levels; over `d = 60` it removes 26. It is self-limiting in exactly the direction the engine wants — strongest where the problem is, quiet where it isn't. Nothing else in the file has that property.

**The rounding choice.** The house uses `>> 8` where `/255` is meant (see `span_screen`'s `(dr*sr) >> 8`). For multiply that is not acceptable, and the reason is the identity end, not the mid-range: with `s' = 255` (which is what `w → 0` produces), `(d·255) >> 8 = d − 1` for every `d ≥ 1`. A multiply layer fading in would step the entire frame down one level on its first non-zero weight — a visible flat-field shift on the smooth grounds this mode is for.

Use `(d · (s' + 1)) >> 8`:

- `s' = 255` ⇒ `(d·256) >> 8 = d`, bit-exact identity.
- `s' = 0` ⇒ `(d·1) >> 8 = 0` for all `d ≤ 255`, bit-exact black.
- Error bound against exact: `d·s'/255 − d·(s'+1)/256 = d·(255 − s')/65280 ≤ 255·255/65280 = 0.996`. Never more than one level low, never high, monotone in both arguments.
- Cost: 3 ops per channel (add, mul, shift) versus 5 for the exact `t = a·b + 128; (t + (t>>8)) >> 8`. Take the cheap one; it is exact where exactness is load-bearing.

```c
/* MULTIPLY: the first mode in this engine that can DARKEN.  Where MAX
 * returns the ground untouched over a bright ground (max(200, accent) is
 * the ground, always) and SCREEN lifts it toward white, MULT scales the
 * ground DOWN by the source, and does so proportionally — hardest on the
 * brightest grounds, which is exactly where the accents were being lost.
 *
 * WEIGHT: this is a darkening mode, so the source's quiet corner is WHITE,
 * not black.  Lifting s toward 255 by (256-w) is algebraically identical to
 * lerp(d, d*s/255, w) — see docs/contrast §1 — so the weight can still be
 * applied to the source in one packed pass, and dst is read exactly once.
 *
 * ROUNDING: (d * (s'+1)) >> 8, not (d * s') >> 8.  The latter returns d-1
 * for s'=255, i.e. a multiply layer at w=1 would step the whole frame down
 * a level on fade-in.  The +1 form is bit-exact at BOTH ends (s'=255 -> d,
 * s'=0 -> 0) and never more than one level low in between. */
static void span_mult(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];

        /* Lift the source toward white by (256-w), two channels at a time.
         * Per-lane 255-x is a plain subtract from the lane mask: no borrow
         * can cross a lane because every lane byte is <= 0xFF.  The product
         * tops out at 0x00FF00FF * 256 = 0xFF00FF00, which still fits. */
        uint32_t srb = 0x00FF00FFu - ((((0x00FF00FFu - (s & 0x00FF00FFu)) * w) >> 8) & 0x00FF00FFu);
        uint32_t sg  = 0x0000FF00u - ((((0x0000FF00u - (s & 0x0000FF00u)) * w) >> 8) & 0x0000FF00u);

        uint32_t sr = (srb >> 16) & 255, sb = srb & 255, sgv = (sg >> 8) & 255;
        uint32_t dr = (d   >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;

        /* No clamp: d*(s'+1)>>8 <= 255 for all 8-bit inputs, by construction. */
        uint32_t r = (dr * (sr  + 1)) >> 8;
        uint32_t g = (dg * (sgv + 1)) >> 8;
        uint32_t b = (db * (sb  + 1)) >> 8;

        dst[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}
```

**Cost.** ~36 integer ops/px: 12 for the packed white-lift, 10 unpack, 9 core, 5 repack. That is **0.92× `span_screen` (39)** and **1.0× `span_diff` (36)**. It is cheaper than screen because it has no clamps and no branches — the whole kernel is straight-line, three independent dependency chains, and it vectorises to NEON without a single horizontal op.

**Failure cases.**
- **Dark grounds (luma < ~90).** Goes to mud, then to black. `d = 40` multiplied by anything is still ≤ 40 and the accent is invisible for the opposite reason. Gate on ground luma (§1).
- **Saturated complementary pairs.** A pure red ground under a pure cyan accent is `(200,0,0) × (0,255,255) = (0,0,0)`. Two vivid colours produce a black hole. `st->sat` on both the layer and the ground should temper the peak.
- **The `g_gain` interaction.** Multiply lowers mean composite luma. Enough of it and the dead-air guard fires, `g_gain` lifts the whole frame, and the ground brightens back to where multiply has to fight it again. Watch `g_gluma` in a battery run with multiply enabled before shipping.
- **Accumulator canvases (`C_CANVAS`, asm modes 15..23).** Modes 17 and 20 paint a near-white canvas. Multiplying *by* a near-white canvas is a near-no-op; multiplying a bright ground *by* mode 23's deep magenta is a very large one. Class-aware gating is worth a look.

---

## 3. `B_MIN` — darken. The cheapest addition, and the exact dual of the most-used mode.

**What it does.** `out = min(d, s')` per channel. Where the accent is darker than the ground it replaces it bit-exact; where it is brighter the ground survives bit-exact. Hard-edged, absolute, zero softness — precisely `span_max` with the comparison flipped and the source lifted the other way.

**When it beats MAX/SCREEN.** When the accent's *shape* matters more than its colour: sprite silhouettes, slashes, particle trails. `MAX` is 69% of the engine's overlays and its entire virtue, per the comment in the source, is "wherever the overlay is darker than the ground, the ground survives bit-exact… that is what keeps a stack readable instead of grey." `MIN` buys the identical guarantee in the other direction: over a bright ground, an accent's dark strokes land at full contrast with a crisp edge, and the ground is untouched everywhere else. There is no grey and no fog because nothing is ever averaged.

**Rounding.** None needed anywhere — `min` is exact, and the white-lift is the only arithmetic. Same bit-exactness argument as `MULT`: `w = 0` ⇒ `s' = 0xFFFFFF` ⇒ `min(d, 255) = d` per channel.

```c
/* DARKEN (per-channel min): the exact dual of span_max, which is 69% of
 * every overlay this engine spawns.  MAX guarantees the ground survives
 * bit-exact wherever the overlay is darker; MIN guarantees the OVERLAY
 * survives bit-exact wherever it is darker than the ground.  Over a bright
 * ground that is the difference between an accent that is discarded by a
 * max() and one that lands at full contrast with a hard edge.
 *
 * WEIGHT: source lifted toward WHITE, because 0xFFFFFF is the identity of
 * min() the way 0x000000 is the identity of max().  Same packed shape as
 * span_max, same overflow envelope, one extra subtract per lane. */
static void span_min(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];

        /* s' = 255 - ((255 - s) * w >> 8), two lanes at a time. */
        uint32_t srb = 0x00FF00FFu - ((((0x00FF00FFu - (s & 0x00FF00FFu)) * w) >> 8) & 0x00FF00FFu);
        uint32_t sg  = 0x0000FF00u - ((((0x0000FF00u - (s & 0x0000FF00u)) * w) >> 8) & 0x0000FF00u);

        uint32_t dr = d & 0x00FF0000u, dg = d & 0x0000FF00u, db = d & 0xFFu;
        uint32_t sr = srb & 0x00FF0000u, sb = srb & 0xFFu;

        /* Channels are compared in place inside their own bit fields, so no
         * unpack is needed — the same trick span_max uses. */
        dst[i] = 0xFF000000u | (dr < sr ? dr : sr) | (dg < sg ? dg : sg)
                             | (db < sb ? db : sb);
    }
}
```

**Cost.** ~27 ops/px versus `span_max`'s ~23. Four extra ops, all in the weight lift, for the entire missing half of the engine's compositing range. This is the best cost-to-benefit item in the document. Branch-free (`csel`), fully vectorisable (`UMIN` on NEON, one instruction for 16 bytes).

**Failure cases.**
- **Invents colours neither operand has.** `min((255,0,0), (0,0,255)) = (0,0,0)`. Per-channel min over two saturated, differently-hued layers produces black or muddy tertiaries. This is the same objection that made `pick_blend` prefer `SCREEN` over `MAX` for "dark but coloured" material — it applies symmetrically, and `MIN` should be gated on low `st->sat` or high `st->cdiv` mismatch the same way.
- **Hard edges.** Same complaint as `MAX` ("the hard edges B_MAX cuts", per the `span_screen` comment). On a soft-focus accent the edge is where the two luma surfaces cross, which is a noisy contour, not the sprite's outline. Use `MULT` for soft material and `MIN` for graphic material.
- **Near-equal luma.** When ground and accent sit within ~15 levels of each other the min surface flickers between them frame to frame as both animate. Visible as shimmer.

---

## 4. `B_HARD` — hard light. The best single mode for "nuanced flashes."

**What it does.** `hardlight(d, s) = overlay(s, d)` — keyed on the **source**. Where the accent is darker than mid-grey it multiplies (by `2s`); where it is brighter it screens (by `2s − 255`). Mid-grey source is a no-op.

**When it beats MAX/SCREEN.** This is the only mode in the set that is bidirectional from a single pass. A sprite whose body is dark and whose highlight is bright will **carve into a bright ground and glow on a dark one, simultaneously, in the same frame**. That is the literal description of J's complaint — "real moments of beauty, but a lot of foggy clutter where the accent is barely visible" — the beauty is the lit half, the fog is what `SCREEN` does with the dark half. Hard light keeps the first and inverts the second into contrast.

It also matters because it makes the accent's own tonal structure the operator. `MAX` throws away everything below the ground's luma; hard light spends it.

**The algebra.** The two-branch definition collapses to one expression. Let `u = 2s − 255` (signed, −255..255):

```
s < 128:  out = d·(2s)/255        = d + d·u/255          (u < 0)
s ≥ 128:  out = d + u·(255−d)/255                        (u ≥ 0)

so        out = d + u · T / 255,   T = (u < 0 ? d : 255 − d)
```

One select on the sign of `u`, one multiply, no branch. Substituting `/255` with the `(u·(T+1)) >> 8` form keeps both endpoints exact:

- `s = 255` ⇒ `u = 255`, `T = 255−d`, `(255·(256−d)) >> 8 = 255−d` ⇒ `out = 255`. Correct.
- `s = 0` ⇒ `u = −255`, `T = d`, `(−255·(d+1)) >> 8 = −(d+1)` after arithmetic shift ⇒ `out = −1`. **One low.** Clamp at zero; that is the only clamp the kernel needs, and it costs one `csel`.
- `s = 128` ⇒ `u = 1`, `T = 255−d`, term is 0 for `d > 0` ⇒ near no-op. Correct.

Weighting must be the explicit `span_diff`-style result lerp — the white-lift trick does not apply because hard light is not a pure multiply.

```c
/* HARD LIGHT: source-keyed contrast.  The accent's own dark half MULTIPLIES
 * the ground and its bright half SCREENS it, in one pass.  This is the mode
 * that fixes the actual complaint: MAX discards everything in the accent
 * darker than the ground (which over a bright ground is all of it), SCREEN
 * pushes the whole accent toward white (the fog).  HARD LIGHT spends the
 * accent's full tonal range — the dark strokes carve, the highlights flare.
 *
 * ALGEBRA: with u = 2s-255 in [-255,255], the two-branch definition
 *   s <128: d*(2s)/255            s >=128: d + (2s-255)*(255-d)/255
 * collapses to  out = d + u * (u<0 ? d : 255-d) / 255,  one sign select.
 *
 * WEIGHT: explicit result lerp (iw = 256-w), same as span_diff.  The
 * white-lift shortcut used by span_mult/span_min does not apply here —
 * hard light is not a pure multiply and has no single identity colour
 * reachable by scaling the source. */
static void span_hard(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    uint32_t iw = 256 - w;
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
        uint32_t sr = (s >> 16) & 255, sg = (s >> 8) & 255, sb = s & 255;
        uint32_t o[3]; uint32_t dc[3] = { dr, dg, db }, sc[3] = { sr, sg, sb };

        /* Unrolled by the compiler; written as a loop only so the three
         * channels cannot drift apart during maintenance. */
        for (int c = 0; c < 3; c++) {
            int32_t D = (int32_t)dc[c];
            int32_t u = (int32_t)(sc[c] << 1) - 255;      /* -255 .. +255   */
            int32_t T = u < 0 ? D : 255 - D;              /* room to move   */
            /* (T+1) keeps both ends exact: u=+255 reaches 255, u=-255
             * reaches -1 (clamped below).  Arithmetic shift on the negative
             * branch floors, which is at most one level dark — acceptable,
             * and it never overshoots upward. */
            int32_t x = D + ((u * (T + 1)) >> 8);
            if (x < 0) x = 0;                             /* only clamp     */
            o[c] = (uint32_t)((D * (int32_t)iw + x * (int32_t)w) >> 8);
        }
        dst[i] = 0xFF000000u | (o[0] << 16) | (o[1] << 8) | o[2];
    }
}
```

**Cost.** ~60 ops/px. That is **1.54× `span_screen`** and the most expensive mode recommended here. Straight-line, branch-free (the `if` is a `csel`), NEON-friendly — the sign select is `CMLT` + `BSL`.

**Failure cases.**
- **Mid-grey accents do nothing.** An accent whose material sits around `s ≈ 128` is a near-perfect no-op. Anything with low `st->sat` and mid `st->luma` should never be assigned hard light — it will look like the layer failed to spawn. Gate on `st->luma < 100 || st->luma > 160`, or on high `st->cdiv`.
- **A contour at `s = 128`.** The derivative is discontinuous at the switch point (slope `2d/255` below, `2(255−d)/255` above). On an accent with a smooth gradient sweeping through mid-grey there is a visible ridge. Hard light wants graphic, high-contrast sources — which is what `R_SPARK` and `R_FIGURE` mostly are.
- **Double-counts the source's own contrast.** Because it scales by `2s`, a source that is already high-contrast becomes brutal. Combined with the fact that `PEAK_HI[3] = 115` (45%) it is usually fine, but at full weight it posterises.

---

## 5. `B_LBURN` — linear burn. Cheap, brutal, use as seasoning.

**What it does.** `out = max(0, d + s − 255)`. Subtractive rather than proportional: the source's *inverse* is subtracted flat from the destination. Removes the same absolute number of levels everywhere, so it crushes shadows and leaves highlights comparatively intact. Preserves hue better than multiply in the midtones because it does not scale the channels differentially.

**When it beats MAX/SCREEN.** When you want a hard, graphic shadow rather than a tint. It is the exact dual of `span_add` — and `B_ADD` is the mode the source file calls "the brightest and least forgiving." Linear burn is the darkest and least forgiving, and it should be deployed with the same restraint: as `pick_blend` seasoning at a capped peak, the way `B_DIFF` already is (capped at 90 in the current code, mean measured peak 89).

**The algebra and the rounding.** The naive simplification is a trap. Algebraically,

```
lerp(d, d + s − 255, w) = d − ((255 − s)·w)/256
```

which is beautifully cheap — one subtract mirroring `span_add`'s one add. **But it clamps in the wrong order.** Clamping the lerp's output at zero is not the same as lerping toward a blend that was already clamped:

- Exact (clamp then lerp), `d = 100, s = 50, w = 128`: blend `= max(0, −105) = 0`, `out = lerp(100, 0, ½) = 50`.
- Cheap (lerp then clamp): `out = max(0, (100·128 − 105·128) >> 8) = max(0, −2) = 0`.

50 versus 0. The cheap form over-darkens catastrophically across the entire region where linear burn should have bottomed out — that is a large fraction of any real frame. Clamp first, then lerp. It costs one `csel` and it is not optional.

```c
/* LINEAR BURN: out = max(0, d + s - 255), the exact dual of span_add.
 * Subtractive rather than proportional — it takes the same number of levels
 * out of every pixel, so it crushes the shadows and leaves the highlights
 * comparatively alone.  That is the opposite failure mode to multiply, and
 * it makes for a harder, more graphic shadow.
 *
 * As unforgiving as B_ADD is at the other end, so treat it the way
 * pick_blend already treats B_DIFF: seasoning, at a capped peak.
 *
 * ORDER OF OPERATIONS (do not "simplify" this):  lerp(d, d+s-255, w)
 * algebraically equals  d - ((255-s)*w >> 8), which is one subtract and
 * looks like a free win.  It is not: that form clamps AFTER the lerp, and
 * everywhere the unclamped blend is negative it over-darkens hard.
 * d=100, s=50, w=128 gives 50 the correct way and 0 the "cheap" way.
 * Clamp the blend, THEN interpolate. */
static void span_lburn(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    uint32_t iw = 256 - w;
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
        uint32_t sr = (s >> 16) & 255, sg = (s >> 8) & 255, sb = s & 255;

        /* Signed intermediates: d+s-255 is in [-255, +255]. */
        int32_t tr = (int32_t)(dr + sr) - 255; if (tr < 0) tr = 0;
        int32_t tg = (int32_t)(dg + sg) - 255; if (tg < 0) tg = 0;
        int32_t tb = (int32_t)(db + sb) - 255; if (tb < 0) tb = 0;

        uint32_t r = (dr * iw + (uint32_t)tr * w) >> 8;
        uint32_t g = (dg * iw + (uint32_t)tg * w) >> 8;
        uint32_t b = (db * iw + (uint32_t)tb * w) >> 8;

        dst[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}
```

**Cost.** ~39 ops/px — the same as `span_screen`, and the same shape as `span_diff` (~36). Branch-free, vectorises to `UQSUB`-style saturating arithmetic almost directly.

**Failure cases.**
- **Flat black plateaus.** Everything with `d + s < 255` clamps to the same value, so large regions collapse to identical black with no detail. On a mid-grey ground (`d ≈ 128`) with a mid-grey accent this is most of the frame.
- **Banding on gradients.** The clamp boundary is a hard contour across any smooth ramp. Combined with 8-bit output there is visible stair-stepping right at the knee.
- **Luma collapse.** It removes more absolute luminance than multiply for the same weight (−109 versus −86 in the §0 table). Fastest route to tripping `JD_LUMA_FLOOR`. Cap the peak at `B_DIFF` levels (≤ 90) and gate it on ground luma ≥ 170.

---

## 6. `B_CBURN` — colour burn. Viable, but only with a reciprocal table.

**What it does.** `out = s == 0 ? 0 : 255 − min(255, (255−d)·255/s)`. Extremely aggressive darkening that increases saturation as it goes: it drives midtones toward the *colour* of the accent rather than toward black or grey. The strongest carve available, and the only darkening mode that makes the result **more** saturated rather than less.

**When it beats MAX/SCREEN.** When the ground is bright *and* the wanted accent is a colour, not a shape. `SCREEN` on a bright ground desaturates toward white — that is the fog, mechanically. Colour burn does the opposite: over `d = 200` with a mid-tone coloured accent it produces a deep, saturated dark of the accent's hue. If J's "moments of beauty" are the coloured ones, this is the mode that makes them reliable instead of accidental.

**The divide problem, and the fix.** The definition contains a per-channel divide by `s`. On ARM64 `UDIV` is ~10-12 cycles, not pipelined for throughput, and **there is no integer divide in NEON at all** — a scalar divide in the inner loop permanently forecloses vectorising this kernel. That is disqualifying on its face for a 7.5 Mpx frame.

It is recoverable with a 1 KB constant reciprocal table:

```c
/* recip[s] = (255 << 12) / s, built once at init.  Q12 rather than Q16:
 * Q16 makes the product (255 * 16711680) = 4.26e9, which fits uint32_t by
 * about 0.8% — too close to the edge to be comfortable in a kernel that
 * will get ported to asm.  Q12 caps the product at 2.66e8 with room to
 * spare and costs at most one level of precision at the top of the range. */
static uint32_t g_recip[256];
static void recip_init(void)
{
    g_recip[0] = 0;                                  /* s=0 -> result 0     */
    for (int s = 1; s < 256; s++) g_recip[s] = (255u << 12) / (uint32_t)s;
}
```

1 KB is L1-resident after the first scanline and never evicted by the streaming pixel traffic (it is touched every pixel). The lookup becomes one dependent load plus a multiply and a shift.

```c
/* COLOUR BURN: the only darkening mode that INCREASES saturation.  SCREEN
 * over a bright ground desaturates toward white — that is the mechanism
 * behind the fog.  Colour burn drives the same pixels toward the accent's
 * own hue instead, deep and saturated.  Strongest carve in the set.
 *
 * The definition divides by s per channel.  ARM64 UDIV is ~10-12 cycles and
 * NEON has no integer divide at all, so a literal implementation cannot be
 * vectorised.  A 1 KB Q12 reciprocal table replaces it with one L1 load, a
 * multiply and a shift.  The table is constant, built at init, and is
 * touched every pixel so it stays hot.
 *
 * Gate this hard.  It clips to black faster than anything else here. */
static void span_cburn(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    uint32_t iw = 256 - w;
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t dc[3] = { (d >> 16) & 255, (d >> 8) & 255, d & 255 };
        uint32_t sc[3] = { (s >> 16) & 255, (s >> 8) & 255, s & 255 };
        uint32_t o[3];

        for (int c = 0; c < 3; c++) {
            uint32_t D = dc[c], S = sc[c], t;
            if (!S) t = 0;                            /* s=0 -> full burn   */
            else {
                /* q = (255-D) * 255 / S, in Q12.  Clamp before the final
                 * subtract: q > 255 means the burn has already bottomed. */
                uint32_t q = ((255u - D) * g_recip[S]) >> 12;
                t = q >= 255u ? 0u : 255u - q;
            }
            o[c] = (D * iw + t * w) >> 8;             /* result lerp        */
        }
        dst[i] = 0xFF000000u | (o[0] << 16) | (o[1] << 8) | o[2];
    }
}
```

**Cost.** ~45 integer ops/px **plus three dependent L1 loads per pixel**. The op count understates it: the loads are on the critical path (address depends on the source pixel, result feeds the multiply), so real throughput is worse than `span_hard` despite the lower op count. Call it 1.6–2.0× `span_screen` in practice, and note that a NEON port needs `TBL` gathers or a polynomial approximation — it is the one mode here that does not port cleanly.

**Failure cases.**
- **Black holes on anything but a bright ground.** `d = 100`, `s = 64` gives `255 − min(255, 155·255/64) = 255 − 255 = 0`. Instantaneous black. It needs `d > ~180` to behave.
- **Noise amplification near `s = 0`.** The reciprocal blows up, so a single dark pixel in a dithered accent punches a black dot. Any accumulator with dither or noise in it (`C_CANVAS` modes) will speckle.
- **Posterises smooth gradients.** The `q >= 255` clamp plus Q12 truncation puts visible contours in any soft ramp.
- **Verdict:** worth having for the look, but it is a fifth priority, and it should be gated on ground luma ≥ 180 *and* `st->sat ≥ 40` *and* a low peak (≤ 100).

---

## 7. Modes I recommend against

### `B_OVERLAY` — do not add

- **What it does.** `overlay(d, s) = hardlight(s, d)`. Multiplies where the *destination* is dark, screens where the destination is bright.
- **Why it fails here specifically.** It is keyed on the destination. The problem case is a **bright** ground — `d > 128` — so overlay takes the screen branch and does exactly what `B_SCREEN` already does, only with a `2×` gain that makes the fog worse. Overlay boosts the *ground's* contrast using the accent as the operator; J needs the opposite, the accent's contrast against the ground.
- The cost is the same ~60 ops/px as hard light, and it is literally the same kernel with the arguments swapped (`span_hard(dst, src)` versus a hypothetical `span_hard`-with-roles-reversed). If it is ever wanted, implement it as a flag on `span_hard` rather than a second kernel — but there is no case in the current telemetry where it beats what is already there.
- Additional failure: the `d = 128` seam. Because the key is the destination, the seam moves *with the ground animation*, so the contour crawls across the frame. Very visible on the slow grounds (`delta_q8 < 512`).

### `B_SOFTLIGHT` — do not add

- **What it does.** A gentle, non-linear push of the destination toward black or white depending on the source, with no clipping and no hard seam. Pegtop's continuous form avoids the `sqrt` and the branch: `out = 2ds/255 + d²(255−2s)/255²`, which factors usefully into `m = d²/255; out = m + 2s(d − m)/255`.
- **Cost.** ~63 ops/px, because it needs two exact `div255` operations per channel (the cheap `>>8` form breaks the "black and white are fixed points" property that is the whole reason the mode looks soft). That is 1.6× screen for the *least* effect of any mode in this document.
- **Why it fails here.** Soft light's defining property is that it cannot move `d = 0` or `d = 255` at all, and that its effect at any `d` is bounded by roughly `d(255−d)/255` — a maximum of 64 levels at mid-grey, falling to nothing at both ends. The engine's complaint is that accents are *too subtle*. Prescribing the subtlest mode in the catalogue for a subtlety problem is backwards. It is also worst exactly where the fog is worst: a blown-out ground near 255 is untouched by soft light entirely.
- If a gentle darkener is ever wanted, `B_MULT` at a low `w_peak` is cheaper, better behaved, and tunable with one integer.

### Modes not worth writing up at all

- **Vivid light / linear light / pin light.** Compound modes built from colour-burn/colour-dodge or from burn/add pairs. Each inherits colour burn's divide and adds a second branch. Their look is a harder version of hard light, which the engine can already reach by raising `w_peak`.
- **Hard mix.** Thresholds every channel to 0 or 255. It is a 3-bit posterizer. It would be genuinely striking for one second per hour and unwatchable otherwise, and it interacts terribly with the `g_gain` guard.
- **Darker colour / lighter colour.** Luma-keyed whole-pixel selects. Cheap, but they produce a hard binary mask over the frame that reads as a mask, not as compositing.

---

## 8. Does anything need the destination read twice? — No, and that is the whole design constraint

**Answer: none of the kernels above reads `dst` twice, and that is deliberate.**

- Every kernel loads `dst[i]` once into a register, uses it for both the blend computation and the result lerp, and stores once. Two memory touches per pixel per layer, identical to `span_max`/`span_screen`/`span_add`/`span_diff`.
- The temptation is a generic weighting wrapper — compute the unweighted blend into a scratch buffer, then `span_lerp(dst, dst, scratch, n, w)`. **Reject it.** The arithmetic saving is nil and the memory cost is severe:

| approach | passes over `npix` | bytes moved per overlay per frame @ 3456×2160 |
|---|---|---|
| in-register lerp (recommended) | 1 read src, 1 read+write dst | 89.6 MB |
| scratch + `span_lerp` | 1 read src, 1 write scratch, 1 read scratch, 1 read+write dst | 149.3 MB |

- At 3456×2160 one frame buffer is **29.86 MB**. The compositor today moves roughly 240 MB/frame (ground copy plus three overlay read-modify-writes) = **14.3 GB/s at 60 Hz**, before the luma sample passes and `span_gain`. Adding 60 MB per darkening overlay is +3.6 GB/s per layer. On a shared LPDDR bus that is the difference between fitting `BUDGET_Q8` (10.5 ms) and not.
- **Corollary: the compositor at 4K is bandwidth-bound, not ALU-bound.** That reframes the whole cost table below — a mode that costs 1.5× the ops of `span_screen` does *not* cost 1.5× the time, because the pixel traffic is identical. Op count matters for the eventual asm port and for the smaller resolutions; **pass count** is what matters at the top of the range. Every kernel here is one pass.
- One aliasing note worth acting on separately: `blend_span`'s `dst` and `src` are always distinct buffers at the call site (the warp scratch is a third buffer), but the signature does not say so. Marking both `restrict` lets the compiler keep `dst[i]` in a register across the whole body instead of reloading defensively — free, and it makes the "read once" property structural rather than incidental. `span_scale(fb, fb, ...)` and `span_lerp(fb, ...)` are called with aliasing arguments elsewhere and must not be given `restrict`.

---

## 9. Cost summary

Integer ops per pixel, all three channels, excluding loads/stores. `csel` counted as one.

| mode | ops/px | vs `span_screen` (39) | clamps | branches | dst reads | NEON-portable |
|---|---|---|---|---|---|---|
| `span_lerp` (`B_MIX`) | 16 | 0.41× | 0 | 0 | 1 | yes |
| `span_max` (`B_MAX`) | 23 | 0.59× | 0 | 0 | 1 | yes (`UMAX`) |
| **`span_min` (`B_MIN`)** | **27** | **0.69×** | 0 | 0 | 1 | **yes (`UMIN`)** |
| `span_add` (`B_ADD`) | 30 | 0.77× | 3 | 0 | 1 | yes (`UQADD`) |
| `span_diff` (`B_DIFF`) | 36 | 0.92× | 0 | 0 | 1 | yes |
| **`span_mult` (`B_MULT`)** | **36** | **0.92×** | **0** | 0 | 1 | **yes** |
| `span_screen` (`B_SCREEN`) | 39 | 1.00× | 3 | 0 | 1 | yes |
| **`span_lburn` (`B_LBURN`)** | **39** | **1.00×** | 3 | 0 | 1 | yes (`UQSUB`) |
| **`span_cburn` (`B_CBURN`)** | 45 + 3 L1 loads | ~1.6–2.0× real | 3 | 0 | 1 | **no — needs `TBL`** |
| **`span_hard` (`B_HARD`)** | **60** | **1.54×** | 3 | 0 | 1 | yes (`CMLT`+`BSL`) |
| `span_overlay` (rejected) | 60 | 1.54× | 3 | 0 | 1 | yes |
| `span_softlight` (rejected) | 63 | 1.62× | 0 | 0 | 1 | yes |

Note the two cheapest recommendations, `B_MIN` (0.69×) and `B_MULT` (0.92×), are **cheaper than the mode they would displace in most cases** (`span_screen`), and `B_MULT` is the same cost as `B_DIFF`, which the engine already runs 71 times per battery. There is no performance argument against adding them.

---

## 10. Recommended order of work

| # | mode | why | risk |
|---|---|---|---|
| 1 | **`B_MULT`** | Direct dual of `SCREEN`. Effect scales with ground brightness — strongest exactly where the problem is. Cheaper than `SCREEN`. | Low. Needs the ground-luma gate. |
| 2 | **`B_MIN`** | Direct dual of `MAX`, which is 69% of overlays. 4 ops more than `span_max`. Best value in the document. | Low. Gate on saturation like `MAX` already should be. |
| 3 | **`B_HARD`** | The only bidirectional mode. Carves and flares from one source. Most likely to produce the "moments of beauty" reliably. | Medium. Mid-grey accents no-op; needs a `st->luma` gate. |
| 4 | **`B_LBURN`** | Cheap, hard, graphic. Seasoning at a capped peak, like `B_DIFF`. | Medium. Fastest to trip `JD_LUMA_FLOOR`. |
| 5 | `B_CBURN` | The only saturation-increasing darkener. Genuinely different look. | High. Needs the reciprocal table, gates hard, does not vectorise. |
| — | `B_OVERLAY` | **Skip.** Destination-keyed, so it screens on the bright grounds that are the problem. | — |
| — | `B_SOFTLIGHT` | **Skip.** Most expensive, least effect, and "not enough effect" is the bug. | — |

Ship 1 and 2 together — they are the two duals, they cost less than what they replace, and between them they cover the flat-tint case and the hard-edge case. Instrument a battery run with `JD_INSTR` and watch `g_gluma` and `g_gain` before adding 3 and 4.

---

## 11. One thing that is not a blend mode but solves the same problem

Worth recording because it may be cheaper than any of the above for the specific case of a small bright sprite on a bright ground:

- The classic fix for an accent that will not separate from its background is not a different blend for the accent — it is a **dark halo underneath it**. Composite the layer twice: once as `B_MULT` at low weight through a dilated or slightly blurred copy of its own coverage (the shadow), then once as `B_MAX` at full peak for the core.
- This gives the accent a guaranteed contrast pedestal regardless of what the ground is doing, and it keeps `B_MAX`'s bit-exact core, which is what makes sprites read as crisp.
- Cost is a second pass over the layer, so it is the one thing in this document that violates §8's one-pass rule. It is only worth it for `R_SPARK` material, which is sparse.
- Mentioned, not recommended, until 1–4 have been evaluated. If `B_HARD` works, this is redundant — hard light produces its own pedestal from the accent's dark half for free.
