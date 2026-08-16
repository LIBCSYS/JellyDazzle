# Arrivals — layer entrance behaviours (J, 2026-08-15)

Today every layer fades in and out IN PLACE. J's ask adds a new axis: layers
that ARRIVE. Two behaviours, verbatim intent:

## 1. The breeze
"every now and again, a breeze came, and blew new content onto the screen"

- Occasionally (not every segment — rare enough to be an event) a new layer
  enters by being CARRIED across the frame: it slides in from one edge on a
  slow, eased path, with a gentle sway (a breeze wanders, it doesn't move in a
  straight line), and settles into place.
- Compositor-level: an entrance mode on the layer — position offset animated
  over ~4-6 s with ease-out plus a low-frequency lateral wobble. The routine
  itself is unchanged; the compositor blits it with an offset and a soft edge
  (feathered leading edge so it doesn't wipe in like a slide).
- Direction random per event; timing tied loosely to the beat when audio is
  live (a gust on a downbeat), free-running otherwise.
- Motion budget still applies: delta < 8. A breeze is slow.

## 2. Depth arrival with a wake
"new shapes could create new content from the back of the screen moving to the
front, leaving a wake of color"

- A layer enters SMALL and dim at centre ("the back"), grows and brightens as
  it comes forward over ~5-8 s, and as it travels it leaves a WAKE: the frames
  it passed through persist briefly behind it as fading colour trails.
- Compositor-level: scale-from-centre entrance (render the routine into its
  buffer, then blit scaled 0.15x -> 1.0x with bilinear taps; scale drives alpha
  too). The wake = a persistent trail buffer that the compositor multiplies by
  ~0.96/frame and adds the arriving layer into each frame, so past positions
  linger as colour. Wake tint from the layer's own palette so it "goes together".
- Cost: one extra full-frame buffer + a decay pass (cheap: multiply-add). Cap
  at one wake at a time.

## Where it lives
Both are compositor entrances (src/engine/compositor.c), NOT patterns — so all
600 patterns get them for free. Rare by design: an entrance mode is picked at
spawn time with low probability (breeze ~1 in 8 spawns, depth-arrival ~1 in 10),
so most layers still fade in place and the events feel special.

## Status
Captured during the v2.4 fleet run; compositor.c is owned by the audio agent
this run. Implement in the follow-up pass once v2.4 gates — do not collide.

## 3. Spin that slows to a crawl and reverses (J, 2026-08-15)
"there is a 360 angle of spin in all directions... I wonder if sometimes a
spin can slow to a crawl and reverse?"

- Yes, and it will look good: sine-DRIVEN angles (a few existing patterns use
  them) already pendulum — accelerate, slow to a standstill, reverse — and it
  is the most organic motion in the app. Make it a deliberate, engine-wide
  behaviour instead of an accident.
- Mechanism: a per-layer WARPED CLOCK. Instead of handing patterns `frame`
  linearly, the compositor integrates a signed velocity v(t) and hands the
  pattern the accumulated warped frame. Patterns that spin with the clock then
  reverse for free — no per-pattern edits across 600 files.
- Three flavours:
    pendulum  v(t) = sin(slow)         — swings, never stops, very natural
    tide      steady, then every 1-3 min ease v -> ~0 over ~5 s, hold a beat,
              ramp back up NEGATIVE     — "the crawl" is the beautiful part
    beat kiss with audio live, a downbeat nudges v; a big hit can flip it
- RULE: only ONE layer reverses at a time, and slowly. Two at once reads as
  jitter; one easing to a stop while the rest glide reads as intent.
- Motion budget applies during the reversal too (delta < 8): the ease is the
  guarantee.
- Same home as the entrances: compositor.c, follow-up pass after v2.4 gates.

## 4. Cogs and gears (J, 2026-08-15)
"using various sized cogs, gears and what not"

A pattern FAMILY, not an entrance — and it ties directly to the spin ideas:
meshed gears MUST counter-rotate, with speeds locked to their tooth ratio
(w1*n1 = -w2*n2). That constraint is what makes gear motion read as RIGHT.

- Geometry: a gear = circle radius r with n teeth as a low-amplitude periodic
  bump on the radius (trapezoid or sine profile), optional hub hole, spokes or
  solid disc. All integer/table math: angle via octant atan approx, radius
  test against r + tooth(angle*n + phase). Cheap.
- Trains: 2-7 gears placed so pitch circles touch; the engine solves rotation
  from the first gear (driver) down the chain, alternating direction. Sizes
  vary widely (a 60-tooth ring wheel driving a 9-tooth pinion). Planetary
  sets (sun + planets inside a ring) look spectacular in kaleidoscope folds.
- Variants for the roles: sparse silhouettes over black (FIGURE — cogs as
  overlays, lower layers show through the holes); full-frame gear fields
  (GROUND — a wall of clockwork); huge slow single gears at the edge (FIELD).
  Symmetric fold variants (gear mandalas, 6/8-fold).
- Colour: from pal, tooth-face vs body vs hub as three palette offsets so a
  gear reads as an object with shading; brass/steel/copper palettes flatter it.
- Motion: driver speed slow; and this family is the PERFECT host for the spin
  reversal (§3) — the whole train easing to a crawl and reversing together is
  exactly how real clockwork feels when it changes direction. Audio: bass can
  push the driver; a beat can be the "tick".
- Homage note: the original DAZZLE had wireframe polygon routines; gears are
  the modern, mechanical cousin — same era of joy.

Target: 20-30 patterns in the next content pass (a "clockwork" range).

### 4b. Colour has no rules (J, 2026-08-15)
"each tooth might be a slightly different and morphing color."

The mechanics are constrained (meshed gears counter-rotate at locked ratios);
the COLOUR answers to nothing. Design accordingly:
- Per-tooth hue: each tooth owns a palette offset that drifts on its own slow
  clock (tooth k on gear g: off = base_g + k*step_g + sin(t*rate_gk)*amp), so
  no two teeth match and none holds still. Faces vs flanks vs body still get
  distinct offsets so the gear reads as an object with shading.
- COLOUR TRANSFER AT THE MESH: when tooth i of gear A engages tooth j of gear
  B, blend a portion of A's tooth colour into B's (and vice-versa) — the pinion
  then carries it around and hands it to the next gear it touches. Colour flows
  through the train like a fluid through machinery. Chromatically lawless,
  mechanically exact — precisely the JellyDazzle move.
- Cheap: per-tooth offset is one small array per gear; transfer is one blend
  per mesh per frame.
- With audio: a beat can inject a hot hue at the driver's teeth and let the
  train carry it outward.

## 5. Lightning colour rule (J, 2026-08-15, on seeing the first bolts)
"looks cool, should not always be white, should have multiple morphing hues,
all of them should"

APPLIES TO EVERY LIGHTNING PATTERN (469+ range) and retroactively to any
lightning already in the library:
- No white-by-default. Core, halo and branches take palette offsets; the core
  may be brightened toward white as an ACCENT but the hue must be present.
- Multiple hues per bolt: colour varies ALONG the bolt (per segment / per
  branch depth) and MORPHS over the bolt's life (offset drifts as it grows and
  fades). Different bolts in one storm carry different hues.
- Where a bolt forks, branches can diverge in colour from the trunk.
- Sheet/ball/ribbon variants: same rule — the wash has a hue field, not a
  brightness field.
This is the family-wide contract, same spirit as gears (§4b): colour has no
rules.

## 6. Matrix rain, from all sides (J, 2026-08-15, "lol")
"one of the segments would be like a matrix thing falling from all sides"

Not a joke — a natural JellyDazzle overlay:
- Streams of glyphs fall from ALL FOUR EDGES toward the centre (and a
  kaleidoscope-fold variant where they fall inward along 6/8 spokes). Where
  streams meet in the middle they can pass through, or pile and dissolve.
- Glyphs: no font — a small 8x8 bank of invented symbols (32-64 shapes) as
  bit patterns; a stream is a column of glyphs whose head is bright and whose
  tail decays. Occasionally a glyph mutates in place (the classic flicker,
  but SLOW — motion budget delta < 8, no strobe).
- Accumulator: persistent canvas, trail decay per frame; sparse over black so
  lower layers show through — FIGURE/SPARK role, sometimes FIELD when dense.
- COLOUR RULE applies: not green-only. Each stream owns a drifting palette
  offset; hue morphs along the stream (head hot, tail cooler) and per stream.
  A "classic" all-green moment is allowed as a rare wink, not the default.
- Audio: streams speed up with level; a beat spawns a burst of new streams from
  a random edge.
- Cost: trivial (glyph blits + decay pass).
Target: 4-6 variants in the next content pass (four-edge, spokes, single-edge
heavy, glyph-mutation slow, dense field, sparse spark).

## Bring your own image (J, 2026-08-16)
Let a user drop their own pictures in and have the engine work them into the scene —
"say I wanted to have a picture of Amanda floated into the screen saver."

Design sketch (target 2.4.b):
- Watch folder `~/Pictures/JellyDazzle/` (and a picker in the command panel).
- Each image becomes a FIGURE-role sprite: decoded once, downsampled to a luma mask
  plus a low-res RGB, then drawn through the same palette window as everything else —
  so it drifts, scales, spins and re-tints with the current scheme instead of sitting
  on screen as a flat pasted photo.
- Two treatments: (a) TRUE COLOR — the photo's own colors, floated and blended;
  (b) DAZZLED — the photo's shape only, filled from the active palette so it belongs
  to the frame. Randomize between them.
- Same lifetime rules as any other overlay (a turn of its own length, fade in/out),
  and the same weight from the panel — so a user can say "more Amanda, less cat."
- Privacy: images never leave the machine, never bundled into the app, never in git.

## Depth exchange — the foreground falls back (J, 2026-08-16)

Watching a rotating tiled figure in 2.5.0, J: *"I wondered if it could fly apart
and reveal what's underneath. I often wish the foreground animation would fall
back and become the background, letting what was in back become the foreground."*

This is a **transition between layers rather than within one**, and we have
nothing like it. Today a layer fades out and a new one fades in; the stack order
never changes and nothing is ever *revealed*.

Two halves, and they are separable:

**1. Fly apart.** The outgoing figure breaks into pieces that scatter outward and
fade, uncovering the layer beneath as they go. Cheap version: modulate the
layer's per-pixel weight by a moving radial or cellular mask so it erodes rather
than dims — dissolve with structure. Richer version needs the pieces to be real
(cells with velocity), which suits the `C_INDEXED` class in
`PALETTE_CYCLE_CLASS.md`: an index plane can be diced into regions and each
region given a drift, because it is a static image being *transformed*, not
redrawn.

**2. Depth exchange.** The FIGURE recedes — shrinks slightly, loses contrast,
drops in the stack — while the GROUND rises to meet it. They swap slots. The
composition keeps both routines but reverses which one commands attention.

Why it is worth doing: it answers the thing that makes our output feel flat
compared to the original. DAZZLE piled patterns up and then cleared them, so
there was always a sense of accumulation and release. We cross-fade, which is
smooth but eventless. A swap is an *event* — the viewer notices something
happened, and it costs nothing but scheduling.

Note the ordering constraint: the compositor blends by brightness with slot 0 at
the bottom, so a genuine swap means exchanging the layers' slot indices AND
their blend modes, not just their draw order. Do it during a fade so no frame
shows the stack mid-flip.

## Beams that live — grow, narrow, curve, tint, go translucent (J, 2026-08-16)

Watching the radial beam/corridor figure: *"I wonder if it can get bigger,
narrower, and curve in other directions, along with change colour and be
translucent."*

Four independent axes, and none of them exist today — a beam pattern picks its
width and direction at spawn and holds both for its whole turn:

1. **Scale over life** — the fan opens and closes across the turn instead of
   being fixed. Cheap: modulate the radius/length term by a slow eased envelope.
2. **Width independent of scale** — beams narrowing to blades while the figure
   itself grows is the interesting combination, and it is what makes something
   read as *moving through space* rather than being rescaled.
3. **Curve** — beams as arcs rather than straight rays, with the curvature
   itself drifting. This is the single biggest visual change; a straight fan is
   a starburst, a curved fan is a vortex.
4. **Translucency** — currently a beam is opaque where drawn. Giving it real
   alpha would let two beam layers cross and *both* survive the intersection,
   instead of the brighter one simply winning under MAX blend.

(4) is the one with engine implications: it wants a blend mode that is not MAX,
which is close to the depth-exchange note above. (1)-(3) are per-pattern and
could be prototyped in a single new routine without touching the compositor.
