# JellyDazzle v2.1 — PATTERN PLUG-IN PERFORMANCE

Profiling and optimisation of the 100 C pattern plug-ins, so the v2.1 layer
compositor has headroom to stack 3–4 routines inside the 13.5 ms budget that
`compositor.md` §7.1 sets.

**Scope.** `patterns_c/pattern_*.c` only. `bridge.c` is owned by another
workstream and was not touched. Nothing in `draw.s`, `gen_tables.py` or the
palette pipeline was changed.

**Rule obeyed throughout: not one pixel moved.** Every optimisation below is a
pure hoist — work that did not depend on the frame counter was lifted out of the
frame loop, or work that did not depend on `y` was lifted out of the row loop.
The arithmetic expressions were copied verbatim into their new home, so the
output is **bit-identical**, not merely similar. That was verified per pattern,
not assumed — see §5.

Measured on Apple M5 (4P+6E, 32 GiB), `clang -O2`, 1280×960, serial.

---

## 0. Method

Bench: `patterns_c/harness.c`, 120 frames from frame 0, best of 3 runs.

```
clang -O2 -DPATTERN=pattern_NNN harness.c pattern_NNN.c -o /tmp/t -lm
/tmp/t bench            # fps
/tmp/t render x.ppm 0 N # frame N-1 of a sequential run from sl=0
/tmp/t delta 300        # mean per-channel frame-to-frame delta
```

One trap worth recording: **this machine throttles measurably over a
100-pattern sweep.** A full serial pass costs ~6 % on the routines benched last.
Every before/after pair in §2 was therefore taken **interleaved** — A, B, A, B,
A, B on the same binary pair in the same minute — so both sides saw the same
clock. The 85 untouched patterns act as a control: their median moved +2.1 %
between the two full sweeps, which is the drift floor.

---

## 1. Baseline: where the time actually went

All 100 patterns, before any change:

| | fps | ms |
|---|---|---|
| min | **133.9** (027) | 7.47 |
| p10 | 276.7 | 3.61 |
| median | 398.7 | 2.51 |
| max | 3950 (092) | 0.25 |

Only **one** routine was under the 150 fps target — 027, at 133.9. So the job
was not "rescue the failures", it was "buy the compositor headroom", and the
15 slowest were taken as the working set.

Four cost structures accounted for essentially all of it:

1. **Frame-invariant transcendentals.** `sqrtf(dx²+dy²)` and `atan2` over a
   *fixed* screen-space centre, recomputed 1.23 M times a frame, every frame,
   for a value that never changes. Nine of the fifteen did this. On ARM64
   `fsqrt`/`fdiv` are not pipelined like the ALU ops around them, so a pair of
   them dominates a loop body of twenty other instructions.
2. **Row-invariant column geometry.** `px = (x+0.5)*isx - 160`, its `fabsf`,
   the cell index it lands in, the LUT index it produces — all pure functions
   of `x`, all recomputed per pixel per row.
3. **Naive bilinear upscales.** Five routines render a 320×240 (or 640×480)
   internal canvas and upscale. At 960 output rows over 240 source rows each
   source row was being horizontally resampled **four to eight times over**.
4. **Table lookups blocking auto-vectorisation.** Two routines had nothing left
   but a pair of genuinely frame-dependent `sqrtf`s, sitting in a loop body whose
   sine/palette gathers stop clang vectorising the whole thing.

---

## 2. Before / after

Interleaved A/B, best of 3 each, identical thermal conditions.

| # | routine | before | after | | ms before | ms after | technique |
|---|---|---:|---:|---|---:|---:|---|
| 020 | Feedback Fractal | 285.3 | **580.0** | ×2.03 | 3.51 | 1.72 | cached blit rows |
| 082 | Greek Key | 182.1 | **560.2** | ×3.08 | 5.49 | 1.79 | geometry cache |
| 084 | Gear Rosettes | 234.5 | **463.4** | ×1.98 | 4.26 | 2.16 | polar cache + packed LUT |
| 015 | Twister Star | 221.5 | **447.6** | ×2.02 | 4.51 | 2.23 | polar cache |
| 004 | Mirror Truchet | 274.5 | **446.0** | ×1.62 | 3.64 | 2.24 | cached upscale rows |
| 071 | Silk Currents | 221.2 | **415.1** | ×1.88 | 4.52 | 2.41 | rewritten upscale |
| 026 | Spoke Moire | 217.5 | **351.3** | ×1.62 | 4.60 | 2.85 | polar cache |
| 083 | Patch Quilt | 234.3 | **342.8** | ×1.46 | 4.27 | 2.92 | column table |
| 090 | Diamond Burst | 259.3 | **301.7** | ×1.16 | 3.86 | 3.31 | column table |
| 088 | Star Cross | 211.5 | **287.2** | ×1.36 | 4.73 | 3.48 | column table + packed cells |
| 093 | Cathedral Fan | 206.0 | **281.6** | ×1.37 | 4.85 | 3.55 | cached blit rows |
| 023 | Silk Gratings | 228.7 | **265.5** | ×1.16 | 4.37 | 3.77 | vectorised distance pass |
| 021 | Ripple Duet | 213.7 | **257.0** | ×1.20 | 4.68 | 3.89 | vectorised distance pass |
| 085 | Vector Machine | 181.8 | **229.3** | ×1.26 | 5.50 | 4.36 | annulus index cache |
| 027 | Wedge Ripples | 130.4 | **207.6** | ×1.59 | 7.67 | 4.82 | polar cache |
| | **total** | | | | **70.5** | **45.5** | **−35 %** |

Whole-library effect, full sweep before vs after:

| | before | after |
|---|---:|---:|
| slowest routine | 133.9 fps | **195.9 fps** |
| routines under 150 fps | 1 | **0** |
| p10 | 276.7 | 317.7 |
| median | 398.7 | 423.6 |

> **Target met.** Nothing in the library is now below 150 fps at 1280×960; the
> floor is 195.9 fps (5.10 ms), 31 % clear of target.

What that buys the compositor. `compositor.md` §7.1 names 027+071+090 as the
**worst legal 3-stack** and has to *reject* it: 14.58 ms of render, 15.7 ms with
blends and cache tax, 116 % of the 13.5 ms budget. On this bench those same
three routines cost 16.05 ms before and **10.54 ms after — a 34 % cut**, which
scales the spec's own figure to **9.6 ms of render, 10.6 ms all-in, 78 % of
budget: admissible.** (The absolute ms differ between the two documents because
the spec measured cold and these runs are interleaved on a warm machine; the
ratio is the transportable number, and the control set puts drift at 2 %.)
Admission control in §7.2 still earns its keep — it now fires on genuinely
pathological stacks instead of on an ordinary one.

---

## 3. What was actually done

### 3.1 Frame-invariant polar maps — 027, 026, 015, 084, 085, 082

The dominant win, and the same shape every time. `r` and the angle depend only
on `(x, y)`; only the *phases* added to them move per frame. So they are built
once per resolution into a `malloc`'d map and read back as a load:

```c
/* was, per pixel, every frame */
float r  = sqrtf(qy + dx * dx);
int   ai = (int)(s_atan2_027(dy, dx) * 651.8986f + 8192.5f);

/* now */
float r  = rr[x];
int   ai = aa[x];
```

Storage is chosen to be the smallest exact form: `r` stays `float` (it is
multiplied by scale factors near 49, so fixed point would move truncation
boundaries), while the angle index fits `uint16_t` exactly — the expression
`atan2()*651.8986 + 8192.5` has range [6144, 10240] by construction. That is
7.3 MB for a 1280×960 map, streamed at ~1.5 GB/s, which is nothing next to the
2.5 M `fsqrt`/`fdiv` it removes.

082 is the extreme case and the ×3.08. Both of its branches called `atan2`, and
*everything* those calls fed — the meander phase `r*3 − a`, the wedge angle
`th*8/τ`, the panel/border classification, the gold-ring test and the border LUT
index — turned out to be frame-invariant. All of it collapsed into one `float`
map plus one `uint16_t` map of packed flags:

```
bit 15 = gold frame ring, bit 14 = inside panel,
low bits = cell index 0..23 (panel) or p82_bval index 0..511 (border)
```

The per-frame loop is now two adds, a `floorf` and two table reads.

084 does the same trick per gear, over the four bounding boxes only, and also
pre-packs its diagonal-stripe ground LUT as ARGB words so the ground pass is one
`uint32` load instead of three `uint8` loads and a shift-or chain.

### 3.2 Column tables — 088, 083, 090

Where the geometry depends on `x` but not `y`, one pass of width `w` replaces
`w×h` of work. 088 was the best of these: it was doing **two `floorf` plus two
float→int→float round-trips per pixel** to find which of two candidate lattice
cells owns it, when both candidates are fixed for a column once the frame's
scroll offset is known. The per-cell scalars were also split across four
separate `[16][16]` arrays; interleaving them into one `[256][4]` makes a cell
a single 16-byte load.

### 3.3 Upscales — 071, 020, 004, 093

Every one of these was resampling each source row once per *output* row that
read it. Caching the horizontal half and stepping it as `y` advances is a 4–8×
cut in that work, and it makes the remaining per-pixel step a two-tap vertical
lerp.

071 got the fullest treatment: its cached rows are stored as `B,G,R,255` so the
vertical lerp is a flat byte loop writing straight into the framebuffer, which
clang vectorises, and the alpha lane falls out as 255 for free. Its `soften1`
also had `%` — a real integer division — twice per pixel for wrap-around
neighbours; those became conditional decrements.

020's blit was calling `lerp2` **three times per output pixel** (two horizontal,
one vertical). With the horizontal pair cached it calls it once. That is the
×2.03.

### 3.4 Splitting for the vectoriser — 021, 023

These two had no frame-invariant work left: their wave sources orbit, so the
distances genuinely change every frame. But the loop body around them — Q14 sine
table reads, a 32 K-entry palette gather — is unvectorisable, and it was holding
the `sqrtf`s hostage in scalar code. Hoisting just the distances into their own
tiled pass over a 512-entry L1-resident scratch lets clang emit `fsqrt.4s`:

```
$ clang -O2 -S pattern_021.c | grep -c 'fsqrt.4s'
10
```

Modest gains (×1.16–1.20) because these routines are now gather-bound on the
palette, not compute-bound. That is the correct place for them to stop.

### 3.5 One thing that was tried and reverted

085's four X-strut terms were rewritten branchless (clamp instead of
`if (m > 0)`). It measured **slower** — 222 → 174 fps. The branches were
skipping three multiply-adds each on the ~90 % of pixels no strut touches, and
that is worth more than the misprediction. Reverted. Hoisting the two shared
products `0.613105f*px` / `1.743315f*px` out of the four terms *was* a real 5 %,
but it also cost the compiler its `fmsub` contraction and changed **4 pixels out
of 1 228 800 by one LSB**. Not worth an asterisk on a bit-exactness claim.
Reverted too.

---

## 4. First-frame cost — checked, and it is a non-issue

The maps in §3.1 are built lazily on a pattern's first call at a given
resolution. That is a one-time cost, but a one-time cost landing inside a frame
is exactly the kind of hitch J has rejected builds for, so it was measured
rather than hand-waved:

| routine | first frame | steady | build overhead | **old** steady |
|---|---:|---:|---:|---:|
| 082 | 5.32 | 1.68 | +3.64 | 5.49 |
| 027 | 6.61 | 4.92 | +1.69 | 7.67 |
| 085 | 5.82 | 4.50 | +1.32 | 5.50 |
| 084 | 3.11 | 1.88 | +1.24 | 4.26 |
| 015 | 3.33 | 2.09 | +1.24 | 4.51 |
| 026 | 3.75 | 2.57 | +1.18 | 4.60 |
| 020 | 2.32 | 1.49 | +0.83 | 3.51 |
| 071 | 2.53 | 2.05 | +0.49 | 4.52 |
| all others | | | ≤ +0.38 | |

Read the last two columns together: **for 14 of the 15, the first frame —
build included — is still cheaper than the routine's old steady-state frame.**
085 is the single exception and it is over by 0.32 ms. So no new hitch is
introduced anywhere; the worst case is one frame that costs what that routine
used to cost every frame. The maps persist for the life of the process and are
rebuilt only on a resolution change.

Allocation failure is handled everywhere and never crashes: each routine falls
back to a per-row (or tiled) scratch that recomputes the same values inline.
The fallback is the original cost, not a wrong image.

---

## 5. Verification

Per pattern, before vs after, three renders each — a single frame, a 138-frame
sequential run and a 400-frame run, so accumulators are exercised from `sl == 0`:

```
n=1    mean 0.0000  max 0  pct>2 0.0000%
n=138  mean 0.0000  max 0  pct>2 0.0000%
n=400  mean 0.0000  max 0  pct>2 0.0000%
```

**All 15 are bit-identical to the originals — mean 0.0000, max 0, on every
frame tested.** Not "visually indistinguishable": zero differing pixels.

Also checked:

- **Motion budget.** `delta 300` is unchanged for all 15 (e.g. 027 6.65 → 6.65,
  026 4.69 → 4.69), all under the limit of 8. No optimisation touched motion.
- **Visual.** A 15-tile contact sheet of frame 400 was rendered and inspected:
  every routine reads as its intended image, no corruption, no seams at the
  tile/block boundaries the new loop nests introduce.
- **Symbol hygiene.** `nm -g` on each object: no file-scope symbol escapes
  except the `pattern_NNN` entry point. All new helpers and tables are `static`.
- **Warnings.** `clang -O2 -Wall -Wextra` clean on all 15.
- **The app builds and runs.** Full `make` links all 150 pattern files. The
  headless whole-engine harness confirms it end to end:

```
$ /tmp/jd_dump run 5000 900
delta    mean 0.723  peak 2.166  frames>8: 0
ms/frame p50 6.85  p90 10.91  p99 16.99
```

---

## 6. Two observations handed to the bridge/compositor workstream

Both are outside this scope (`bridge.c` is not mine to edit), and neither is
caused by anything here.

1. **~600 ms on the very first `jd_frame` call.** Reproducible from any start
   frame, so it is engine-level init (table/palette load), not a routine. The
   heaviest pattern build in the library is 3.6 ms. Harmless as a splash-time
   cost, but if v2.1 ever wants a clean first second it should be moved off the
   first frame.
2. **The upscale fix generalises.** 54 of the pattern files use the
   render-small-and-upscale idiom, and only the four in the working set were
   fixed. The other 50 are all above target already, but the same change is
   worth a routine 1.3–2× on any of them that later ends up in a hot stack —
   it is mechanical: cache the horizontally-resampled source row, step it with
   `y`, and the per-pixel cost drops to one vertical lerp.
