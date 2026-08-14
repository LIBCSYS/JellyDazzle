# DAZZLE.EXE — Footage Catalog (visual analysis of client reference frames)

Source: `/Users/exeter/dev/m5/assembly/dzzle1/reference/` — 100 PNG frames of the original DOS
dazzle.exe filmed off a monitor by the client. Frame series:

- **a01–a18** — close-up phone footage of one full-screen instance (heavy camera smear/moiré)
- **b01–b15** — close-up, wire-lattice era of the run
- **c01–c14** — close-up, windowed-DOSBox era (desktop visible at edges)
- **d01–d42** — close-up, many different routines incl. windowed white-canvas variants
- **f01–f11** — wide shots of the whole monitor: **~8 DAZZLE instances tiled at once in DOSBox 0.74
  windows** (title bars read "DOSBox 0.74, Cpu speed: 3000 cycles, Frameskip 0, Program: DAZZLE").
  These are the cleanest evidence: each tile is a different routine evolving across f01→f11.

## Filming artifacts to IGNORE (not part of the program)

- Vertical smearing/streaking (a05–a07: red/green "melting curtains") = camera motion blur +
  rolling shutter on CRT/LCD, not a routine behavior.
- Moiré banding, off-axis keystone, desktop chrome, ping terminal window in f-series.
- d26/d27 black frames = the program's own between-routine clear (useful evidence, see below).

## Global engine behaviors (consistent across all footage)

- **Resolution/look**: VGA 320×200 (or 320×240), 256-color palette, hard pixel edges. Almost
  certainly mode 13h with palette-register animation.
- **Kaleidoscope mirroring is the core trick**: nearly every routine stamps primitives through a
  symmetry operator — most often **4-fold mirror** (reflect across vertical + horizontal center
  axes, i.e. draw in one quadrant, copy to the other three), sometimes 2-fold (left/right only),
  sometimes 6-fold/12-fold rotational.
- **Persistent canvas / accumulation**: most routines do NOT clear per frame. Primitives pile up
  over seconds until the frame is a dense quilt (see fireworks d11→d17, lattice b01→b15,
  Sierpinski tile across f01→f08). Routines end with a **full clear to black** (d26, d27) or a
  clear to a flat color (solid blue tile in f07–f11) before the next routine starts.
- **Palette cycling is pervasive and provably separate from geometry**: a02→a03→a04 show the
  *identical* maze geometry recolored yellow→red→blue; d28→d30 the same oval geometry
  green→red; the f-series pinwheel keeps its shape while hue rolls green→red. Reproduce by
  rotating/remapping DAC palette entries, not by redrawing.
- **Multi-hue by default**: routines pull saturated rainbow colors (pure R/G/B/C/M/Y + ramps).
  A few routines are deliberately restricted (blue-only orbits c06, red/blue machine c01–c05,
  white-canvas variants d32–d40).
- **Pacing feel** at 3000 DOSBox cycles: incremental draw is visible — hundreds of primitives per
  second, a routine matures in ~5–20 s, then transitions.

---

## Routine catalog

### R1 — Diamond (rhombus) kaleidoscope
- **Frames**: a01
- **Geometry**: one large rhombus (diamond) centered on screen, edges are thick concentric
  outlines (cyan outer, magenta inner); interior filled with dense multicolor pixel noise/confetti
  detail, itself 4-fold mirrored; small horizontal streaks exit the left/right vertices.
- **Symmetry**: 4-fold mirror about center.
- **Motion**: concentric diamond outlines suggest outward zoom (new outlines born at center,
  expanding); interior sparkles/churns.
- **Color**: multi-hue interior; outline colors cycle (cyan/magenta/green rims).
- **Canvas**: accumulating interior, stable silhouette.

### R2 — Greek-key maze panel (palette-cycled)
- **Frames**: a02, a03, a04 (same geometry, three palette states)
- **Geometry**: a central filled rectangle covered in interlocking rectilinear "greek key"/maze
  motifs laid out on a grid; mirrored quadrant layout inside the panel; surrounding border of
  blue kaleidoscope wedges and small 6-lobed rosettes; fine horizontal rays shoot outward
  left/right behind the panel.
- **Symmetry**: 4-fold mirror; maze motifs repeat in a translational grid within each quadrant.
- **Motion**: geometry essentially static while the **whole palette rotates** — panel goes
  yellow-on-yellow → red accents on blue → all-blue with red keys across the three frames.
- **Color**: wholesale palette cycling; high-saturation primaries.
- **Canvas**: persistent; recolor without redraw.

### R3 — Cathedral fan / stairstep spires
- **Frames**: a05–a07 (smeared close-ups), mid-left tile in f01–f06
- **Geometry**: black background; a horizontal "vanishing line" across the center with a dense
  fan of straight rays converging to the center point (sunburst pinched at the horizon);
  stair-stepped blocky towers ("spires" made of stacked shrinking rectangles) rise above and
  hang below the centerline; small concentric arc sets in the outer corners.
- **Symmetry**: 4-fold mirror (left/right + top/bottom).
- **Motion**: rays sweep/accumulate around the center like a fan opening; spires grow step by
  step; arcs pulse. Slow build.
- **Color**: red/white/pink rays on black; green/gold accents; palette drifts (f06 shows the
  same tile gone dim olive — cycling through dark palette phase).
- **Canvas**: accumulating.

### R4 — Thread-web X (fine-line kaleidoscope)
- **Frames**: a08, a09, a10
- **Geometry**: flat purple background; bundles of fine, slightly-curved multicolor threads
  (1-px polylines) sweep corner-to-center forming X/butterfly crossings; small rainbow wedge
  fans parked at left/right edges; green pinwheel triangles at corners.
- **Symmetry**: 4-fold mirror.
- **Motion**: threads are added continuously (bundle orientation slowly rotates); web densifies.
- **Color**: each thread a different hue on constant purple ground.
- **Canvas**: accumulating on colored (not black) ground — the routine first floods the screen
  purple, then draws.

### R5 — Mirrored random-stamp collage (the workhorse)
- **Frames**: a11–a18, c07–c09, d04–d11; white-canvas variants d32–d40
- **Geometry**: random primitives — triangles, bars, checkered squares, rainbow-shaded blobs,
  concentric-ring "eyes", zigzags — stamped at random positions/sizes through the mirror
  operator, producing a dense symmetric quilt with a distinct rectangular "frame within frame"
  structure (central panel + border cells, like a mandala quilt).
- **Symmetry**: 4-fold mirror; stamps often ALSO repeat at ±x offsets giving a 4×-per-row look
  (a12–a18 show 3–4 columns of the same motif per row → likely mirror + horizontal tiling).
- **Motion**: no global motion; the composition churns as new stamps land (several per second).
  Old content is overdrawn, never erased, so the scene continuously mutates.
- **Color**: unconstrained multi-hue, frequently rainbow-gradient-filled shapes; background
  usually deep blue/purple, sometimes black, sometimes **white** (d32–d40 windowed variants —
  same routine on a white flood, pastel reading).
- **Canvas**: accumulating; periodic full reflood with a new ground color.

### R6 — Spirograph lattice tunnel
- **Frames**: b01–b15 (whole b-series is one long run of this routine mutating)
- **Geometry**: stacked horizontal "wire mesh" bands — rows of short vertical dashes forming
  curved sheets that bow toward the center like a perspective tunnel/vortex; spirograph loop
  chains (petaled epicycloid doodles) laid along the top and bottom edges; big multicolor
  gradient diamonds/pinwheels parked in the four corners; center holds a small intense core
  (rings or bar cluster).
- **Symmetry**: 4-fold mirror, strong left/right emphasis.
- **Motion**: mesh rows advance toward center (tunnel breathing); loop chains extend; core
  flickers. Reads as inward flow.
- **Color**: red/orange/blue dominant with green/pink corner diamonds; b03 shows the same scene
  gone cyan/yellow → palette cycling over the accumulated lattice.
- **Canvas**: accumulating; density rises steadily b01→b12.

### R7 — Red/blue ring machine ("H-frame")
- **Frames**: c01–c05
- **Geometry**: bold graphic composition, almost CGA-poster look: two large red concentric
  ring/arc stacks left and right (like giant parentheses "( )"), blue-to-magenta
  gradient-shaded rectangles forming an "H" / brackets in the center, a red X lattice of
  straight struts behind everything, small diamond at dead center.
- **Symmetry**: 2-fold left/right mirror (top/bottom near-symmetric).
- **Motion**: arcs redraw with slight rotation phase per frame (ring segments creep); central
  rectangles pulse in shading. Slow, mechanical feel.
- **Color**: restricted palette — pure red + blue/magenta gradient + white; black ground.
- **Canvas**: mostly persistent with local overdraw (composition holds shape across c01–c05).

### R8 — Orbit ellipses (routine opening state)
- **Frames**: c06
- **Geometry**: black screen, a single 4-lobed figure: two crossed dotted ellipses (electron
  orbit / atom shape) centered on screen, drawn in blue dots.
- **Symmetry**: 4-fold.
- **Motion**: early-accumulation frame — dots trace along elliptical paths; later frames of
  this routine likely densify (not captured further).
- **Color**: monochrome blue at this stage.
- **Canvas**: accumulating from clean black — direct evidence routines start on cleared screen.

### R9 — Scanline moiré butterfly
- **Frames**: c10–c13 (also right-mid tile family in f)
- **Geometry**: full-field horizontal-scanline interference: closely spaced horizontal lines
  whose spacing modulates to form large butterfly/diamond/bowtie silhouettes; c13 variant is a
  large yellow/green diamond ring on a multicolor noise quilt; c10 shows green/red version with
  central red spindle.
- **Symmetry**: 4-fold mirror.
- **Motion**: interference bands crawl (line phase shifts), silhouette slowly morphs; strong
  shimmering moiré feel (amplified by camera but real: banding is in the render).
- **Color**: gradient ramps across bands (green→yellow→orange, or blue→green); background black.
- **Canvas**: repaints in bands (line-by-line rewrite), reads as continuous animation rather
  than accumulation.

### R10 — Rainbow radial panel
- **Frames**: d01, d02
- **Geometry**: central rectangle filled with a smooth concentric rainbow gradient (nested
  rectangles/radial ramp: blue core → green → yellow → magenta rim); flanked by red filled
  circles at the corners, blue orb clusters, and layered chevron/wing motifs left and right;
  fine white spray lines under the panel.
- **Symmetry**: 4-fold mirror.
- **Motion**: gradient rolls (palette cycle through the ramp — d01 vs d02 shows ramp phase
  shifted); wings restack.
- **Color**: full spectrum ramp + saturated primaries.
- **Canvas**: persistent furniture, cycling fill.

### R11 — Fireworks
- **Frames**: d11 (fresh bursts) → d13–d17 (saturation) → d18 (burnt-out dark noise)
- **Geometry**: classic particle fireworks: radial bursts of 30–80 thin particle trails from
  random points in the upper 2/3 of screen; trails have slight gravity droop at the tips.
- **Symmetry**: NONE — the only clearly asymmetric routine; bursts at random x/y.
- **Motion**: each burst expands outward over ~1 s leaving permanent trails; several bursts
  per second.
- **Color**: each burst one or two hues (red, cyan, yellow, blue…) on a **hot magenta flood**
  (d11); as trails accumulate the field becomes a dense multicolor fibrous wash (d15–d16 —
  screen ~fully covered), then palette darkens toward mud (d17–d18) before clear.
- **Canvas**: strongly accumulating; this is the best proof of no-per-frame-clear.

### R12 — Neon chevron stair-lasers
- **Frames**: d19, d20, d21
- **Geometry**: black ground; central horizontal band of hot green/pink scanline blocks (a
  striped core rectangle); dense stair-stepped chevrons (V after V, pixel-stepped like LED
  arrows) marching diagonally in each quadrant toward the center; straight laser rays fanning
  from corners; d21 shows the routine's sparse phase: gold vertical pillars framing black.
- **Symmetry**: 4-fold mirror.
- **Motion**: chevrons march (new V's spawn at edges, step inward); rays sweep; core strobes.
  Fast, aggressive feel.
- **Color**: magenta/green/gold neon on black; palette strobes between magenta-dominant and
  green-dominant.
- **Canvas**: accumulating with periodic partial blanking (d21's black middle = fresh clear
  mid-routine or wipe band).

### R13 — Gear-flower quad
- **Frames**: d22, d23, d24, d25 (decay)
- **Geometry**: 2×2 array of large gear/sunflower rosettes (toothed disc with dotted "seed"
  core arranged in concentric dot rings) over a ground of diagonal blue/cyan stripes and
  stepped triangle borders; small mirrored motif column on the center vertical.
- **Symmetry**: 2×2 translational repeat × mirror (each rosette itself ~16-fold rotational).
- **Motion**: rosette teeth rotate slowly; palette swaps figure/ground (yellow-on-blue d22–d23
  → blue-on-yellow d24); d25 shows terminal state melted into uniform yellow-green mush —
  looks like a deliberate dissolve/decay (random pixel smear) ending the routine.
- **Color**: blue/cyan/yellow/green family.
- **Canvas**: persistent, palette-animated, dissolve at end.

### R14 — Oval racetrack rings
- **Frames**: d28, d29, d30
- **Geometry**: full-screen concentric oval (stadium/racetrack) rings centered on screen,
  drawn as thick multicolor bands; inside the innermost oval sit two horizontally-striped
  rectangles side by side (shaded bars, look like rolling drums); ring perimeter decorated
  with small mirrored blobs.
- **Symmetry**: 4-fold mirror.
- **Motion**: drum stripes roll vertically (palette cycle on the stripe ramp — green→red
  across d29→d30); rings shimmer outward.
- **Color**: acid green/yellow ground with blue/red rings (d28) shifting to blue ground with
  red/green drums (d30) — heavy cycling.
- **Canvas**: persistent, palette-animated.

### R15 — Rotational ring stamp (white canvas)
- **Frames**: d31
- **Geometry**: clean white screen; a single ring of ~12 identical triangular/angular motifs
  arranged with pure rotational symmetry around center (a "sun gear" of dancing figures),
  drawn in red and blue-purple with slight per-copy color offset.
- **Symmetry**: ~12-fold rotational (no mirror — motifs have consistent chirality).
- **Motion**: ring appears stamp-by-stamp or rotates while restamping (double red/blue edges
  = two phase-offset stamp passes).
- **Color**: red + blue/purple on white.
- **Canvas**: accumulating on white flood.

### R16 — Smooth pinwheel swirl
- **Frames**: f01–f11 top-left tile (largest, cleanest)
- **Geometry**: full-tile smooth-shaded pinwheel: 6 curved comma/wave arms spiraling out from
  center, rendered with dithered continuous shading (looks like a plasma/interference function,
  not line art); background black with leftover triangle stamps from previous routine visible
  behind it early on.
- **Symmetry**: 6-fold rotational.
- **Motion**: arms rotate slowly clockwise; arm shape breathes (spiral tightness modulates
  f04→f08); hue rolls green-dominant → red-dominant across f01→f11 with yellow rims —
  simultaneous rotation + palette cycle.
- **Color**: smooth ramps green/yellow/red/black.
- **Canvas**: full repaint per frame (no accumulation artifacts) — this one is a computed
  field, unlike the stamp routines.

### R17 — Hexagon tunnel
- **Frames**: f01–f07 second tile (purple)
- **Geometry**: dusty purple flood with sparse dark blotches; centered white/neon outlined
  hexagon (pointy-top) containing small mirrored glyphs (two arrow/eye marks); faint
  concentric hexagon ghosts around it suggest zoom steps; occasional tiny green sparkles at
  random positions; squiggly worm glyphs along the bottom.
- **Symmetry**: 2-fold mirror inside hexagon; hexagonal frame.
- **Motion**: hexagon outline pulses/zooms in discrete steps; sparkles twinkle; very sparse,
  slow, minimal routine — good contrast to the dense ones. In f08 this tile has cleared to
  black with a single red fuzzy blob → routines within a tile hand off over time.
- **Color**: near-monochrome purple + white outline + rare green/red accents.
- **Canvas**: persistent with slow decay/overdraw.

### R18 — Hex mosaic kaleidoscope
- **Frames**: f01–f05 center tile; close-up kin: a02–a04 panel style
- **Geometry**: dense edge-to-edge kaleidoscope built on a hexagonal/triangular lattice:
  nested hexagon rings around center, filled with fine tessellated triangles, key-maze
  micro-patterns, and rainbow striping; reads as a stained-glass hex mandala.
- **Symmetry**: 6-fold (hex) + mirror.
- **Motion**: micro-tiles reflow continuously (center-out waves); palette shifts
  green/blue-dominant → red inserts.
- **Color**: full RGB primaries at maximum saturation, highest density of any routine.
- **Canvas**: continuous overdraw, always full coverage.

### R19 — Interference ripple pond
- **Frames**: f01–f11 right-mid tile
- **Geometry**: full-field concentric elliptical ripple rings from 1–3 centers (main center
  mid-tile), fine 1–2 px ring spacing → strong interference/moiré; embedded fragments of
  previous imagery distort along the rings (looks like a feedback/displacement effect eating
  the frame); big soft green blob upper-left corner persists.
- **Symmetry**: radial around ripple centers, otherwise none.
- **Motion**: rings propagate outward continuously; the whole texture crawls; hypnotic and
  smooth. Center drifts slightly across f01→f11.
- **Color**: green/blue/black with white ring highlights.
- **Canvas**: feedback-style repaint (previous frame content visibly smeared into rings) —
  reproduce with radial displacement + slight blur/decay per frame.

### R20 — Sierpinski rosette fractal
- **Frames**: f03, f05–f08 bottom-center tile (f03: red star-of-David outline phase)
- **Geometry**: black ground; large equilateral triangle outlines subdividing
  Sierpinski-style (triangle-in-triangle), drawn as thin green/yellow lines; at triangle
  vertices sit small mandala rosettes (hex flowers of tiny circles, cyan/red/pink);
  phase seen in f03: a simple red hexagram (two overlaid triangles) alone before rosettes;
  f08 shows added cyan trapezoid "wings" and pink slabs — stamps layering onto the fractal
  scaffold.
- **Symmetry**: 3-fold/mirror (triangle) with 6-fold rosettes.
- **Motion**: scaffold draws first (line by line), rosettes stamped one by one at vertices,
  then bigger filled shapes layer in — a clear multi-stage build.
- **Color**: green/yellow lines, multicolor rosettes on black.
- **Canvas**: accumulating, staged.

### R21 — Circuit city collage
- **Frames**: f01–f09 bottom-left tile; close-up kin d19–d21 family
- **Geometry**: dense sci-fi collage: base strip of fractal checkerboard/stepped-pyramid
  tiling (Sierpinski-carpet-ish blocks) along the bottom, overlapping thin circles/arc rings
  mid-field, equalizer-bar blocks, diagonal laser rays, scribbled loop clusters left/right
  (cyan/pink), red arrow wedges lower corners (f09–f11 phase).
- **Symmetry**: 2-fold left/right mirror; bottom-heavy composition.
- **Motion**: constant stamp churn — arcs and bars appear a few per second; checker base
  grows cell by cell; later phase (f09–f11) big red wedges overpaint from the corners.
- **Color**: cyan/magenta/green neon on black, red accents late.
- **Canvas**: accumulating heavily.

### R22 — Sphere garden
- **Frames**: f01–f11 bottom-right tile
- **Geometry**: hot magenta/pink flood; pairs of glossy shaded 3D-looking blobs — toroids/
  spheres/crescents (green, red, teal with shading rings) — placed symmetrically left/right;
  thin white scribble polylines wander over the field; top edge holds small ornate framed
  panels (tiny mandala windows); green soft blob top-left.
- **Symmetry**: 2-fold left/right for the blob pairs; scribbles free.
- **Motion**: blobs re-render in place with shifting shading phase (they seem to rotate);
  scribbles extend slowly; framed panels flicker.
- **Color**: magenta ground, green/red/teal shaded objects — most "solid/3D" looking routine.
- **Canvas**: persistent ground with object overdraw.

### R23 — Flood-clear + slow sketch (transition behavior)
- **Frames**: f07–f11 mid-left tile (solid blue), f10–f11 bottom-left (glowing red wedge)
- **Geometry/behavior**: a tile floods to a single flat color (pure blue) and stays ~empty for
  many seconds while a faint sketch (dim red arcs, barely visible f10–f11) begins; separately
  a glowing red V/wedge "carpet" with soft gradient edges gets painted bottom-up.
- **Takeaway for reproduction**: between routines the engine (a) floods the screen with black
  or a random solid color, (b) may idle briefly, (c) begins the next accumulation from
  near-nothing. Black frames d26–d27 are the same behavior full-screen.

---

## Reproduction notes (engine-level summary for the programmer)

1. **Framework**: 320×200×256 indexed canvas; all routines are index-painted; run palette
   animation (rotate ranges of the 256-entry DAC) on a timer independent of drawing.
2. **Symmetry operator**: `stamp(x,y)` → also paint `(W-x,y)`, `(x,H-y)`, `(W-x,H-y)`
   (4-fold); variants: 2-fold, hex 6-fold rotation about center, 12-fold ring, plus optional
   horizontal tiling offsets. This one operator + varied primitive generators covers R1–R15,
   R18, R20–R22.
3. **Primitive vocabulary observed**: 1-px polylines/threads, dotted ellipses/orbits, arcs and
   concentric ring stacks, filled triangles/bars/rects with rainbow ramp fills, stair-stepped
   chevrons, greek-key maze tiles, spirograph epicycloids, particle bursts w/ gravity,
   dot-ring rosettes, checker/fractal block tilings, shaded blobs (nested shrinking ellipses
   with ramped colors).
4. **Lifecycle**: clear (black or random flood color) → build/accumulate 5–20 s (no per-frame
   clear) → optional decay/dissolve or palette darkening → next routine. A few routines are
   full-repaint computed fields instead (R16 pinwheel, R9 scanline moiré, R19 feedback
   ripples) — these need per-frame evaluation, not stamping.
5. **Color discipline**: pick per-routine palette family (full rainbow, red/blue duotone,
   blue mono, neon magenta/green, pastel-on-white) and cycle it; identical geometry under a
   rolling palette is half of the original's motion feel.
