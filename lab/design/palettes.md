# Palette Taxonomy — JellyDazzle v2.1

**Status:** governing spec for the 60 new palettes.
**Enforced by:** `lab/design/palette_score.py` (numpy only).
**Validation harness:** `lab/design/_refcheck.py` — one reference palette per class, all 10 pass.

---

## 1. The measured problem

J: *"even when I look at the palettes they are all basically similar."* The data agrees, and it is worse than a hue-coverage problem.

Measured over the shipped corpus — 24 Lospec imports + the 15 designed `lab/palettes/P*` + 6 house schemes:

| Finding | Number |
|---|---|
| Schemes covering ≥11 of 12 hue bins | **17 of 30** in `palette.bin` |
| Cross-palette spread of mean saturation | **σ = 0.113** |
| Cross-palette spread of mean brightness | **σ = 0.086** |
| Median nearest-neighbour distance (54-palette corpus) | **0.115** |
| Tightest pair | `resurrect-64` ~ `endesga-32`, **d = 0.064** |
| Runner-up | `cyberpunk-neons` ~ `ink-crimson`, **d = 0.072** |

The centroids are all in the same place. Every palette lands near *satMean 0.5, valMean 0.65, most of the hue wheel* — so they differ on paper and converge on screen.

**The sharpest number.** Classifying all 54 against the taxonomy below, **4 of the 10 classes have zero members**:

| class | best-fit members today | actually pass |
|---|---|---|
| `split_complement` | 19 | 12 |
| `metallic` | 11 | 0 |
| `earth` | 9 | 4 |
| `full_spectrum` | 8 | 4 |
| `analogous` | 4 | 2 |
| `duotone` | 3 | 3 |
| **`mono_accent`** | **0** | 0 |
| **`neon_on_black`** | **0** | 0 |
| **`pastel_wash`** | **0** | 0 |
| **`stark`** | **0** | 0 |

The library occupies six regions of colour space, piles 19 of 54 into one of them, and has never once tried restraint (`mono_accent`, `pastel_wash`, `stark`) or true electric contrast (`neon_on_black`). That is the whole of "stark to amazing colors" — the two ends J asked for are the two ends we do not have.

---

## 2. Three structural rules

These are not stylistic. Each one is a measured defect in the current build, and each applies to every palette regardless of class.

### RULE 1 — Author as a **loop**, not a gradient

`gen_tables.py` expands anchors cyclically (`cols[(k + 1) % M]`), and `draw.s` indexes the ramp with a wrapping index. Position 32767 → 0 is *traversed on screen*. A palette that runs dark → light and stops has a cliff at the wrap.

Measured on the reference gold ramp: `step_ratio` **8.16**, and **1.43** with that single closing edge removed. The entire defect is the wrap.

> Every palette must return to its darkest anchor by a path. Climb one side, descend the other — through a desaturated or hue-shifted return leg. Adding a six-anchor cool-grey descent to `Struck Gold` took it from `step_ratio` 8.16 to passing.

### RULE 2 — Order the anchors before expansion

Author grouped by hue family (readable). Then run `order_ramp()` — a greedy OKLab nearest-neighbour tour with 2-opt — before writing `palette.json`. `gen_tables.py` consumes file order verbatim, so a hue-grouped list writes an ~80-unit cliff straight into the ramp. This is the direct cause of J's *"rough breaks"*.

Measured across the 30 designed palettes: mean `max_step` **71.5 → 34.9**. On the full 54 corpus, post-ordering `step_ratio` median is **2.08**.

### RULE 3 — Smoothness is a **ratio**, not a magnitude

The ramp is 32768 entries over ~20 anchors: an absolute ΔE step of 30 is spread across ~1600 samples and is invisible. What the eye catches is one segment scrolling *faster than its neighbours*. So the governing metric is

```
step_ratio = max_step / mean_step
```

which is scale-free and anchor-count-independent. Budget ≤ 2.4–3.0 for every class; `stark` alone is allowed up to 5.0, because there the cliff *is* the look.

---

## 3. Why OKLab and not HSV

The current generator authors in HSV. HSV lightness is a lie — pure blue `0000ff` and pure yellow `ffff00` are both `V = 1.0`, but their OKLab L is 0.45 and 0.97. Ramps built to look balanced in HSV are wildly unbalanced perceptually, and palettes with very different HSV numbers land in the same perceptual place. Everything below is OKLab / OKLCh.

One consequence worth stating, because it drives four of the ten class definitions:

- **`C_mean`** — mean chroma over *all* anchors. Dragged down by dark anchors, so it really measures *how much of this palette is colour at all*.
- **`C_lit`** — mean chroma over anchors that are lit and chromatic (`L ≥ 0.35`, `C ≥ 0.08`): *how saturated is the colour that is actually there*.

`neon_on_black` and `earth` are indistinguishable on `C_mean` (both ~0.3, one because it is half black, the other because it is muted) and 4× apart on `C_lit`. Conflating the two is a large part of how 30 different specs converged.

---

## 4. Metric definitions

All computed on the **anchor list**, chroma-weighted so near-greys get no vote on hue. `C` is normalised by 0.33 (the sRGB OKLCh chroma ceiling) into 0..1.

| metric | meaning |
|---|---|
| `hue_bins` | 30°-bins holding ≥5% of chroma weight (0–12) |
| `hue_arc` | smallest arc in degrees holding 90% of chroma weight |
| `accent_frac` | chroma weight falling *outside* the best 60° window |
| `pole_sep` | angular gap between the two heaviest hue lobes |
| `C_lit` / `C_mean` / `C_sd` / `C_max` | chroma, as above |
| `L_mean` / `L_sd` / `L_range` | OKLab lightness |
| `dark_frac` / `light_frac` | fraction of anchors with `L < 0.30` / `L > 0.80` |
| `neutral_frac` | fraction with `C < 0.08` |
| `step_ratio` | `max_step / mean_step` over the cyclic anchor tour |

---

## 5. The taxonomy

Ten classes. **Bold** = the *key* metrics that define the class: a miss there fails the palette outright, not merely docks points. Blank = unconstrained. `n` = how many of the 60 the class gets.

| class | n | hue_bins | hue_arc | accent_frac | pole_sep | C_lit | C_mean | C_sd | C_max | L_mean | L_sd | L_range | dark_frac | light_frac | neutral_frac | step_ratio |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `mono_accent` | 7 | **1–3** | · | **0.04–0.22** | · | 0.3–0.75 | · | · | · | · | · | 0.6–1 | · | · | · | 0–2.6 |
| `duotone` | 7 | **2–4** | 110–250 | · | **110–180** | 0.35–0.9 | · | · | · | · | · | 0.6–1 | · | · | · | 0–2.6 |
| `analogous` | 8 | **3–5** | **50–125** | 0–0.32 | · | 0.3–0.85 | · | · | · | · | · | 0.6–1 | · | · | · | 0–2.4 |
| `split_complement` | 6 | **3–6** | **170–290** | 0.2–0.6 | · | 0.4–0.9 | · | · | · | · | · | 0.65–1 | · | · | · | 0–2.8 |
| `neon_on_black` | 6 | 2–5 | · | · | · | 0.6–1 | · | · | **0.8–1** | 0.18–0.48 | · | · | **0.4–0.7** | · | · | 0–3 |
| `pastel_wash` | 5 | · | · | · | · | · | 0.06–0.3 | · | · | **0.72–0.92** | 0–0.14 | 0.1–0.45 | **0–0.08** | · | · | 0–2.4 |
| `metallic` | 5 | · | **0–60** | · | · | 0.2–0.6 | · | · | · | · | **0.22–0.4** | 0.75–1 | · | 0.08–0.4 | · | 0–2.8 |
| `stark` | 6 | 1–3 | · | · | · | · | 0–0.3 | · | · | · | **0.28–0.5** | 0.85–1 | · | · | **0.35–1** | 0–5 |
| `earth` | 6 | · | 40–160 | · | · | **0.12–0.4** | · | 0–0.16 | · | **0.38–0.65** | 0.12–0.28 | · | · | · | · | 0–2.4 |
| `full_spectrum` | 4 | **9–12** | 240–360 | · | · | 0.45–0.95 | · | · | · | · | · | 0.7–1 | · | · | · | 0–2.6 |

Each band carries a tolerance: score 1.0 inside the range, ramping linearly to 0 at `range ± tol`. A palette **passes** when every key metric is 1.0 *and* the mean of all sub-scores ≥ 0.80.

### Class notes and reference palettes

Every reference below is validated by `_refcheck.py` and scores ≥ 0.88.

| class | reference | intent |
|---|---|---|
| `mono_accent` | **Ironworks Teal** — deep teal field, two amber embers | One hue family carries the frame; a single opposing accent under a fifth of it. Restraint. |
| `duotone` | **Cobalt & Rust** | Two hue poles ≥110° apart, each with a full ramp, no third family. A two-ink print. |
| `analogous` | **Fern Shallows** — green → teal → blue | A continuous slice of the wheel, ≤⅓ turn. Warm and cool instances are separate palettes. |
| `split_complement` | **Reliquary** — green vs magenta + violet | A dominant hue against the two neighbours of its opposite, or a true triad. Tension without mud. |
| `neon_on_black` | **Voltage** — magenta/cyan on void | Half the ramp near-black; what is lit is at gamut edge. The dark floor is what makes it read electric. |
| `pastel_wash` | **Sugar Haze** | High L, low C, tight value band. Nothing in it is dark. |
| `metallic` | **Struck Gold** (+ cool-grey return leg) | Narrow hue band on a long steep value ramp. The shine is lightness, not chroma. |
| `stark` | **Newsprint** — black/white/one red slash | Two or three values, no midtones. The only class permitted a high `step_ratio`. |
| `earth` | **Kiln** — clay, moss, bark | The only class deliberately *muted* rather than dark or pale. |
| `full_spectrum` | **Prism Riot** | The whole wheel. Legitimate as a class, illegitimate as half the library. **Capped at 4 of 60.** |

**`pastel_wash` and `stark` are the two poles J asked for.** Their `L_mean` bands (0.72–0.92 vs a bimodal 0.28–0.50 `L_sd`) do not overlap with anything else in the set. Between them the library finally spans "stark to amazing".

---

## 6. Scoring a palette against a class

```python
from palette_score import score, classify, metrics, order_ramp

cols = order_ramp(cols)          # RULE 2, always first
s, passed, detail = score(cols, 'neon_on_black')
# -> 0.91, True, {'dark_frac': (0.50, 1.0, (0.4, 0.7)), 'C_max': (0.98, 1.0, (0.8, 1.0)), ...}

classify(cols)                   # audit an existing palette: best-fit class + fit
```

`score` returns `(score 0..1, passed bool, detail)` where `detail` maps each metric to `(measured, sub_score, target_range)` — so a failing palette tells you exactly which number to move.

---

## 7. Diversity metric

A palette is described by a **signature**: a chroma-weighted 12-bin hue histogram plus a 6-vector of tone `(L_mean, L_sd, C_mean, C_sd, dark_frac, light_frac)`.

```python
distance(a, b) = 0.60 * cyclic_EMD(hue_a, hue_b) / 6
               + 0.40 * ||tone_a - tone_b|| / scale
```

Hue uses **cyclic Earth-Mover distance** — not bin-wise difference — because hue is a circle and "all the red moved 30° toward orange" must read as a small change, not a total mismatch.

The metric is validated by what it flags: its two tightest pairs in the shipped corpus are `resurrect-64 ~ endesga-32` (0.064) and `cyberpunk-neons ~ ink-crimson` (0.072) — exactly the pairs a human would name.

### Floors, and why they are what they are

Both numbers were set by measurement, not taste.

- **`MIN_DIST = 0.16`** — per-pair hard floor. Sits above the shipped corpus p75 (0.147) and ~1.4× its median (0.115). A floor of 0.22 was tried first and rejected as infeasible: greedy packing admitted only **13 of 54** existing palettes.
- **`MIN_HUE_EMD_SAME_CLASS = 1.2 bins`** (36°) — within a class the tone vector is pinned by the class definition, so the tone term is near zero and *all* separation must come from hue. This forces the 7 `mono_accent`s to spread around the wheel instead of being seven teals.

```python
ok, reason, detail = accept(cols, cls, existing)
# False, 'same class as Ironworks Teal and hue profiles overlap
#         (emd=0.81 < 1.2 bins) -- rotate this one to a different hue family'
```

### Corpus-level gate

Per-pair distance stops duplicates; it does not stop the whole set from clustering. `library_report(lib)` additionally requires:

| target | value | shipped v2.0 |
|---|---|---|
| min nearest-neighbour | ≥ 0.16 | 0.064 |
| **median nearest-neighbour** | **≥ 0.20** | **0.115** |
| class counts match quota | exact | — |

### Feasibility was checked before the floors were fixed

Building all 60 mechanically on a class × hue-rotation grid:

| construction | min NN | median NN |
|---|---|---|
| hue rotation only | 0.068 | 0.148 |
| hue rotation **+ tone variation** | 0.096 | **0.189** |
| *shipped v2.0* | *0.064* | *0.115* |

**Hue rotation alone is not diversification.** Symmetric palettes are near rotation-invariant — rotating a split-complement by 180° reproduces it (`split_complement#1 ~ #4`, d = 0.068; two full-spectrums differed by 0.44 bins of hue EMD). Within-class variety must move **tone as well as hue**, inside the class band: vary `dark_frac`, `L_mean` and `C_lit` across a class's members, not just where they sit on the wheel.

This applies hardest to `full_spectrum`, which is rotation-invariant by definition. Its 4 members must be separated *entirely* by tone: one dark-ground, one blazing high-chroma, one muted, one light.

Median NN 0.189 from a crude mechanical transform of a single reference per class is the floor of what hand-designed palettes should achieve; the 0.20 target is reachable.

---

## 8. Build checklist

For each of the 60:

1. Author the anchor list grouped by hue family, **as a loop** (RULE 1) — climb one side, descend the other.
2. `cols = order_ramp(cols)` (RULE 2) before writing `palette.json`.
3. `score(cols, cls)` → must pass; `detail` names any off-target metric.
4. `accept(cols, cls, existing)` → must pass both the distance floor and, within class, the hue-EMD floor.
5. Write `lab/palettes/PNN_slug/{palette.json, spec.md, swatch.png}` in the existing P16–P30 format.

Then once, over the finished set:

6. `library_report(lib)` → `ok: True` (min NN ≥ 0.16, median NN ≥ 0.20, quota exact).

Run `python3 lab/design/_refcheck.py` at any time to confirm all ten classes are still achievable, and `python3 lab/design/palette_score.py` to re-audit the existing library.

---

## 9. Allocation

| | | | | |
|---|---|---|---|---|
| `analogous` 8 | `mono_accent` 7 | `duotone` 7 | `split_complement` 6 | `neon_on_black` 6 |
| `stark` 6 | `earth` 6 | `pastel_wash` 5 | `metallic` 5 | `full_spectrum` 4 |

**Total 60.** Weighted toward the classes that make palettes *differ* — and away from `full_spectrum`, which is 4 of 60 here against 17 of 30 today. That single reallocation is most of the fix.
