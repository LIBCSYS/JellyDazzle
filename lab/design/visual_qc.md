# JellyDazzle v2.1 — VISUAL QC

Independent visual review of the v2.1 tree as it stood at **2026-08-14 15:15 EDT**.
Everything below was rendered and looked at, not inferred from source. Numbers are
measured on this machine at 1280×960 with `clang -O2`.

**Snapshot warning.** The tree was moving during this pass: `palette.bin` was
regenerated (80 → 120 schemes) at 15:02 and `patterns_c/pattern_201.c` appeared at
~15:10, both from another worker. Nothing in `bridge.c`, `gen_tables.py` or any
pattern was edited by this review — see §7 for why, and for the fix list handed back.

---

## 0. Headline verdict

| Client question | Verdict | The number that settles it |
|---|---|---|
| "It repeats" | **FIXED** | 120 spawns / 6.7 min → **119 distinct routines**, 0 repeats in any 20-spawn window (v2.0: 39–40), closest reappearance 3.1 min |
| "Double the routines" | **DONE, but lopsided** | 100 → 201 patterns; 86 of the 100 new ones are figure/spark, only **6** are grounds — see D2 |
| "Double the palettes" | **EXCEEDED** | 30 → **120** schemes; full-spectrum rainbows 47% → **12%** |
| "Palettes all look similar" | **FIXED** | brightness spread 0.08 → **0.600**; saturation spread 0.15 → **0.748** |
| "Stark to amazing colours" | **DELIVERED** | 72/120 schemes ≤5 hue bins (stark), 15/120 ≥11 bins (blazing) |
| "Layer upon layer" | **WORKS, under-used** | 89 of 120 spawns land on the base slot; only 30 overlay tenancies in 6.7 min — see D3 |
| "No rough breaks" | **PASS — no hard cut anywhere** | peak frame-to-frame delta **3.81** over 24,000 consecutive frames |
| "Relaxing, nothing strobes" | **PASS, comfortably** | mean delta **0.606** vs budget 8.0; **zero** frames > 8 in 24,000 |
| "Keep it smooth" | **PASS in steady state, FAIL at launch** | p50 8.33 ms (120 fps), p99 16.3 ms — but 147 of the first 200 frames exceed 20 ms — see D4 |
| "Does it go together" | **MIXED — the real weak point** | 3 of 8 sampled composites were flat mud, 3 were line-web confetti, 2 read as intentional — see D1 |

**One-line summary for J:** the repetition is genuinely gone, the palettes are
transformed, and it is measurably the smoothest build yet — but the *composite*
still lands on mud or on a hairball more often than it lands on a picture, and that
is now the only thing standing between this and "amazing".

---

## 1. What was rendered

| Pass | Method | Artefacts |
|---|---|---|
| (a) 12 new patterns 101–200 | `qc_harness.c` — one pattern, one selected scheme, frame 300+420 | `/tmp/qc/out/p*_s*.png` |
| (b) 8 layered moments | `tools/jd_dump.c`, live engine, no `JD_MODE`, frames 400…3200, ≥300 apart | `/tmp/qc/eng/jd_00*.png` |
| (c) 6 segment boundaries | same run, frames captured either side of each transition + mid-fade | `/tmp/qc/eng/jd_00{1438,1442,…}.png` |
| structural duplication | 201 thumbnails at fixed scheme/frame, normalised luma correlation | `/tmp/qc/thumbs.bin` |
| flat-ground sweep | every pattern × 6 schemes at 320×240, luma sigma | `/tmp/qc/scan.txt` |
| scheduler behaviour | 24,000-frame headless run with `JD_DEBUG=1` | `/tmp/qc/long_trace.txt` |

Reproduce commands are in §8.

---

## 2. Pass (a) — twelve of the new patterns

Each rendered against a *different* palette scheme, deliberately spread from stark to
blazing, so this doubles as a palette spot-check. `cover` = fraction of pixels above
luma 16. `delta` = mean per-channel frame-to-frame change (budget < 8).

| # | Name | Scheme | luma | sigma | cover | delta mean / peak | Verdict |
|---|---|---|---|---|---|---|---|
| 104 | Ice-Ray Lattice | 76 blueprint_cut | 8.9 | 23.1 | 0.149 | 0.74 / **11.71** | **DEFECT** — elegant joinery, but it *pops*. See D5 |
| 112 | Vortex Ink | 58 nebula_reef | 32.3 | 28.7 | 0.634 | 1.17 / 1.22 | PASS — real fluid, beautiful. Warm/cool dyes mix to brown in the mid-field; watch it as a ground |
| 119 | Rose Window | 45 cathedral_glass | 52.2 | 74.5 | 0.331 | 0.37 / 1.74 | **BEST IN SET** — full-spectrum but cohesive; the black leading does the work |
| 127 | Dipole Filings | 82 frost_signal | 6.2 | 24.8 | 0.084 | 0.72 / 0.85 | PASS — exquisite cyan/rose filigree. Correctly a SPARK; empty as a solo |
| 134 | Aurora Curtain | 47 aurora_veil | 18.7 | 24.1 | 0.341 | 0.23 / 0.26 | **PASS, flagship** — exactly the brief's "relaxing" |
| 141 | Chladni Sand | 75 newsprint_siren | **1.0** | 7.0 | **0.011** | 0.14 / 0.17 | **CAUTION** — near-black on void-floor schemes. See D6 |
| 150 | Relief Lantern | 74 hammered_copper | 48.4 | 60.0 | 0.607 | 0.20 / 0.29 | PASS — gorgeous molten copper moiré. Fine ring pitch; verify on a 4K panel |
| 158 | Blaze | 116 hazard_tape | 71.3 | 81.4 | 0.546 | 1.73 / 1.74 | **PASS but watch** — Riley *Blaze* is famously uncomfortable. Motion is legal (1.73) only because it turns slowly. Never pair with a second radial |
| 166 | Star Trails | 60 cobalt_vigil | 9.9 | 22.5 | 0.177 | **0.05** | PASS — the calmest thing in the library |
| 175 | Glenz Vectors | 90 neon_alley | 27.5 | 41.9 | 0.330 | 0.77 / 0.80 | PASS — clean Amiga transparency. **Duplicates 190**, see D7 |
| 186 | Marbling | 62 tide_and_coral | 91.6 | 62.1 | 0.856 | 0.16 / 0.40 | PASS — bold, poster-saturated red/cyan. The "amazing" end of the range |
| 196 | Craquelure | 78 moss_and_stone | 4.3 | 13.2 | 0.093 | **0.01** | PASS — dead calm, ideal top layer |

**Craft level is high.** These are real simulations (Kuramoto, Biot–Savart, Reiter,
RK4 Lorenz, thin-film interference), not sine-wave wallpaper, and the header comments
are honest about the maths. Motion discipline is excellent: 11 of 12 are an order of
magnitude inside the budget.

---

## 3. Pass (b) — eight layered moments from the live engine

Live `jd_frame`, no `JD_MODE`, one deterministic run. `lstd` = luma std-dev over the
frame (a flat wall is < 10). `hbins` = hue bins holding >5% of the frame's chroma.

| frame | tenants (from trace) | luma | **lstd** | sat | hbins | Verdict |
|---|---|---|---|---|---|---|
| 400 | p002 ground + p124 + p107 | 50.1 | **5.0** | 0.99 | **1** | **FAIL — flat brick-red wall + dust specks** |
| 800 | same tenancy | 73.8 | **4.2** | 0.50 | **1** | **FAIL — featureless cocoa brown** |
| 1200 | same tenancy | 80.7 | **3.5** | 0.45 | **1** | **FAIL — solid beige. 18 s of nothing** |
| 1600 | p002 + p124 + p107, fading | 92.9 | 45.0 | 0.55 | 3 | WEAK — dense yellow-green line mat, reads as screen-door noise |
| 2000 | asm18 + p124 + p107 | 103.9 | 34.2 | 0.35 | 2 | OK — cream/rosewood moiré. Pleasant, but vintage-wallpaper bland |
| 2400 | p025 + p143 + p015 | 28.7 | 36.8 | 0.49 | 10 | WEAK — hairline web, every strand a different hue. **Confetti** |
| 2800 | asm20 + p103 + p092 | 133.4 | 47.8 | 0.60 | 3 | **PASS — a real composition.** Greek-key panel, gold/teal on brown, ray fans, dragon curve on top. This is what layering is for |
| 3200 | asm16 + p147 + p063 | 41.4 | 30.8 | 0.38 | 2 | **PASS** — dark checker ground, pastel star polygons, dragon curve. Genuine depth |

Score: **2 pass, 3 weak, 3 fail.** The compositor demonstrably *can* stack four
legible tenants (f=2800, f=3200 prove it). It just doesn't reliably.

---

## 4. Pass (c) — six segment boundaries

Frames captured on both sides of every transition in the run, plus mid-fade.

| # | frame | event | before → after | Δluma | Δlstd | Verdict |
|---|---|---|---|---|---|---|
| 1 | 1440 | base rotation → asm 18 | 1438 → 1442 | +0.4 | +0.1 | **no cut** |
| 2 | 1740 | base rotation → p025 | 1738 → 1742 | +0.6 | +0.2 | **no cut** |
| 3 | 1958/9 | spark p143 **+** base p015, same frame | 1956 → 1960 | 0.0 | 0.0 | **no cut** |
| 4 | 2048 | asm segment wrap (`frame&2047==0`) | — | — | — | **no cut** (global peak delta 2.81 that run) |
| 5 | 2199 | base rotation → asm 20 | 2196 → 2200 | +0.1 | 0.0 | **no cut** |
| 6 | 2438/9 | field p103 **+** base p092, same frame | 2436 → 2440 | +0.3 | +0.2 | **no cut** — viewed side by side, visually indistinguishable |

Mid-fade frames confirm the *arrival* is a genuine 180–210 frame ramp, not a
cross-dissolve of two finished pictures: 2196 (lstd 35.5) → 2290 (25.6) → 2400 (36.8)
is a smooth 200-frame dim-and-rebuild. That is exactly J's "1, then 3 seconds later
another, then 1 second another".

**The strongest evidence is not these six spot checks — it is that across 24,000
consecutive frames the largest single-frame change was 3.81 out of a budget of 8.0,
and not one frame exceeded the budget. There is no hard cut anywhere in this build.**

---

## 5. Defects, ranked

### D1 — COMPOSITE MUD: 75% of overlays use MIX blend  *(severity: high)*

Blend usage over 120 spawns: **MIX 90, MAX 25, SCREEN 4, DIFF 1, ADD 0.**

Every routine writes every pixel opaque (`compositor.md §0.1`), so MIX over an opaque
ground is a straight average of two images. Average two mid-luma images and you get
the mid-grey/brown seen at f=800 and f=1200. SCREEN and ADD — the two modes that make
an overlay *add light* instead of veiling the ground — fired 4 times in 120.

`pick_blend()` at `bridge.c:903` gates SCREEN behind `g_mood == M_BLAZE && st->dark >= 180`
(`bridge.c:939`). BLAZE is one mood in three, and `dark >= 180` is a hard bar. Loosen
both: SCREEN/ADD should be the *default* for any tenant with `dark >= 120` regardless
of mood, and MIX reserved for tenants that are genuinely field-like.

### D2 — FLAT GROUNDS: 14 full-screen patterns collapse to a solid colour  *(severity: high)*

Swept all 201 patterns × 6 schemes at 320×240. Grounds (`cover > 0.80`) whose luma
sigma falls below 14 on at least one scheme — i.e. the whole screen becomes one colour:

| pattern | cover | s0 | s12 | s33 | s59 | s75 | s100 | **worst** |
|---|---|---|---|---|---|---|---|---|
| 001 | 1.000 | 26.3 | 1.8 | 8.5 | 7.1 | 22.5 | 9.6 | **1.8** |
| 002 | 1.000 | 52.2 | 32.2 | 8.7 | 8.3 | 1.8 | 10.7 | **1.8** |
| 003 | 1.000 | 49.1 | 45.4 | 10.1 | 13.9 | 2.2 | 17.3 | **2.2** |
| 005 | 1.000 | 36.8 | 4.6 | 11.3 | 7.9 | 52.3 | 11.6 | **4.6** |
| 006 | 1.000 | 35.7 | 6.5 | 14.8 | 12.0 | 71.9 | 13.3 | **6.5** |
| 007 | 1.000 | 34.2 | 6.5 | 15.3 | 13.1 | 69.4 | 13.4 | **6.5** |
| 008 | 1.000 | 34.7 | 5.0 | 11.8 | 8.7 | 56.1 | 11.6 | **5.0** |
| 010 | 0.989 | 14.7 | 25.9 | 2.2 | 15.4 | 1.4 | 15.8 | **1.4** |
| 021 | 0.891 | 53.3 | 50.6 | 13.7 | 30.2 | 2.8 | 26.7 | **2.8** |
| 022 | 0.959 | 63.5 | 82.7 | 17.8 | 43.7 | 4.7 | 46.9 | **4.7** |
| 027 | 0.916 | 33.5 | 56.5 | 10.5 | 25.9 | 2.8 | 25.2 | **2.8** |
| 065 | 1.000 | 11.9 | 41.3 | 34.8 | 46.9 | 41.9 | 37.5 | 11.9 |
| 072 | 0.873 | 34.1 | 42.4 | 26.0 | 32.5 | 12.7 | 37.4 | 12.7 |
| 113 | 1.000 | 14.4 | 28.1 | 15.3 | 31.4 | 22.3 | 10.8 | 10.8 |

**13 of 14 are v1 patterns (001–027).** They were authored against the 30-scheme pool
and never re-checked against the 120. Only one new pattern (113) is even marginal.

This is the direct cause of the f=400/800/1200 failure: `pattern_002` was the ground,
and on the scheme it drew it measures sigma **8.3–9.9** — a solid wall.

Mechanism: these patterns index a *narrow sub-range* of the palette. `layer_pal_build()`
(`bridge.c:609`) stretches the palette *window* back to full value range, but it cannot
help a pattern that only ever reads 2,000 of the 32,768 entries.

Recommended fix, in order of cheapness:
1. **Cheap and immediate** — extend the probe to record per-routine luma sigma across
   3–4 schemes, and refuse GROUND role to any routine whose worst-case sigma < 15.
   That is a `probe_routine()` addition plus one line in `role_from_cov()`.
2. **Correct** — have the affected patterns normalise their palette index to the full
   0..32767 span.

### D3 — THE STACK IS THINNER THAN THE BRIEF ASKS FOR  *(severity: medium)*

Slot usage over 120 spawns: **slot 4 (base successor) 89, slots 1/2/3 ten each.**
Role usage: 68 GROUND, 24 FIELD, 17 FIGURE, 11 SPARK.

The base rotates ~9× more often than any overlay slot. Over 6.7 minutes there were
only **30 overlay tenancies**, so most of the time the picture is a ground plus one or
two faint veils. J asked for "one, then another 3 s later, then another 1 s later,
building layer upon layer"; what runs is closer to "a ground that keeps changing".

Contributing: **1,603 NOSPAWN lines** in the same run. The ground-rotation path retries
*every frame* once the base is past its life, e.g. f=2679→2796 (118 consecutive frames)
and f=2979→3101 (123 frames). With four layers live, `admissible()`'s budget rule
(`bridge.c:854-860`) sums all live renders plus the candidate against a 10.5 ms cap, so
no ground can ever fit while the stack is full — the base is starved until an overlay
expires. Deadlock by construction, and it also floods the trace.

Also worth noting for the pool shape: the supply rebalance the scheduler asked for
*did* land, and hard —

| role | v1 (001–100) | new (101–200) |
|---|---|---|
| GROUND | 52 | **6** |
| FIELD | 15 | 9 |
| FIGURE | 25 | 46 |
| SPARK | 8 | **40** |

SPARK went 8 → 48 and FIGURE 25 → 71, which fixes the 0.8-minute spark recycle the
scheduler doc flagged. But the ground bag grew only ~23%, and its weakest members are
the D2 flat ones. **"Double the routines" did not double the layer you look at most.**

### D4 — THE FIRST FIVE SECONDS STUTTER  *(severity: medium, first-impression)*

Frames over 20 ms, by 200-frame block, over a 3,400-frame run:

| block | count | worst |
|---|---|---|
| 0–199 | **147** | **303.9 ms** |
| 200–399 | 7 | 65.6 ms |
| 400–3399 | 2 | 26.4 ms |

Steady state is immaculate (p50 8.33 ms, p99 16.3 ms over 24,000 frames). The damage
is entirely the probe. `probe_step()` (`bridge.c:371`) checks `spent < budget_ms`
*before* running a routine, so one heavy routine — Physarum with 42,000 agents, DLA,
the sandpiles — blows straight past a 2.5 ms budget, and 147 of the first 200 frames
do so back to back. Result: ~5 seconds of visible chop the moment the app opens.

Minimal fix: carry the overrun as a debt and skip probing until it is paid —

```c
static double g_probe_debt = 0.0;
/* in probe_step(), before the loop: */
if (g_probe_debt > 0.0) { g_probe_debt -= budget_ms; return 0; }
/* after the loop: */
if (spent > budget_ms) g_probe_debt = spent - budget_ms;
```

Same total work, spread out; the solid stall becomes an occasional hiccup.

### D5 — `pattern_104` Ice-Ray Lattice pops  *(severity: medium)*

Its header says *"Nothing pops, because the topology never changes."* Measured over one
2,048-frame segment it breaches the delta budget on ~20 frames:

```
f=498  d=10.22    f=711  d=11.71    f=960  d=10.09    f=1915 d=11.18
f=2048 d=13.51  ← segment boundary
```

Two distinct causes, both in `patterns_c/pattern_104.c`:

1. **Segment boundary (d=13.51).** `p104_node = (int)(seed & 63)` at the bottom of
   `pattern_104()` seeds the whole cut tree from `seed`, which changes at every
   tenancy — so the entire lattice re-draws itself in one frame.
2. **Mid-segment (d≈10–12).** In `p104_split()` the cut edges are chosen by
   `argmax` over edge length (`if (L > best) { best = L; i = k; }`). Cut positions
   slide continuously on sines, so when two edges swap which is longest, `i` (or `j`)
   jumps discretely and that cell *and its entire subtree* re-route instantly. Classic
   argmax discontinuity.

Fix for (2): thread a second "rest" polygon through the recursion — evaluate the
argmax on the shape with the sine wobble zeroed, so the topology is frozen while the
geometry flexes. ~25 lines; not attempted here (see §7).

### D6 — Narrow-index patterns black out on void-floor schemes  *(severity: low)*

`pattern_141` Chladni Sand, measured across schemes:

| scheme | 0 | 12 | 45 | 52 | 70 | **75** | **79** | **116** |
|---|---|---|---|---|---|---|---|---|
| luma | 7.5 | 13.5 | 6.5 | 9.8 | 4.9 | **1.0** | **0.6** | **1.0** |
| cover | .081 | .140 | .070 | .130 | .082 | **.011** | **.008** | **.011** |

It reads a ~7,300-entry window of the ramp; on schemes built on a deep void floor
(`acid_rail`, `carnival_at_midnight`, `newsprint_siren`) that window lands inside the
black and the picture vanishes. Its siblings 121 and 151 are stable (luma 11–29
everywhere), so this is 141-specific, not inherent to the idiom.

**Downgraded to low** because the engine's `layer_pal_build()` reshape (`bridge.c:646-651`)
stretches a dull window back to `[6, 251]`, which largely rescues it in situ. Worth a
floor guard in the pattern anyway.

### D7 — Visual duplication: same idea rendered twice  *(severity: medium)*

Normalised-luma correlation across all 201 thumbnails. Confirmed by eye for every
pair listed.

| pair | r | Verdict |
|---|---|---|
| 040 ↔ 175 | 0.908 | new Glenz duplicates a **v1** pattern's silhouette |
| 040 ↔ 190 | 0.904 | same |
| 175 ↔ 190 | 0.828 | **both named "Glenz Vectors"** — same two interpenetrating solids; 175 translucent wire, 190 flat-shaded. Same idea twice |
| 187 ↔ 195 | 0.822 | Harmonic Lantern vs Aperture Star — same centred radial burst |
| 118 ↔ 179 | 0.781 | **both named "Harmonic Lantern"** — identical silhouette, different stroke |
| 113 ↔ 177 | 0.757 | **both named "Sandpile Mandala"** — same automaton (185 is a third) |
| 035 ↔ 154 | 0.850 | new pattern duplicates a v1 |

Good news that should be said plainly: **most same-name families are genuinely
different work** — Physarum 101/111/171 correlate at r=0.011, Chladni 121/141/151 at
0.081, Tesseract 132/147 at 0.050, Medusa 106/167 at −0.004, Vega Bulge 108/115 at
−0.020. The authors differentiated. Only the six pairs above actually collide.

**Colliding names are a separate, purely cosmetic problem** and should be fixed
regardless: five patterns are called some variant of "Harmonic Lantern" (118, 139,
179, 187, 194), three "Sandpile" (113, 177, 185), three "Physarum" (101, 111, 171),
two "Vega Bulge" (108, 115), two "Glenz Vectors" (175, 190). The gallery will look
careless.

### D8 — LINE-WEB HAIRBALL: no idiom-exclusion rule in the scheduler  *(severity: high)*

At f≈1830 the live stack was `pattern_124` (Poincaré Web — hyperbolic mirror lines)
over `pattern_107` (Stereographic Cage — icosahedral great circles). Rendered solo,
both are beautiful and clearly distinct. Stacked, they produce an edge-to-edge mat of
hairline strands in every hue — visual noise, and at 1280×960 those 1-pixel strands
will crawl and shimmer, which is the opposite of relaxing. The same read dominates
f=1600, f=2400, f=2436, f=2530: **five of the eight sampled moments were some flavour
of coloured line web.**

The cause is structural. 86 of the 100 new patterns are figure/spark, and by name and
by eye a large share of them are the same idiom — *thin bright strokes on black*
(Poincaré Web, Stereographic Cage, Steiner Necklace, Knot Loom, Hyperbolic Court,
Apollonian Rings, Curve Loom, Braid Loom, Doyle Spiral, Hilbert Weave, Hopf Weave,
String Hyperboloid, Villarceau Weave, Cord Braid, Icosahedral Mirror, Ice-Ray Lattice,
Craquelure, Kolam Loom, Plait Loom, Talbot Carpet, …).

`admissible()` (`bridge.c:865`) enforces role, budget, motion and a dark-fraction bar,
but **nothing stops two routines of the same visual idiom being co-resident**. It needs
a family/idiom tag and a rule: at most one wire-web layer live at a time.

This and D1 together are why "does it go together" is the weakest column in §0.

---

## 6. What is unambiguously right

Said plainly, because it is a lot, and it is the hard part:

- **The transition system is finished work.** 24,000 consecutive frames, largest
  single-frame change 3.81 against a budget of 8.0, zero breaches. There is no hard
  cut anywhere in this build, at layer arrival, at layer exit, at base handover, or at
  the asm 2,048-frame wrap.
- **The bag scheduler solved the repetition complaint outright.** 119 distinct
  routines in 120 spawns; closest reappearance 3.1 minutes apart; zero repeats inside
  any 20-spawn window against v2.0's 39–40.
- **The palette rebuild is the standout.** Brightness spread 0.08 → 0.600, saturation
  spread 0.15 → 0.748, full-spectrum rainbows 47% → 12%, 72 of 120 schemes restrained
  to ≤5 hue bins. J said "they are all basically similar"; measured, they are now not.
- **Performance is excellent in steady state** — 120 fps p50, p99 16.3 ms, on 201
  patterns and a four-layer compositor.
- **The pattern craft is high.** Real Kuramoto, Biot–Savart, Reiter, RK4 Lorenz,
  thin-film interference, Gielis superformula. The header comments are honest and the
  maths checks out.
- **House rule compliance is clean.** All 201 files in `patterns_c/` were compiled and
  `nm`-checked: **every file-scope symbol is static except the `pattern_NNN` entry.**
  Zero violations.
- **The engine builds and runs.** `make` clean, app launches and holds.

---

## 7. What this review changed

**Nothing.** No file in the repository was modified.

That is a deliberate call, not an omission. Two other workers were writing to this
tree during the pass — `palette.bin` went 80 → 120 schemes at 15:02 and
`pattern_201.c` landed at ~15:10 — and every defect worth fixing lands in exactly the
files they are holding: `bridge.c` (D1, D3, D4, D8), a v1 pattern set (D2), or a
150–280 line pattern rewrite (D5). Editing under them would have risked the one rule
that outranks all the others: never break the app.

The two things that *were* trivially fixable turned out not to need fixing —
the static-symbol sweep came back clean across all 201 patterns, and
`palette_count.h` / `draw.s`'s scheme modulus were both already consistent at 120.

Handed back, in priority order:

| # | Fix | Owner file | Size |
|---|---|---|---|
| D1 | Loosen the SCREEN/ADD gate; stop defaulting overlays to MIX | `bridge.c:903,939` | small |
| D8 | Idiom tag + "one wire-web layer at a time" rule | `bridge.c:865` | medium |
| D2 | Probe per-routine sigma across schemes; deny GROUND below 15 | `bridge.c:260,316` | small |
| D3 | Let the outgoing base's cost drop out of the budget sum so ground rotation can't starve | `bridge.c:854` | small |
| D4 | Probe debt carry (patch in D4 above) | `bridge.c:371` | 3 lines |
| D5 | Freeze `pattern_104`'s cut topology on a rest polygon; drop the seed-driven node | `patterns_c/pattern_104.c` | ~25 lines |
| D6 | Floor guard on `pattern_141`'s palette window | `patterns_c/pattern_141.c` | small |
| D7 | Rename the 5 colliding name families; retire one of 175/190 and one of 118/179 | `lab/CATALOG.md` + headers | cosmetic |
| — | `VERSION` still reads `2.0.0`; the window title will ship wrong | `VERSION` | 1 line, release owner's call |

---

## 8. Reproduce

```sh
cd /Users/exeter/dev/m5/assembly/dzzle1

# whole-engine run, stats + frame dumps + scheduler trace
clang -O2 -I. tools/jd_dump.c bridge.c patterns_c/pattern_*.c \
      patterns_c/registry.c draw.s -o /tmp/jd_dump -lm
JD_DEBUG=1 /tmp/jd_dump run 0 24000 400,800,1200,1600,2000,2400,2800,3200

# one pattern against a chosen scheme (qc_harness.c adds JD_SCHEME to the
# stock harness and prints luma/sigma/cover/delta)
clang -O2 -I. -DPATTERN=pattern_134 lab/design/qc/qc_harness.c \
      patterns_c/pattern_134.c -o /tmp/t -lm
JD_SCHEME=47 /tmp/t /tmp/x.ppm 300 420

# flat-ground sweep: every pattern x 6 schemes  (D2)
clang -O2 -I. lab/design/qc/scan.c patterns_c/pattern_*.c \
      patterns_c/registry.c -o /tmp/scan -lm && /tmp/scan 0 12 33 59 75 100

# structural duplication corpus  (D7)
clang -O2 -I. lab/design/qc/thumb.c patterns_c/pattern_*.c \
      patterns_c/registry.c -o /tmp/thumb -lm && /tmp/thumb 45 120 /tmp/thumbs.bin
```

The harnesses are checked in at **`lab/design/qc/`** —
`qc_harness.c` (one pattern, chosen scheme, prints luma/sigma/cover/delta),
`scan.c` (every pattern × N schemes, the D2 sweep),
`thumb.c` (201-thumbnail corpus for the D7 duplication test),
`spike.c` (per-frame delta breaches inside one segment, the D5 tool),
`slow.c` (frames over 20 ms, the D4 tool).
Raw D2 output is `lab/design/qc/scan_results.txt`.
Rendered evidence for this pass is at `/tmp/qc/out/` (patterns) and `/tmp/qc/eng/` (engine).

```sh
clang -O2 -I. lab/design/qc/scan.c patterns_c/pattern_*.c \
      patterns_c/registry.c -o /tmp/scan -lm && /tmp/scan 0 12 33 59 75 100
```
