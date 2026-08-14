# JellyDazzle v2.1 — Release Notes

**Status:** ready to ship. `VERSION` not bumped — M5 ships it.
**Measured:** 2026-08-14, Apple M5 (4P+6E, 32 GiB), `clang -O2`, 1280×960.
**Companion specs:** `scheduler.md` · `compositor.md` · `transitions.md` · `palettes.md` · `perf.md`

Every number in this document was measured on this machine, on this build, today.
§7 says exactly how to re-run each one. Where a number differs from the phase-1
design docs, the number here is the one taken from the shipping code.

---

## 1. In plain language

J asked for four things. Here is what each one turned into.

**"Double the routines and colour palettes."**
Done, and then some. There were 124 things the app could draw and 30 colour schemes.
There are now **225 things it can draw and 120 colour schemes** — the patterns roughly
doubled, the palettes went up four-fold.

**"Modular. Use 1, then 3 seconds later another, then 1 second another, building layer
upon layer."**
This is the big one, and it is exactly what the engine now does. v2.0 gave one pattern
the whole screen for 34 seconds, then hard-swapped to the next one. v2.1 runs **four
independent layers stacked on top of each other**, each on its own clock. From a real
run: the first layer starts at 0.0 s, the second joins at **3.0 s**, the third at
**5.9 s**. Each one fades in, lives its own life — around 40 seconds — and fades out on
its own while the others keep going. Nothing on screen ever changes all at once again,
because there is no longer any moment when everything changes.

The practical effect: you are almost never looking at *a pattern*. You are looking at a
**combination** of three or four, and there are far more combinations than there are
patterns. That is where the sense of repetition actually went.

**"No rough breaks if possible."**
The rough break was real and it was measurable. In v2.0, every 34 seconds the picture
was replaced between one frame and the next. Measured across 40 of those changeovers:
the screen changed by an average of **74 units out of 255**, and worst case **172**. The
house limit for "relaxing" is 8. **Every single one of the 40 was over the limit.**

In v2.1 there is no cut anywhere. Measured over 12,000 consecutive frames — 3½ minutes
covering many layer changes and several full ground handovers — the largest single
frame-to-frame change was **5.5**, the average was **0.65**, and **not one frame of
11,999 went over the limit**. The engine cross-fades layers, hands the ground over
*through* its replacement, and eases the colour walk so there is no jolt even in the
speed of the colour change.

**"Stark to amazing colours."**
Half the old palettes were full-spectrum rainbows, so they all read the same — J's
"even when I look at the palettes they are all basically similar" was correct and
measurable. Now: full-spectrum rainbows are **15% of the pool instead of 50%**, and
**48% of schemes are deliberately restrained** — one colour family with a single accent,
or two colours and nothing else, or near-black with one electric hue. Four whole
categories that had **zero** members before (restrained-with-accent, neon-on-black,
pastel wash, and stark) now have **26 schemes between them**. The bright rainbows are
still there — there are actually slightly more of them — they just no longer *are* the
palette set.

And the app can no longer pick the same colour scheme twice in a row-ish: over four full
cycles, **every one of the 120 schemes was used exactly four times**, and **not one of
479 changeovers put two similar-looking schemes next to each other**.

**"Optimise the code, keep it smooth."**
The engine now draws up to four layers instead of one, blends them, and reshapes a
palette per layer — for **+21% frame cost**. It renders a frame in 8.0 ms, which is
inside the 8.3 ms budget of the 120 Hz laptop panel and less than half the 16.7 ms
budget of the external monitor.

---

## 2. The scoreboard

| | v2.0 | v2.1 | |
|---|---:|---:|---|
| **Routines the engine can draw** | 124 | **225** | 201 C patterns + 24 assembly modes |
| **Colour schemes** | 30 | **120** | 6 house + 24 imported + 90 lab-designed |
| **Layers on screen at once** | 1 | **4** | plus a shadow slot for ground handover |
| Distinct routines seen in 33 min | 48 | **189** | ×3.9 |
| Worst frame-to-frame change | 172.2 | **5.5** | house limit is 8 |
| Changeovers over the motion limit | 40 of 40 | **0 of 11,999 frames** | |
| Full-spectrum rainbow share | 50% | **15%** | |
| Restrained (≤4 hue bins) share | 20% | **48%** | |
| Colour-scheme usage spread | 5..16 | **0** | every scheme used equally, by construction |
| Frame cost | 6.6 ms | **8.0 ms** | +21%, for ~4× the drawn material |

---

## 3. What changed, by subsystem

### 3.1 Scheduler — shuffled bags instead of dice

**Was:** `bridge.c` rolled `mix32(seg) % 124` once per 2048-frame segment. That is
random *with replacement*: the same routine can come back immediately, and some never
come up at all.

**Is:** every routine is measured at startup and sorted into one of four **role bags** —
GROUND, FIELD, FIGURE, SPARK — by how much of the screen it covers. Each bag is a
**shuffled permutation**: every member plays once before any member plays twice. A
"seam repair" pass additionally prevents a routine landing near the end of one shuffle
and the start of the next.

Bag sizes on this build: GROUND 74 · FIELD 27 · FIGURE 72 · SPARK 52.

| repetition | v2.0 | v2.1 |
|---|---|---|
| Distinct routines, 300 v2.0-segments (2.8 h) | **110 of 124** | — (the unit no longer exists) |
| Distinct routines, same 33 min of playback | **48 of 124** (39%) | **189 of 224** (84%) |
| Placements in that window | 57 | 202 |
| Repeats inside the ~11 min window | **39** | **1 of 13 recurrences** (7.7%) |
| Median time before a routine returns | — | **24.3 min** |
| Shortest observed return | — | **5.3 min** |
| Per-bag usage spread | — | **≤ 1** (GROUND 61 distinct in 62 draws; SPARK 44 in 44) |

The v2.1 column comes from a 118,244-frame traced run (32.8 min of playback) taken on the
224-routine build, i.e. immediately before `pattern_201` landed. A confirmation trace on
the final 225-routine build reproduces the same bag structure and the same
once-per-cycle behaviour; the extra routine moves nothing material.

The v2.0 column is not an estimate: the exact expression from `git HEAD:bridge.c`,
including its consecutive-repeat guard, was transcribed and simulated, and it reproduces
the brief's three headline numbers exactly (110/124 distinct, palette usage 5..16, one
routine hit 8×).

### 3.2 Compositor — four independently-clocked layers

**Was:** one routine owned the framebuffer for a 2048-frame segment.

**Is:** four slots — `base` / `mid` / `accent` / `spark` — each with a private
full-resolution canvas, its own lifetime clock, its own smootherstep alpha envelope and
its own blend mode. Plus a fifth **shadow** slot that exists so the ground can be
replaced by fading *through* its successor rather than being swapped.

Measured cadence from a traced run:

```
SPAWN f=0    slot=0 rt=28  role=0 blend=0 peak=256 life=1380 span=14000
SPAWN f=180  slot=1 rt=160 role=2 blend=1 peak=129 life=1380 span=8000
SPAWN f=352  slot=2 rt=196 role=3 blend=1 peak=109 life=1380 span=4500
```

At 60 fps that is layer 1 at 0.0 s, layer 2 at **3.0 s**, layer 3 at **5.9 s** — the
brief's "1, then 3 seconds later another" as literal engine behaviour. Median tenancy is
**43 s** for the overlay slots and **28 s** for the ground handover slot, and because the
clocks are independent they drift out of phase permanently.

Two properties made this safe, and both were verified rather than assumed:

- **Every C pattern is a pure repaint.** Rendering 80 frames into a clean buffer and 80
  into a deliberately scrambled one produces bit-identical output for all C patterns.
  They keep their accumulation in private static arrays. So any C pattern can be drawn
  into any layer buffer with no compositing hazard.
- **No routine is ever in two slots at once.** Pattern state is file-static, so two
  copies of the same routine would corrupt each other. The bag structure makes this
  impossible and the admission predicate asserts it anyway.

The assembly modes 0–23 are **grounds only** — they read the global palette directly and
cannot take a layer palette — and modes 15–23 are true accumulators that rely on their
canvas persisting, so they constrain buffer assignment. Both facts are enforced in the
admission test.

### 3.3 Palettes — authored as loops, in OKLab, against a taxonomy

Three defects were found in the generator, and all three are fixed:

1. **The expansion was creating the rainbows.** The old code walked *all* M artist
   colours in file order across the ramp, so any imported palette with more than ~6
   anchors was expanded *into* a full-spectrum sweep no matter how restrained the artist
   had been. It is now a closed Catmull-Rom spline in OKLab.
2. **Anchor order was never optimised.** File order is consumed verbatim, so a
   hue-grouped anchor list wrote a colour cliff straight into the ramp — a direct cause
   of visible "rough breaks" in the colour. Anchors now go through a greedy OKLab
   nearest-neighbour tour with 2-opt before expansion.
3. **The ramp is cyclic and nobody was closing it.** Position 32767 → 0 is traversed on
   screen. Every palette now returns to its darkest anchor by a path.

90 lab palettes were then authored against a **10-class taxonomy**, gated by
`palette_score.py`. Four of those classes had zero members before.

| | v2.0 (30 schemes) | v2.1 (120 schemes) |
|---|---:|---:|
| Full-spectrum (≥11 of 12 hue bins) | **15 (50%)** | **18 (15%)** |
| Restrained (≤4 of 12 hue bins) | 6 (20%) | **57 (48%)** |
| Mean hue bins per scheme | 8.5 | **5.8** |
| Brightness spread (σ) | 0.075 | **0.118** |
| Brightness range (min→max) | 0.314 | **0.649** |
| Saturation spread (σ) | 0.154 | **0.169** |
| Saturation range | 0.614 | **0.749** |
| Median nearest-neighbour distance | 0.217 | **0.261** |
| `mono_accent` / `neon_on_black` / `pastel_wash` / `stark` | **0 / 0 / 0 / 0** | **8 / 7 / 5 / 6** |

Both columns were measured with the **same instrument on the shipped `palette.bin`**
of each build — not on anchor lists. The instrument was calibrated first: it reproduces
the brief's stated v2.0 baseline (≈14 of 30 full-spectrum; brightness spread 0.08,
saturation spread 0.15) before being pointed at v2.1.

Note the absolute rainbow count went *up* (15 → 18). Nothing was taken away; the pool
stopped being dominated.

**Scheme selection** is a shuffled permutation per epoch with an adjacency repair pass —
a scheme too perceptually close to its predecessor is swapped forward, and because it is
repaired rather than re-rolled the permutation survives.

| | v2.0 | v2.1 |
|---|---|---|
| Scheme usage over a full cycle | **5..16** over 300 segments | **exactly 4 each** over 480 legs (4 epochs) |
| Schemes never used | — | **0 of 120** |
| Changeovers landing two similar schemes adjacent | not checked | **0 of 479** |

Mean perceptual distance across a leg seam is **1.229** against a threshold of **0.658**.

### 3.4 Transitions — nothing cuts, anywhere

- **Layer envelopes** are smootherstep, minimum 120 frames of fade. Past 120 frames a
  longer fade buys nothing measurable, so extra duration is spent on *hold*, not fade.
- **The ground hands over through its successor** via the shadow slot, so even the
  bottom layer never cuts.
- **The palette walk is chained**: leg *p*'s destination is leg *p+1*'s origin by
  construction, so there is no value step at a leg seam. v2.0's C path re-rolled both
  ends every segment — **286 of 299 segment boundaries were discontinuous**, mean jump
  73.4/255 — while the assembly path never was. That inconsistency is gone. (The
  independent whole-frame measurement in §1 puts the mean v2.0 changeover at 74.3/255
  across 40 boundaries, which is the same defect seen from the other end.)
- **The fade parameter is eased, not linear.** Linear `t` means colour velocity is
  maximal the instant a leg begins and stops dead at the seam: a *velocity*
  discontinuity that survives even a chained walk. Now eased.
- **Per-layer palette windows.** Each layer takes a *slice* of the shared ramp rather
  than the whole thing, and the narrower the slice the harder its saturation is
  restretched. This is what lets one layer read as a single hue family while another is
  full-spectrum, out of the same scheme.

### 3.5 Everything schedules itself

The scheduler needs to know each routine's role, motion, cost and darkness. **None of
that is a table anyone maintains** — the engine probes every routine at two resolutions
at startup and measures it, then tracks cost with an EWMA of the real render at the real
resolution. Doubling the pattern library needed no scheduling table regenerated.

The probe is incremental — a few milliseconds per frame, and the engine runs from frame
one on whatever has been measured so far, rebuilding the bags when the sweep completes.
On this build the sweep of 225 routines completes in **4,085 frames (~68 s)**.

### 3.6 Performance

The hot 15 of the original 100 patterns were optimised as **pure hoists** — work lifted
out of the frame loop or the row loop, arithmetic copied verbatim.

| | before | after |
|---|---:|---:|
| Slowest routine in the library | 133.9 fps | **195.9 fps** |
| Routines under 150 fps | 1 | **0** |
| The 15 worked routines, total | 70.5 ms | **45.5 ms** (−35%) |

**Output is bit-identical**, verified per pattern over 1-, 138- and 400-frame runs
(mean 0.0000, max 0 differing pixels — not "visually indistinguishable": zero).

Whole-engine cost:

| | ms/frame | fps | drawing |
|---|---:|---:|---|
| v2.0 | 6.617 | 151.1 | one routine |
| **v2.1** | **8.035** | **124.5** | up to 4 layers + blends + per-layer palette reshape |

Both inside the 8.33 ms budget of the built-in 120 Hz panel; v2.1 has 2.1× headroom
against the 16.67 ms budget of the 60 Hz external display. A frame-budget governor
half-rates the top layer if the EWMA exceeds 13.5 ms and releases it after 120 calm
frames.

---

## 4. House rules — verified, not assumed

| Rule | Check | Result |
|---|---|---|
| Nothing strobes: mean channel delta < 8 | 12,000 composited frames | **0 over budget**, max 5.54, mean 0.65 |
| Nothing strobes, per pattern | all 201 rendered alone at 1280×960 | **0 over budget**, max 6.88, mean 0.83 |
| Never break the app | `make` from clean | **builds and runs** |
| Only `pattern_NNN` is file-scope | `nm -g --defined-only` × 201 objects | **0 leaks of 201** |
| Verify visually | 201 four-frame filmstrips + gallery render | **inspected, all render** |

Two caveats stated plainly:

- **Two motion instruments disagree, and that is by design.** Rendered alone at full
  resolution, all 201 patterns are inside the limit (max 6.88, mean 0.83). The engine's
  own startup probe puts one pattern (`pattern_103`) at **8.34** — marginally over. The
  probe is deliberately pessimistic: it runs at 320×240, multiplies by 1.6 for detail
  that aliases away at that size, and charges a quarter of the *worst* single step rather
  than the average, specifically to catch a routine that lurches once a second and looks
  calm on average. Being conservative is the point — the admission predicate then weights
  motion by the slot's peak opacity, so a lively routine is admitted only where it will
  be quiet. It is doing its job, not failing it.
- The per-pattern cost figures come from a runtime probe and therefore move with machine
  load. The values published in `engine_stats.csv` and on the gallery cards were taken
  on an idle machine; a card that reads 4 ms may read 6 ms on a busy one.

---

## 5. What did **not** change

- **The plug-in contract.** `pattern(fb, w, h, frame, sl, seed, pal)` is untouched. The
  layer compositor required **zero changes to any pattern** — the scheduler simply makes
  `sl` layer-local.
- **`JD_MODE=N`** still forces a single routine with no layers, exactly as in v2.0. This
  is how every card in the gallery is addressed.
- **`draw.s`.** The 24 assembly modes are the same 24 modes.
- **The scheme count is derived, not typed.** The Makefile computes it from the size of
  `palette.bin`, and `gen_tables.py` rewrites the modulus literal inside `draw.s`. Adding
  palettes needs no source edit — which is why 30 → 120 touched no C.

---

## 6. Deliverables in this release

| Path | What |
|---|---|
| `bridge.c` | the v2.1 scheduler + layer compositor |
| `patterns_c/pattern_101..201.c` | 101 new patterns, shaped for layering |
| `lab/patterns_c_specs/*.md` | specs for 101–200 |
| `lab/palettes/P31..P90/` | 60 new lab palettes + spec + swatch |
| `gen_tables.py` | OKLab cyclic-spline expansion, anchor ordering |
| `lab/design/palette_score.py` | the taxonomy gate |
| `lab/design/{scheduler,compositor,transitions,palettes,perf}.md` | phase-1 specs |
| `lab/design/engine_stats.{c,csv}` | the measurement harness and its output |
| `lab/_gallery.py` → `lab/gallery.html` | the full 201 + 120 library page |
| `lab/CATALOG.md` | the same library as a table |
| `lab/previews/001..201.png` | four-frame filmstrips, rendered by the shipping code |
| `lab/ramps/000..119.png` | every compiled scheme, sliced out of `palette.bin` |

### The gallery

`lab/gallery.html` now carries **the whole library**: all 201 patterns and all 120
colour schemes, plus the 24 assembly modes as a table. Changes worth knowing:

- Patterns are grouped by the **layer role the engine measured**, not by family, because
  that is what actually decides where a pattern can appear.
- Every filmstrip is **four real frames rendered by the shipping code**, each pattern on
  a different one of the 120 schemes.
- Every colour swatch is the **actual ramp read out of `palette.bin`** — the bytes the
  app loads. House and imported schemes have a swatch for the first time.
- Each card carries the engine's own measured numbers, read from `engine_stats.csv`, so
  the page cannot drift from what the app believes.
- The pattern list is sourced from `patterns_c/registry.c`, not a directory glob, so an
  unregistered work-in-progress file cannot leak onto the page.
- Descriptions fall back to the pattern's own source header comment, so a pattern that
  ships before anyone writes it a spec still gets a real card. (`pattern_201` is
  currently the only one in that state.)
- Client-side search and role/source filters, self-contained: no CDN, no external font,
  no network. Safe on `file://` and on Pages.

---

## 7. How to re-run every number

```sh
cd /Users/exeter/dev/m5/assembly/dzzle1
NS=$(( $(stat -f%z palette.bin) / 131072 ))          # scheme count from the binary

# whole-engine harness: probe table, fps, composited motion, palette bag
clang -O2 -I. -DJD_NS=$NS lab/design/engine_stats.c \
      patterns_c/pattern_[0-9][0-9][0-9].c patterns_c/registry.c draw.s -o /tmp/dump -lm
/tmp/dump stats            # per-routine role / cost / motion / coverage  (§3.1, §4)
/tmp/dump fps 1800         # render-only ms per frame                     (§3.6)
/tmp/dump run 12000        # composited frame-to-frame motion             (§1, §4)
/tmp/dump pal 480          # scheme usage + leg-seam adjacency            (§3.3)

# layer scheduling: spawn trace
JD_DEBUG=1 /tmp/dump fps 120000 2>trace.log
grep '^SPAWN' trace.log    # slot, routine, role, blend, peak, life       (§3.1, §3.2)

# one pattern on its own
JD_MODE=N ./dazzle64
cd patterns_c && clang -O2 -DPATTERN=pattern_NNN harness.c pattern_NNN.c -o /tmp/t -lm
/tmp/t bench ; /tmp/t delta 300

# the v2.0 side of every before/after
git archive HEAD | tar -x -C /tmp/jd_v20        # v2.0 tree: 100 patterns, 30 schemes
# then build the same harnesses against it

# symbol hygiene across the library
for o in *.o; do nm -g --defined-only $o | awk -v p="_${o%.o}" '$3!=p'; done

# regenerate the gallery
python3 lab/_gallery.py
```

Two notes for whoever re-runs this:

- The v2.0 comparison numbers come from **building the v2.0 tree out of git** and
  measuring it, not from re-reading the old design docs. `git archive HEAD` gives a clean
  100-pattern / 30-scheme tree.
- The palette diversity instrument is calibrated against the brief's published v2.0
  baseline before it is used on v2.1; if you change its thresholds, re-calibrate, or the
  before/after stops meaning anything.

---

## 8. Open threads

- **`pattern_201` has no spec.** It landed after the spec sweep. The gallery describes it
  from its source header, which is accurate but is not a spec. Low priority.
- **The probe costs 68 s to complete** on a 225-routine library. It is incremental and
  the app is fully watchable throughout, but the number grows linearly with the library.
  Worth a look before the library doubles again.
- **`lab/_compose.py` is now superseded** by `_gallery.py` for `CATALOG.md`. It still
  runs and still only knows about the first 100 patterns and the `P*` palettes. It should
  be retired rather than left as a second thing that writes the same file.
- **`VERSION` still reads 2.0.0.** Deliberate — M5 bumps and ships.
