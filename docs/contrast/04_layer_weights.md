# 04 — Layer weights: are they right, and what should they be?

JellyDazzle 3.0.4 · `src/engine/compositor.c` · analysis against `docs/contrast/spawn_telemetry.csv`
(1,962 spawns / 6 runs / 199.6 min), the probe cache `~/Library/Application Support/JellyDazzle/probe-dec4f6e0.bin`
(634 routines, all probed), and per-pixel composites simulated over the 634 catalogue renders in `catalog/img/`.

---

## Verdict, up front

- **The weights are wrong, but not uniformly wrong.** Mid is roughly right. Accent and spark are set below the level at which they can affect the picture *at all* under the blend they actually get.
- **35% is not "subtle" — under `B_MAX` it is an invisibility ceiling.** A layer at weight `w` can only alter ground pixels whose channel value is below `w`. Ground routines measure median luma **105**. A spark at `w=90` cannot mark a median ground *anywhere*, at any pixel, ever. That is arithmetic, not taste.
- **The hierarchy is doubled up.** Amplitude currently descends in the same order as coverage (ground 100% area → spark 6% area). The two axes multiply, so the last layer gets 6% of the frame at 35% strength. Coverage is already enforced elsewhere (`st->dark >= 70` admission + role sorting); `w_peak` is a *second* sparsity control that pays for it in amplitude, which is the wrong currency.
- **Two bugs are eating a third of the intended effect** and must be fixed alongside the constants, or the new numbers get clawed back: the `B_SCREEN` gate (`st->sat >= 25`) fires for **1 of 145 spark routines** because `sat` is a whole-frame mean over a 94%-black image; and the screen weight cap is keyed on the ground's *static probe luma*, capping accents at 112 over a median ground.
- **The envelope is largely exonerated.** 74% of a tenancy sits at ≥90% of peak. But 12–16% of it sits below `FREEZE_W` (38/256), which is precisely the "fog" band — a wash with no mark. That part is fixable with the floor trick already used by the boot ramp.
- **`audio_surge()` cannot produce a flash.** It is driven by bass *level*, never reads `g_audio.beat`, and at full scale moves a spark's legible coverage from 1.0% to 1.8% of the frame. `B_ADD` — the one blend with headroom above the source — exists in the enum, in `blend_span`, and is **selected zero times in 1,962 spawns**. That is where the flash should live.

---

## 1. The compositing maths: why 35% is absence, not subtlety

### 1.1 `B_MAX` — the dominant blend — has a hard visibility ceiling

`span_max` computes, per channel:

```
out = max(d, (s * w) >> 8)
```

with ground channel `d`, source channel `s`, weight `w` in Q8. Write ω = w/256. The overlay changes the pixel **iff**

```
s·ω > d          i.e.     s > d/ω
```

Since `s ≤ 255`, this requires `d < 255ω`. Define the **mark ceiling** `M = 255ω`: *the ground channel value above which the layer is invisible, full stop, at every pixel, for its whole life.*

| slot | median `w_peak` | ω | mark ceiling `M` | ground channels it can never touch |
|---|---|---|---|---|
| GROUND | 256 | 1.000 | 255 | — |
| mid | 150 | 0.586 | 149 | above p75 of grounds (131) — just clears |
| accent | 111 | 0.434 | 111 | **above the ground median (105)** |
| spark | 90 | 0.352 | 90 | **above the ground's 30th percentile** |

Ground-routine luma from the probe cache (n=224, all probed): median **105**, p25 82, p75 131, deciles `22 64 74 86 95 105 116 127 136 149`.

And the maximum contrast the layer can *add* where it does win is

```
Δmax = 255ω − d
```

For the accent over a median ground: `Δmax = 111 − 105 = 6`. **Six levels out of 255.** That is J's "barely visible", to the digit.

### 1.2 It is worse than the peak-pixel case

A routine's *average lit pixel* is far below 255. From the probe (`luma`, `dark`):

| role | mean luma | `dark` (near-black frac) | implied mean **lit** pixel | mean sat |
|---|---|---|---|---|
| GROUND | 106 | 0.00 | 106 | 82 |
| FIELD (mid) | 65 | 0.376 | 104 | 21 |
| FIGURE (accent) | 22 | 0.729 | 81 | 17 |
| SPARK | 5 | 0.937 | 84 | 3 |

An *average* spark pixel is luma ≈84. To beat an *average* ground pixel (105) under MAX it needs ω > 1.25 — **impossible at any legal weight.** Only the spark's brightest pixels can ever mark anything, and at ω=0.35 they write 90, which loses to two thirds of the ground.

> The spark's visible set under MAX is `{its brightest pixels} ∩ {the ground's darkest pixels}`. At `w=90` that intersection is nearly empty. What remains is a faint lift of the blacks — which is the literal definition of fog.

### 1.3 Raising the spark weight cannot produce clutter

Measured over 250 random (ground, overlay) pairs per cell, real catalogue renders, `B_MAX`, fraction of frame changed by ≥20 luma levels:

| `w` | mid | accent | spark |
|---|---|---|---|
| 72 | 1.3% | 0.7% | 0.2% |
| 90 | 2.0% | 1.2% | **1.0%** |
| 111 | 2.5% | **1.4%** | 1.2% |
| 150 | **6.0%** | 3.5% | 1.8% |
| 190 | 9.0% | 5.4% | 3.0% |
| 230 | 13.4% | 5.8% | 3.7% |
| 256 | 16.5% | 6.6% | **4.7%** |

**Even at the absolute maximum weight of 256, a spark marks 4.7% of the frame.** The ceiling on a spark's footprint is its own 94% blackness, not its weight. There is no setting of `PEAK_LO[3]`/`PEAK_HI[3]` that makes sparks busy. The weight is doing nothing except attenuating the 6% that matters.

Mid is the opposite: 16.5% at full weight is genuinely a lot of frame. **Mid is the only slot where restraint is load-bearing.**

### 1.4 Fog versus mark

Same simulation, decomposed into what happens to the blacks (p5 of composite luma, minus ground) and the highlights (p99):

| blend | `w` | mid fog / mark | accent fog / mark | spark fog / mark |
|---|---|---|---|---|
| MAX | 90 | 1.9 / 0.9 | 0.4 / 0.8 | 0.4 / 1.7 |
| MAX | 111 | 2.2 / 1.6 | 0.3 / 2.5 | 0.5 / 1.7 |
| MAX | 190 | 2.6 / 13.6 | 2.7 / 10.6 | 1.5 / 10.5 |
| MAX | 256 | 4.5 / 31.9 | 0.7 / 28.8 | 0.6 / 27.2 |
| SCREEN | 90 | 4.9 / 13.4 | 2.7 / 10.5 | 1.5 / 8.3 |
| SCREEN | 190 | 7.3 / 37.2 | 4.6 / 33.3 | 2.8 / 27.6 |

The mark/fog ratio for the spark goes **4.3 at w=90 → 7.0 at w=190 → 45 at w=256**. Fog is essentially flat while the mark grows superlinearly.

> Raising a sparse layer's weight does not add fog. It converts existing fog into a mark. The current low weights are not buying subtlety — they are buying the haze and throwing away the highlight.

---

## 2. What the telemetry actually contains

`life` in the CSV is `hold` — the flat top only, not the tenancy. Total tenancy = `fin + hold + fout`.

| slot | n | peak min/med/max | blend mix (MAX / SCREEN / DIFF / ADD) | med hold |
|---|---|---|---|---|
| 0 GROUND | 6 | 256 / 256 / 256 | MIX 100% | 903 |
| 1 mid | 360 | 64 / **150** / 180 | 82.5% / 10.3% / 7.2% / **0%** | 854 |
| 2 accent | 362 | 64 / **111** / 140 | 60.8% / 31.5% / 7.7% / **0%** | 708 |
| 3 spark | 352 | 64 / **90** / 115 | 63.9% / 31.3% / 4.8% / **0%** | 554 |
| 4 SHADOW | 882 | 256 | MIX 100% | 844 |

Peaks by blend, with the fraction pushed *below* `PEAK_LO` by the caps:

| slot | blend | n | median peak | % below `PEAK_LO` |
|---|---|---|---|---|
| mid | MAX | 297 | 154 | 0% |
| mid | SCREEN | 37 | 102 | **81.1%** |
| mid | DIFF | 26 | 90 | **100%** |
| accent | MAX | 220 | 120 | 0% |
| accent | SCREEN | 114 | 102 | **40.4%** |
| accent | DIFF | 28 | 90 | **100%** |
| spark | MAX | 225 | 93 | 0% |
| spark | SCREEN | 110 | 88 | 4.5% |
| spark | DIFF | 17 | 90 | 0% |

**16% of mids and 20% of accents never reach their own floor**, because `pick_blend`'s two caps override the draw.

Duty cycle, from the same data (6 runs × 33 min, span 718,391 frames):

- **E[overlays live] = 1.70** — the picture is a ground plus, on average, 1.7 overlays.
- Slot duty: mid 0.674, accent 0.587, spark 0.437.

---

## 3. Two bugs that will eat the fix if left alone

### 3.1 The SCREEN gate cannot fire for the routines it was written for

```c
if (st->dark >= 215 && st->sat >= 25) mode = B_SCREEN;   /* line 1623 */
```

The comment says *"a routine measured as mostly dark BUT coloured"*. But `sat` is computed in `stat_image` as a **whole-frame mean of `max-min`**, including every black pixel:

```c
lum += l; sat += mx - mn; if (l < 16) dark++;   /* line 330 */
...
s->sat  = (uint8_t)(sat / cnt);                 /* line 335 */
```

For a 94%-black image that mean is crushed toward zero. Measured over the probe cache:

| role | median `sat` | passes `dark≥215 && sat≥25` |
|---|---|---|
| FIELD | 21 | **0 / 155** |
| FIGURE | 17 | **0 / 110** |
| SPARK | 3 | **1 / 145** |

**The metric cannot express "coloured" for a mostly-dark image.** The only reason 31% of accents and sparks get SCREEN at all is the *next* line — the `M_BLAZE` fallback (`dark >= 180`), which fires for all 145 sparks and 65 of 110 figures, and blaze is one mood in three. Telemetry confirms it: slot 3 is BLAZE 35.8% of spawns and SCREEN 31.3%.

Fix — measure saturation over the **lit** pixels:

```c
/* SATURATION MUST BE MEASURED WHERE THERE IS SOMETHING TO SATURATE.
 * A spark routine is 94% near-black, so a whole-frame mean of (max-min)
 * reports ~3 however vivid its lit pixels are, and the SCREEN gate below
 * ("mostly dark BUT coloured") could never fire for the exact material it
 * was written for: 1 of 145 spark routines passed it.  Average over the
 * pixels that are actually lit instead. */
uint64_t lum = 0, sat = 0; uint32_t dark = 0; int cnt = 0, lit = 0;
for (int i = 0; i < n; i += 3) {
    /* ... r,g,bl,l,mx,mn as before ... */
    lum += l;
    if (l < 16) dark++; else { sat += mx - mn; lit++; }
    cnt++;
}
s->sat = (uint8_t)(lit ? sat / lit : 0);
```

Expected: FIGURE `sat` ≈ 63, SPARK ≈ 48 — both comfortably over the 25 threshold, so the gate starts working as documented.

> **Trap:** `probe_stamp()` hashes `sizeof(jd_stat)`, not the semantics of its fields. Redefining `sat` in place does **not** invalidate the cache, so every existing install keeps the old numbers forever and the change appears to do nothing. Bump `JD_CACHE_MAGIC` in the same commit.

### 3.2 The SCREEN cap is keyed on a stale, static number

```c
uint32_t gl = g_L[0].live ? g_st[g_L[0].routine].luma : 100;   /* line 1627 */
uint32_t cap = gl >= 160 ? 154u : gl <= 40 ? 64u : 64u + (gl - 40) * 90u / 120u;
```

Two problems:

- It reads the **probe-time** luma of the ground routine, not `g_gluma` — the live 0.5 s EWMA of the ground buffer that already exists two hundred lines away and that the DIM guard relies on *precisely because* a ground's on-screen luma walks with its palette leg.
- It caps by ground brightness alone, ignoring the source's sparsity. Milkiness under SCREEN is proportional to the source's *lit coverage*: a 94%-black spark cannot go milky, because `span_screen` adds nothing where `s == 0`. Capping a spark at 112 over a median ground is protecting against a failure mode the material makes impossible.

For a median ground (luma 105) the cap is **112** — which is below every proposed accent and spark peak, and would silently nullify the fix on 31% of spawns.

```c
/* Cap by the ground's LIVE luma (g_gluma), and let a sparse source out of
 * the cap: span_screen adds nothing where the source is black, so the milky
 * failure this cap exists to prevent scales with the source's lit coverage.
 * A 94%-black spark cannot go milky; a 38%-black field can. */
uint32_t gl  = g_L[0].live ? g_gluma : 100u;
uint32_t cap = gl >= 160 ? 154u : gl <= 40 ? 64u : 64u + (gl - 40) * 90u / 120u;
if (st->dark > 128)                                   /* release the sparse */
    cap += ((256u - cap) * (st->dark - 128u)) / 127u;
if (p > cap) p = cap;
```

Over a median ground: field (`dark` 96) stays at 112; figure (186) rises to 178; spark (239) rises to 238.

### 3.3 The DIFF cap becomes a no-op floor

`if (p > 90) p = 90;` pins 100% of mid and accent DIFF spawns to exactly 90. Under the new table that is below every `PEAK_LO`, so DIFF would become "always 90" for every slot. Raise to **128** — still the stark end without the strobe, and no longer a constant.

---

## 4. The envelope: assessed, and mostly acquitted

`layer_w_q16()` is a smootherstep in, a flat top, a smootherstep out. `ease_ss` is factored and rounded, monotonic, correct.

Inverting smootherstep: `ss(x)=0.5` at x=0.500, `ss(x)=0.8` at **0.673**, `ss(x)=0.9` at **0.753**. And because smootherstep is symmetric about ½, **its mean over a ramp is exactly 0.5 — one ramp frame is worth half a hold frame in integrated weight.**

Measured over the real hold distribution with the real jitter model (`fin/fout × (154 + U[0,255])/256`, plus the 1-in-16 `fin *= 2`):

| slot | median `fin` | median hold | median `fout` | total tenancy | ramp % of wall clock | % of life at ≥90% peak | % at ≥50% peak | mean weight / peak |
|---|---|---|---|---|---|---|---|---|
| mid | 198 f (3.3 s) | 854 f (14.2 s) | 231 f (3.9 s) | **21.4 s** | 33.7% | **74.6%** | 83.2% | **0.832** |
| accent | 165 f (2.8 s) | 708 f (11.8 s) | 198 f (3.3 s) | **17.9 s** | 33.6% | **74.7%** | 83.2% | **0.832** |
| spark | 132 f (2.2 s) | 554 f (9.2 s) | 165 f (2.8 s) | **14.2 s** | 35.5% | **73.2%** | 82.2% | **0.822** |

**The hypothesis "a layer spends most of its life ramping" is false.** A third of the *wall clock* is ramp, but only ~17% of the *integrated weight*, and three quarters of every tenancy sits at ≥90% of peak. The envelope is not why accents are invisible; the peak is.

### What the envelope *does* contribute

The tail below `FREEZE_W` (38/256) is the fog band — a weight too low to mark anything under MAX, high enough to lift the blacks and (under SCREEN) veil the whole frame:

| slot | % of life below w=38 | seconds of fog per tenancy |
|---|---|---|
| mid | 12.3% | 2.77 s |
| accent | 13.5% | 2.63 s |
| spark | 15.8% | 2.35 s |

- **E[overlays sitting in the sub-38 band] = 0.232.** Roughly **23% of the running time, at least one overlay is contributing nothing but haze.**
- `FREEZE_W` already exists but only skips the *render* of a *falling* layer (`if (falling && L->w_now < FREEZE_W …) continue;`). The layer is still in `ov[]` and still **blended**, on the way in and on the way out.

Fix — a **floor on the envelope**, exactly the trick `JD_BOOT` already uses (`bw = 48 + ((e * 208) >> 16)`, i.e. the boot ramp starts at 19% rather than 0):

```c
/* ENTRY FLOOR.  Smootherstep from zero spends its first and last ~15% of a
 * tenancy under FREEZE_W — a weight too low to mark anything under MAX but
 * high enough to lift the blacks, which is the "fog" in the report.  Start
 * the ramp at 18% of peak instead, so a layer APPEARS rather than seeps.
 * The step this creates is 18% of a peak applied to a source that is 73-94%
 * near-black: at most ~40 levels on under 6% of the frame, invisible.
 *
 * OVERLAYS ONLY.  The ground handover normalises a/(a+b), so a ground
 * entering at 18% would open the crossfade at 15% of the WHOLE FRAME — a
 * cut, which is the one thing this engine will not do. */
#define JD_WFLOOR 11796u                       /* 0.18 in Q16 */

static uint32_t layer_w_q16(const jd_layer *L, int f, int overlay)
{
    if (f <= L->t_in || f >= L->t_end) return 0;
    uint32_t e;
    if (f >= L->t_full && f <= L->t_out) e = 65536;
    else if (f < L->t_full)
        e = ease_ss((uint32_t)(((int64_t)(f - L->t_in) << 16) / (L->t_full - L->t_in)));
    else
        e = ease_ss((uint32_t)(((int64_t)(L->t_end - f) << 16) / (L->t_end - L->t_out)));
    if (overlay && e < 65536)
        e = JD_WFLOOR + (((65536u - JD_WFLOOR) * e) >> 16);
    return (e * L->w_peak) >> 8;
}
```

Time below `w=38` goes to zero for every proposed peak; integrated weight rises from 0.83 to 0.86 of peak. Do **not** shorten `FADE_IN`/`FADE_OUT` — `asm_room()` budgets on them and the asm modes must fit their whole envelope inside one `draw.s` period.

---

## 5. Should `w_peak` be dynamic? Yes — but as a per-frame multiplier, not a mutated peak

`w_peak` is fixed at spawn and read by two different consumers with two different meanings:

- `pick_blend` / the render path: **how bright is this layer** (visual).
- `admissible()` line 1573–1578: **how much motion do we charge it** (budget).

Those must not move together. So the dynamic term goes on `w_now`, per frame, and `w_peak` stays the static contract.

### The control signal already exists and cannot feed back

`g_gluma` is a ~0.5 s EWMA of the **ground buffer's** luma (`g_L[0].buf`), computed *before any overlay is composited*. Overlay weight therefore cannot influence it — **there is no loop.** That is the property that makes this safe; a control derived from `fb` would oscillate.

Both MAX and SCREEN want the *same* direction:

- MAX: mark ceiling is `255ω` — a brighter ground needs more weight to be marked at all.
- SCREEN: lift is `s·ω·(1 − d/255)` — a brighter ground suppresses the lift, so it needs more weight; a dark ground gets the full lift and needs less.

(The existing SCREEN cap is already directionally right. It is just applied to one blend, once, at spawn, off a stale number.)

```c
/* ---- SCENE -> WEIGHT.  Under MAX an overlay can only mark ground pixels
 * darker than its own weight; under SCREEN its lift is scaled by
 * (1 - ground/255).  Both blends need MORE weight over a bright ground and
 * less over a dark one, so a single scalar serves both.
 *
 * Driven from g_gluma, which is measured on the GROUND BUFFER before any
 * overlay is composited — so this control cannot feed back on itself.  That
 * is the whole reason it is safe to close the loop at all.
 *
 * Slower than the audio surge in both directions (~0.5 s attack, ~1 s
 * release): this is a property of the SCENE, not of the beat, and the beat
 * has to ride on top of it without the two multiplying into a pump. */
static uint32_t g_head = 256;                       /* Q8 */
static void scene_headroom(void)
{
    /* g_gluma is not updated during a handover (the ground is a lerp of two
     * buffers then) and is pinned to 60 for a new ground's first 60 frames.
     * Acting on a stale reading is how a slow control becomes a fast one, so
     * freeze instead. */
    if (g_L[JD_SHADOW].live || !g_L[0].live || g_L[0].sl <= 60) return;

    uint32_t k = 200u + ((g_gluma * 3u) >> 2);      /* 0.78x @ black ground
                                                     * 1.00x @ luma 75
                                                     * 1.09x @ luma 105 (median)
                                                     * 1.37x @ luma 200      */
    if (k > 350u) k = 350u;
    if (k > g_head) g_head += (k - g_head + 31u) >> 5;    /* ~0.5 s */
    else            g_head -= (g_head - k + 63u) >> 6;    /* ~1.0 s */
}
```

Anti-pumping checklist, matched to the rest of the engine:

| discipline | where else it is used | here |
|---|---|---|
| asymmetric one-pole, attack ≠ release | `audio_surge`, `g_gain`, `delta_q8` learn | `>>5` / `>>6` |
| the input is already an EWMA | `g_gluma` is `>>5` per frame (~0.5 s) | inherited |
| hard clamp on the output | `bump > 272`, `JD_LUMA_MAXGAIN` | `k ≤ 350`, plus `W_CAP` below |
| freeze rather than act on a stale signal | DIM guard skips during handover | same predicate |
| measured pre-composite so there is no loop | — | `g_L[0].buf`, not `fb` |

### The combined ceiling needs a per-slot clamp

`g_head` (≤1.37) × `g_surge` (≤1.60) = 2.19. Applied to a mid at 164 that saturates at 256, where a mid covers 16.5% of the frame at ≥20 levels — the "different mess". Clamp per slot, not at 256:

```c
/* The modulated ceiling is per slot, because saturating is only harmless for
 * a layer that is mostly black.  A spark at 256 marks 4.7% of the frame; a
 * mid at 256 marks 16.5% and starts replacing the ground rather than
 * enriching it, which is the failure this whole table exists to prevent. */
static const uint16_t W_CAP[JD_NSLOT] = { 256, 200, 236, 256 };
```

Hook, replacing the surge block in the weight loop (`~line 2441`):

```c
if (s != 0 && s != JD_SHADOW && L->w_now) {
    uint32_t m = ((uint32_t)g_head * g_surge) >> 8;   /* scene x sustained bass */
    m += g_flash[s];                                  /* onset, additive (below) */
    uint32_t v = ((uint32_t)L->w_now * m) >> 8;
    if (v > W_CAP[s]) v = W_CAP[s];
    L->w_now = (uint16_t)v;
}
```

---

## 6. Audio: `audio_surge()` cannot flash, by construction

```c
k = 115 + ((uint32_t)g_audio.bass * 295 >> 10);       /* 0.449 .. 1.602 */
if (k > g_surge) g_surge += (k - g_surge) >> 2;       /* attack ~58 ms  */
else             g_surge -= (g_surge - k) >> 4;       /* release ~250 ms to 63%, ~780 ms to 95% */
```

The **shape is already right** — 58 ms attack, ~800 ms release is a textbook flash envelope. Three things stop it landing:

1. **It is driven by a level, not an onset.** `g_audio.bass` is band energy. A loud bassline holds `g_surge` at 1.5 for the whole passage — that is a level shift, not a flash. `g_audio.beat` (documented in `jellydazzle.h` as *"1024 on an onset, decays over ~0.2 s"*) is the transient, and **nothing in the weight path reads it.** It is read by `audio_rotate` (colour), by the brightness bloom, and by the spawn puller at line 2150 — never by weight.
2. **It is one global scalar applied identically to every overlay.** Three layers pumping in lockstep is a frame-wide brightness pulse, which is exactly what the seizure-safety note was right to fear. "Nuanced flashes" means *one* layer, briefly.
3. **The ceiling makes it moot.** A spark at `w_peak = 90` on the loudest possible drop reaches 90 × 1.60 = 144. Measured, that takes its legible coverage from **1.0% to 1.8% of the frame** — a change of 0.8 percentage points. That is the honest answer to "is it strong enough": no, and not by a little.

### The envelope a real flash needs

```c
/* ---- AUDIO -> FLASH.  audio_surge follows the bass LEVEL, so a loud
 * passage holds the overlays up for its whole duration.  A flash is a
 * TRANSIENT, and the transient is already measured: g_audio.beat is 1024 on
 * the onset and decays over ~0.2 s.  Nothing in the weight path reads it.
 *
 * Attack is instantaneous BY DESIGN.  The peak of a flash is one frame;
 * easing the attack is what turns every attempt at this into a swell — which
 * is what audio_surge already is.  The RELEASE is what keeps it from being a
 * strobe, and it is PER SLOT: the spark snaps back in ~0.3 s, the mid takes
 * ~1 s.  Three different release constants are the difference between
 * "nuanced flashes" and the whole frame blinking, because the three overlays
 * then never fall together.
 *
 * Bounded so the broad layer barely moves and the sparse one carries the
 * hit.  Frame-mean luma excursion on the biggest possible onset is under
 * 3/255 (~1.2%), because a spark is 94% near-black — well inside the 1.06x
 * the brightness bloom is already allowed. */
static uint16_t g_flash[JD_NSLOT];
static const uint16_t FLASH_MAX[JD_NSLOT] = { 0, 38, 102, 140 };  /* +0.15 / +0.40 / +0.55 */
static const uint8_t  FLASH_REL[JD_NSLOT] = { 0,  6,   5,   4 };  /* >>n per frame */

static void audio_flash(void)
{
    uint32_t b = g_audio.live ? g_audio.beat : 0u;                /* 0..1024 */
    for (int s = 1; s < JD_NSLOT; s++) {
        uint32_t hit = (b * FLASH_MAX[s]) >> 10;
        if (hit > g_flash[s]) g_flash[s] = (uint16_t)hit;          /* instant attack */
        else g_flash[s] -= (uint16_t)((g_flash[s] + (1u << FLASH_REL[s]) - 1u)
                                       >> FLASH_REL[s]);           /* eased release */
    }
}
```

Release times: `>>4` → 63% in 0.27 s, gone in ~0.5 s (spark). `>>6` → 63% in 1.07 s (mid, reads as a swell). Call it next to `audio_surge()` at line 2419.

### Where the flash gets headroom: `B_ADD`, which the engine has never once used

`B_ADD` is in the enum (line 122), implemented in `span_add`, wired into `blend_span` — and `pick_blend` **can never select it**. Telemetry confirms: `blend=3` appears 0 times in 1,962 spawns. It is dead code, and it is precisely the tool a flash needs, because it is the only blend that can push a pixel *above* what MAX allows.

Measured, spark slot, real catalogue material:

| blend | `w` | %px ΔL≥20 | mark (Δp99) | fog (Δp5) | Δ frame mean | %px clipped |
|---|---|---|---|---|---|---|
| MAX | 90 | 1.0% | 0.8 | 0.7 | +0.60 | 0.08% |
| MAX | 256 | 5.0% | 27.8 | 3.7 | +5.55 | 1.73% |
| SCREEN | 216 | 11.9% | 34.6 | 2.8 | +7.38 | 0.05% |
| **ADD** | **90** | **7.2%** | **21.7** | **1.9** | **+4.58** | **0.30%** |
| **ADD** | **150** | **11.3%** | **35.3** | 3.9 | +7.55 | 1.64% |

**ADD at w=90 out-marks MAX at w=256.** So the flash is not a weight problem at all past a point — it is a blend problem.

```c
/* THE FLASH COMPOSITES ADD, ON TOP OF THE LAYER'S OWN BLEND.
 *
 * MAX saturates: no weight can push a pixel above the source's own value,
 * so a spark at 256 is as loud as a spark can get and that is only 4.7% of
 * the frame.  ADD is the one blend with headroom above the source — and it
 * is in the enum, in blend_span, and selected ZERO times in 1,962 spawns,
 * because pick_blend has no branch that reaches it.
 *
 * Two passes, so the RESTING picture is bit-for-bit what it is today: the
 * layer lays down under its own blend exactly as before, and the onset is
 * added over it at the flash weight.  When g_flash is zero the second pass
 * does not run.  Gated on measured sparsity, not on slot: ADD over a
 * non-sparse source is a white-out.
 *
 * Cost: one extra span pass, on at most one or two layers, only on the
 * frames after an onset. */
uint32_t fw = (sidx < JD_NSLOT) ? g_flash[sidx] : 0u;
blend_span(fb, srcp, npix, L->w_now, L->blend);
if (fw && g_st[L->routine].dark >= 200 && L->blend != B_DIFF)
    blend_span(fb, srcp, npix, ((uint32_t)L->w_now * fw) >> 8, B_ADD);
```

`dark >= 200` covers all 145 spark routines and ~40 of 110 figures; it excludes every field, which is what protects the frame.

---

## 7. Concrete replacement numbers

```c
/* PEAK WEIGHT PER SLOT, Q8.
 *
 * The old table descended in the same order as COVERAGE (ground 100% of the
 * frame, mid ~62%, accent ~27%, spark ~6%), so the two axes multiplied and
 * the last layer arrived at 6% of the frame at 35% strength.  Under B_MAX —
 * 61-83% of overlay spawns — a layer at weight w can only alter ground
 * pixels DARKER than w, and ground routines measure median luma 105: an
 * accent at 111 could add at most SIX levels over a median ground, and a
 * spark at 90 could not touch one at all.
 *
 * Coverage is already controlled, twice: by role sorting and by the
 * admission rule st->dark >= 70.  Sparks measure 94% near-black.  So the
 * amplitude is free to run the other way — broad layers quiet, narrow
 * layers loud — which is the ordinary figure/ground rule and is what the
 * old table inverted.
 *
 * Measured: at the MAXIMUM legal weight of 256 a spark still marks only
 * 4.7% of the frame at >=20 luma levels, because its own material is 94%
 * black.  There is no setting of the spark row that produces clutter.  The
 * mid row is the only one where restraint is load-bearing (16.5% at 256),
 * and it comes DOWN. */
static const uint16_t PEAK_LO[JD_NSLOT] = { 256, 112, 144, 184 };
static const uint16_t PEAK_HI[JD_NSLOT] = { 256, 164, 200, 248 };

/* The motion budget must NOT follow the visual peak.  admissible() charges
 * a candidate delta_q8 * PEAK_HI * amp, and amp reaches 1.85x on the spark
 * slot; carrying the new peaks into that test raises spark refusals from
 * 6.2% to 8.9% for no perceptual reason, because perceived churn scales
 * with the AREA that changes and a spark changes 6% of it.  Charge the old
 * numbers here until the coverage-weighted charge (see below) is measured. */
static const uint16_t MOT_W[JD_NSLOT]   = { 256, 180, 140, 115 };
```

Then in `admissible()`, line 1573, `PEAK_HI[slot_i]` → `MOT_W[slot_i]`.

### Side by side

| slot | now LO–HI | proposed LO–HI | median ω now → proposed | mark ceiling `M` now → proposed |
|---|---|---|---|---|
| GROUND | 256–256 | 256–256 | 1.00 | 255 |
| mid | 128–180 | **112–164** | 0.586 → 0.539 | 149 → 137 |
| accent | 100–140 | **144–200** | 0.434 → 0.672 | 111 → **171** |
| spark | 72–115 | **184–248** | 0.352 → 0.844 | 90 → **215** |

Rationale per row:

- **mid −9%.** Mid is the only slot with the coverage to make clutter (16.5% of the frame at full weight). It is also the layer that currently supplies most of the wash. Pulling it down buys back ~1.2 points of total marked area and re-establishes the hierarchy from the top.
- **accent +43%.** `M` rises from 111 (the ground *median*) to 171 (roughly the ground's p95). An accent's lit pixels can now beat the ground almost everywhere they land, which is what "accent" means.
- **spark +115%.** `M` rises to 215. A spark's brightest pixels finally clear a bright ground. Its footprint is unchanged — bounded at 4.7% by its own material.

### What the whole stack does

Ground + all three overlays at their peaks, real blend and role mixes, 400 random stacks, marginal contribution of each layer as it is composited:

| configuration | mid ΔL≥20 | accent ΔL≥20 | spark ΔL≥20 | composite mean luma | Δ(p95−p5) vs ground alone |
|---|---|---|---|---|---|
| **current** | 10.0% | 3.0% | **1.7%** | 94.2 | **+5.0** |
| new peaks only | 7.6% | 4.1% | 4.4% | 94.6 | +6.4 |
| **new peaks + §3 cap fixes** | 8.8% | 6.7% | **5.1%** | 97.3 | **+16.9** |

- The hierarchy goes from **6 : 1.8 : 1** to **1.7 : 1.3 : 1**.
- **Frame mean luma moves +3.1 out of 255 — 1.2%.** The picture does not get brighter.
- **Contrast (p95−p5) added by the stack goes from +5.0 to +16.9 — 3.4x.** That is the direct answer to "foggy".
- Total frame carrying a legible overlay mark goes 14.7% → 20.6%. **This is the honest cost:** more of the frame is marked. The bet is that 20.6% of legible marks over a clean ground reads better than 14.7% of faint ones plus a wash — which is what "foggy clutter" describes. The mid reduction is the compensator; if it still reads busy, `PEAK_HI[1]` is the first knob, not the spark row.

---

## 8. What I expect to break

| # | What | Why | Mitigation |
|---|---|---|---|
| 1 | **Motion-budget refusals rise** | `admissible()` charges `delta_q8 × PEAK_HI × amp_q8`; `amp_q8` reaches 473 (1.85x) on the spark slot. Simulated over the full stack: mid 5.4→5.9%, accent 5.5→7.2%, **spark 6.2→8.9%**. Effect is a longer mean gap before a slot fills, not a loss — a refusal costs a place at the front of the bag, not in it. | `MOT_W[]` above. Longer term the charge should be `delta × PEAK × (255 − dark)/255`, since churn is area × amplitude; that would *loosen* the budget for sparks by ~16x and needs its own measurement pass before shipping. |
| 2 | **`p > 90` DIFF cap becomes a constant floor** | It already pins 100% of mid and accent DIFF spawns to exactly 90; under the new table it is below every `PEAK_LO`. | Raise to 128. |
| 3 | **SCREEN cap nullifies the change on 31% of spawns** | Caps at 112 over a median ground. | §3.2 rewrite. Non-optional. |
| 4 | **Probe cache silently keeps stale `sat`** | `probe_stamp()` hashes `sizeof(jd_stat)`, not field semantics. A redefinition in place changes nothing on existing installs. | Bump `JD_CACHE_MAGIC` in the same commit. Costs one cold start — accept it. |
| 5 | **Strobe/jitter handlers under-react** | `JUMP` cuts an overlay to `w_peak * 5/8` (line 2524), `TRIM` to `7/8` (line 2772). A strobing spark at 248 lands at 155 — still louder than anything shipping today. | `5/8 → 1/2` for `JUMP`, and add a floor-relative form: cut toward `PEAK_LO[sidx]` rather than by a ratio. |
| 6 | **Dead-air guard fires less** | Composite mean luma rises 3 levels; `JD_LUMA_FLOOR` is 30. | Desirable. But it means the guard stops masking genuinely dark grounds — watch the `DIM`/`DARKF` trace counts for a regression that was previously being papered over. |
| 7 | **Palette amplification stacks with the new peaks** | `amp_q8` stretches the spark slot's narrow window by up to 1.85x, so a spark's lit pixels are already contrast-stretched before the weight is applied. Higher weight × stretched palette can clip. | Measured clipping at spark `w=256` MAX is 1.73% of pixels; with the ADD flash it reaches 2.09%. Acceptable for a glint. Re-check on `M_STARK`, where `SPAN[0][3] = 2600` gives the hardest stretch. |
| 8 | **Catalogue and any golden-frame comparisons shift** | Every stack composite changes. | Regenerate `docs/contrast/` reference frames after the change, not before. |
| 9 | **Simulation caveat** | The `catalog/img` renders use one fixed scheme pair and palette window per routine; the engine varies span per mood and slot and applies `amp_q8`. Per-pixel *distributions* are representative, an individual tenancy is not. The amp stretch makes lit pixels brighter, so these numbers understate the effect — the estimates are conservative in the direction of the argument. | Confirm against a live capture before finalising the mid row. |

---

## 9. Order of operations

1. **`PEAK_LO`/`PEAK_HI` + `MOT_W` + DIFF cap 128.** Self-contained, reversible, four lines. Ship and look at it.
2. **SCREEN cap rewrite (§3.2).** Without it, step 1 is ~two thirds of a change.
3. **Envelope floor (§4), overlays only.** Kills the 23%-of-runtime haze band.
4. **`scene_headroom()` + `W_CAP` (§5).** The dynamic term. Independently testable by pinning `g_head = 256`.
5. **`audio_flash()` + the ADD pass (§6).** The largest visual change and the one most likely to need its constants tuned by eye. Last, so it can be judged against a picture that is already right.
6. **`sat` over lit pixels (§3.1) + `JD_CACHE_MAGIC` bump.** Deliberately last: it re-sorts the blend choice for ~250 routines and would confound the measurement of steps 1–5 if landed early.
