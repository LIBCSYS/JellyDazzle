# 06 — The measurement harness: proving an accent is visible

**Status: built, landed, and already used.** `gate contrast` is implemented in
`tools/gate_harness.c`, the three-line tap it needs is in
`src/engine/compositor.c`, both compile clean, `make` still builds the shipping
app, and the baseline below is measured, not estimated.

Nothing in the fix debate — darkening blends, a ground-yielding control law,
revised layer weights, blend selection by ground state — is worth landing until
it moves a number in this file. Two of the proposals have already been run
through it and the result is at the bottom; it is not what anyone expected.

---

## 1. The headline

- **The complaint is real and it is specific to the upper slots.** With the
  accent on screen and covering at least 2% of the frame, it is above its own
  local visibility threshold in **48-55% of frames**. The spark slot manages
  **37%**. The mid slot manages **65%**.
- **It is not mainly the ground's brightness.** Ground luma is over 140 in only
  **~5-9% of frames**. Accent visibility only collapses (0.49 -> 0.18) once
  ground luma passes 150, and that band is **2.6% of accent-present frames**.
  A fix aimed at bright grounds is aimed at a rare case.
- **It is mainly weight.** Accent visibility by `w_now`: **0.03** below 32,
  **0.13** at 32-63, **0.56** at 64-95, **0.62** at 96-127. 13% of
  accent-present frames sit in the two dead bands — the fade-in and fade-out
  tails, where the layer is on the books and invisible on the screen.
- **Blend mode barely matters.** MAX 0.556, SCREEN 0.505, DIFF 0.344 — and DIFF
  is 2.7% of samples. "Blend selection by ground state" is a fix for the
  smallest term in the equation.
- **Fog is real and measurable.** The median frame has 20-30% of its tiles flat
  and bright; the 90th percentile frame has 97-100%. The harness dumps the
  foggiest frame it saw, and it looks exactly like the complaint.

---

## 2. The trap, and how the design defeats it

The compositor adapts to measured frame cost — `g_ewma_ms`, `g_mood`,
`g_hot` — so it is a **closed loop with wall-clock time in it**. Two runs of the
same start frame are two different shows.

Measured, this build, same seed, same binary, same warm probe cache:

| run | `acc_visible` | `acc_v50` | `acc_present` |
|---|---|---|---|
| seed 1700000, run A | 0.2705 | 0.806 | 0.919 |
| seed 1700000, run B | 0.5720 | 1.565 | 0.820 |

**That is a 2.1x swing on the headline metric from nothing but re-running the
binary.** Any before/after built on single runs, single frames, or image diffs
is measuring the scheduler's mood. This is the failure that ended an earlier
investigation and it is why the harness is built the way it is.

**The design answer: never compare across runs.** The tap fires *inside*
`jd_frame` — once when the ground composite is down, once after each overlay is
blended. Compositing a given frame's layer buffers is a pure function, so
"what was underneath the accent on this exact frame" is exact, not
reconstructed. Every number is a within-frame comparison.

Divergence is then demoted from a measurement problem to a **sampling**
problem: it can only change *which* frames get sampled, and sampling problems
have ordinary statistics (§6).

### 2b. The observer effect — found and fixed

`jd_frame` times itself, and `g_ewma_ms` decides `g_hot`, which half-rates the
top overlay, which changes the picture. A tap that walks the frame three times
costs milliseconds, so **the instrumented engine throttled itself and reported
on a show the user never sees**:

| | `acc_visible` | `acc_present` |
|---|---|---|
| tap time charged to the frame | 0.31 | 0.75 |
| tap time refunded (shipped) | 0.62 | 0.89 |

The fix is in the compositor: `g_tap_ms` accumulates time spent inside the
callback and is subtracted before `g_ewma_ms` is updated. **The observer pays
its own bill.** Any future instrumentation added to this engine must do the
same or its numbers are fiction.

---

## 3. What is measured

For every pixel the layer actually changed:

```
S   = sqrt(dL^2 + (0.5*d_rg)^2 + (0.4*d_yb)^2)      signal
thr = 0.5*sigma_local + 0.05*pedestal_local + 2      threshold
V   = S / thr                                        visibility index
```

| term | what it is | why that value |
|---|---|---|
| `dL` | Rec.709 luma step the layer makes | same weights as `luma_of()` already in the harness |
| `d_rg`, `d_yb` | red-green and yellow-blue opponent steps | an accent that reads by **hue** at equal luma must not score as invisible |
| `0.5`, `0.4` | chroma weights | chromatic contrast sensitivity falls off far faster with spatial frequency than luminance; **judgement call, the most calibratable constant here** |
| `0.5*sigma` | contrast masking by the texture directly under the accent | Legge-Foley masking slope, rounded. Local, per tile — an accent is swamped by what is under it, never by the frame average |
| `0.05*pedestal` | Weber term, 5% | the textbook JND is ~1% for a static edge under fixation. A soft-edged moving overlay glimpsed on animated content is nothing like that. 5% is deliberately conservative: it is **harder** to pass |
| `+2` | 8-bit floor | stops V going infinite over a black underlay |
| tile size | `min(w,h)/24`, clamped 8-64 px | ~1.5 degrees of visual angle full-screen — the scale these soft large-blob overlays work at |

**Verdicts:** `V >= 1` just reads · `V >= 2` clearly reads · `V < 1` **swamped**
— present in the buffer, absent to the eye.

A frame counts as **present** for a slot when the slot changed at least 2% of
the frame, and **visible** when the *median* V over that footprint is >= 1.
Median, not mean, so one bright speck cannot carry a dead layer.

These are gamma-encoded code values, not linear luminance. That is deliberate:
the judgement a viewer makes is a **lightness** judgement, lightness tracks the
encoded value far better than the linear one, and it keeps every number here
directly comparable with a pixel value.

### Ground and fog

| metric | definition |
|---|---|
| ground luma distribution | mean luma of the ground composite, one histogram bin per code value; reported as p10/p50/p90 and the fraction of frames over 140 and under 40 |
| headroom | mean of `(255 - local pedestal)/255` over the accent footprint — how much room an additive accent has left |
| **fog** | fraction of tiles with local RMS contrast `< 0.15` **and** mean luma `> 90`. Fog is not "dark" and it is not "low contrast" on its own; it is **low local contrast over a bright base**, which is exactly that conjunction |
| local RMS contrast | mean over tiles of `sd/max(mean,8)` — the dynamic-range compression term |
| dynamic range | p99 - p01 of frame luma |

Fog is measured on the **delivered** frame (after boot ramp, dead-air gain,
audio bloom, emblem, HUD), because that is what the eye gets. V is measured on
the pre-gain buffers, because a global gain scales accent and ground alike and
cannot change a ratio.

---

## 4. Usage

```
gate contrast W H START TOTAL [EVERY] [frames.csv] [tenancies.csv]

  START      is the seed: engine_init derives g_run from the start frame
  EVERY      measure every Nth frame (default 3). Frames inside a tenancy are
             near-perfectly correlated, so this costs nothing in accuracy
  JD_CT_WARM warm-up frames to discard (default 600)
  JD_CT_DUMP prefix -> writes the best/worst accent frame and the foggiest
             frame AS THEY HAPPEN
```

**`JD_CT_DUMP` is not a convenience, it is a requirement.** You cannot go back
and re-render an interesting frame — `gate render` on the same start frame
produces a different show. The extremes must be written out during the run that
scored them.

It writes `_best.ppm`, `_best_under.ppm`, `_best_vmap.ppm`, the same three for
`_worst`, and `_fog.ppm`. The **vmap** is the calibration instrument: untouched
pixels are the underlay at half brightness, and every pixel the accent changed
is tinted by its V — red below 1, yellow at 1-2, white above 2. Put the frame
and its vmap side by side and "does this model agree with my eyes" is
answerable in four seconds.

It already passes that test. The worst frame this build produced (V50 = 0.03)
is a fine string-art spark over a blobby pink-grey ground: the vmap shows the
spark drawn in solid red, and in the actual frame you cannot find it at all.
The foggiest frame (fog = 1.00) is a milky salmon wash with washed-out
string-art over it — J's sentence, rendered.

---

## 5. The baseline — measured, this build

8 seeds x 18,000 frames at 640x360, `EVERY=5`, warm probe cache, one private
`HOME` per seed. 3,480 measured frames per seed; 66 accent tenancies pooled.

```
series   onscr present VISIBLE   clear    cov    V50    sig  sigma    ped
mid       1334   94.2%   72.6%   26.8%  33.0%   1.47   19.7   12.3   85.6
ACCENT    1124   90.7%   31.3%    0.1%  19.2%   0.70   12.6   13.6   89.0
spark      750   74.4%    8.1%    0.0%  38.8%   0.45    8.3   14.6   70.6
ALL-OV    1680   92.7%   48.6%   17.3%  46.9%   1.13   17.3   12.6   83.3
```
*(one seed's table, for shape; the pooled figures with confidence intervals are below)*

**Pooled over tenancies, 95% bootstrap CI:**

| series | tenancies | visible fraction | median V | ground luma |
|---|---|---|---|---|
| mid (slot 1) | 77 | 0.653 [0.573, 0.729] | 2.01 | 74.9 |
| **ACCENT (slot 2)** | **66** | **0.483 [0.399, 0.565]** | **1.39** | **79.7** |
| spark (slot 3) | 56 | 0.369 [0.259, 0.482] | 0.89 | 73.4 |
| all overlays | 163 | 0.592 [0.534, 0.647] | 1.85 | 79.8 |

**Ground:** p10 = 32, p50 = 70-80, p90 = 128-148. Over 140 in 5-9% of frames.
Under 40 in 13-27%. Mean headroom under the accent: 62-69%.

**Fog:** p50 0.14-0.35, p90 0.97-1.00, mean 0.22-0.38. Local RMS contrast
0.12-0.29. Dynamic range 87-138 of 255.

### Where the accent actually dies

Accent-present frames, 4 seeds x 18,000 frames pooled:

| by blend | n | visible |
|---|---|---|
| MAX | 4956 | 0.556 |
| SCREEN | 1916 | 0.505 |
| DIFF | 195 | 0.344 |

| by ground luma | n | visible |
|---|---|---|
| 0-29 | 645 | 0.569 |
| 30-59 | 1802 | 0.630 |
| 60-89 | 2607 | 0.512 |
| 90-119 | 1256 | 0.510 |
| 120-149 | 570 | 0.489 |
| **150-209** | **187** | **0.180** |

| by weight `w_now` (Q8) | n | visible |
|---|---|---|
| **0-31** | **439** | **0.032** |
| **32-63** | **482** | **0.131** |
| 64-95 | 651 | 0.561 |
| 96-127 | 2889 | 0.618 |
| 128-159 | 2533 | 0.589 |
| 160+ | 73 | 1.000 |

**Read that last table before writing any fix.** The accent is invisible where
it is faint, not where the ground is bright. `PEAK_LO/PEAK_HI` are
`{256,128,100,72}` and `{256,180,140,115}` — the accent's ceiling is 100-140 of
256 and the spark's is 72-115, and the envelope spends real time below both.
The ground-brightness story explains 2.6% of the failures; the weight story
explains 13% outright and shades the rest.

---

## 6. The pass/fail line

### Primary gate

> **Accent visible fraction >= 0.80, with the lower bound of the 95% CI >= 0.72.**

Justification, from the engine's own constants rather than taste:

- An accent tenancy runs `HOLD_LO[2]=700` to `HOLD_HI[2]=1500` frames plus
  `FADE_IN 150` / `FADE_OUT 180`. At 0.80, roughly a fifth of a tenancy is
  submerged — about 250 frames, four seconds, spread across the envelope's
  tails. That reads as the accent **breathing**.
- At the measured 0.48, **half** of every accent's life is invisible. A layer
  the engine spent a spawn, a bag draw and a palette build on is not on screen
  for half its tenancy. That is J's sentence.
- 0.80 rather than 1.00 because the fades genuinely should start and end below
  threshold. Demanding 1.00 would demand a hard cut, and the engine's one law
  is that nothing strobes.

**Secondary gates**

| metric | target | why |
|---|---|---|
| spark visible fraction | >= 0.65 | measured 0.37. Spark is the smallest, briefest layer; it may sit below the accent but not below half |
| fog p50 | <= 0.20 | measured 0.14-0.35. Under a fifth of the median frame flat-and-bright |
| fog p90 | <= 0.60 | measured 0.97-1.00. **This is the worst number in the file.** Today the 90th-percentile frame is essentially *entirely* flat and bright |
| local RMS contrast | >= 0.25 | measured 0.12-0.29 |

### Guardrails — a fix must not buy contrast with these

The obvious way to make an accent pop is to darken everything or to cut harder.
Both are regressions wearing a fix's clothes. All four must hold:

| guardrail | baseline | limit |
|---|---|---|
| `gate maxdelta` `ge8` (strobe count) | 0 | **must stay 0** |
| `gate maxdelta` max single-frame delta | 7.35 | <= 9.0 |
| final mean luma | 76-101 | within +/-10 of the arm it is compared to |
| ground luma p10 | 28-36 | **must not fall below 25** |
| `gate battery` FPS (640x360, uninstrumented) | 295 | >= 280 |

The luma guardrail is the important one. `acc_visible` can be driven to 1.0 by
crushing the ground to black — the harness would applaud and the app would be
ruined.

---

## 7. How many frames and seeds before a difference is believable

**The unit of independent information is a tenancy, not a frame.** Consecutive
frames inside one tenancy are near-identical in every metric here; the picture
only becomes new information when a layer is replaced. An accent tenancy plus
its rest gap is ~1,100-2,200 frames, so **18,000 frames buys about 8 accent
tenancies, not 3,600 observations.**

Measured, at 640x360, `EVERY=5`, on the M5:

| configuration | frames | accent tenancies | CI half-width on visible fraction | wall clock |
|---|---|---|---|---|
| 1 seed x 18k | 18,000 | ~8 | ~0.24 | 97 s |
| 4 seeds x 18k | 72,000 | ~33 | ~0.12 | ~100 s (parallel) |
| **8 seeds x 18k** | **144,000** | **66-73** | **0.080-0.091** | **~2 min (parallel)** |
| 24 seeds x 18k | 432,000 | ~200 | ~0.048 | ~6 min |
| 64 seeds x 18k | 1,152,000 | ~530 | ~0.030 | ~15 min |

**Recommended standard: 8 seeds x 18,000 frames per arm.** It resolves a
difference of about **0.17** in visible fraction at 95% confidence, costs two
minutes, and the failure this project is chasing is a 0.48-to-0.80 move — twice
that. Go to 24 seeds only when a specific proposal claims a small effect.

**And say the quiet part out loud: if a fix needs 500 tenancies to show up, it
is not the fix for a complaint J can see with his own eyes.** An effect that is
obvious in a screensaver should be enormous in this metric. A fix that needs
fifteen minutes of statistics to prove is, at best, a rounding error on the
real problem.

**Method:** pool tenancies across seeds, weight each by the number of frames it
was present, and bootstrap over tenancies (never over frames — that would
report a CI ~7x too narrow). `tools/ct_pool.py` in §9 does it.

**Never** compare two runs frame-by-frame, image-diff two runs, or quote a
single seed. §2 has the numbers showing why.

---

## 8. Capturing the before-measurement

The baseline must be pinned against four things that silently move: the probe
cache, the cross-launch opening memory, concurrent writers to that cache, and
the source tree itself.

```sh
cd /Users/exeter/dev/m5/assembly/jellydazzle/v3.0.4

# 0. record exactly what is being measured
git rev-parse HEAD > /tmp/ctbase/COMMIT
git diff > /tmp/ctbase/DIRTY.patch

# 1. build the harness (headless; needs the vendored SDL headers for listen.c)
NS=$(awk '/define JD_SCHEMES/{print $3}' src/engine/palette_count.h)
clang -O2 -Isrc/engine -Ivendor/sdl2/include/SDL2 -D_THREAD_SAFE \
      -DJD_VERSION='"gate"' -DJD_NS=$NS \
      tools/gate_harness.c src/engine/compositor.c src/engine/routines_asm.s \
      src/audio/listen.c src/audio/systap.m \
      src/patterns/[0-9]*.c src/patterns/_registry.c \
      -o /tmp/gate -Lvendor/sdl2/lib -lSDL2 \
      -framework Cocoa -framework CoreAudio -framework Foundation -lobjc

# 2. build ONE warm probe cache and freeze it as the reference
#    NOTE: probe_cache_path() calls mkdir() NON-RECURSIVELY, so a fresh HOME
#    silently never persists anything. Create the tree by hand.
REF=/tmp/jd_ref; mkdir -p "$REF/Library/Application Support/JellyDazzle"
HOME=$REF /tmp/gate run 640 360 1700000 20000      # full sweep, once
cp "$REF/Library/Application Support/JellyDazzle/"probe-*.bin /tmp/ctbase/probe.ref

# 3. one private HOME per seed, each seeded from the frozen cache
for s in 1700000 2300000 3100000 4700000 5900000 6700000 7300000 8100000; do
  H=/tmp/ctbase/home_$s
  mkdir -p "$H/Library/Application Support/JellyDazzle"
  cp /tmp/ctbase/probe.ref "$H/Library/Application Support/JellyDazzle/"
  ( HOME=$H /tmp/gate contrast 640 360 $s 18000 5 \
      /tmp/ctbase/f_$s.csv /tmp/ctbase/t_$s.csv > /tmp/ctbase/r_$s.txt 2>&1 ) &
done; wait

# 4. one dump run, for eyes
HOME=/tmp/ctbase/home_1700000 JD_CT_DUMP=/tmp/ctbase/eye \
  /tmp/gate contrast 640 360 1700000 18000 5 - -
for f in /tmp/ctbase/eye_*.ppm; do sips -s format png "$f" --out "${f%.ppm}.png"; done

# 5. the guardrail baselines
/tmp/gate maxdelta 640 360 1700000 6000
/tmp/gate battery  640 360 1700000 6000 6000 | tail -1

# 6. pool
python3 tools/ct_pool.py 2 /tmp/ctbase/t_*.csv    # accent
python3 tools/ct_pool.py 5 /tmp/ctbase/t_*.csv    # all overlays
```

**Why each step exists:**

- **Warm cache, frozen and copied per seed.** A cold start spends ~2.5 minutes
  probing 625 routines, during which GROUND holds almost nothing and every
  ground is drawn under `g_force` in probe order. A cold-start baseline is a
  measurement of the probe sweep.
- **One HOME per seed.** `probe_cache_save()` is unconditional and fires on
  every first spawn (`recent_note`). Eight parallel processes sharing one HOME
  are eight processes writing one file.
- **The frozen cache also pins `g_recent`,** the 72-entry cross-launch opening
  memory. Left alone it drifts every run and changes what each run opens on.
- **Commit + dirty patch recorded.** Learned the hard way in this very session:
  another agent landed `JD_BLENDW` and `JD_WARPFIX` into `compositor.c`
  mid-measurement, so an early baseline and a later one were taken against
  different engines. Record the tree or the baseline is worthless.
- **640x360.** Big enough for the tile grid (43x24 tiles at 15px) and fast
  enough to run 144,000 frames in two minutes. Re-baseline at 1280x720 before
  shipping — tile size scales with the frame, so V is resolution-stable in
  principle, but that is an assumption, not a measurement.

Keep `/tmp/ctbase` — copy it to `docs/contrast/baseline/`. The CSVs are the
before-measurement; the summary lines alone are not enough to re-pool.

---

## 9. `tools/ct_pool.py`

```python
#!/usr/bin/env python3
"""ct_pool.py - pool `gate contrast` tenancy CSVs and put a confidence
interval on the headline numbers.

    ct_pool.py SLOT tenancy_*.csv       (slot 2 = accent, 5 = all overlays)

The unit of independent information is a TENANCY, not a frame: consecutive
frames inside one tenancy are almost perfectly correlated, so a run of 18k
frames is worth ~8 accent observations, not 3600.  Everything below is a
bootstrap over tenancies, weighted by how long each one was on screen."""
import sys, csv, random, statistics as st

slot = int(sys.argv[1]); rows = []
for path in sys.argv[2:]:
    with open(path) as f:
        for r in csv.DictReader(f):
            if int(r["slot"]) == slot and int(r["present"]) > 0:
                rows.append((int(r["present"]), float(r["vis_frac"]),
                             float(r["v50"]), float(r["gluma"])))
if not rows: sys.exit("no tenancies for slot %d" % slot)

def wmean(sample, idx):
    return (sum(r[0] * r[1 + idx] for r in sample) / sum(r[0] for r in sample))

def boot(idx, n=4000, conf=0.95):
    pt = wmean(rows, idx)
    xs = sorted(wmean([random.choice(rows) for _ in rows], idx) for _ in range(n))
    lo = xs[int((1 - conf) / 2 * n)]; hi = xs[int((1 - (1 - conf) / 2) * n)]
    return pt, lo, hi

random.seed(1)
print("tenancies n = %d   (median %d samples each)" %
      (len(rows), int(st.median(r[0] for r in rows))))
for name, idx in (("visible fraction", 0), ("median V", 1), ("ground luma", 2)):
    p, lo, hi = boot(idx)
    print("%-18s %7.4f   95%% CI [%.4f, %.4f]   half-width %.4f"
          % (name, p, lo, hi, (hi - lo) / 2))
```

---

## 10. It has already caught something

`JD_BLENDW` (landed while this harness was being built: `span_max/screen/add`
scaled the *source* by the layer weight, making weight a threshold rather than
an opacity) was A/B'd through the harness at the recommended sample size —
8 seeds x 18,000 frames per arm, same seed list, same frozen cache, arms run
back to back on the same machine.

| arm | accent tenancies | visible fraction, 95% CI | median V |
|---|---|---|---|
| `JD_BLENDW=0` (pre-3.0.4) | 70 | 0.498 [0.407, 0.590] | 1.34 |
| `JD_BLENDW=1` (fix, default) | 73 | **0.551** [0.470, 0.631] | 1.53 |

**Verdict: not demonstrated.** +0.053 with CIs that overlap across three
quarters of their width. The algebra behind the fix is right and the fix should
stay, but *on accent visibility* its effect is smaller than this harness can
resolve at 143 tenancies, and it does not come close to the 0.80 gate.

Stratified by ground luma (4 seeds/arm, accent-present frames) it does what it
was designed to do in the band it was designed for:

| ground luma | `BLENDW=0` | `BLENDW=1` |
|---|---|---|
| 0-39 | 0.580 | 0.666 |
| 40-79 | 0.605 | 0.594 |
| 80-119 | 0.537 | 0.569 |
| 120-159 | 0.495 | **0.640** |
| 160+ | 0.000 (n=6) | 0.193 (n=176) |

**Read that with suspicion.** Ground luma is downstream of the change — the fix
alters the luma distribution itself (n=6 vs n=176 in the top band), so this is
conditioning on a post-treatment variable and cannot carry a causal claim on
its own. It is a hypothesis generator, not evidence. Quoted here because it is
the shape you would expect if the fix works and the effect is simply diluted by
the 97% of frames where ground luma was never the problem.

**What this tells the fix debate:** the harness's own diagnosis (§5) says the
accent dies at low **weight**, not on bright grounds. `PEAK_LO/PEAK_HI` for the
accent are 100-140 of 256, and 13% of accent-present frames sit under `w_now`
64 where visibility is 0.03-0.13. **The next thing to A/B is the weight
envelope, not another blend mode.**

---

## 11. What this cannot measure — bring eyes

The harness is honest about its ceiling. `V` says only that a difference exceeds
an estimated local threshold. It cannot tell you:

- **Whether the accent reads as an accent** rather than as noise, dirt or a
  rendering artefact. A dense speckle can score V=4 and look like dead pixels.
- **Whether the eye is drawn to it.** Saliency is about structure, coherence and
  motion relative to the field. This model has none of that.
- **Whether the frame is beautiful.** The metric would happily approve a
  high-contrast frame nobody wants to look at.
- **Temporal coherence.** V is per-frame. An accent that flickers around
  threshold at 4 Hz scores identically to one that holds steadily just above it,
  and one of those is a defect.
- **Whether "fog" as scored is the same fog J means.** The tile rule
  (contrast < 0.15, luma > 90) is a definition, not a discovery. It found a
  frame that matches his description, which is encouraging, not conclusive.
- **Any of it at real resolution, on a real display, in a dark room.** All of the
  above is 640x360 sRGB code values in a headless process.

**The calibration ritual, once, before trusting any of these numbers:** run with
`JD_CT_DUMP`, put `_worst.ppm` beside `_worst_vmap.ppm` and `_best.ppm` beside
`_best_vmap.ppm`, and confirm the red regions are the ones J cannot see and the
white regions are the ones he can. If the ordering is right, the constants are
close enough. If it disagrees, the chroma weights (0.5 / 0.4) and the Weber
term (0.05) are the two knobs — change them there and nowhere else, and
re-baseline everything.

---

## 12. The code

Three pieces. All landed.

### 12.1 `src/engine/jellydazzle.h` — the hook

```c
/* ---- CONTRAST TAP (3.0.4, measurement only) --------------------------
 * A hook into the composite, for tools/gate_harness.c `contrast`.  NULL in
 * every shipping run, so a normal frame pays three predictable branches.
 *
 * Why a tap rather than two runs of the engine: the compositor ADAPTS to
 * measured frame cost (g_ewma_ms, g_mood, g_hot), so two runs of the same
 * start frame diverge and a with/without-overlays diff across runs measures
 * the divergence, not the accent.  Within ONE frame the composite is a pure
 * function of that frame's layer buffers, so the counterfactual "what was
 * under the accent" is exact and free.
 *
 *   stage = JD_TAP_GROUND    fb holds the ground composite, no overlays yet;
 *                            slot is the incumbent ground slot.
 *   stage = JD_TAP_OVERLAY   fb holds the composite with overlay `slot` just
 *                            blended on top; routine/w_now/blend describe it.
 *   stage = JD_TAP_FINAL     fb is the delivered frame: boot ramp, dead-air
 *                            gain, audio bloom, emblem and HUD all applied.
 * The callback must not write fb. */
enum { JD_TAP_GROUND = 0, JD_TAP_OVERLAY, JD_TAP_FINAL };
extern void (*jd_tap)(int stage, int slot, int routine, int w_now, int blend,
                      const uint32_t *fb, int w, int h);
```

### 12.2 `src/engine/compositor.c` — the tap, and the refund

Defined next to the other engine-state globals; three call sites: after the
ground composite, after each `blend_span` in the overlay loop, and after the
HUD at the end of `jd_frame`. `NULL` in every shipping run, so a normal frame
pays three predictable branches. `jd_frame` also zeroes `g_tap_ms` at entry and
subtracts it before updating `g_ewma_ms` (§2b).

```c
/* CONTRAST TAP (3.0.4).  Declared in jellydazzle.h; NULL unless a measurement
 * tool installs one, so the shipping cost is three predictable branches per
 * frame.  See the header for why the counterfactual is taken INSIDE the frame
 * rather than by re-running the engine with the overlays suppressed. */
void (*jd_tap)(int stage, int slot, int routine, int w_now, int blend,
               const uint32_t *fb, int w, int h) = NULL;
static double g_tap_ms = 0.0;      /* time spent INSIDE the tap this frame */

/* Call the tap and give the time back.
 *
 * This matters more than it looks.  jd_frame times ITSELF (g_ewma_ms), and
 * g_ewma_ms decides g_hot, which half-rates the top overlay, which changes
 * the picture.  A measurement callback that walks the frame three times costs
 * several milliseconds, so an instrumented run would silently push the engine
 * into its thermal-throttle behaviour and then report on a show the user never
 * sees.  Measured: an instrumented run scored accent visibility 0.31 where the
 * same seed uninstrumented scored 0.62.  The observer has to pay its own bill. */
static void tap(int stage, int slot, int routine, int w_now, int blend,
                const uint32_t *fb, int w, int h)
{
    if (!jd_tap) return;
    double t = now_ms();
    jd_tap(stage, slot, routine, w_now, blend, fb, w, h);
    g_tap_ms += now_ms() - t;
}
```

```c
/* in jd_frame, at entry */
    double t_frame = now_ms();
    g_tap_ms = 0.0;                 /* measurement time, refunded below */

/* after the ground composite */
    tap(JD_TAP_GROUND, gnd[0], g_L[gnd[0]].routine,
        g_L[gnd[0]].w_now, B_MIX, fb, w, h);

/* inside the overlay loop, after blend_span */
        tap(JD_TAP_OVERLAY, ov[k], L->routine, L->w_now, L->blend, fb, w, h);

/* after jd_audio_meter_draw / jd_about_draw */
    tap(JD_TAP_FINAL, -1, -1, 0, 0, fb, w, h);

/* at the end, instead of `now_ms() - t_frame` */
    double ms = now_ms() - t_frame - g_tap_ms;   /* the tap does not get to
                                                   change what it measures */
```

### 12.3 `tools/gate_harness.c` — the `contrast` subcommand

```c
/* ============================================================================
 * contrast (3.0.4) — is the accent actually VISIBLE, and how much of the
 * problem is the ground?
 * ============================================================================
 *
 *   gate contrast W H START TOTAL [EVERY] [frames.csv] [tenancies.csv]
 *
 * The complaint this answers is "foggy clutter where the accent is barely
 * visible".  Three separate claims hide in that sentence and the harness
 * keeps them apart:
 *
 *   1. the accent is there but SWAMPED   -> accent visibility index V
 *   2. the ground is too BRIGHT to sit under an accent at all
 *                                        -> ground luma distribution, headroom
 *   3. the whole frame is FOGGY          -> low local contrast over a bright
 *                                           base, measured per tile
 *
 * HOW THE ACCENT IS ISOLATED.  Not by running the engine twice.  The
 * compositor adapts to measured frame cost (g_ewma_ms, g_mood, g_hot), so two
 * runs from the same start frame diverge within seconds and any cross-run
 * diff measures the divergence.  Instead jd_tap fires INSIDE the frame: once
 * when the ground composite is down and once after each overlay is blended.
 * The composite of a given frame's layer buffers is a pure function, so
 * "what was underneath the accent on this exact frame" is exact, not a
 * reconstruction.  Every number below is a within-frame comparison; run-to-run
 * divergence can only change WHICH frames get sampled, which makes it a
 * sampling problem with ordinary statistics, not a measurement problem.
 *
 * WHAT V MEANS.  For every pixel the accent actually changed:
 *
 *      S   = signal   = sqrt(dL^2 + (0.5*d_rg)^2 + (0.4*d_yb)^2)
 *      thr = threshold = 0.5*sigma_local + 0.05*pedestal_local + 2
 *      V   = S / thr
 *
 * dL is the Rec.709 luma step the accent makes (same weights as luma_of
 * above); d_rg and d_yb are the red-green and yellow-blue opponent steps, so
 * an accent that reads by HUE at equal luma is not scored as invisible.  They
 * are weighted down because chromatic contrast sensitivity falls away much
 * faster with spatial frequency than luminance does; 0.5/0.4 is a judgement
 * call and is the single most calibratable constant here.
 *
 * The threshold is a plain masking model: 0.5*sigma is contrast masking by
 * the texture immediately under the accent (Legge-Foley slope, rounded),
 * 0.05*pedestal is the Weber term — 5%, deliberately several times the ~1%
 * textbook JND, because a soft-edged moving overlay glimpsed on animated
 * content is nothing like a static edge under fixation — and +2 code values
 * is the 8-bit floor so a black underlay cannot make V infinite.
 *
 *      V >= 1  the accent equals its local detection threshold: just reads
 *      V >= 2  clearly reads
 *      V <  1  swamped — present in the buffer, absent to the eye
 *
 * These are gamma-encoded code values, not linear luminance.  That is on
 * purpose: the judgement a viewer makes is a LIGHTNESS judgement and lightness
 * tracks the encoded value far better than the linear one, and it keeps every
 * number in this report directly comparable with a pixel value.
 *
 * WHAT IT CANNOT MEASURE.  Whether the accent is BEAUTIFUL, whether it reads
 * as an accent rather than as noise, and whether the eye is drawn to it.  V
 * says only that a difference is above threshold.  Calibrate it once against
 * J's eyes with `render` on the frames this command ranks best/median/worst,
 * then trust the ordering.
 */
#define CT_NSLOT   5              /* JD_NBUF: slots 0..3 plus the shadow ground */
#define CT_ALL     CT_NSLOT       /* pseudo-series: all overlays taken together */
#define CT_NSER    (CT_NSLOT + 1)
#define CT_ACCENT  2              /* JD_NSLOT names them base, mid, accent, spark */
#define CT_VBINS   161            /* V in [0,8) at 0.05 wide; 160 = overflow     */
#define CT_VSCALE  20.0           /* bins per unit of V                          */
#define CT_COVMIN  0.02           /* below 2% of the frame it is not an accent   */
#define CT_FOGC    0.15           /* tile RMS contrast under this = flat         */
#define CT_FOGL    90.0           /* ... and over this luma = flat AND bright    */

static double ct_luma(uint32_t c) {      /* same weights as luma_of() above */
    return ((c >> 16 & 255) * 54 + (c >> 8 & 255) * 183 + (c & 255) * 19) / 256.0;
}

/* ---- tile grid ------------------------------------------------------------
 * Masking is LOCAL.  An accent is swamped by the texture directly under it,
 * never by the average of the whole screen, so sigma and the pedestal are
 * taken from the tile the pixel falls in.  A tile of ~1/24 of the shorter
 * side is roughly 1.5 degrees of visual angle on a full-screen display, which
 * is the scale these soft, large-blob overlays actually work at. */
static int     ct_tsz, ct_ntx, ct_nty, ct_nt;
static float  *ct_tm, *ct_tsd;                    /* per-tile mean / stddev */
static double *ct_s1, *ct_s2; static long *ct_cn;

static void ct_tiles(const uint32_t *b, int w, int h)
{
    for (int t = 0; t < ct_nt; t++) { ct_s1[t] = 0; ct_s2[t] = 0; ct_cn[t] = 0; }
    for (int y = 0; y < h; y++) {
        int ty = y / ct_tsz; if (ty >= ct_nty) ty = ct_nty - 1;
        const uint32_t *row = b + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            int tx = x / ct_tsz; if (tx >= ct_ntx) tx = ct_ntx - 1;
            int t = ty * ct_ntx + tx;
            double l = ct_luma(row[x]);
            ct_s1[t] += l; ct_s2[t] += l * l; ct_cn[t]++;
        }
    }
    for (int t = 0; t < ct_nt; t++) {
        double m = ct_cn[t] ? ct_s1[t] / ct_cn[t] : 0.0;
        double v = ct_cn[t] ? ct_s2[t] / ct_cn[t] - m * m : 0.0;
        ct_tm[t] = (float)m; ct_tsd[t] = (float)(v > 0 ? __builtin_sqrt(v) : 0);
    }
}

/* percentile out of a fixed-width histogram */
static double ct_pct(const long *hist, int nb, long tot, double p, double scale)
{
    if (tot <= 0) return 0.0;
    long want = (long)(p * (double)tot), acc = 0;
    for (int i = 0; i < nb; i++) { acc += hist[i]; if (acc > want) return (i + 0.5) / scale; }
    return (nb - 0.5) / scale;
}

typedef struct {
    double cov;          /* fraction of the frame this layer actually changed */
    double v50, v90;     /* visibility index over that footprint              */
    double vf1, vf2;     /* fraction of the footprint at V>=1 and V>=2        */
    double sgn, sig, ped, head;   /* mean signal, masker, pedestal, headroom  */
} ct_meas;

/* Compare the composite before and after one layer went on.  Everything is
 * measured only where the layer CHANGED something: averaging a strong accent
 * over the 96% of the frame it never touched is how an accent gets scored as
 * invisible when it is merely small. */
static void ct_measure(const uint32_t *under, const uint32_t *over,
                       int w, int h, ct_meas *m)
{
    long hist[CT_VBINS]; memset(hist, 0, sizeof hist);
    long touched = 0, npix = (long)w * h;
    double s_sgn = 0, s_sig = 0, s_ped = 0, s_head = 0;
    memset(m, 0, sizeof *m);
    ct_tiles(under, w, h);
    for (int y = 0; y < h; y++) {
        int ty = y / ct_tsz; if (ty >= ct_nty) ty = ct_nty - 1;
        for (int x = 0; x < w; x++) {
            size_t i = (size_t)y * w + x;
            uint32_t a = under[i], b = over[i];
            if (a == b) continue;
            int ar = a >> 16 & 255, ag = a >> 8 & 255, ab = a & 255;
            int br = b >> 16 & 255, bg = b >> 8 & 255, bb = b & 255;
            double dL  = ((br - ar) * 54 + (bg - ag) * 183 + (bb - ab) * 19) / 256.0;
            double drg = (double)((br - bg) - (ar - ag));
            double dyb = ((br + bg) * 0.5 - bb) - ((ar + ag) * 0.5 - ab);
            double S   = __builtin_sqrt(dL * dL + 0.25 * drg * drg + 0.16 * dyb * dyb);
            if (S < 0.5) continue;         /* under half a code value: not a change */
            int tx = x / ct_tsz; if (tx >= ct_ntx) tx = ct_ntx - 1;
            int t  = ty * ct_ntx + tx;
            double sd = ct_tsd[t], pe = ct_tm[t];
            double V  = S / (0.5 * sd + 0.05 * pe + 2.0);
            int bin = (int)(V * CT_VSCALE); if (bin >= CT_VBINS) bin = CT_VBINS - 1;
            hist[bin]++; touched++;
            s_sgn += S; s_sig += sd; s_ped += pe; s_head += (255.0 - pe) / 255.0;
        }
    }
    if (!touched) return;
    m->cov  = (double)touched / (double)npix;
    m->v50  = ct_pct(hist, CT_VBINS, touched, 0.50, CT_VSCALE);
    m->v90  = ct_pct(hist, CT_VBINS, touched, 0.90, CT_VSCALE);
    long ge1 = 0, ge2 = 0;
    for (int i = (int)(1.0 * CT_VSCALE); i < CT_VBINS; i++) ge1 += hist[i];
    for (int i = (int)(2.0 * CT_VSCALE); i < CT_VBINS; i++) ge2 += hist[i];
    m->vf1 = (double)ge1 / (double)touched;
    m->vf2 = (double)ge2 / (double)touched;
    m->sgn = s_sgn / touched; m->sig = s_sig / touched;
    m->ped = s_ped / touched; m->head = s_head / touched;
}

/* ---- looking at what was scored ------------------------------------------
 * You cannot go back and re-render an interesting frame.  The engine adapts
 * to measured frame cost, so `gate render` on the same start frame produces a
 * DIFFERENT show — this is the trap that ended one earlier investigation.  So
 * the extremes are written out DURING the run that scored them.
 *
 * JD_CT_DUMP=/tmp/ct   writes six files, overwritten each time a new extreme
 * appears, so what survives to the end of the run is the genuine best and
 * worst the run produced:
 *      _best.ppm  _best_under.ppm  _best_vmap.ppm
 *      _worst.ppm _worst_under.ppm _worst_vmap.ppm
 * plus _fog.ppm, the foggiest delivered frame.
 *
 * The vmap is the calibration instrument: untouched pixels are the underlay
 * at half brightness, and every pixel the accent changed is tinted by its V —
 * red below 1 (swamped), yellow at 1-2, white above 2.  Put the frame and its
 * vmap side by side and the question "does this model agree with my eyes"
 * becomes answerable in about four seconds. */
static const char *ct_dump_pfx;

static void ct_vmap(const uint32_t *under, const uint32_t *over,
                    uint32_t *dst, int w, int h)
{
    ct_tiles(under, w, h);
    for (int y = 0; y < h; y++) {
        int ty = y / ct_tsz; if (ty >= ct_nty) ty = ct_nty - 1;
        for (int x = 0; x < w; x++) {
            size_t i = (size_t)y * w + x;
            uint32_t a = under[i], b = over[i];
            int l = (int)(ct_luma(a) * 0.5);
            dst[i] = 0xFF000000u | (uint32_t)l << 16 | (uint32_t)l << 8 | (uint32_t)l;
            if (a == b) continue;
            int ar = a >> 16 & 255, ag = a >> 8 & 255, ab = a & 255;
            int br = b >> 16 & 255, bg = b >> 8 & 255, bb = b & 255;
            double dL  = ((br - ar) * 54 + (bg - ag) * 183 + (bb - ab) * 19) / 256.0;
            double drg = (double)((br - bg) - (ar - ag));
            double dyb = ((br + bg) * 0.5 - bb) - ((ar + ag) * 0.5 - ab);
            double S   = __builtin_sqrt(dL * dL + 0.25 * drg * drg + 0.16 * dyb * dyb);
            if (S < 0.5) continue;
            int tx = x / ct_tsz; if (tx >= ct_ntx) tx = ct_ntx - 1;
            int t  = ty * ct_ntx + tx;
            double V = S / (0.5 * ct_tsd[t] + 0.05 * ct_tm[t] + 2.0);
            /* red -> yellow -> white as V crosses 1 and then 2 */
            int R = 255, G, B;
            if (V < 1.0)      { G = (int)(V * 120);            B = 0; }
            else if (V < 2.0) { G = 120 + (int)((V - 1) * 135); B = 0; }
            else              { G = 255; B = (int)((V - 2) * 160); if (B > 255) B = 255; }
            dst[i] = 0xFF000000u | (uint32_t)R << 16 | (uint32_t)G << 8 | (uint32_t)B;
        }
    }
}

static void ct_dump(const char *tag, const uint32_t *under,
                    const uint32_t *over, int w, int h)
{
    static uint32_t *vm; static int vn;
    char p[512];
    if (vn < w * h) { free(vm); vm = malloc((size_t)w * h * 4); vn = vm ? w * h : 0; }
    snprintf(p, sizeof p, "%s_%s.ppm", ct_dump_pfx, tag);            ppm(p, over, w, h);
    snprintf(p, sizeof p, "%s_%s_under.ppm", ct_dump_pfx, tag);      ppm(p, under, w, h);
    if (!vm) return;
    ct_vmap(under, over, vm, w, h);
    snprintf(p, sizeof p, "%s_%s_vmap.ppm", ct_dump_pfx, tag);       ppm(p, vm, w, h);
}

/* FOG.  Fog is not "dark" and it is not "low contrast" on its own — it is low
 * LOCAL contrast over a BRIGHT base, which is exactly a tile whose RMS
 * contrast is under 0.15 while its mean luma is over 90.  Counting those
 * tiles turns the word into a number between 0 and 1. */
static void ct_fog(const uint32_t *b, int w, int h,
                   double *fog, double *dr, double *lrms, double *mean)
{
    long lh[256]; memset(lh, 0, sizeof lh);
    double sum = 0; long npix = (long)w * h;
    for (long i = 0; i < npix; i++) {
        double l = ct_luma(b[i]); sum += l;
        int q = (int)l; if (q < 0) q = 0; if (q > 255) q = 255; lh[q]++;
    }
    ct_tiles(b, w, h);
    long flat = 0; double rms = 0;
    for (int t = 0; t < ct_nt; t++) {
        double m = ct_tm[t], sd = ct_tsd[t];
        double c = sd / (m > 8.0 ? m : 8.0);
        rms += c;
        if (c < CT_FOGC && m > CT_FOGL) flat++;
    }
    *fog  = (double)flat / (double)ct_nt;
    *lrms = rms / ct_nt;
    *mean = sum / (double)npix;
    *dr   = ct_pct(lh, 256, npix, 0.99, 1.0) - ct_pct(lh, 256, npix, 0.01, 1.0);
}

/* ---- accumulators ---------------------------------------------------------
 * Everything is kept per SERIES: one per slot, plus CT_ALL for the union of
 * the overlays (if a fix moves the accent from slot 2 to slot 3 the per-slot
 * numbers move and the union does not, and that difference is the point). */
typedef struct {
    long   seen, present, visible, clear;
    double s_cov, s_v50, s_v90, s_vf1, s_sgn, s_sig, s_ped, s_head;
} ct_series;

typedef struct {                    /* one tenancy = one routine's whole run  */
    int    open, routine, f_in, miss;
    long   n, present, visible;
    double s_cov, s_v50, s_gl;
} ct_ten;

static ct_series ct_S[CT_NSER];
static ct_ten    ct_T[CT_NSER];
static long ct_gh[256];             /* ground mean luma, one bin per code value */
static long ct_fh[101];             /* fog fraction, 1% bins                    */
static long ct_meas_n, ct_frames, ct_ten_n;
static double ct_s_fog, ct_s_dr, ct_s_lrms, ct_s_fin;
static int  ct_every = 3, ct_warm = 600, ct_seed;
static int  ct_f, ct_active, ct_gluma_now, ct_gnd_rt;
static uint32_t *ct_prev, *ct_gbuf;
static double ct_vbest = -1e9, ct_vworst = 1e9, ct_fogworst = -1e9;
static FILE *ct_fcsv, *ct_tcsv;

static void ct_ten_close(int s)
{
    ct_ten *T = &ct_T[s];
    if (!T->open || T->n < 4) { T->open = 0; return; }
    ct_ten_n++;
    if (ct_tcsv)
        fprintf(ct_tcsv, "%d,%d,%d,%d,%ld,%ld,%.4f,%.3f,%.4f,%.1f\n",
                ct_seed, s, T->routine, T->f_in, T->n, T->present,
                T->s_cov / T->n, T->s_v50 / T->n,
                T->present ? (double)T->visible / (double)T->present : 0.0,
                T->s_gl / T->n);
    T->open = 0;
}

static void ct_note(int s, int routine, const ct_meas *m, int w_now, int blend)
{
    ct_series *S = &ct_S[s];
    ct_ten    *T = &ct_T[s];
    int present = m->cov >= CT_COVMIN;
    int visible = present && m->v50 >= 1.0;
    S->seen++; S->present += present; S->visible += visible;
    S->clear += (present && m->v50 >= 2.0);
    S->s_cov += m->cov;  S->s_v50 += m->v50;  S->s_v90 += m->v90;
    S->s_vf1 += m->vf1;  S->s_sgn += m->sgn;  S->s_sig += m->sig;
    S->s_ped += m->ped;  S->s_head += m->head;

    if (T->open && T->routine != routine) ct_ten_close(s);
    if (!T->open) { memset(T, 0, sizeof *T); T->open = 1;
                    T->routine = routine; T->f_in = ct_f; }
    T->miss = 0; T->n++; T->present += present; T->visible += visible;
    T->s_cov += m->cov; T->s_v50 += m->v50; T->s_gl += ct_gluma_now;

    if (ct_fcsv)
        fprintf(ct_fcsv, "%d,%d,%d,%d,%d,%d,%d,%.5f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f\n",
                ct_seed, ct_f, s, routine, w_now, blend, ct_gluma_now,
                m->cov, m->v50, m->v90, m->vf1, m->sgn, m->sig, m->ped);
}

/* The tap.  Ground first, then one call per overlay in composite order, then
 * the finished frame.  Sampling every EVERY-th frame costs nothing in
 * accuracy: consecutive frames are almost perfectly autocorrelated (a tenancy
 * lasts 500-1600 frames), so the independent information is in the tenancies,
 * not the frames. */
static void ct_tap(int stage, int slot, int routine, int w_now, int blend,
                   const uint32_t *fb, int w, int h)
{
    long npix = (long)w * h;
    if (stage == JD_TAP_GROUND) {
        ct_f++;
        ct_active = (ct_f > ct_warm) && (ct_f % ct_every == 0);
        if (!ct_active) return;
        ct_frames++;
        memcpy(ct_prev, fb, (size_t)npix * 4);
        memcpy(ct_gbuf, fb, (size_t)npix * 4);
        double s = 0; for (long i = 0; i < npix; i++) s += ct_luma(fb[i]);
        ct_gluma_now = (int)(s / npix + 0.5);
        if (ct_gluma_now > 255) ct_gluma_now = 255;
        if (ct_gluma_now < 0)   ct_gluma_now = 0;
        ct_gh[ct_gluma_now]++;
        ct_gnd_rt = routine;
        for (int i = 0; i < CT_NSER; i++) if (ct_T[i].open) ct_T[i].miss++;
        return;
    }
    if (!ct_active) return;
    if (stage == JD_TAP_OVERLAY) {
        ct_meas m;
        ct_measure(ct_prev, fb, w, h, &m);
        ct_note(slot, routine, &m, w_now, blend);
        /* keep the best and worst ACCENT frame this run produced — they
         * cannot be recovered afterwards, see ct_dump */
        if (ct_dump_pfx && slot == CT_ACCENT && m.cov >= CT_COVMIN) {
            if (m.v50 > ct_vbest)  { ct_vbest  = m.v50; ct_dump("best",  ct_prev, fb, w, h); }
            if (m.v50 < ct_vworst) { ct_vworst = m.v50; ct_dump("worst", ct_prev, fb, w, h); }
        }
        memcpy(ct_prev, fb, (size_t)npix * 4);
        return;
    }
    /* JD_TAP_FINAL: ct_prev is the composite as the overlays left it; fb has
     * the boot ramp, the dead-air gain, the audio bloom and the HUD on top.
     * The union of the overlays is measured against the ground on the
     * PRE-gain buffers (gain scales accent and ground alike, so it cannot
     * change V) while fog is measured on the frame the eye is given. */
    { ct_meas m; ct_measure(ct_gbuf, ct_prev, w, h, &m);
      ct_note(CT_ALL, ct_gnd_rt, &m, 0, -1); }
    { double fog, dr, lrms, mean;
      ct_fog(fb, w, h, &fog, &dr, &lrms, &mean);
      ct_s_fog += fog; ct_s_dr += dr; ct_s_lrms += lrms; ct_s_fin += mean;
      int q = (int)(fog * 100.0 + 0.5); if (q > 100) q = 100; ct_fh[q]++;
      if (ct_dump_pfx && fog > ct_fogworst) {
          char p[512]; ct_fogworst = fog;
          snprintf(p, sizeof p, "%s_fog.ppm", ct_dump_pfx); ppm(p, fb, w, h);
      } }
    ct_meas_n++;
    for (int i = 0; i < CT_NSER; i++)
        if (ct_T[i].open && ct_T[i].miss > 20) ct_ten_close(i);
}

static int ct_setup(int w, int h)
{
    ct_tsz = (w < h ? w : h) / 24; if (ct_tsz < 8) ct_tsz = 8; if (ct_tsz > 64) ct_tsz = 64;
    ct_ntx = (w + ct_tsz - 1) / ct_tsz; ct_nty = (h + ct_tsz - 1) / ct_tsz;
    ct_nt  = ct_ntx * ct_nty;
    ct_tm  = malloc((size_t)ct_nt * sizeof *ct_tm);
    ct_tsd = malloc((size_t)ct_nt * sizeof *ct_tsd);
    ct_s1  = malloc((size_t)ct_nt * sizeof *ct_s1);
    ct_s2  = malloc((size_t)ct_nt * sizeof *ct_s2);
    ct_cn  = malloc((size_t)ct_nt * sizeof *ct_cn);
    ct_prev = malloc((size_t)w * h * 4);
    ct_gbuf = malloc((size_t)w * h * 4);
    return ct_tm && ct_tsd && ct_s1 && ct_s2 && ct_cn && ct_prev && ct_gbuf;
}

static void ct_report(void)
{
    static const char *NM[CT_NSER] = { "base", "mid", "ACCENT", "spark", "shadow", "ALL-OV" };
    printf("\n== contrast: seed(start)=%d  frames run=%d  frames measured=%ld"
           "  tiles=%dx%d@%dpx ==\n", ct_seed, ct_f, ct_frames, ct_ntx, ct_nty, ct_tsz);
    printf("%-7s %6s %7s %7s %7s %6s %6s %6s %6s %6s\n",
           "series", "onscr", "present", "VISIBLE", "clear", "cov", "V50", "sig", "sigma", "ped");
    for (int s = 0; s < CT_NSER; s++) {
        ct_series *S = &ct_S[s];
        if (!S->seen) { printf("%-7s      -\n", NM[s]); continue; }
        double n = (double)S->seen, p = (double)S->present;
        printf("%-7s %6ld %6.1f%% %6.1f%% %6.1f%% %5.1f%% %6.2f %6.1f %6.1f %6.1f\n",
               NM[s], S->seen, 100.0 * p / n,
               p ? 100.0 * (double)S->visible / p : 0.0,
               p ? 100.0 * (double)S->clear   / p : 0.0,
               100.0 * S->s_cov / n, S->s_v50 / n, S->s_sgn / n,
               S->s_sig / n, S->s_ped / n);
    }
    /* ground: how often is it bright enough to swamp anything put on it? */
    long gt = 0; for (int i = 0; i < 256; i++) gt += ct_gh[i];
    long bright = 0, dark = 0;
    for (int i = 141; i < 256; i++) bright += ct_gh[i];
    for (int i = 0;   i <  40; i++) dark   += ct_gh[i];
    printf("\nground luma  p10=%.0f p50=%.0f p90=%.0f   >140: %.1f%% of frames"
           "   <40: %.1f%%   mean headroom under the accent %.0f%%\n",
           ct_pct(ct_gh, 256, gt, 0.10, 1.0), ct_pct(ct_gh, 256, gt, 0.50, 1.0),
           ct_pct(ct_gh, 256, gt, 0.90, 1.0),
           gt ? 100.0 * bright / gt : 0.0, gt ? 100.0 * dark / gt : 0.0,
           ct_S[CT_ACCENT].seen ? 100.0 * ct_S[CT_ACCENT].s_head / ct_S[CT_ACCENT].seen : 0.0);
    long ft = 0; for (int i = 0; i <= 100; i++) ft += ct_fh[i];
    double m = ct_meas_n ? (double)ct_meas_n : 1.0;
    printf("fog          p50=%.2f p90=%.2f mean=%.2f | local RMS contrast %.3f"
           " | dyn range %.0f/255 | final luma %.1f\n",
           ct_pct(ct_fh, 101, ft, 0.50, 100.0), ct_pct(ct_fh, 101, ft, 0.90, 100.0),
           ct_s_fog / m, ct_s_lrms / m, ct_s_dr / m, ct_s_fin / m);
    if (ct_dump_pfx)
        printf("dumped       %s_best*.ppm (V50 %.2f)  %s_worst*.ppm (V50 %.2f)"
               "  %s_fog.ppm (fog %.2f)\n",
               ct_dump_pfx, ct_vbest, ct_dump_pfx, ct_vworst, ct_dump_pfx, ct_fogworst);
    /* the one line a script should parse */
    { ct_series *A = &ct_S[CT_ACCENT], *U = &ct_S[CT_ALL];
      double ap = A->seen ? (double)A->present / A->seen : 0.0;
      double av = A->present ? (double)A->visible / A->present : 0.0;
      double uv = U->present ? (double)U->visible / U->present : 0.0;
      printf("CONTRAST seed=%d meas=%ld ten=%ld acc_present=%.4f acc_visible=%.4f"
             " acc_v50=%.4f all_visible=%.4f fog=%.4f lrms=%.4f dr=%.1f"
             " gluma=%.1f finluma=%.1f\n",
             ct_seed, ct_meas_n, ct_ten_n, ap, av,
             A->seen ? A->s_v50 / A->seen : 0.0, uv,
             ct_s_fog / m, ct_s_lrms / m, ct_s_dr / m,
             ct_pct(ct_gh, 256, gt, 0.50, 1.0), ct_s_fin / m); }
}
```

And the dispatch, in `main`:

```c
    if (!strcmp(cmd, "contrast")) {
        /* Warm-up is discarded, not measured: the boot ramp, the opening
         * emblem, the first-run card and the 4-slot cascade all land inside
         * the first few hundred frames and none of them is the show. */
        if (argc < 6) { fprintf(stderr, "usage: contrast W H START TOTAL [EVERY] [f.csv] [t.csv]\n"); return 2; }
        int start = atoi(argv[4]), total = atoi(argv[5]);
        if (argc > 6) ct_every = atoi(argv[6]);
        if (ct_every < 1) ct_every = 1;
        { const char *e = getenv("JD_CT_WARM"); if (e) ct_warm = atoi(e); }
        ct_dump_pfx = getenv("JD_CT_DUMP");
        if (!ct_setup(w, h)) { fprintf(stderr, "contrast: out of memory\n"); return 2; }
        if (argc > 7 && strcmp(argv[7], "-")) {
            ct_fcsv = fopen(argv[7], "w");
            if (ct_fcsv) fprintf(ct_fcsv, "seed,frame,slot,routine,w,blend,gluma,"
                                          "cov,v50,v90,vf1,signal,sigma,pedestal\n");
        }
        if (argc > 8 && strcmp(argv[8], "-")) {
            ct_tcsv = fopen(argv[8], "w");
            if (ct_tcsv) fprintf(ct_tcsv, "seed,slot,routine,f_in,samples,present,"
                                          "cov,v50,vis_frac,gluma\n");
        }
        ct_seed = start;
        jd_tap = ct_tap;
        for (int f = 0; f < total; f++) jd_frame(fb, w, h, start + f);
        jd_tap = NULL;
        for (int i = 0; i < CT_NSER; i++) ct_ten_close(i);   /* flush the open ones */
        if (ct_fcsv) fclose(ct_fcsv);
        if (ct_tcsv) fclose(ct_tcsv);
        ct_report();
        return 0;
    }
```

---

## 13. Open

- **Re-baseline at 1280x720** before any gate is enforced. Everything above is
  640x360.
- **Temporal coherence metric** — flicker around threshold is invisible to V and
  is probably a real defect. Cheapest version: per-tenancy stddev of per-frame
  V50, gated alongside the mean.
- **`probe_cache_path()` uses a non-recursive `mkdir()`**, so a fresh `HOME`
  silently persists nothing and every run cold-starts. It costs the harness
  nothing now that the protocol pre-creates the tree, but it is a live bug for
  any user whose `~/Library` is missing, and it makes ad-hoc measurement runs
  quietly wrong.
- **Store the baseline CSVs in the repo** (`docs/contrast/baseline/`), not just
  the summary lines. Re-pooling needs the tenancy rows.
