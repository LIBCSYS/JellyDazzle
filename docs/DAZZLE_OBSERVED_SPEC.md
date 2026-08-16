<!-- PROVENANCE: this document is an OBSERVATIONAL spec — a description of DAZZLE
5.0 written from watching the archive.org build run. It is not from Shiflett, not
from the manual, and not from disassembly. Treat it as a careful eyewitness account:
excellent for guiding our own work, NOT citable as historical fact on the public
tribute page. Anything here that reaches dazzle.jelia.nyc needs its own source.
Added to the repo 2026-08-16. -->

# DAZZLE 5.0 — visual spec & build prompt

Reference: MS-DOS shareware "DAZZLE" by J.R. Shiflett / MicroTronics, 1992
(archive.org/details/msdos_festival_DAZZLE50). Below is a description of what
the program actually puts on screen, written from watching it run, followed by
a build prompt you can paste to Claude.

---

## Part 1 — What the images actually are

### The frame

- **Canvas: 640 × 400**, drawn as hard-edged 1-pixel lines. No antialiasing.
  Everything should look like direct VGA framebuffer plotting — jaggies and
  aliasing are part of the aesthetic, not a defect.
- The screen is **never cleared between patterns.** Several patterns are drawn
  on top of each other, the composite is held for a while, then a *fade* wipes
  it and a new set begins.

### The single most important rule: mirror symmetry

Every pattern is drawn once and **mirrored**. This is what makes DAZZLE read as
"kaleidoscope" rather than "random lines."

- Most frames use **left↔right mirroring** about the vertical centerline.
- Many also add **top↔bottom mirroring**, giving 4-way quadrant symmetry.
- A minority are horizontally mirrored only, or offset-mirrored into two
  side-by-side copies (see the blue frame with two identical bowtie motifs).

Implement this as a draw wrapper: generate a primitive in the top-left
quadrant's coordinate space, then emit 2 or 4 reflected copies. Pick the
symmetry mode per pattern, not globally.

### Everything is wireframe — solid areas are an illusion

There are almost no filled shapes. What looks like a solid block or a smooth
gradient panel is **hundreds of parallel 1px lines packed 1–3 pixels apart.**
The visible moiré, banding, and dither-like texture come from that packing
interacting with the pixel grid. Do not substitute `fillRect` or gradient
fills — you lose the entire look.

### Color: sequential palette walk

Each pattern draws a *sequence* of lines. The color index advances by a fixed
step every line (or every few lines) and wraps. Because the palette is a
saturated rainbow ramp, a swept family of lines becomes a smooth rainbow band.
Change the step size and you get everything from smooth gradients to harsh
strobing color noise.

Palette character (EGA/VGA 1992):

- Fully saturated primaries and secondaries: pure red, green, blue, cyan,
  magenta, yellow, white, black. Nothing muted, nothing pastel.
- Plus smooth rainbow ramps through those hues (this is a 256-color VGA
  palette, so ~64-step hue ramps are available).
- **The background is a palette entry too.** Most frames are on black, but the
  background flips to flat royal blue, mint green, teal, or light grey between
  pattern sets — the whole scene sits on a solid color field.
- Palette *cycling* (rotating the color table under a static image) is used to
  make finished patterns shimmer while they're being held on screen.

### The pattern primitives I observed

1. **Line-sweep envelopes (string art).** A family of straight lines whose two
   endpoints each walk along a path (edges of a box, arcs, opposing lines). The
   lines never curve, but their envelope does — you get hyperbolas, bowties,
   hourglass and X shapes with sharp bright caustic edges. This is the most
   common primitive in DAZZLE and appears in nearly every frame.
2. **Concentric nested rectangles.** Boxes shrinking toward a center, each a
   different palette index — reads as a rainbow-striped rectangular tunnel.
3. **Nested diamonds / chevrons / lozenges.** Same idea rotated 45°, and as
   stacked chevron arrowheads.
4. **Circle and ellipse clusters.** Overlapping rings arranged in a ring or
   grid, producing flower / soap-bubble clusters. Rings are outlines only, so
   the overlaps create dense interference texture.
5. **Radial fans.** Lines from a single vanishing point sweeping through an
   angle range — often anchored at bottom-center or at a quadrant corner.
6. **Staircase polylines.** Thin single-color (often green or dark red)
   stepped zigzag lines that run across the composition. These sit *on top* of
   everything as sparse detail and are visually important — they break up the
   density.
7. **Thick diagonal rainbow bars.** A wide band of parallel diagonal lines
   sweeping the full palette, crossing the whole screen corner to corner.
8. **Hatch/weave fills.** Two perpendicular line families over the same
   rectangle at slightly different spacings — pure moiré, reads as woven or
   checkered fabric.
9. **Wedge fans that read as solid.** A radial fan packed dense enough to look
   like a filled pie slice or crescent, with a hard color edge.

### Composition and timing

- **3–6 patterns overlay per set.** They're placed at different scales and
  centers — some fill the screen, some occupy a band, some are small motifs
  repeated near the edges. Later patterns partially obscure earlier ones.
- Each pattern **draws progressively over ~1–3 seconds** — you watch the lines
  accumulate, you don't see it appear instantly.
- Once the set is complete it **holds for roughly 5–15 seconds**, optionally
  with palette cycling animating it.
- Then a **fade** clears the screen: fade to black via palette ramp, a
  split-screen wipe (two halves closing or opening), a horizontal or vertical
  pan-off, or a dissolve. Then the next set starts, sometimes on a new
  background color.
- Nothing ever repeats exactly. Every parameter — center, scale, angle, line
  count, spacing, color step, symmetry mode — is randomized per pattern.

---

## Part 2 — Build prompt (paste this to Claude)

> Build me a single self-contained HTML file that recreates the 1992 MS-DOS
> screensaver DAZZLE — a generative kaleidoscopic line-art engine. HTML5 canvas
> + vanilla JS, no libraries, no build step, no localStorage. It should run
> forever unattended and never look the same twice.
>
> **Canvas & rendering**
> - Internal resolution 640×400, scaled up to fit the window with
>   `image-rendering: pixelated`. Black page background, canvas centered.
> - Draw with 1px hard-edged lines: `ctx.imageSmoothingEnabled = false`,
>   `lineWidth = 1`, and offset coordinates by 0.5 so lines land on pixel
>   centers. Aliasing and jaggies are wanted — do not smooth anything.
> - Never clear the canvas except during an explicit fade.
>
> **Palette**
> - Build a 256-entry palette: index 0 is the background color, indices 1–255
>   are a saturated rainbow ramp (full HSL saturation, lightness swinging
>   between ~35% and ~65% across the ramp so there are bright caustics and dark
>   troughs). All colors fully saturated — no pastels, no greys except pure
>   white.
> - Support palette cycling: rotate the 1–255 window by an offset that
>   increments each frame, and repaint the held image from a stored command
>   list so cycling actually animates the finished art.
> - Between pattern sets, occasionally change the background entry to a flat
>   saturated color (royal blue, mint green, teal, light grey) instead of black.
>
> **Symmetry wrapper**
> - A `drawMirrored(fn, mode)` helper. `mode` is one of `'x'` (left/right),
>   `'xy'` (4-way quadrant), `'y'` (top/bottom), or `'dual'` (two side-by-side
>   mirrored copies). The pattern generator draws in normal coordinates and the
>   wrapper emits the reflected copies via canvas transforms. Pick the mode
>   randomly per pattern, weighted toward `'x'` and `'xy'`.
>
> **Pattern generators** — implement all nine, each taking randomized params
> (center, scale, rotation, line count 40–400, spacing, color start index,
> color step 1–17, symmetry mode):
> 1. `lineSweep` — string-art envelope: two endpoint paths, N straight lines
>    between interpolated points. Endpoint paths can be line segments, arcs, or
>    box edges. This is the workhorse; make it the most-used generator.
> 2. `nestedRects` — concentric rectangles shrinking to a center.
> 3. `nestedDiamonds` — same rotated 45°, plus a stacked-chevron variant.
> 4. `ringCluster` — 4–12 circle/ellipse outlines arranged in a ring or grid,
>    overlapping heavily.
> 5. `radialFan` — lines from one origin sweeping an angle range.
> 6. `staircase` — thin single-color stepped zigzag polyline, drawn sparse and
>    on top.
> 7. `diagonalBar` — wide band of parallel diagonals sweeping the full palette,
>    corner to corner.
> 8. `weave` — two perpendicular parallel-line families at slightly different
>    spacings over one rect, for moiré.
> 9. `wedgeFan` — radial fan packed dense enough to read as a solid crescent or
>    pie slice.
>
> **Sequencing loop**
> - Pick 3–6 generators for a set. Draw each one progressively — spread its
>   lines across ~1–3 seconds of animation frames so you watch it build, don't
>   emit the whole pattern in one frame.
> - When the set finishes, hold 5–15 seconds with palette cycling running.
> - Then fade: randomly choose fade-to-black-via-palette, split-screen wipe
>   (halves closing or opening), vertical or horizontal pan-off, or a random
>   pixel dissolve. Fade should take ~1–2 seconds.
> - Then start the next set, occasionally with a new background color.
>
> **Controls** (echoing the original's semi-automatic mode)
> - `Space` — skip to next pattern set immediately
> - `F` — trigger a fade now
> - `P` — pause/resume
> - `C` — toggle palette cycling
> - `1`–`9` — force the corresponding generator for the next pattern
> - Fullscreen on `Enter`. Hide the cursor after 3 seconds of no mouse movement.
>
> The failure mode to avoid: smooth, tasteful, modern generative art. This
> should look like a 1992 VGA demo — hard aliased lines, blazing saturated
> color, dense moiré interference, strict mirror symmetry, and compositions
> that pile up until they're almost too busy.
