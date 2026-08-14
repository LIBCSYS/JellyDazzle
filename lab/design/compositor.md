# JellyDazzle v2.1 — COMPOSITOR

How multiple pattern layers combine into one frame: buffers, blend modes, alpha
envelopes, accumulator participation, the exact C to add to `bridge.c`, and what
happens when the frame budget runs out.

**Companion spec:** `lab/design/transitions.md` owns *succession* (scene scheduling,
the palette walk, easing curves, accumulator retirement in `draw.s`). This document
owns *stacking* — what happens on a single frame when N layers are co-resident.
§1 reconciles the two where they disagree; the disagreement is load-bearing and was
settled by measurement.

Every number below was measured on this machine (Apple M5, 4P+6E, L1d 64 KiB,
P-cluster L2 16 MiB, E-cluster L2 6 MiB, 32 GiB) at 1280×960, with `clang -O2`.
Nothing here is estimated unless it says so.

---

## 0. Measured baseline

Four facts were established before any design was written, because all four could
have invalidated it.

### 0.1 Every routine writes every pixel, opaque

Filled the framebuffer with `0xDEADBEEF`, ran 60 frames, counted survivors.

| Source set | Pixels left unwritten | Pixels with alpha ≠ 0xFF |
|---|---|---|
| 100 C patterns | **0 / 100 patterns leak** | 0 |
| 24 asm modes | **0 / 24 modes leak** | 0 |

Consequences, all good:

- A layer buffer **never needs clearing** before its tenant renders. Saves 0.030 ms
  per layer per frame and removes an entire class of stale-pixel bug.
- No routine carries meaningful per-pixel alpha. **The compositor owns alpha
  outright** — the A byte is free real estate (§3.6 uses it).
- There is no such thing as a "sparse layer" at the buffer level. Sparseness is a
  property of the *image* (how much of it is near-black), not of buffer coverage.
  That distinction drives the whole blend-mode section.

### 0.2 Which routines read back the framebuffer they are handed

Rendered 80 frames into a clean buffer, then 80 frames into a buffer scrambled with
noise before every single frame, and diffed the results. A routine that ignores prior
framebuffer contents produces identical output; one that feeds back does not.

| Source set | Differing pixels | Verdict |
|---|---|---|
| All 100 C patterns | 0.0000 | **pure** w.r.t. `fb` |
| asm modes 0–14 | 0.0000 | **pure** w.r.t. `fb` |
| asm modes 15–23 | 0.9861 – 0.9999 | **canvas-owning** |

This is the single most useful measurement in the document. The 100 C patterns —
including the accumulator ones like `pattern_037`, which prime a 280-frame history on
entry — keep their accumulation in *private static arrays* and emit a full repaint
every frame. They are pure functions of `(frame, sl, seed, pal)` plus their own state.
**Any C pattern can be rendered into any buffer with no compositing hazard whatsoever.**

The nine asm modes 15–23 are the real accumulators: they stamp a few pixels per frame
onto whatever canvas they are given and rely on it persisting. They are the only
sources that constrain buffer assignment (§5).

### 0.3 Cost of every source

Serial, one process at a time (an 8-way parallel sweep understates by up to 25% through
memory-bandwidth contention — do not use parallel numbers for budgeting).

| Source class | Count | Cost per frame @1280×960 |
|---|---|---|
| C patterns | 100 | min 0.17 · p25 1.48 · **median 2.35** · p75 2.84 · p90 3.47 · max **7.60** ms |
| asm modes 0–14 (repaint) | 15 | 3.40 – **10.24** ms (mode 1 is the worst thing in the engine) |
| asm modes 15–23 (accumulator) | 9 | **0.005 – 0.012 ms** — effectively free |

61 of the 100 C patterns cost more than 2 ms; 5 cost more than 4 ms; the tail is
`pattern_027` at 7.60 ms. The asm repaint modes are *much* heavier than the C patterns
and mostly cannot be stacked at all.

### 0.4 The rough break, quantified

House rule: mean per-channel frame-to-frame delta < 8. Measured at a v2.0 segment
boundary, where `mix32(seg) % 124` swaps the routine between one frame and the next:

| Transition | Peak delta | Mean delta |
|---|---|---|
| **v2.0 hard cut** | **95.53** | — |
| raised-cosine crossfade, 30 frames (0.5 s) | 5.58 | 3.95 |
| 60 frames (1.0 s) | 3.70 | 2.67 |
| **120 frames (2.0 s)** | **3.36** | 1.97 |
| 240 frames (4.0 s) | 3.51 | 1.74 |
| 480 frames (8.0 s) | 3.30 | 1.66 |

The v2.0 cut is **twelve times over budget**. That is J's "rough break" as a number.

Note the floor: peak delta bottoms out around 3.3 and will not go lower, because that
is the intrinsic motion of the two routines, not the transition. **Past 120 frames a
longer fade buys nothing measurable** — it only costs screen time. Use 120 frames as
the floor for any envelope and spend extra duration on *hold*, not on *fade*.

---

## 1. Two operations, not one

`transitions.md` §2.2 specifies a **normalized weighted average** for compositing:
weights are normalized once per frame to sum to 256, so the inner loop is a
multiply-accumulate with no per-pixel divide. That is exactly right — for one of the
two things the compositor does, and exactly wrong for the other.

**Succession** — routine A hands off to routine B in the same visual role. Both images
are trying to be *the picture*. Brightness must be conserved across the handoff or the
screen pulses. Normalized average is correct and nothing else will do.

**Stacking** — J's actual ask: *"use 1, then 3 seconds later, another, then 1 second
another, building layer upon layer."* Here the overlay is not replacing the ground, it
is *adding to* it. Averaging is destructive: it does not add the overlay, it dilutes
the ground by the overlay's weight.

Measured. Ground = `pattern_019` (dense, 0.0% near-black); overlays = `pattern_071`
(81.5% near-black) and `pattern_059` (96.5% near-black); frame 400.

| Composite | Mean luma | **Contrast (σ)** | Saturation |
|---|---|---|---|
| ground alone | 115.9 | **62.3** | 63.8 |
| normalized average, 3× equal weight | 45.6 | **22.6** | 29.1 |
| normalized average, 60/20/20 | 73.8 | **37.9** | 42.2 |
| ground + 2× **MAX** @ 0.50 | 116.7 | **61.5** | 62.1 |
| ground + 2× **SCREEN** @ 0.50 | 122.6 | **60.0** | 63.5 |
| normalized average, 2× *dense* layers | 139.8 | **35.9** | 75.5 |

Averaging a bright ground with two mostly-black overlays throws away **64% of the
frame's contrast** and 54% of its saturation. Even averaging two *dense* layers halves
contrast (62.3 → 35.9), because uncorrelated fields average toward their mean.

Low contrast, low saturation, mid-grey luma, on every frame, regardless of which
routines are playing — that is a machine for producing exactly the complaint J already
has about the palettes, and it would reproduce it at the compositing layer no matter
how good the 60 new palettes are. MAX and SCREEN preserve contrast to within 4%.

**Resolution — the governing rule of this document:**

> Normalized averaging is used **only between layers competing for the same role**
> (the ground crossfading to its successor). Layers stacked *above* the ground are
> composited with a non-averaging mode — MAX, SCREEN, ADD or DIFF — never by
> averaging into the ground.

This keeps both specs coherent. `transitions.md`'s normalized-average kernel becomes
the **ground path**, over which §3's blend ladder runs. Slot 0 in that spec is already
defined as "always `JD_MIX`", which is precisely the ground path; this document only
insists that slots 1..3 must not be.

---

## 2. Per-layer buffers

### 2.1 Layout and memory

One 1280×960 ARGB buffer = 1,228,800 px × 4 B = **4,915,200 B = 4.6875 MiB**.

| Allocation | Count | Bytes |
|---|---|---|
| Layer canvases `g_buf[JD_LAYERS]` | 4 | 18.75 MiB |
| Per-layer blended palettes (32768 × 4 B) | 4 | 512 KiB |
| Ground crossfade partner (see §2.3) | 0 | 0 |
| **New static footprint** | | **≈ 19.25 MiB** |
| (existing) `framebuffer` in `main.c` | 1 | 4.69 MiB |
| (existing) `jd_palette` in `draw.s` .rodata | | 3.75 MiB |

19.25 MiB of BSS on a 32 GiB machine is not a memory question. It is a **cache**
question, and the cache question was measured rather than argued.

### 2.2 Shared scratch vs. per-layer buffers — measured

Two candidate layouts, same three routines (`019` ground + `071` + `059`), same
alpha-lerp blend, 300 frames:

| Layout | Live buffers | ms/frame |
|---|---|---|
| Shared scratch (1 extra buffer, reused per layer) | 2 | **9.77** (repeat: 9.79) |
| Per-layer buffers | 4 | **9.92** (repeat: 10.12) |
| Per-layer + half-rate decimation (§7.3) | 4 | **6.44** |

Four live 4.69 MiB buffers total 18.75 MiB and do not fit the 16 MiB P-cluster L2 —
and it barely matters. The penalty for the extra two buffers is **1.5–3.5%**, because
every access pattern here is a pure sequential stream that the M5's prefetchers and
memory bandwidth absorb. Blend kernels achieve ~36 GB/s effective, nowhere near a wall.

Meanwhile per-layer buffers are what make the 34% half-rate saving possible at all,
and they are mandatory for the canvas-owning asm modes (§5.3).

> **Decision: per-layer persistent buffers.** Cost 0.2 ms and 14 MiB over a shared
> scratch. Buys a 3.4 ms emergency valve, correct accumulator support, and the ability
> to reuse a layer's last frame without re-rendering it.

Allocate once in a `jd_compositor_init()` called on the first frame; never free.
Use `calloc`, not BSS, so the resolution can change later without a recompile — but
size to the actual `w`/`h` handed to `jd_frame` and re-init if they change.

### 2.3 The n == 1 fast path, and its one trap

When exactly one layer is live it should render straight into `fb` and skip the blend
entirely — that is v2.0's cost, which is the point of the fast path.

**This is legal only for pure tenants.** A Class-C tenant (asm 15–23) reads back its
canvas: point it at `fb` and it will accumulate onto whatever `main.c` last presented,
then get clobbered the moment a second layer appears and the blend writes `fb`.

```c
/* fast path admissible iff the sole tenant is pure w.r.t. fb */
int direct = (n == 1) && (jd_src_class(g_layer[idx[0]].routine) != JD_CLASS_ACC);
```

For a Class-C sole tenant, render into its own canvas and `memcpy` to `fb` —
**0.105 ms measured**, cheaper than any blend, and it keeps the canvas intact.

---

## 3. Blend modes

### 3.1 The ladder, with measured cost

All kernels in-place (`dst` is the accumulator), 1280×960, weight `w` in Q8 (0..256),
200 iterations. `dst` is the composite so far; `src` is the incoming layer.

| Mode | Formula (per channel) | ms/frame | Use |
|---|---|---|---|
| `JD_MIX` | `d + (s − d)·w` | **0.393 – 0.450** | ground succession only (§1) |
| `JD_MAX` | `max(d, s·w)` — packed | **0.334** | **the default overlay mode** |
| `JD_MAX` | same, scalar per-channel | 0.473 | (reference; do not ship) |
| `JD_DIFF` | `d + (|d − s| − d)·w` | 0.557 | rare, stark accent |
| `JD_ADD` | `min(255, d + s·w)` | 0.616 | dark grounds only |
| `JD_SCREEN` | `d + s·w − (d·s·w)/255` | 0.692 | soft glow overlay |
| — | `memcpy` (n==1 Class-C path) | 0.105 | |
| — | `memset` clear (never needed, §0.1) | 0.030 | |

Two blends per frame in a 3-stack costs **0.67 – 1.38 ms** depending on modes chosen —
under 8% of the frame. Blending is not the expensive part; rendering is.

### 3.2 Why MAX is the default for kaleidoscope art

A kaleidoscope image is *structure*: hard mirror seams, radial spokes, petal
boundaries, concentric rings. Its legibility lives in the edges, and the measurement
in §1 says averaging is an edge-destroying operation (σ 62.3 → 22.6).

MAX has the property the art form needs: **wherever the overlay is darker than the
ground, the ground survives untouched, bit-exact.** With 24 of the 100 C patterns
sitting above 60% near-black, MAX leaves the majority of the frame showing the ground
at full contrast and lets the overlay's figure punch through only where it is actually
bright. You get two readable images instead of one averaged fog. Measured contrast
preservation: 61.5 vs. the ground's 62.3 — a 1.3% loss.

MAX is also the cheapest mode in the ladder (0.334 ms) when written packed, and it is
**bit-exact against a scalar reference** — verified across all 1,228,800 pixels, 0
mismatches:

```c
/* JD_MAX, packed: two channels per multiply, no per-channel unpack.
   Verified identical to the scalar reference over a full 1280x960 frame. */
static inline uint32_t jd_blend_max(uint32_t d, uint32_t s, uint32_t w)
{
    uint32_t srb = (((s & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;
    uint32_t sg  = (((s & 0x0000FF00u) * w) >> 8) & 0x0000FF00u;
    uint32_t drb =   d & 0x00FF00FFu, dg = d & 0x0000FF00u;
    uint32_t dr  = drb & 0x00FF0000u, sr = srb & 0x00FF0000u;
    uint32_t db  = drb & 0x000000FFu, sb = srb & 0x000000FFu;
    return 0xFF000000u | (dr > sr ? dr : sr)
                       | (dg > sg ? dg : sg)
                       | (db > sb ? db : sb);
}
```

The R and B lanes cannot corrupt each other because `(x & 0x00FF00FF) * w >> 8` keeps
R's overflow above bit 23, which the mask discards, and B's product never exceeds 16
bits. Comparing the masked lanes is therefore a correct per-channel max.

Verified exhaustively against the scalar reference over all 257 weights × 65 536
colour pairs (16.8 M cases): **0 mismatches**, and `w = 0` and `w = 256` both return
the ground unchanged, as MAX requires.

### 3.3 SCREEN, and the cap that keeps it from washing out

SCREEN never darkens and never hard-clips, which makes it the right mode for glow,
sparks, ray fans, and anything that should read as *light* rather than *paint*.

Its failure mode is measurable and depends entirely on the ground. Over the dense
ground (luma 115.9) two screen overlays at w=0.50 lifted luma only to 122.6 with
contrast intact at 60.0. Over a dark ground, the same operation drove mean luma from
39.6 to **138.2** — the frame goes milky.

`transitions.md` §2.4 caps screen weight at 0.60 globally. Make the cap adaptive
instead, using the ground's *known* luma — we have it as a static per-source statistic
(§6.3), so no per-frame image analysis is required:

```c
/* Q8 cap on a SCREEN overlay's weight, from the ground's mean luma 0..255. */
static uint32_t screen_cap_q8(uint32_t ground_luma)
{
    if (ground_luma >= 160) return 154;              /* 0.60 — bright ground, hard cap */
    if (ground_luma <=  40) return  64;              /* 0.25 — dark ground washes fast */
    return 64 + ((ground_luma - 40) * 90) / 120;     /* linear 0.25 → 0.60 */
}
```

ADD is SCREEN's harsher sibling: it clips instead of rolling off, which bands on bright
grounds. **Admit ADD only when the ground's mean luma < 40**, where clipping cannot
occur; above that, use SCREEN.

### 3.4 DIFFERENCE-lite

Full difference (`|d − s|`) is a strobe machine: a bright overlay crossing a bright
ground drives channels from 255 to 0 in a few frames. It is also the only mode in the
ladder that produces genuinely *stark* imagery — inverted seams, dark rims, the
graphic end of J's "stark to amazing" range. Keep it, defanged:

```c
/* d' = lerp(d, |d - s|, w) — the pull toward inversion is weight-limited. */
static inline uint32_t jd_blend_diff(uint32_t d, uint32_t s, uint32_t w)
{
    uint32_t dr=(d>>16)&255, dg=(d>>8)&255, db=d&255;
    uint32_t sr=(s>>16)&255, sg=(s>>8)&255, sb=s&255;
    uint32_t xr=dr>sr?dr-sr:sr-dr, xg=dg>sg?dg-sg:sg-dg, xb=db>sb?db-sb:sb-db;
    uint32_t iw=256-w;
    return 0xFF000000u | (((dr*iw + xr*w) >> 8) << 16)
                       | (((dg*iw + xg*w) >> 8) <<  8)
                       |  ((db*iw + xb*w) >> 8);
}
```

Verified over the same 16.8 M cases: no channel overflow, alpha preserved, and the
endpoints are exact — `w = 0` returns the ground, `w = 256` returns `|d − s|`.

Admission rules, both derived from the delta budget:

- **`w ≤ 90` (0.35).** Above that the inversion dominates and delta climbs fast.
- **Overlay's measured single-layer delta must be < 2.0.** A fast overlay under DIFF
  multiplies its own motion by the ground's gradient. 54 of the 100 C patterns qualify.
- Budget DIFF at **≤ 8% of admitted overlays**. It is seasoning.

`JD_MULT` from `transitions.md`'s enum is deliberately absent. Multiply darkens
monotonically and with 24 overlays that are >60% near-black it would black out most of
the frame — the exact inverse of the contrast failure in §1, and just as bad.

### 3.5 Mode selection, driven by measured coverage

Classify every source once, offline, by its near-black fraction (`dark` = fraction of
pixels with luma < 16, measured at segment frame 400):

| Role | Criterion | C patterns | Can be |
|---|---|---|---|
| `GROUND` | dark < 0.30 | **56** | slot 0 only |
| `EITHER` | 0.30 ≤ dark < 0.60 | **20** | slot 0, or overlay at reduced weight |
| `OVERLAY` | dark ≥ 0.60 | **24** | any slot above 0 |

The 24 overlay-qualified patterns today are:
`031 033 038 039 040 042 052 053 054 055 056 057 058 059 060 071 073 074 075 079 080 081 090 091`
(cost min 1.03, median 2.81, max 3.63 ms — conveniently, the overlays are cheap).

Default mode per overlay, chosen from its own statistics:

```
dark >= 0.85 and sat >= 25   -> JD_SCREEN   (sparse, colourful: glow)
dark >= 0.60                 -> JD_MAX      (default overlay)
0.30 <= dark < 0.60          -> JD_MAX at w <= 0.45, or JD_MIX if the ground is retiring
delta < 2.0, dice 8%         -> JD_DIFF at w <= 0.35 (overrides the above)
ground luma < 40             -> JD_ADD permitted in place of JD_SCREEN
```

### 3.6 The alpha byte

Since no routine writes meaningful alpha (§0.1), byte 3 of each layer buffer is free.
Reserve it now, unused in v2.1, as a **per-pixel coverage mask** so a future routine
can declare "I only occupy this region" and get a genuinely local composite without
another buffer. Blend kernels must keep writing `0xFF000000` into the output so that
`SDL_PIXELFORMAT_ARGB8888` stays valid on present.

---

## 4. Alpha envelopes

### 4.1 Shape

`transitions.md` §2.1/§4 already specifies the Q16 envelope and the smoothstep easing;
this section only constrains the *durations* and adds the stacking-specific cap.

Every layer's weight follows `0 → peak → 0` through a raised-cosine / smoothstep ramp.
There is **no code path that sets a layer's weight discontinuously.** Retirement,
emergency shedding (§7.4), and resolution changes all route through the envelope.

| Phase | Duration | Source |
|---|---|---|
| fade in | **≥ 120 frames (2.0 s)**, default 180 | §0.4 — below 120 the peak delta climbs (3.36 → 3.70 → 5.58) |
| hold | 900–3600 frames | `transitions.md` §1.1 |
| fade out | **≥ 120**, default 240 | a slower exit reads as dissolving, not cutting |

Extra fade duration beyond 120 frames is cosmetic, not corrective — see §0.4. Spend it
if the scene has room; do not spend it to fix a delta problem, because it will not.

### 4.2 Peak weight is not 1.0 for overlays

A stacked overlay that reaches full weight has replaced the ground rather than
enriched it. Peak weight per role:

```
slot 0 (ground)                w_peak = 256    (1.00, always)
overlay, JD_MAX                w_peak = 115..179  (0.45..0.70), dice per layer
overlay, JD_SCREEN             w_peak = min(dice(0.30..0.60), screen_cap_q8(ground_luma))
overlay, JD_ADD                w_peak = 64..115   (0.25..0.45)
overlay, JD_DIFF               w_peak = 51..90    (0.20..0.35)
```

Dicing the peak per layer is cheap variety: the same two routines stacked at 0.45 and
at 0.70 read as two different images.

### 4.3 Ground succession

When the ground is replaced, the incoming ground enters as slot 0's *partner* and the
two are composited with `JD_MIX` under the normalized-average kernel — brightness
conserved, no pulse. When the incoming weight reaches 256 the outgoing tenant is
released. Overlays above are untouched and keep their own envelopes running: the
picture underneath them changes without the stack breaking. This is the mechanism that
lets a scene evolve indefinitely without ever showing a full-frame change.

**Invariant:** at most one ground succession in flight at a time, and no overlay may
enter or leave during one. Otherwise two normalized-average denominators are moving at
once and the brightness conservation argument no longer holds.

---

## 5. Accumulators

Three classes, from the §0.2 measurement. The class is a static property of the source
and belongs in the generated table (§6.3).

### 5.1 Class A — pure repaint (100 C patterns, asm 0–14)

Pure functions of their arguments plus private static state. Render into any buffer,
any slot, any time. No constraints beyond cost.

### 5.2 Class B — private accumulators (a subset of Class A, ~25 C patterns)

Patterns like `pattern_037` keep a private history and key their reset off **`sl`
continuity**: `if (sl < 2 || sl != last_sl + 1) { rebuild history; }`. They are still
pure w.r.t. `fb`, so they are Class A for buffer purposes, but they impose a contract
on the compositor:

- **Feed a layer-local `sl`, not the global segment clock.** `sl` starts at 0 the frame
  the layer is admitted and increments by exactly 1 while the layer lives. A layer that
  enters at global frame 100 003 must see `sl = 0`, or it will not clear.
- **Never re-enter a layer's `sl`.** Pausing and resuming a layer trips the
  discontinuity test and triggers a full history rebuild mid-scene.
- **Entry costs up to 2× a steady frame.** Measured across all 100: worst case
  `pattern_082` at **17.01 ms** on its entry frame (steady 8.16 ms); it is the only
  pattern that exceeds a 16.67 ms frame on entry, and only barely. Mitigation:
  **admit at most one layer per frame**, which the §7 scheduler enforces anyway.

Because two co-resident layers would share one pattern's static state, **no two live
slots may hold the same routine** — already invariant 3 in `transitions.md` §1.2, and
now it has a reason: it is not an aesthetic rule, it is a correctness rule.

### 5.3 Class C — canvas-owning accumulators (asm modes 15–23)

These stamp a handful of pixels per frame onto a persisting canvas. Their properties,
all measured:

- **Cost 0.005 – 0.012 ms.** Running one is free to three decimal places.
- **They read `fb` back** (diff 0.986–0.9999 under the scramble test). Their canvas must
  be private and write-owned; nothing else may touch it.
- **They take ~300 frames (5 s) to reach a plateau.** Mode 15: mean luma 15.8 at frame
  60, 29.5 at frame 300, then flat through frame 2047.
- **They self-clear on their own cadence, near a 2048-frame period.** Measured luma
  collapse between frame 2047 and 2400 on modes 15, 17, 19 and 22. Their lifecycle is
  keyed to the *global* frame counter and cannot be rebased to a layer-local clock.

The scheduling consequences fall straight out:

> **Run Class-C tenants continuously from app start in their own canvas, and modulate
> only their weight.** Never reset them, never reassign their canvas mid-life, never
> hand them a layer-local `sl`.

Since they cost 0.01 ms, keeping two hot permanently costs 0.02 ms of compute and
9.4 MiB — and it makes them the ideal "instant" layer: by the time the scheduler wants
one, its canvas is already a fully-formed image, so its fade-in reveals a finished
picture instead of watching one draw itself.

Two hard admission rules for Class C:

1. **Reserve two canvases** (`JD_ACC_SLOTS = 2`) outside the general layer pool. A
   Class-C layer binds a reserved canvas as its source; the pool buffer for that slot
   goes unused.
2. **A Class-C layer's entire envelope must fit inside one self-clear period.** Admit
   only when `2048 - (frame & 2047) >= t_in + t_hold + t_out + 120`. Fading a Class-C
   layer *across* its self-clear will dissolve its image on screen at full weight —
   a rough break the envelope cannot mask, because the envelope is not the thing that
   changed.

Rule 2 is the one that will get forgotten and then produce an intermittent, hard-to-
reproduce "the picture just vanished" bug. Assert it in the debug build.

---

## 6. The C to add to `bridge.c`

### 6.1 Structures

Extends `transitions.md` §1's `jd_layer` rather than replacing it; new fields marked.

```c
/* ---- compositor.h ---- */
#define JD_LAYERS     4          /* pool slots                                  */
#define JD_ACC_SLOTS  2          /* reserved Class-C canvases, outside the pool */

enum { JD_MIX = 0, JD_MAX, JD_SCREEN, JD_ADD, JD_DIFF, JD_BLEND_N };
enum { JD_CLASS_PURE = 0, JD_CLASS_PRIV, JD_CLASS_ACC };      /* new, §5 */
enum { JD_ROLE_GROUND = 0, JD_ROLE_EITHER, JD_ROLE_OVERLAY }; /* new, §3.5 */

typedef struct {
    int       live;
    int       routine;        /* 0..23 asm mode, 24.. lab pattern            */
    int       t_in, t_full, t_out, t_end;   /* global frame marks            */
    uint32_t  seed;
    uint8_t   blend;          /* JD_MIX for slot 0; §3.5 chooses for slots>0 */
    uint8_t   style;          /* JD_DISSOLVE | JD_BLOOM | JD_COLLAPSE        */
    uint32_t *buf;            /* pool canvas, or a reserved Class-C canvas   */

    /* ---- new in compositor.md ---- */
    uint16_t  w_peak;         /* Q8 0..256, §4.2 — overlays never reach 256  */
    uint16_t  w_now;          /* Q8, this frame's post-cap weight            */
    int32_t   sl;             /* LAYER-LOCAL frame counter, §5.2             */
    uint8_t   cls;            /* JD_CLASS_*                                  */
    uint8_t   half;           /* 1 = render on alternate frames, §7.3        */
    uint8_t   parity;         /* which parity this layer renders on          */
    uint8_t   pad;
    float     cost_ms;        /* EWMA of measured render cost, §7.1          */
} jd_layer;

static jd_layer  g_layer[JD_LAYERS];
static uint32_t *g_buf[JD_LAYERS];              /* 4 x 4.69 MiB              */
static uint32_t *g_acc_buf[JD_ACC_SLOTS];       /* 2 x 4.69 MiB, §5.3        */
static int       g_acc_mode[JD_ACC_SLOTS];      /* asm mode 15..23, or -1    */
static uint32_t *g_pal[JD_LAYERS];              /* 4 x 128 KiB               */
```

### 6.2 Call flow

```c
void jd_frame(uint32_t *fb, int w, int h, int frame)
{
    jd_compositor_init(w, h);              /* first call / resolution change */

    /* 1. Class-C canvases advance every frame regardless of visibility.
     *    Measured 0.005-0.012 ms each; this is what makes them instant. (§5.3) */
    for (int a = 0; a < JD_ACC_SLOTS; a++)
        if (g_acc_mode[a] >= 0) {
            g_mode = (uint32_t)g_acc_mode[a];
            draw_frame(g_acc_buf[a], w, h, frame);
        }

    /* 2. Scheduling. Admits/retires at most one layer per frame; every state
     *    change goes through an envelope, never a direct weight write. (§7.2) */
    jd_sched_tick(frame);

    /* 3. Gather live layers in z-order, evaluate envelopes, apply caps. */
    int idx[JD_LAYERS], n = 0;
    for (int i = 0; i < JD_LAYERS; i++) {
        jd_layer *L = &g_layer[i];
        if (!L->live) continue;
        uint32_t wq16 = layer_weight(L, frame);            /* transitions.md §2.1 */
        uint32_t wq8  = (wq16 * L->w_peak) >> 16;          /* §4.2 peak cap       */
        if (L->blend == JD_SCREEN) {
            uint32_t cap = screen_cap_q8(jd_src_luma(g_layer[idx[0]].routine));
            if (wq8 > cap) wq8 = cap;                      /* §3.3                */
        }
        L->w_now = (uint16_t)wq8;
        if (!wq8) continue;
        idx[n++] = i;
    }
    if (!n) return;                        /* invariant 1 says unreachable  */

    /* 4. Render. Layer-local sl, cost-metered, half-rate honoured. */
    int direct = (n == 1) && (g_layer[idx[0]].cls != JD_CLASS_ACC);   /* §2.3 */
    for (int k = 0; k < n; k++) {
        jd_layer *L = &g_layer[idx[k]];
        if (L->cls == JD_CLASS_ACC) continue;              /* already drawn, step 1 */
        if (L->half && ((frame & 1) != L->parity)) { L->sl++; continue; }  /* §7.3 */
        uint32_t *dst = direct ? fb : L->buf;
        double   t0   = jd_now_ms();
        jd_render(dst, w, h, frame, L->sl, L->seed, g_pal[idx[k]], L->routine);
        L->cost_ms = 0.90f * L->cost_ms + 0.10f * (float)(jd_now_ms() - t0);
        L->sl++;                                           /* §5.2 monotonic +1    */
    }
    if (direct) return;                                    /* v2.0-cost fast path   */

    /* 5. Ground: slot 0, or slot 0 normalized-averaged with its successor. (§4.3) */
    jd_composite_ground(fb, w * h, idx, n);

    /* 6. Overlays: slots 1.. blended over the ground, bottom-up. (§3) */
    for (int k = 1; k < n; k++) {
        jd_layer *L = &g_layer[idx[k]];
        if (L->style == JD_DISSOLVE) jd_blend_span(fb, L->buf, w*h, L->w_now, L->blend);
        else                         jd_blend_bloom(fb, L->buf, w*h, L, frame);
    }
}
```

`jd_render()` is the one-line dispatcher that hides the asm/C split:

```c
static void jd_render(uint32_t *dst, int w, int h, int frame, int sl,
                      uint32_t seed, const uint32_t *pal, int routine)
{
    if (routine < 24) { g_mode = (uint32_t)routine; draw_frame(dst, w, h, frame); }
    else              { jd_patterns[routine - 24](dst, w, h, frame, sl, seed, pal); }
}
```

Note what step 4 does *not* do: it never clears `dst`. §0.1 proved that is safe for
all 124 sources, and the assertion belongs in the debug build, not in the hot loop.

### 6.3 The generated statistics table

Blend selection (§3.5), role assignment, screen capping (§3.3), DIFF admission (§3.4)
and cost-based admission (§7.1) all read the same five numbers per source. Generate
them, do not hand-maintain them:

```c
/* patterns_c/srcstats.c — auto-generated by tools/gen_srcstats.sh */
typedef struct {
    uint16_t cost_us;   /* median frame cost, microseconds  */
    uint8_t  luma;      /* mean luma 0..255                 */
    uint8_t  dark;      /* near-black fraction, /255        */
    uint8_t  sat;       /* mean (max-min) channel spread    */
    uint8_t  delta;     /* single-layer motion, x16         */
    uint8_t  cls, role; /* JD_CLASS_*, JD_ROLE_*            */
} jd_srcstat;
extern const jd_srcstat jd_srcstats[124];
```

`tools/gen_srcstats.sh` runs the same three probes used to write this document — the
sentinel-coverage probe, the fb-readback probe, and a stats+bench render at segment
frame 400 — and emits the table. Wire it into `make` alongside `gen_registry.sh` so
adding a routine cannot leave the compositor scheduling blind. `cost_us` seeds
`L->cost_ms`; the runtime EWMA then tracks the actual machine.

### 6.4 Other `bridge.c` changes

- **Per-layer palettes.** v2.0 rebuilds one 32768-entry blended palette per frame.
  Measured cost: **0.0131 ms**. Four of them cost 0.052 ms — 0.3% of a frame. Per-layer
  palettes are affordable, but `transitions.md` §3.1 is right that layers should
  normally *share* one walk; keep the array so a layer can deliberately diverge for a
  "stark" accent, and skip the rebuild when the scheme pair and phase are unchanged.
- **Delete the `mix32(seg) % total` dispatcher.** Replace with the shuffled bag from
  `transitions.md` §3.3/§3.4. Random-with-replacement is what produced 110-of-124
  coverage and 39 repeats per 20-segment window; a bag makes those numbers 124 and 0
  by construction.
- **Keep `JD_MODE`.** Force a single layer, `n == 1`, direct path — the per-routine
  test workflow must keep working.
- Add `JD_LAYERS=n` and `JD_BLEND=name` env overrides for visual verification.

---

## 7. Performance

### 7.1 The budget

60 Hz vsync gives **16.67 ms**. Reserve **2.0 ms** for `SDL_UpdateTexture` + present +
OS jitter (assumption, not measured — the 4.69 MiB texture upload has a measured
`memcpy` floor of 0.105 ms, and the GPU path is the unmeasured part). That leaves:

> **Compositor budget B = 13.5 ms** of render + blend, with ~1.2 ms of margin.

| Scenario | Render | Blends | Cache tax | Total | % of B |
|---|---|---|---|---|---|
| 1 layer, median C pattern (direct path) | 2.35 | 0 | 0 | **2.35** | 17% |
| 2 layers, median | 4.70 | 0.33 | +2% | **5.13** | 38% |
| 3 layers, median | 7.05 | 0.67 | +3% | **7.95** | 59% |
| 3 layers, p90 each | 10.41 | 0.67 | +3% | **11.4** | 84% |
| 3 layers, **worst legal stack** (027+071+090) | 14.58 | 0.67 | +3% | **15.7** | **116% — rejected** |
| 3 layers, cheapest legal (094+040+038) | 2.23 | 0.67 | +3% | **2.99** | 22% |
| 4 layers, median | 9.40 | 1.00 | +4% | **10.8** | 80% |
| Ground = asm mode 1 (10.24 ms) + 2 overlays | 16.0 | 0.67 | +3% | **17.2** | **rejected** |

Two conclusions. The median case is comfortable — a 3-stack of typical routines uses
under 60% of budget. And the worst case is **not** survivable, so admission control is
mandatory rather than defensive.

Empirical check on the model: the measured 3-layer composite (019+071+059, alpha
blends) came in at **9.77–10.12 ms** against a predicted 9.39 render + 0.79 blend +
cache = 10.2. The model is accurate to ~4%.

### 7.2 Admission control

Before admitting a candidate routine into slot `k`:

```c
static int jd_affordable(int routine, int k)
{
    float sum = 0.0f;
    for (int i = 0; i < JD_LAYERS; i++)
        if (g_layer[i].live && i != k) sum += g_layer[i].cost_ms;
    sum += jd_srcstats[routine].cost_us * 0.001f;
    sum += 0.35f * (float)(jd_live_count() + 1);      /* blend allowance      */
    sum *= 1.04f;                                     /* measured cache tax   */
    return sum <= 13.5f;
}
```

When the bag's next entry is unaffordable, **do not skip it** — that reintroduces
selection bias and hands the cheap routines more screen time, which is the repetition
problem wearing a different hat. Instead: scan forward in the bag for the first
affordable entry and **swap it into the current position**, leaving the expensive one
in place for a later, cheaper scene. Every routine still appears exactly once per bag.

### 7.3 Fallback ladder

Triggered by an EWMA of measured frame time. Hysteresis prevents oscillation:
engage at **> 14.5 ms sustained 20 frames**, disengage at **< 11.0 ms sustained 120
frames**.

**Rung 1 — half-rate decimation (−34%, measured).** Render the topmost overlay on
alternate frames, reusing its buffer between. Measured 9.77 → **6.44 ms**.

The obvious worry is judder, and it does not materialize *if the parities are
staggered* so that at most one layer skips on any given frame:

| Mode | Per-frame deltas | Mean | Peak |
|---|---|---|---|
| full rate | 1.05 1.11 1.09 1.07 1.13 0.95 1.29 1.10 … | 1.10 | 1.29 |
| **half rate, staggered** | 1.05 0.93 1.08 0.91 1.09 0.84 1.26 0.93 … | **1.01** | **1.26** |

A 12% ripple at a mean delta of 1.0, against a budget of 8. Imperceptible. Note the
mean actually *drops* — this rung costs nothing visually and should be reached for
early. Do **not** put two layers on the same parity; that halves the composite update
rate and will be visible.

**Rung 2 — accelerate the oldest retirement.** Scale the oldest layer's `t_out` window
by 0.5, floored at 120 frames (§0.4 — below 120 the peak delta starts climbing). Frees
a full layer's cost within 2 s. Still an envelope, still not a cut.

**Rung 3 — refuse the scene.** Cap `JD_LAYERS` effectively at 2 until the EWMA
recovers. The scheduler keeps running; it just stops admitting.

**Rung 4 — floor.** Never fewer than one layer, and the last layer never gets
decimated. A single layer on the direct path is exactly v2.0's cost, which by
definition ran at 60 fps.

There is no rung that drops a frame, changes resolution, or cuts a layer. All four
degrade *composition density*, which is invisible, rather than *motion*, which is not.

### 7.4 What is deliberately not done

- **No threading.** Layers are trivially parallel (independent buffers, no shared
  writes) and 4 P-cores are sitting there. But the ladder above already fits the budget
  with margin, and thread wake-up jitter at a 16.67 ms cadence is a good way to
  introduce exactly the hitching J has already rejected builds for. Revisit only if
  1920×1440 becomes a target — at which point the per-layer buffer design already
  supports it unchanged.
- **No low-res compositing.** Many patterns already render internally at 320×240 and
  upscale; compositing at reduced resolution would double-soften them.
- **No per-pixel divides.** Weight normalization happens once per frame (`transitions.md`
  §2.2), and every kernel here is shift-based.

---

## 8. Verification

Each check maps to a measurement already performed in writing this spec, so the
tooling exists.

1. **Coverage** — sentinel-fill every layer buffer with `0xDEADBEEF`, assert 0 survivors
   after any routine renders. Catches a new pattern that forgets a region. Debug build,
   every frame.
2. **Purity** — the scramble test from §0.2, run by `tools/gen_srcstats.sh` over every
   routine. A new routine that reads `fb` gets classified `JD_CLASS_ACC` automatically
   instead of silently corrupting a shared buffer.
3. **Motion budget** — whole-engine frame dump over every scheduled transition window
   `[t_in − 30, t_end + 30]`, assert mean per-channel delta < 8 on every frame pair.
   The hard-cut regression this catches measures 95.53.
4. **Contrast floor** — assert composite σ ≥ 0.75 × the ground's own σ over a 600-frame
   run. This is the §1 failure mode as a test: it fires at 22.6 vs 62.3 and passes at
   61.5. Without this test, "the compositor made everything grey" is a subjective
   argument; with it, it is a build failure.
5. **Weight invariants** — `sum(w) > 0`; at most one `live` change per 120 frames; no
   duplicate routine across live slots (§5.2 — correctness, not taste); slot 0 blend is
   always `JD_MIX`; no weight ever changes by more than `1/120` of its range in a frame.
6. **Class-C phase** — assert `t_end < (frame | 2047) + 1` for every Class-C layer at
   admission (§5.3 rule 2).
7. **Budget** — log p99 frame time over a 10-minute run; assert < 16.0 ms and that the
   fallback ladder engaged at rung ≤ 1.

---

## 9. What this spec asks of the other v2.1 workstreams

**Routines (the "double the routines" work).** Today 56 of 100 C patterns are grounds
and only **24 are overlay-qualified** (near-black ≥ 60%). A 3-deep stack consumes one
ground and two overlays, so the current library is inverted: it has a 2.3× surplus of
exactly the wrong thing. **Bias the 100 new routines to ≥ 60% overlay-qualified** —
sparse figures on black, designed to be seen *through*. Additional asks, all cheap to
honour at design time and expensive to retrofit:

- Keep new overlays under **3 ms**. The existing overlays already oblige (median
  2.81 ms, max 3.63 ms); the expensive tail of the library is all grounds.
- Keep single-layer delta under **2.0** for anything intended for DIFF (§3.4).
- Stay pure w.r.t. `fb`. All 100 current patterns are; it costs nothing to keep.
- If a pattern accumulates, key the reset off **`sl` discontinuity**, exactly as
  `pattern_037` does. The compositor guarantees a monotonic layer-local `sl`.

**Palettes.** §1's contrast measurement is the compositing analogue of the palette
problem: averaging is what makes distinct things look alike. Two implications. First,
the 60 new palettes will only read as distinct if the compositor stops averaging them
together, which §1 fixes. Second, the `stark` end of the new taxonomy (mono + accent)
is where DIFF and MAX overlays pay off most, because a restrained ground gives an
overlay somewhere to be bright.

**Transitions.** One reconciliation and one addition. §1 splits `transitions.md`'s
single normalized-average kernel into a ground path (unchanged, correct) and an
overlay path (new, non-averaging). And §5.3 adds an admission constraint the
scheduler must honour: a Class-C layer's whole envelope has to fit inside one
self-clear period.

---

## Appendix — measurement reference

| Quantity | Value | How |
|---|---|---|
| Buffer, 1280×960 ARGB | 4.6875 MiB | — |
| memcpy / memset a buffer | 0.105 / 0.030 ms | 200 iterations |
| `JD_MIX` blend | 0.393 – 0.450 ms | in-place, Q8 weight |
| `JD_MAX` packed / scalar | **0.334** / 0.473 ms | bit-exact, 0 mismatches over 1.23 Mpx |
| `JD_DIFF` / `JD_ADD` / `JD_SCREEN` | 0.557 / 0.616 / 0.692 ms | |
| Palette blend, 32768 entries | 0.0131 ms | |
| C pattern cost | median 2.35, p90 3.47, max 7.60 ms | serial, all 100 |
| asm repaint modes 0–14 | 3.40 – 10.24 ms | |
| asm accumulator modes 15–23 | 0.005 – 0.012 ms | |
| Layer entry spike | ≤ 2× steady; worst 17.01 ms (`pattern_082`) | |
| Routines reading back `fb` | asm 15–23 only (0 of 100 C patterns) | scramble test |
| Routines leaving pixels unwritten | none of 124 | sentinel test |
| Hard cut delta | **95.53** | v2.0 segment boundary |
| Crossfade peak delta @ 30/60/120/240/480 f | 5.58 / 3.70 / 3.36 / 3.51 / 3.30 | |
| 3-layer: shared scratch / per-layer / half-rate | 9.77 / 9.92 / **6.44** ms | |
| Half-rate staggered delta | mean 1.01, peak 1.26 (vs 1.10 / 1.29) | |
| Contrast σ: ground / norm-avg / MAX / SCREEN | 62.3 / **22.6** / 61.5 / 60.0 | |
| Role split, 100 C patterns | 56 ground · 20 either · **24 overlay** | dark fraction |
| Worst / cheapest legal 3-stack | 14.58 / 2.23 ms render | |
