# Gap analysis — the observed DAZZLE spec vs. what JellyDazzle actually does

Source: `DAZZLE_OBSERVED_SPEC.md` (eyewitness description of DAZZLE 5.0 running).

Reading it against our engine, JellyDazzle is not a near-miss with a few features
outstanding. On several axes it is doing the **opposite** of the original, deliberately,
because of decisions taken early and never revisited. That is worth knowing before 2.5
picks a direction.

## Where we are the opposite

| | DAZZLE 5.0 | JellyDazzle 2.4 |
|---|---|---|
| **Mark-making** | 1px hard-edged lines. Almost **no filled shapes**. Solid-looking areas are hundreds of parallel lines packed 1–3 px apart | Per-pixel continuous fields and Gaussian-capsule glows. Almost **no lines** |
| **Aliasing** | Jaggies are *the aesthetic* — deliberate, and the moiré comes from packed lines beating against the pixel grid | Smoothness is our stated motion law. We spent a whole review cycle eliminating hard edges |
| **Clearing** | Screen is **never cleared** between patterns. 3–6 patterns pile on top of each other, then a fade wipes the lot | Every layer repaints per frame; only accumulators persist, and only within their own turn |
| **Composition** | Patterns pile up **until almost too busy**, then reset | We cap at four layers and actively police coverage so nothing gets too dense |
| **Colour** | Sequential palette walk: index advances a fixed step **per line**, so a swept family becomes a rainbow band | Palette *windows* per layer, advanced per frame. No per-primitive stepping |
| **Background** | A palette entry that flips between sets — royal blue, mint, teal, light grey | Effectively always black |
| **Build-up** | Each pattern draws **progressively over 1–3 s**; you watch lines accumulate | Repaint patterns appear whole; only accumulators build |
| **Transitions** | Fade-to-black via palette, split wipe, pan-off, dissolve | One cross-fade, every time (see `TRANSITIONS.md`) |

## The uncomfortable bit

J's complaint about the 400 generated patterns was "jagged edges, looks amateurish."
The original is **entirely** jagged. So the objection was never to aliasing as such — it
was to *unstructured* aliasing. DAZZLE's jaggies are periodic: packed parallel lines at
a controlled spacing, producing intentional moiré. The rejected patterns had incidental
aliasing from crude polygon fills, which reads as a rendering defect rather than a
texture. Same pixels, opposite intent.

That distinction should drive 2.5. "Add line-art primitives" is a legitimate direction.
"Stop antialiasing everything" is not — not without the periodic structure that makes it
read as deliberate.

## What is genuinely missing, ranked by payoff

1. **Line-sweep string art** (spec calls it the workhorse, "nearly every frame"). Two
   endpoint paths, N straight lines between interpolated points; the envelope curves
   even though every line is straight. We have `A21 string-art fans` in assembly and
   nothing comparable in C. **Highest payoff for the least work.**
2. **Transition repertoire** — already specced in `TRANSITIONS.md`.
3. **Per-line palette stepping** — a colour-index step of 1–17 per primitive, rather
   than per frame. This is what produces DAZZLE's signature rainbow sweeps, and it is a
   small change to how patterns sample `pal`.
4. **Non-black backgrounds** between sets. Cheap, and it would transform the feel.
5. **Progressive draw-in** for repaint patterns, not just accumulators.
6. **The remaining primitives**: nested rects, nested diamonds/chevrons, ring clusters,
   radial fans, staircase polylines, diagonal rainbow bars, weave/hatch, wedge fans.

## Two routes, and they are not the same project

**(a) Fold the insights into JellyDazzle.** Add a line-art pattern family that obeys the
existing contract, add transitions, add per-line palette stepping. Keeps one product.
Risk: bolting a hard-edged 1992 aesthetic onto a smooth continuous engine may just look
incoherent, and the smooth look is the thing that got praised.

**(b) Build the faithful recreation separately.** Part 2 of the spec is a ready-to-paste
build prompt for a self-contained HTML/canvas version — 640×400, pixelated, nine
generators, the full sequencing loop. That is a couple of hours of work, runs in a
browser, and would sit beautifully on dazzle.jelia.nyc next to the tribute page: *here is
the homage, and here is the original's actual grammar, faithfully.*

Recommendation: **(b) first.** It is fast, it is separable, it settles what the original
grammar really feels like at full speed, and whatever we learn from it can then inform
(a) with evidence instead of speculation.


---

## Second capture (83 s) — the biggest finding yet

`~/Desktop/.tmp.driveupload/685653`, 42 frames sampled. Contact sheet:
`dazzle_capture_palette_cycle.png`.

**Eight of twelve sampled frames are the same composition.** The geometry does not move
at all — same bars, same wing spikes, same corner ellipse clusters, pixel for pixel.
What changes is the colour: yellow/green -> red/purple -> magenta/cyan -> green/cyan ->
blue/red, sweeping continuously.

That is **palette cycling on a held static image**, and it is not a minor flourish — on
this evidence it is the engine's dominant animation mode. Shiflett drew a frame once and
then animated it for ten or fifteen seconds by rotating the DAC. No pixels were touched.

### Why this matters more than anything else in this document

JellyDazzle **redraws every pixel of every layer, every frame, at 60 fps.** The original
mostly drew *once* and then let the hardware colour-cycle. Those are opposite
architectures, and it explains several things at once:

- why the original could look this rich on an 8088 while we need an M-series chip;
- why its colour moves in smooth continuous sweeps (a palette rotation is inherently
  continuous) while ours has to be carefully enveloped to avoid strobing;
- why its geometry reads as *crystalline and still* while ours reads as *flowing*.

We already have the ingredients: a 32768-entry palette, a walk offset, and accumulator
patterns that hold a canvas across a turn. What we have never done is **stop redrawing
and just move the palette**. A pattern class that renders once, stores an index buffer,
and then animates purely by advancing the palette offset would be cheap, would be
completely strobe-free by construction, and would be the single most authentic thing we
could add.

### Also confirmed by this capture

- **Non-black backgrounds are real.** The final frame is fireworks on a flat **magenta**
  field. The spec called this and the footage proves it.
- **Particle fireworks confirmed** — the effect the research attributes to Shiflett
  watching a *Star Trek* explosion.
- **Sparse resets confirmed again** — frame 3 is near-empty black with two small cyan
  chevrons before the next set builds.
