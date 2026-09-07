# 05 — Perceptual contrast: why the accents fog, and what "visible" means numerically

Scope: the colour-science half of the "foggy clutter, accent barely visible" defect.
Everything below is measured against this tree — `src/engine/compositor.c`,
`docs/contrast/spawn_telemetry.csv` (1,962 spawns), and the 634 renders in `catalog/img/`.
Benchmarks are scalar C, `-O3`, this machine, 3456x2160 (7.46 Mpx — the size `MAX_PIX`
actually selects, `src/app/main.c:31`).

---

## 0. Verdict first

- **There is a real, unnoticed bug.** Every blend kernel applies its Q8 weight to
  *gamma-encoded* channel values. A "43% opacity" accent delivers **15.5% of its light**,
  not 43%. The under-delivery factor runs **3.1x to 8.6x** across the weights the engine
  actually uses. Fixing it costs one `powf` per layer per frame.
- **SCREEN in encoded space is not a mild approximation, it is a haze generator.**
  Compared to the physically-correct linear-light screen it is up to **+10.5 code values
  too bright in the clutter regime** (bright ground, dim accent) and **−65.6 code values
  too dark in the beauty regime** (dark ground, bright accent). It is wrong in exactly
  the direction J complained about, in both regimes at once.
- **MAX at the measured weights cannot draw on a bright ground at all.** `max(d, w·s)`
  has a hard ceiling of `w·255`. At slot 2's mean weight (119/256) that ceiling is
  **118 code**; on a real bright ground (`catalog/img/006.png`, luma 173) the composite
  altered **0.0% of pixels**. Not "faint". Zero.
- **Working metric: ΔE in OKLab**, with a WCAG-style luminance ratio as a secondary
  floor. Justification and the calibration constant (1 JND ≈ **ΔE_OK 0.020**) in §6.
- **Where the engine sits today**: a measured SCREEN composite scores **ΔE_OK 0.034**
  = 1.7 JND. That is above detection threshold and below "reads as an element". It is,
  precisely, haze. Target is **ΔE_OK ≥ 0.10** local, **≥ 0.20** over busy ground (§7).
- **OKLab is in the build, not in the runtime.** `tools/gen_palettes.py` interpolates
  anchors in OKLab; the runtime then crossfades two schemes with an sRGB channel lerp
  and composites in sRGB. The build-time work is partly undone downstream (§8).

---

## 1. What the engine is actually doing (measured, not assumed)

`blend_span` (`compositor.c:1412`) dispatches five kernels. All five take `w` as Q8
(0..256) and all five apply it to **sRGB code values**:

| kernel | line | operation (per channel, 0..255) |
|---|---|---|
| `span_lerp` (B_MIX) | 1274 | `d·(1−w) + s·w` |
| `span_max` (B_MAX) | 1288 | `max(d, w·s)` |
| `span_screen` (B_SCREEN) | 1304 | `d + w·s − d·w·s/255` |
| `span_add` (B_ADD) | 1324 | `min(255, d + w·s)` |
| `span_diff` (B_DIFF) | 1341 | `d·(1−w) + |d−s|·w` |

Telemetry, 1,962 tenancies (`docs/contrast/spawn_telemetry.csv`):

| slot | MAX share | SCREEN share | DIFF share | mean MAX peak | mean SCREEN peak |
|---|---|---|---|---|---|
| 1 (field) | 82.5% | 10.3% | 7.2% | 153.9 (60.1%) | 100.1 (39.1%) |
| 2 (figure) | 60.8% | 31.5% | 7.7% | 119.4 (46.6%) | 101.7 (39.7%) |
| 3 (spark) | 63.9% | 31.2% | 4.8% | 92.4 (36.1%) | 89.2 (34.8%) |
| 0 / shadow (ground) | — | — | — | **B_MIX at 256 (100%)** | — |

So: the ground is composited at full strength in encoded space, and everything the eye
is supposed to *notice* arrives at 35–60% weight through one of two operators that are
each broken in a different way against a bright ground.

Frame budget for context: `BUDGET_Q8` = **10.5 ms** of render+blend (`compositor.c:116`);
the hot-throttle trips at `g_ewma_ms > 13.5` ms (line 2796).

---

## 2. Why SCREEN over a bright base produces fog — the maths

Work in normalised code values `d, s ∈ [0,1]`, weight `w`, and write `s' = w·s`.

```
SCREEN:   o = d + s' − d·s'
              = 1 − (1 − d)(1 − s')
              = d·(1 − s') + s'                     ... (2.1)
```

**(2.1) is an affine map.** For a locally-constant accent value `s'`, SCREEN takes the
ground and does exactly two things:

- **multiplies every ground difference by `(1 − s')`** — contrast compression;
- **adds a constant `s'` floor** — a lift.

Two ground pixels `d₁, d₂`:

```
o₁ − o₂ = (d₁ − d₂)(1 − s')                          ... (2.2)
```

### This is literally the atmospheric-haze equation

Koschmieder's law for a viewed object through a scattering medium, and the airlight
model every dehazing paper uses, is

```
I(x) = J(x)·t(x) + A·(1 − t(x))
```

— observed = scene × transmittance + airlight × (1 − transmittance). Set `t = 1 − s'`
and `A = 1`. **(2.1) is the same equation.** SCREEN with a bright accent is not *like*
fog; within a constant-accent region it is the identical operator, with the accent
playing the role of airlight and `1 − s'` the role of transmittance. J's word for the
defect is the correct technical term.

### Numbers

Contrast retained after SCREEN, using the measured weights (`s` = a bright accent, 255):

| w (Q8) | s' | ground detail retained `(1−s')` | accent Δ over ground d=0.2 | d=0.5 | **d=0.8** |
|---|---|---|---|---|---|
| 96 | 0.375 | 62.5% | 0.300 | 0.188 | **0.075** |
| 110 | 0.430 | 57.0% | 0.344 | 0.215 | **0.086** |
| 154 (bright-ground cap) | 0.602 | 39.8% | 0.481 | 0.301 | **0.120** |

The accent's own signal is `Δ = s'·(1 − d)`. **It is proportional to the headroom above
the ground.** Over a ground at d = 0.8 the accent has 20% of the range to work in, so a
43%-weight accent produces an 8.6% code excursion while simultaneously flattening 43% of
the ground's own detail. Both halves of "foggy clutter where the accent is barely
visible" fall out of (2.1) and (2.2).

### Distribution of output values

If the ground channel is distributed with mean μ and standard deviation σ, then after a
locally-constant screen:

```
μ' = μ(1 − s') + s'          σ' = σ(1 − s')          ... (2.3)
```

The whole histogram is squeezed into `[s', 1]` and shifted up. Measured on a real pair
(`catalog/img/006.png` ground under `catalog/img/074.png` accent, w = 96), the ground's
encoded standard deviation in the inked region fell from **35.96 to 32.03 (×0.891)**;
at w = 154, to **29.79 (×0.828)**.

### The cap in `pick_blend` is keyed on the wrong variable

`compositor.c:1629-1635`:

```c
uint32_t gl = g_L[0].live ? g_st[g_L[0].routine].luma : 100;
uint32_t cap = gl >= 160 ? 154u : gl <= 40 ? 64u : 64u + (gl - 40) * 90u / 120u;
```

- `gl` is the **whole-frame mean luma of the ground routine**, a single scalar per
  tenancy. Contrast is a *local* phenomenon — (2.2) depends on the ground value under
  each accent pixel, not on the frame mean.
- Second problem: `st->luma` is itself computed from **gamma-encoded** values
  (`stat_image`, line 321: `(r*77 + g*150 + b*29) >> 8`). Those are Rec.601 coefficients
  applied to sRGB code — that is **luma Y′, not luminance Y**. On sRGB/Rec.709 primaries
  it over-weights red by 42% (0.301 vs 0.2126) and blue by 56% (0.113 vs 0.0722). A
  saturated-red ground reads far brighter to `gl` than it is.

---

## 3. Luminance vs perceptual lightness — the gamma bug, quantified

### The setup, in your terms

sRGB code `v` is not light. The display applies the sRGB EOTF, which is a power curve
with a small linear toe:

```
L(v) = v/12.92                     v ≤ 0.04045
L(v) = ((v + 0.055)/1.055)^2.4     otherwise
```

Effective exponent through the midtones ≈ **2.2**. Encoded values are, near enough,
`Q_γ` — a fixed-point format whose *step size varies with the value*.

### The bug

Every kernel computes `s' = (s · w) >> 8` on the encoded value. In light:

```
L(s') = L(w·s) = w^2.2 · L(s)                        ... (3.1)
```

**The engine dials `w`. The display delivers `w^2.2`.**

| w (Q8) | intended opacity | `w^2.2` | exact sRGB-decoded ratio at s=255 | **under-delivery** |
|---|---|---|---|---|
| 64 | 25.0% | 0.047 | 0.051 | **19.7x** |
| 90 | 35.2% | 0.100 | 0.101 | **9.9x** |
| 96 (mean SCREEN, slot 2) | 37.5% | 0.116 | 0.116 | **8.6x** |
| 110 (43%, the figure in the brief) | 43.0% | 0.156 | **0.155** | **6.5x** |
| 125 | 48.8% | 0.207 | 0.203 | **4.9x** |
| 154 (bright-ground cap) | 60.2% | 0.327 | 0.320 | **3.1x** |
| 180 (slot-1 max) | 70.3% | 0.461 | 0.452 | **2.2x** |
| 256 | 100% | 1.000 | 1.000 | 1.0x |

**So no, the accent is not being weighted the way the author intends.** `PEAK_LO/PEAK_HI`
(`compositor.c:1442-1443`) were tuned as opacities — the comment says
"*an overlay that reaches 256 has replaced the ground instead of enriching it*", which
is opacity language. At 110 the author is asking for 43% and getting 15.5%.

### The operator error, separate from the weight error

Two distinct wrongs. (3.1) is the *weight*; this is the *operator*. Comparing the
shipping encoded SCREEN against the linear-light SCREEN at w = 110, over the whole
256×256 (ground, accent) grid:

| regime | definition | encoded − linear-correct (code values) |
|---|---|---|
| all pixels | — | mean **−2.4**, worst **−65.6** |
| **fog** | ground ≥ 100, accent ≤ 160 | **+5.0 mean, +10.5 worst** (too bright) |
| **beauty** | ground ≤ 60, accent ≥ 200 | **−41.9 mean, −65.6 worst** (too dark) |
| — | encoded is brighter than correct over | **68.0% of the grid** |

Read that table again. Doing SCREEN in encoded space **adds veil where the ground is
already bright and subtracts punch where the accent should sing.** It is not a small
uniform error; it is a systematic error whose sign is exactly the shape of the complaint.

Whole-grid summary, encoded vs true linear SCREEN at w = 110: **mean absolute error 8.59
code, worst 65.61 code** — 26% of full scale.

### What is *not* broken

- **`span_max` is gamma-invariant as an operator.** `max()` commutes with any monotone
  transform, so `enc(max(L(a), L(b))) = max(a, b)`. Only its *weight* (3.1) is wrong.
  That matters for the fix ordering in §9.
- `span_diff` is a lerp of an absolute difference; it inherits (3.1) and nothing worse.

---

## 4. MAX's hard ceiling — the other half of the invisibility

`span_max` computes `max(d, w·s)`. The accent's maximum possible output is `w·255`.

```
Accent can never alter a ground pixel whose channel exceeds  w·255.   ... (4.1)
```

| w (Q8) | ceiling (code) | ceiling as % encoded | **ceiling as % of display light** |
|---|---|---|---|
| 72 | 71 | 28.1% | **6.4%** |
| 92 (slot-3 mean) | 91 | 36.1% | **10.6%** |
| 119 (slot-2 mean) | 118 | 46.6% | **18.1%** |
| 154 (slot-1 mean) | 153 | 60.1% | **31.9%** |
| 180 | 179 | 70.3% | 45.2% |

Measured over the catalog:

- **43.7%** of pixels in a ground-eligible render (luma ≥ 55, n = 257) exceed code 124.
- **76.5%** of pixels in a bright ground (luma ≥ 100, n = 72) exceed code 124.
- Composite of `catalog/img/006.png` (luma 173) with `catalog/img/074.png` at w = 125:
  **0.0% of pixels altered**. At w = 180: **0.0%**. 100% of that ground's pixels are
  above the ceiling.

Two consequences worth stating plainly:

- **MAX is a no-op on a bright ground, and no amount of weight below 256 fixes it.**
  It is a switch, not a blend.
- **A MAX layer's fade-in is not a fade.** Because `max(d, w·s) ≠ lerp(d, max(d,s), w)`,
  raising `w` does not dissolve the accent in; it *erodes the set of pixels that win the
  comparison*, and shows the winners at `w·s` — i.e. darkened. Contrast with SCREEN and
  ADD, which are affine in `s` and therefore satisfy
  `screen(d, w·s) = lerp(d, screen(d, s), w)` exactly. The envelope means what it says
  for SCREEN/ADD and does not for MAX.

---

## 5. Should blending happen in linear light?

### Benchmarks (3456x2160 = 7.46 Mpx, scalar C, -O3, this machine)

| variant | ms/frame | note |
|---|---|---|
| `span_max`, ARGB8888 encoded (ships) | **2.27** | SWAR, auto-vectorises |
| `span_screen`, ARGB8888 encoded (ships) | **4.98** | |
| SCREEN on u16 Q12 **linear** buffers (no conversion in the loop) | **5.02** | *same cost* |
| final encode pass, linear u16 → ARGB8888 (4096-entry LUT) | **4.11** | once per frame |
| decode pass, ARGB8888 → linear u16 | 4.88 | |
| SCREEN, one-pass 256-LUT decode + Q12 blend + 4096-LUT encode | **13.06** | 2.6x — LUT gather defeats vectorisation |
| SCREEN, gamma-2.0 (square inline) + 64 KB sqrt LUT | 19.43 | worse, same reason |
| `memcpy` 4 B/px baseline | 0.78 | |

### Reading

- **The arithmetic is not the cost. The table lookup is.** Screening on 16-bit linear
  buffers costs the *same* as screening on 8-bit encoded buffers (5.02 vs 4.98 ms) —
  both are bound by the load/store, and the Q12 version needs no per-pixel LUT. It is
  the `decode → blend → encode` *round trip inside the inner loop* that costs 2.6x,
  because a 256-entry gather kills the vector unit.
- **Therefore: a full linear pipeline is affordable; a per-blend linear conversion is
  not.** Keep every layer buffer in linear Q12 u16, blend in linear all the way up the
  stack, and pay **one** encode pass at the end.

```
today:      4 encoded blends                       ≈ 4 x 4.98 = 19.9 ms   (scalar)
linear:     4 linear blends + 1 encode pass        ≈ 4 x 5.02 + 4.11 = 24.2 ms
delta:      +4.1 ms/frame scalar  (+21%)  = the encode pass, and nothing else
memory:     7.46 Mpx x 8 B x 5 buffers = 298 MB   (vs 149 MB today)
```

- +4.1 ms scalar against a 10.5 ms blend budget is too much **as written**, but the
  encode pass is the single most NEON-friendly thing in the engine: it is a pure
  4096-entry `tbl`-free lookup that can be replaced by a polynomial. `x^(1/2.4)` over
  [0,1] fits a 5-term minimax to <0.5 code, entirely in `vmul/vfma`, no gather. Budget
  it at ~1.0-1.5 ms with NEON. That is affordable.
- Precision check: **Q12 (12-bit) linear is sufficient.** Round-tripping all 256 sRGB
  codes through Q12 gives **worst error 0 code, mean 0.000 code, and 0 of 255 adjacent
  codes collide into the same linear bucket.** Do not use 8-bit linear — it destroys
  the shadows, which is where this engine lives (median catalog luma is 39.4).

### If the full pipeline is too expensive: the cheap approximations, ranked

**(a) Fix the weight only — free, do this first.**
Pre-warp the Q8 weight once per layer per frame:

```c
/* w_lin: the opacity the author means, Q8.  w: what the encoded kernels need. */
static uint8_t g_wgamma[257];   /* built once: 256*pow(i/256.0, 1/2.2) */
w = g_wgamma[w_lin];
```

Zero per-pixel cost, one table of 257 bytes. Delivered linear fraction against target:

| target w | corrected w′ | delivered at s=255 | s=192 | s=128 | s=64 | s=32 |
|---|---|---|---|---|---|---|
| 64 (25%) | 136 | 0.244 | 0.252 | 0.268 | 0.312 | 0.388 |
| 96 (37.5%) | 164 | 0.368 | 0.376 | 0.391 | 0.432 | 0.502 |
| 110 (43%) | **174** | **0.420** | 0.427 | 0.442 | 0.481 | 0.546 |
| 125 (48.8%) | 185 | 0.481 | 0.488 | 0.501 | 0.538 | 0.598 |
| 154 (60%) | 203 | 0.592 | 0.598 | 0.609 | 0.640 | 0.689 |

Accurate to ~2% for accent values above 128; over-delivers on very dark accents (the toe
of the curve), which is harmless — dark accent pixels are the ones you want lifted.

This alone also **raises the MAX ceiling from 124 to 185 code**, which is the difference
between "cannot touch a ground above 49% code" and "cannot touch a ground above 73%".

**(b) Gamma-2.0 approximation — good accuracy, but only pays off in a linear pipeline.**
Replace the sRGB curve with `L ≈ v²`, `v ≈ √L`. Accuracy against the true linear-light
SCREEN, over the whole (d, s) grid at w = 110:

| operator | mean error | worst error |
|---|---|---|
| shipping encoded SCREEN | 8.59 code | **65.61 code** |
| gamma-2.0 SCREEN | **1.22 code** | **8.18 code** |

Gamma-2.0 removes **86%** of both the mean and the worst-case error. But note the
benchmark: as a *per-blend round trip* it measured 19.4 ms — worse than the LUT version.
It is only worth having if the buffers are already linear, where `√` and `x²` let you
skip the 4096-entry table on the encode pass and use NEON `vsqrt` instead.

**(c) Pre-warp the layer palette instead of the pixels.**
`layer_pal_build` (`compositor.c:1117`) already runs a per-entry transfer through
`LL->lut[256]` and rebuilds only every 5–15 frames. A layer's colours all come from
`g_pal[s]` — **32,768 entries, versus 7.46 M pixels: a 228:1 reduction in work.** Any
fixed per-layer tone shaping (the linearisation of the *palette*, chroma boost, a
contrast target from §7) belongs there and is effectively free. Only the frame-varying
envelope weight has to stay per-pixel — and (a) handles that.

**Recommendation.** Do (a) now: it is free, it is a one-line change, and it is
unambiguously a bug fix. Then move the layer buffers to linear Q12 and do (b)'s encode
pass with a NEON polynomial. Do not do a per-blend LUT round trip; the benchmark says it
costs 2.6x for no benefit over keeping the buffers linear.

---

## 6. The metric: use ΔE in OKLab, with a luminance floor

### Candidates

| metric | definition | fit for this engine |
|---|---|---|
| **Weber** `(L−L_b)/L_b` | patch on uniform background | Right *shape* (accent on ground) but luminance-only, unbounded, and undefined at L_b→0 — half this library is near-black. |
| **Michelson** `(L_max−L_min)/(L_max+L_min)` | periodic gratings | Wrong model. Defined for a full-field sinusoid; a moiré field and a lone spark get the same score. |
| **WCAG contrast ratio** `(Y₁+0.05)/(Y₂+0.05)` | text legibility, sRGB | Luminance only — it scores an equal-luminance hue accent at **1.00:1**, i.e. invisible, which is flatly wrong for this content. Its 0.05 flare constant is tuned for a reading screen, and its known failures on dark backgrounds are exactly this engine's operating region. |
| **ΔE\*ab (CIELAB)** | full colour difference | Correct in kind, but CIELAB's hue non-uniformity (the blue-purple problem) is bad news for a palette engine whose ramps sweep hue continuously. |
| **ΔE OKLab** | full colour difference, uniform hue | **Pick this.** |

### Why OKLab, specifically for this case

- **It scores hue and chroma differences, which are half the accent budget here.** The
  palette generator's whole design is hue movement (`SPAN[M_N][JD_NSLOT]`,
  `compositor.c:1447`). A metric that ignores hue would declare a working accent
  invisible and drive the fix in the wrong direction.
- **It is close to uniform, so one threshold works everywhere.** Measured: the neutral
  code step required for a fixed ΔE_OK barely moves with ground level, whereas the WCAG
  ratio for the *same* perceptual step swings by 14%:

| ground code | Δcode for ΔE_OK = 0.10 | WCAG ratio at that step |
|---|---|---|
| 40 | 25.4 | 1.45:1 |
| 80 | 28.0 | **1.54:1** |
| 128 | 30.2 | 1.48:1 |
| 180 | 32.0 | 1.40:1 |
| 220 | 33.1 | **1.35:1** |

  One ΔE_OK number is a usable global gate. One WCAG number is not.
- **The engine already ships the maths.** `tools/palette_score.py` has `to_oklab()` and
  `to_oklch()`; `tools/gen_palettes.py` has `oklab_to_rgb8()`. Nothing new to author.
- **Calibration.** Solving for the neutral step that gives ΔE\*ab = 2.3 (the classical
  JND) at base codes 64/110/128/160/200 gives ΔE_OK = **0.0198 at every one of them**.
  So:

```
1 JND  ≈  ΔE_OK 0.020          1 ΔE*ab  ≈  ΔE_OK 0.0086
```

### The secondary floor, and why you need it

OKLab alone will pass a purely chromatic accent at equal lightness. Human chromatic
contrast sensitivity is **low-pass and cuts off around 10–12 cycles/degree**, against
~50 cpd for luminance (Mullen 1985). A fine chromatic accent at equal luminance is
genuinely weak even at a large ΔE_OK. So gate on both:

```
VISIBLE(accent, ground) :=
      ΔE_OK(accent, ground) ≥ T_dE
  AND |ΔL_OK|              ≥ 0.35 · T_dE      /* at least a third of it is lightness */
```

Measure it **locally** — a 32×32 or 64×64 tile grid over the composite, accent-on vs
accent-off, scoring the tiles where the accent actually has ink. That is Peli's argument
for complex images: global Michelson/Weber are the wrong statistic and band-limited
*local* contrast is the right one.

---

## 7. The visibility threshold — actual numbers

Anchored on the calibration above (1 JND = ΔE_OK 0.020) and on suprathreshold /
masking data.

| tier | ΔE_OK | JNDs | ≈ neutral Δcode | ≈ WCAG ratio | reads as |
|---|---|---|---|---|---|
| below threshold | < 0.020 | < 1 | < 6 | < 1.08:1 | nothing |
| **haze** | 0.020 – 0.05 | 1 – 2.5 | 6 – 16 | 1.08 – 1.22:1 | a tint on the ground — **this is where the engine is now** |
| texture | 0.05 – 0.10 | 2.5 – 5 | 16 – 30 | 1.2 – 1.5:1 | part of the ground, not a thing on it |
| **element (plain ground)** | **0.10 – 0.20** | 5 – 10 | 30 – 62 | 1.4 – 2.3:1 | a distinct element |
| **element (busy ground)** | **≥ 0.20** | ≥ 10 | ≥ 62 | ≥ 1.9:1 | survives masking |
| assertive | ≥ 0.30 | ≥ 15 | ≥ 90 | ≥ 3:1 | reads instantly |

**Where the engine sits.** Measured composite, `catalog/img/006.png` ground (luma 173)
under `catalog/img/074.png` at the mean SCREEN weight w = 96, over inked pixels:

```
mean ΔE_OK  0.0342   (1.7 JND)      p90 ΔE_OK  0.0492   (2.5 JND)
mean Weber  0.140                   mean WCAG  1.124:1
at w = 154 (the bright-ground cap):  mean ΔE_OK 0.0547 (2.7 JND), WCAG 1.204:1
```

**Haze tier. Exactly as described. The measurement agrees with the complaint.**

### Where the tier boundaries come from

- **Detection floor (≈1 JND).** Blackwell's threshold data put luminance-increment
  detection near a **1% Weber fraction** at photopic levels for a large, isolated,
  static target under ideal conditions. Mahy et al. established **ΔE\*ab ≈ 2.3** as the
  practical JND for colour differences, which is the constant calibrated above.
- **Why "element" is 5–10x threshold, not 1x.** Detection ≠ segmentation. An accent
  over a moving, textured, high-contrast ground is *masked*. Legge & Foley's contrast-
  masking data give threshold elevation rising as roughly a power law in masker
  contrast, exponent ≈ 0.6–0.7 above the masker's own threshold. A ground running at
  ~30x its detection threshold elevates the accent's threshold by ≈ 30^0.6 ≈ **7.7x**.
  Rounding to a 5–10x band for a plain-to-busy ground is the honest read of that.
- **Why not the WCAG numbers.** WCAG 2.1 SC 1.4.11 asks 3:1 for non-text UI components
  and 1.4.3 asks 4.5:1 for body text. Both are calibrated for *identifying shape at
  small angular size under adverse viewing* — the wrong task. A visualiser accent is
  large, moving, and only has to *segment* from its ground. 3:1 would be a punishing and
  ugly target here; the ≈1.4–2.3:1 luminance band that the ΔE_OK tiers imply is right.
  Note also that the WCAG 2.x ratio is a known-poor predictor at low luminance, which
  is where this engine spends most of its time; APCA (WCAG 3 draft) exists precisely
  because of that failure, and it too is a text metric.

### Concrete acceptance test for the engine

> For every live overlay layer, over the tiles where that layer has ink, the median
> ΔE_OK between the composite-with-layer and the composite-without-layer must be
> **≥ 0.12**, and at least **20% of inked tiles must reach ΔE_OK ≥ 0.25**.
> Layers failing the first bound for more than 30 consecutive frames are either
> re-weighted or evicted.

That is measurable inside `motion_probe`'s existing per-frame sampling, on a subsampled
tile grid, for well under a millisecond. It turns "foggy" into a number the scheduler
can act on the same way it already acts on `g_motion > 5.5` (`compositor.c:2751`).

---

## 8. OKLab in this codebase — where it is, and where it should also be

**Confirmed true, at build time.** `tools/gen_palettes.py` and `tools/palette_score.py`:

- `to_oklab()` decodes sRGB → linear (`c ≤ 0.04045 ? c/12.92 : ((c+0.055)/1.055)^2.4`)
  before the LMS matrix. **The gamma handling is correct there.**
- Anchor ordering is a greedy OKLab nearest-neighbour tour with 2-opt (`order_ramp`).
- Anchor → 32,768-entry expansion is a cyclic Catmull-Rom **in OKLab**, then
  `oklab_to_rgb8()` back out. The header comment at `gen_palettes.py:38-40` states the
  reason exactly right: sRGB lerping "drives every ramp through a desaturated grey
  midpoint".
- `palette_score.py` scores class membership in OKLCh, explicitly rejecting HSV.

**Confirmed false, at run time.** Everything downstream of `palette.bin` is sRGB channel
arithmetic:

| stage | file:line | space | consequence |
|---|---|---|---|
| scheme A→B crossfade | `compositor.c:1079-1084` | **sRGB lerp** | The exact defect `gen_palettes.py` fixed at build time is reintroduced on every frame of the leg transition. The crossfade dips through desaturated grey between two schemes that were each individually built to avoid it. |
| C-key palette walk | `compositor.c:1095-1100` | sRGB lerp | same |
| hue rotation `hrot` | `compositor.c:1150-1170` | RGB channel swap + **Rec.601 luma-on-encoded** rescale | A channel rotation is a 120° hue shift only in a linear RGB cube; on encoded values it also shifts chroma. The luma-preserving correction uses `(r*77+g*150+b*29)>>8`, i.e. Y′ not Y — so it preserves the wrong quantity. |
| window re-shape / `LL->lut` | `compositor.c:1200-1233` | max/min of encoded channels | This is HSV-style V and S, the very thing `palette_score.py`'s docstring says lies about lightness. |
| all compositing | `compositor.c:1247-1360` | sRGB | §2, §3, §4 |

**Should compositing decisions use OKLab? Yes — but split the two uses:**

- **Per-pixel blending: no.** OKLab needs a cube root per channel. Even a LUT'd
  approximation would be far past the 2.6x that a plain sRGB LUT already costs (§5).
  Blend in **linear light**; that is the space where a screen/add/lerp is physically
  meaningful. OKLab is a *uniform-appearance* space, not a *light-mixing* space —
  mixing in OKLab is a design choice for gradients, not a correctness fix for compositing.
- **Per-decision and per-palette: yes, and cheaply.** Three places, all off the
  per-pixel path:
  1. **`pick_blend` and the peak cap** (line 1618) should key on an OKLab lightness /
     chroma summary of the ground, sampled on a tile grid, not on `st->luma` (Y′).
  2. **`palette_update`'s scheme crossfade** (line 1079) — 32,768 entries every ~5–15
     frames, not 7.46 M pixels. Lerping those in OKLab is affordable and restores the
     build-time guarantee. Or cheaper still: crossfade in **linear light**, which
     removes the grey-midpoint dip for a fraction of the cost of a cube root.
  3. **The visibility test in §7** — pure OKLab, on a tile grid.

---

## 9. What to change, in order

| # | change | cost | expected effect |
|---|---|---|---|
| 1 | Gamma-warp the Q8 weight: `w = g_wgamma[w_lin]`, 257-byte table | free | Accents deliver the opacity the tables were tuned for. MAX ceiling 124 → 185 code. Directly addresses "barely visible". |
| 2 | Re-key the SCREEN peak cap on **local** ground lightness (tile grid, OKLab L), not `st->luma` | ~0.1 ms | Stops capping by a frame mean that no accent pixel actually sits on. |
| 3 | Add the §7 visibility probe to `motion_probe`; feed failures to the existing trim/evict path at line 2760 | <1 ms | "Foggy" becomes a scheduler input instead of an aesthetic complaint. |
| 4 | Stop using MAX on grounds whose local lightness exceeds the ceiling — fall through to SCREEN or a linear ADD | free (a branch in `pick_blend`) | Removes the 0.0%-of-pixels-altered case entirely. |
| 5 | Move layer buffers to Q12 u16 linear; blend in linear; single NEON encode pass at the end | +1.0–1.5 ms with a minimax polynomial (+4.1 ms with a naive LUT) | Removes the 65-code operator error. Fog and punch both fixed at the source. |
| 6 | Crossfade `palette_update` in linear light (or OKLab) | negligible — 32,768 entries per rebuild | Restores what `gen_palettes.py` guarantees at build time. |

Items 1, 2 and 4 are bug fixes and cost nothing. Item 5 is the real fix and is
affordable if and only if the conversion stays out of the inner loop.

---

## 10. Sources

**Colour encoding and linear light**
- IEC 61966-2-1:1999, *Multimedia systems and equipment — Colour measurement and
  management — Part 2-1: Default RGB colour space — sRGB.*
  https://webstore.iec.ch/publication/6169
- Poynton, C. *Gamma FAQ* and *Color FAQ* — the Y vs Y′ (luminance vs luma) distinction
  used in §2 and §8. https://poynton.ca/GammaFAQ.html · https://poynton.ca/ColorFAQ.html
- Gritz, L. and d'Eon, E. (2008). "The Importance of Being Linear." *GPU Gems 3*, ch. 24.
  https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-24-importance-being-linear
- ITU-R BT.709-6 (Rec.709 luminance coefficients 0.2126/0.7152/0.0722) and BT.601-7
  (0.299/0.587/0.114 — the ones `stat_image` uses).
  https://www.itu.int/rec/R-REC-BT.709 · https://www.itu.int/rec/R-REC-BT.601

**Perceptual thresholds and contrast**
- Blackwell, H. R. (1946). "Contrast Thresholds of the Human Eye." *JOSA* 36(11):624–643.
  https://doi.org/10.1364/JOSA.36.000624
- Mahy, M., Van Eycken, L., Oosterlinck, A. (1994). "Evaluation of Uniform Colour Spaces
  Developed after the Adoption of CIELAB and CIELUV." *Color Research & Application*
  19(2):105–121. — source of the ΔE\*ab ≈ 2.3 JND used for the OKLab calibration.
  https://doi.org/10.1002/col.5080190206
- Legge, G. E. and Foley, J. M. (1980). "Contrast masking in human vision."
  *JOSA* 70(12):1458–1471. — the masking exponent behind the 5–10x suprathreshold band.
  https://doi.org/10.1364/JOSA.70.001458
- Peli, E. (1990). "Contrast in complex images." *JOSA A* 7(10):2032–2040. — why global
  Michelson/Weber are the wrong statistic for complex imagery, and the case for
  band-limited *local* contrast. https://doi.org/10.1364/JOSAA.7.002032
- Mullen, K. T. (1985). "The contrast sensitivity of human colour vision to red-green and
  blue-yellow chromatic gratings." *J. Physiol.* 359:381–400. — chromatic CSF cutoff,
  behind the luminance floor in §6. https://doi.org/10.1113/jphysiol.1985.sp015591
- Barten, P. G. J. (1999). *Contrast Sensitivity of the Human Eye and Its Effects on
  Image Quality.* SPIE Press. https://doi.org/10.1117/3.353254
- Whittle, P. (1986). "Increments and decrements: luminance discrimination."
  *Vision Research* 26(10):1677–1691. https://doi.org/10.1016/0042-6989(86)90055-6

**Colour spaces**
- Ottosson, B. (2020). "A perceptual color space for image processing" (OKLab) — the
  matrices in `tools/palette_score.py`. https://bottosson.github.io/posts/oklab/
- Ottosson, B. (2020). "How software gets color wrong." https://bottosson.github.io/posts/colorwrong/
- CIE 15:2018, *Colorimetry, 4th ed.* https://cie.co.at/publications/colorimetry-4th-edition

**Contrast standards (cited for what they are *not* right for)**
- WCAG 2.1 contrast ratio definition. https://www.w3.org/TR/WCAG21/#dfn-contrast-ratio
- WCAG 2.1 SC 1.4.11 Non-text Contrast (the 3:1 figure).
  https://www.w3.org/WAI/WCAG21/Understanding/non-text-contrast.html
- Somers, A. — APCA / WCAG 3 lightness-contrast, and the documented failure modes of
  WCAG 2.x on dark backgrounds. https://git.apcacontrast.com/

**Haze model (§2)**
- Koschmieder, H. (1924). "Theorie der horizontalen Sichtweite." *Beiträge zur Physik
  der freien Atmosphäre* 12:33–53, 171–181. — the original visibility/airlight law.
- He, K., Sun, J., Tang, X. (2011). "Single Image Haze Removal Using Dark Channel Prior."
  *IEEE TPAMI* 33(12):2341–2353 (CVPR 2009 best paper). — the modern statement of
  `I = J·t + A·(1−t)`, identical in form to equation (2.1).
  https://kaiminghe.github.io/publications/pami10dehaze.pdf

---

## Appendix — reproducing the measurements

All figures above come from four throwaway scripts run against this tree. The inputs:

- `src/engine/compositor.c` — kernels at 1247–1360, `pick_blend` 1618, `stat_image` 321,
  `layer_pal_build` 1117, `palette_update` 1062, `PEAK_LO/HI` 1442–1443.
- `docs/contrast/spawn_telemetry.csv` — 1,962 spawn records; blend-mode shares and mean
  peaks in §1 are `groupby(slot, blend)` over that file.
- `catalog/img/*.png` — 634 renders. Ground/accent classification uses the engine's own
  rules (`luma ≥ 55` for ground-eligible; `dark ≥ 215 && sat ≥ 25` for SCREEN-eligible),
  computed with the engine's own `(r*77 + g*150 + b*29) >> 8`.
- Composite simulations reimplement `span_max` and `span_screen` bit-for-bit in float
  with integer truncation, so the ΔE figures are of the shipping arithmetic, not of an
  idealisation.
- Benchmarks: scalar C, `cc -O3`, 3456x2160, 8 iterations, warm cache, single core.
  NEON figures in §5 are estimates and are labelled as such.
