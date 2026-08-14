# Classic DOS/Demoscene Per-Pixel & Accumulator Effects — Research for dazzle.exe-style Kaleidoscope Screensaver

Research for the dzzle1 project: 16 canonical effects, each with (a) core math,
(b) how 1990s coders did it in **integer math with lookup tables** on 386/486-class
hardware in Mode 13h (320×200×256, palette-indexed, VGA DAC), and (c) how it behaves
when rendered **inside a kaleidoscope fold** (a pie-slice source region mirrored N ways
around a center, dazzle-style).

Sources consulted: [Demo effect (Wikipedia)](https://en.wikipedia.org/wiki/Demo_effect),
[Plasma effect (Wikipedia)](https://en.wikipedia.org/wiki/Plasma_effect),
[Raster bar (Wikipedia)](https://en.wikipedia.org/wiki/Raster_bar),
[seancode.com/demofx](https://seancode.com/demofx/),
[The Demo Effects Collection](https://demo-effects.sourceforge.net/),
[oldskool.org demos explained](http://www.oldskool.org/demos/explained/demo_graphics.html),
[snes-kefrens-bars (yupferris)](https://github.com/yupferris/snes-kefrens-bars),
[xscreensaver shadebobs.c](https://github.com/GalliumOS/xscreensaver/blob/master/hacks/shadebobs.c),
plus the classic Denthor/Asphyxia trainers and Lode Vandevenne's "Computer Graphics Tutorial" lineage.

---

## 0. Shared 1990s toolbox (applies to every effect below)

- **Fixed point**: 8.8 or 16.16 integers. Multiply, then `>> 8` / `>> 16`. No FPU needed
  (386SX-safe); even on 486DX, `fmul` latency lost to `imul` + table lookups in inner loops.
- **Sine/cosine LUT**: precomputed `sin_tab[256]` or `[1024]` of signed 8.8 or 1.15 values.
  Angle is a byte (or 10 bits) so wraparound is a free `& 0xFF` mask. `cos(a) = sin_tab[(a+64)&255]`.
- **Palette tricks**: Mode 13h pixels are 8-bit indices into the VGA DAC (256×18-bit RGB).
  Rewriting the DAC (ports 3C8h/3C9h) recolors the whole screen in ~microseconds —
  **palette cycling/rotation is free animation**. Most "accumulator" effects render a
  static or slowly-changing index buffer and animate purely with the palette.
- **atan2/sqrt/distance LUTs**: for polar effects, precompute per-pixel `angle[x][y]`
  and `distance[x][y]` (or radius) once at init — the frame loop is then pure table reads.
- **Double buffer**: draw to a 64,000-byte system-RAM buffer, `rep movsd` to A000:0000
  after vsync (poll port 3DAh bit 3).
- **Kaleidoscope fold (dazzle-style)**: pick N-fold symmetry (dazzle used mirrored
  quadrants; general form is N pie slices). Two classic implementations:
  1. **Cheap 4-fold**: render one quadrant, then blit it mirrored horizontally,
     vertically, and both — pure `movsb` with negative strides. This is what
     dazzle.exe-class savers actually did.
  2. **General N-fold**: per-pixel, look up `(angle, radius)` from the polar LUTs, fold
     the angle into one wedge (`a = angle % wedge; if (a > wedge/2) a = wedge - a;` —
     doable with masks when N is a power of two and angle is a byte), then index the
     source effect by folded `(a, r)`. This is just *another* LUT: precompute a
     `fold_map[64000]` of source offsets once, and any effect becomes kaleidoscopic
     with one indirection per pixel: `screen[i] = src[fold_map[i]]`.
- **Rule of thumb for what folds well**: effects with strong asymmetric motion
  (starfields, fire, bars) gain the most drama from folding; effects that are already
  radially symmetric (tunnel, interference circles) gain less and may need an off-center
  fold axis to stay interesting.

---

## 1. Plasma

**Core math.** Sum of several low-frequency sinusoids over screen position and time:

```
c(x,y,t) = sin(x·f1 + t·s1) + sin(y·f2 + t·s2)
         + sin((x+y)·f3 + t·s3) + sin(sqrt((x-cx)² + (y-cy)²)·f4 + t·s4)
```

The sum (range roughly −4..+4) is scaled into a palette index. The radial term is what
gives it "blobby" rather than "diagonal-stripey" character.

**90s integer implementation.**
- All sines from `sin_tab[256]` (values −128..127). Sum four bytes, add bias, use the
  result directly as a color index — no scaling multiply needed if the table is prescaled.
- The sqrt term precomputed into a per-pixel radius LUT at init.
- The **real** classic trick: don't compute per-pixel at all. Precompute two or three
  static "phase buffers" (`p1[64000]`, `p2[64000]`, each a byte of precomputed spatial
  sine-sum), then per frame do `screen[i] = p1[i + off1] + p2[i + off2]` where the
  offsets drift by table-driven sine motion — 2 adds and 2 reads per pixel. Combined
  with palette rotation, this is the canonical 1993 plasma.
- Cheapest variant of all: static index buffer + **pure palette cycling** (zero per-pixel
  work per frame).

**In a kaleidoscope fold.** Excellent. Plasma is isotropic, so folding creates flower/
mandala structure it doesn't naturally have — the seams at wedge boundaries read as
petal edges. Fold in *index space* (fold_map into the plasma buffer) so palette cycling
still animates all wedges in lockstep. Drifting the phase-buffer offsets makes petals
"breathe" toward/away from the center.

---

## 2. Rotozoom

**Core math.** Inverse texture mapping of a rotation+scale. For screen pixel (x,y):

```
u = (x·cosθ − y·sinθ) · zoom + u0
v = (x·sinθ + y·cosθ) · zoom + v0
color = texture[u & (TW-1)][v & (TH-1)]     // tiling texture
```

Key property: u,v are **affine** in x,y — so across a scanline, u and v change by a
constant (du,dv) per pixel.

**90s integer implementation.**
- Per frame: compute 16.16 `du = cos·zoom`, `dv = sin·zoom` **once** (from sin LUT),
  plus per-row start values. Inner loop is: `u += du; v += dv;
  pixel = tex[((v>>8)&0xFF00) | ((u>>16)&0xFF)]` — with a 256×256 texture the (u,v)→offset
  combine is two shifts, an AND, an OR. ~6 instructions/pixel; screaming fast on a 486.
- Zoom pulsing = multiply du,dv by another sine-LUT value (one imul per frame).
- Texture was usually 256×256 so wrap is a free mask; often the texture itself was a
  precomputed plasma or a hand-drawn logo.

**In a kaleidoscope fold.** One of the best fold candidates. Rotation motion + mirror
seams = classic "turning kaleidoscope" illusion: mirrored copies counter-rotate against
each other at every wedge boundary, which is exactly what a physical kaleidoscope does.
Cheap route: rotozoom into a quadrant buffer, mirror-blit 4-fold. The zoom pulse makes
the whole mandala inhale/exhale.

---

## 3. Tunnel

**Core math.** Per-pixel polar transform mapped onto a tiling texture:

```
angle(x,y)    = atan2(y-cy, x-cx)                → texture u
distance(x,y) = k / sqrt((x-cx)² + (y-cy)²)      → texture v   (1/r gives depth illusion)
color = texture[(u + t_rot) & 255][(v + t_fly) & 255]
```

Adding time to v "flies" down the tunnel; adding to u spins it.

**90s integer implementation.**
- **Everything precomputed**: two 64,000-byte LUTs, `angle_tab[i]` and `depth_tab[i]`
  (each a byte). Frame loop is `screen[i] = tex[((depth_tab[i]+fly)&255)*256 +
  ((angle_tab[i]+rot)&255)]` — two adds, two masks, one read. The atan2 and divide
  happen only at init (or were table-built offline).
- Moving the tunnel center: precompute the LUTs on a buffer **larger than the screen**
  (e.g. 640×400) and slide the viewport window across it — center motion with zero
  recompute.
- Darken-with-distance was done by baking shading into the texture rows or via palette.

**In a kaleidoscope fold.** Weakest raw candidate — a centered tunnel is *already*
radially symmetric, so an N-fold about the same center only quantizes the angle texture.
Two fixes: (1) fold about an **off-center axis** while the tunnel center orbits (the
sliding-window trick above), giving multiple mirrored tunnel mouths; (2) use an
angularly-asymmetric texture (text, checkboard with diagonal features) so the mirror
seams visibly kink. Fly motion survives folding beautifully — every wedge rushes inward.

---

## 4. Moiré (interference of patterns)

**Core math.** Overlay two similar high-frequency patterns with a small relative
transform; the beat frequency between them creates large slow structures. Classic form:
XOR/sum of two ring or grid fields:

```
c = f(r1) XOR f(r2)      where r1,r2 = distances to two moving centers
```

with f a fast periodic function of distance (e.g. `(r >> 2) & 1` for rings).

**90s integer implementation.**
- Precompute one big **distance table** for a virtual screen 2× the display size
  (distance from its center, bytes). Per frame, pick two moving offsets into that table
  (centers driven by sine LUT), and per pixel: `screen[i] = (dist[o1+i] ^ dist[o2+i]) & ringmask`
  — two reads, XOR, AND. This is the canonical "XOR moiré rings" and needs no per-pixel
  math beyond table reads.
- Palette: black/white bands, or map the XOR result through a gradient for the softer
  "silk" look.

**In a kaleidoscope fold.** Very strong — moiré already produces emergent curves, and
mirroring multiplies the interference (each wedge's pattern beats against its mirrored
neighbor at the seam, producing *extra* moiré that isn't in the source). Keep centers
orbiting off the fold axis or the effect collapses into static symmetry.

---

## 5. Copper bars (raster bars)

**Core math.** Horizontal bars of vertical color gradients whose y-positions follow
sine paths: `y_k(t) = cy + A·sin(ω·t + φ_k)`. Each bar is a small gradient strip
(bright center, dark edges); overlapping bars sort by "height" or simply draw in order.

**90s integer implementation.**
- Amiga heritage: the Copper coprocessor changed the background color **per scanline**
  with zero CPU — the "bar" is just a per-line background color list
  ([Raster bar](https://en.wikipedia.org/wiki/Raster_bar)). On PC, either race the beam
  rewriting DAC color 0 per scanline (timing hell, CGA/VGA-dependent per
  [8088 MPH's writeup](https://www.reenigne.org/blog/more-8088-mph-how-its-done/)) — or,
  in Mode 13h, just draw the bars into the framebuffer.
- Framebuffer version: per frame, clear a 200-entry per-scanline color array; for each
  bar, add its gradient bytes at rows `y_k` (sine LUT). Then each screen row is
  `memset(row, color[y], 320)` — per-**row** work, essentially free.
- Overlap trick: write gradients with saturation or priority so crossing bars look
  layered.

**In a kaleidoscope fold.** Transformative — this is where folding shines. A per-row
effect fed through an angular fold turns each horizontal bar into a **polygonal ring /
starburst**: N-fold symmetry maps "scanline y" bands into concentric N-gon bands
sweeping in and out with the sine motion. Implementation: treat the copper color array
as a function of *radius* instead of row (`screen[i] = colorline[depth_or_radius_tab[i]]`)
— i.e. copper bars become **radial copper rings** using the tunnel's radius LUT. Dirt
cheap and extremely dazzle-appropriate.

---

## 6. Kefrens bars (alcatraz bars)

**Core math.** Named for the Amiga group Kefrens (first done by Alcatraz). There is
**no framebuffer**: one single scanline buffer is displayed on *every* row of the screen,
and is mutated between rows. Per frame, for each screen row y (top→bottom): plot a small
gradient "bar" at `x(y) = cx + A·sin(a·y + b·t) + B·sin(c·y + d·t)` into the line
buffer, then display the buffer as row y. Because the buffer is never cleared, each row
inherits all bars drawn above it → melting vertical streaks with sinuous edges.

**90s integer implementation.**
- Amiga/SNES: point the display hardware at a 1-line bitmap and rewrite scroll/CLUT
  registers per scanline in hblank ([snes-kefrens-bars](https://github.com/yupferris/snes-kefrens-bars),
  [stardot raster-timing thread](https://stardot.org.uk/forums/viewtopic.php?t=13382)).
- DOS Mode 13h software version: keep a 320-byte `linebuf`; for y=0..199 { plot ~8–16
  gradient bytes at sine-LUT x; `memcpy(screen+y*320, linebuf, 320)` }. All integer,
  two sine lookups per row. The copy dominates and is still only 64 KB/frame.
- Multiple bars = multiple plots per row with different phase constants.

**In a kaleidoscope fold.** Spectacular and rarely seen. The vertical melt becomes a
**radial melt**: run the same algorithm over *radius* instead of row — one line buffer
indexed by angle, iterated from r=0 outward, blitted through the polar LUT — and bars
become spiral tendrils dripping outward from the center, mirrored N ways. Cheap
approximation: render classic Kefrens into a quadrant and 4-fold mirror; the melt
streaks then converge/diverge at the mirror seams like pulled taffy.

---

## 7. Twister

**Core math.** A vertical "twisted column". For each row y, the column is a rotated
square (or n-gon) seen edge-on: compute rotation phase `φ(y,t) = a·sin(y·f + t) + t·spin`,
project the 4 corner angles to screen-x: `x_j = cx + R·sin(φ + j·π/2)`, sort adjacent
pairs, and fill each visible face between `x_j..x_{j+1}` with a shade based on face
angle (fake lighting), optionally texture-mapped by interpolating u across the face.

**90s integer implementation.**
- Per **row**, not per pixel: 4 sine lookups → 4 x-coordinates → 2–3 horizontal
  `rep stosb` fills with face-shade colors. Fixed-point interpolation for texture u
  if textured. A whole twister is ~200 rows × trivial work; commonly several twisters
  ran at once.
- The wave in φ(y) is what makes it "twist"; adding a second sine to `cx` makes the
  column sway.

**In a kaleidoscope fold.** Same promotion as Kefrens: swap rows for **radius** and the
twister becomes a twisting starfish/flower — each face-fill becomes an angular arc fill
at radius r, so you get a rotating N-armed braid. Alternatively, a normal twister
rendered off-axis in one wedge yields mirrored "DNA helix" pairs at every seam. Face
shading (light/dark alternation) survives folding and reads as 3D relief in the mandala.

---

## 8. Shadebobs

**Core math.** An accumulator effect. A small blob (disc or square) moves on a
Lissajous path; instead of drawing color it **adds** (or subtracts) a small value to the
framebuffer under itself: `buf[p] = clamp(buf[p] ± k)`. Where the path self-crosses,
values accumulate → glowing trails through a smooth palette
([Demo Effects Collection](https://demo-effects.sourceforge.net/),
[xscreensaver shadebobs](https://github.com/GalliumOS/xscreensaver/blob/master/hacks/shadebobs.c)).

**90s integer implementation.**
- Path: `x = cx + (sin_tab[a]·Ax >> 8); y = cy + (sin_tab[b]·Ay >> 8)` with a,b
  stepping at different rates (Lissajous). Multiple bobs = multiple phase pairs.
- Blob: 16×16 or 32×32 loop of `add byte [buf+ofs], k` with saturation via a 512-entry
  clamp LUT (`clamped = sat_tab[old + k]`) — no branches. Some coders alternated
  add/subtract bobs so the buffer never saturates globally.
- Palette: smooth gradient (e.g. black→blue→cyan→white); palette *rotation* on top makes
  even a static accumulation writhe.
- Only the blob's footprint is touched per frame — per-pixel cost is tiny; the full
  screen is just re-shown.

**In a kaleidoscope fold.** Natural fit — this *is* the dazzle genus. Dazzle.exe-style
savers are essentially mirrored Lissajous trail-drawers. Accumulate in one wedge buffer
and mirror-blit: one bob becomes N synchronized bobs weaving a symmetric rosette, and
self-crossings at seams double-expose into bright knots. Add slow global decay
(subtract 1 from every pixel every few frames, via the same clamp LUT) so old rosettes
fade as new ones bloom.

---

## 9. Metaballs

**Core math.** Isosurface of summed fields. Each ball i at (xi,yi) contributes
`f_i(p) = R_i² / ((x-xi)² + (y-yi)²)`; color/threshold on `Σ f_i`. Where balls approach,
fields sum and the contours merge — the "blobby fusion" look. (2D screen version; some
implementations multiply distances instead of summing inverse squares, as noted on
[pouët/demofx discussions](https://seancode.com/demofx/).)

**90s integer implementation.**
- Division per pixel per ball is death on a 486, so: precompute **one radial falloff
  table** `falloff[d²]` (or `falloff[dist]`) as bytes, sized to the max ball radius.
  Per pixel: `sum = falloff[d²_1] + falloff[d²_2] + ...` where `d²` uses incremental
  integer deltas across the scanline (d² of adjacent pixels differs by `2(x-xi)+1` —
  add, no multiply).
- Bounding boxes: only evaluate pixels near each ball; clear/redraw the union rect.
- Sum → palette index directly (gradient palette gives the glow; hard threshold via
  palette makes it look like mercury).
- Ball motion: sine LUT Lissajous, same as shadebobs.

**In a kaleidoscope fold.** Very good. Fold N ways and each ball gains N mirrored twins
that genuinely **merge with each other across seams** (because the fold happens before
thresholding if you accumulate in the wedge) — blobs kiss their own reflections at
wedge boundaries and fuse into rings around the center when they approach the fold
axis. That merge-at-the-seam behavior is unique among these effects.

---

## 10. Fire

**Core math.** Upward heat diffusion with decay. Seed the bottom row(s) with random hot
values; each pixel above becomes the average of the pixels below/around it minus a
cooling term:

```
buf[x][y] = ( buf[x-1][y+1] + buf[x][y+1] + buf[x+1][y+1] + buf[x][y+2] ) / 4  −  cool
```

Rendered through a black→red→orange→yellow→white palette.

**90s integer implementation.**
- All byte math: three/four `mov al,[...]` + `add` + `shr ax,2` + saturating subtract
  (or the divide-by-4 folded into a 1024-entry LUT that also applies cooling — one table
  read replaces the whole kernel).
- Randomness: cheap LFSR or a precomputed random byte table for the seed row.
- Half-vertical-resolution buffers (320×100 doubled on blit) were common — halves the
  work and adds vertical smear that reads as flame.
- The palette *is* the effect: same buffer with a blue-white palette is "plasma smoke".

**In a kaleidoscope fold.** Change the propagation axis: run the diffusion **inward or
outward along radius** (seed the outer rim, propagate toward center — or seed the
center) using a polar buffer (angle × radius), then map through the fold LUT. Result:
a breathing corona / burning mandala with flames licking along mirror seams. The lazy
version — normal fire in a quadrant, 4-fold mirror — yields four flame fronts colliding
at the axes, which looks great and costs nothing extra.

---

## 11. Starfield

**Core math.** 3D points with perspective divide. Star at (X,Y,Z): screen
`x = cx + X·k/Z, y = cy + Y·k/Z`; each frame `Z -= speed`; respawn at Z=Zmax when it
passes the camera. Brightness ∝ 1/Z. 2D cheat version: stars move radially outward
from center with speed ∝ current radius.

**90s integer implementation.**
- The divide was the enemy: use a **reciprocal table** `recip[z] = k/z` in 8.8 fixed
  point (Z quantized to a byte or 10 bits), so projection is two imuls + shifts, or
  precompute per-Z-plane scale factors. Brightness = `bright_tab[z >> 4]` indexing a
  grayscale palette ramp.
- Typically 200–1000 stars, plot + erase old pixel (cheaper than clearing the screen).
- "Warp" variants drew short trails: plot at both `recip[z]` and `recip[z+speed]`
  positions and line between (or just 2–3 plots).

**In a kaleidoscope fold.** Turns "flying through space" into a **radiating jewel**:
fold N ways and each star becomes an N-point symmetric constellation streaming outward —
essentially fireworks/snowflake sparkle, extremely dazzle-like. Implementation is
trivial: keep stars in one wedge (spawn with angle in [0, wedge)), and plot each star N
times via precomputed per-wedge mirror transforms (for 4-fold: (±x,±y) — no trig at
all). Trails become symmetric rays.

---

## 12. Feedback zoom (video feedback)

**Core math.** Each frame, the previous frame is resampled slightly zoomed/rotated
toward or away from a center, then new elements are stamped on top:

```
new[x,y] = decay( old[ cx + (x-cx)·s·cosθ − (y-cy)·s·sinθ, ... ] )
```

with s ≈ 0.97 (zoom-in feedback → outward-flying trails) or ≈ 1.03 (inward collapse).
It's rotozoom applied **to the framebuffer itself**, plus decay — the camera-pointed-at-
its-own-monitor effect.

**90s integer implementation.**
- Exactly the rotozoom inner loop (16.16 du/dv per scanline) but sampling the previous
  frame buffer into the next; the two buffers ping-pong. Since s and θ are constant per
  frame, per-pixel cost equals rotozoom.
- Decay/color-shift by mapping each fetched byte through a 256-byte remap table
  (`pixel = fade_tab[old_pixel]`) — same read anyway, so nearly free. The remap table
  can also *hue-shift* trails as they age.
- Blockier cheat used on slow CPUs: zoom by pixel duplication on a coarse grid
  (precomputed source-offset table again — `fold_map`-style).
- Stamp source: a bouncing sprite, text, or any effect above; the feedback does the rest.

**In a kaleidoscope fold.** The king of dazzle effects — **fold + feedback is the
fractal kaleidoscope**. Apply the fold *inside* the loop (feedback samples through the
fold_map with slight rotation): every stamped shape recursively re-mirrors and spirals
into the center, generating infinite self-similar mandalas from any seed. This single
combination can host every other effect in this file as its "stamp".

---

## 13. Bump-mapped lightmaps (2D bump mapping)

**Core math.** A static heightfield H(x,y); per pixel compute the surface normal from
finite differences `nx = H[x-1,y] − H[x+1,y]`, `ny = H[x,y-1] − H[x,y+1]`, then shade by
a moving light: the classic 2D trick indexes a precomputed **light texture** by the
normal offset relative to the light position:

```
lx = x − Lx + nx ;  ly = y − Ly + ny
color = light_tab[clamp(lx)][clamp(ly)]      // radial falloff spot
```

Bumps displace the lookup into the spot, producing convincing embossed relief.

**90s integer implementation.**
- Precompute per-pixel (nx,ny) as signed bytes **once** (heightfield never changes).
  Per frame, only Lx,Ly move (sine LUT). Inner loop: two adds, two clamps (via
  256+margin clamp tables), one read from a 256×256 light texture. No multiplies at all.
- The light texture is a radial gradient computed at init (`255 − dist`), sometimes
  two lights via summing two lookups with a saturation table.
- Heightfields: text logos, stucco noise, or a blurred random buffer (blur via the
  fire-kernel trick).

**In a kaleidoscope fold.** Two distinct wins: (1) fold the **heightfield** — an
embossed mandala with a free-roaming light, so relief highlights sweep asymmetrically
across a symmetric surface (very physical, very jewel-like — arguably the most
"dazzle" material response here); (2) fold the **light** — N mirrored lights orbiting a
plain heightfield. (1) reads better. Combine with palette gold/chrome ramps for the
engraved-brooch look.

---

## 14. Cyclic cellular automata (CCA)

**Core math.** Each cell holds a state 0..K−1. A cell in state s advances to s+1 (mod K)
iff at least T of its neighbors (von Neumann or Moore) are in state s+1 (mod K).
From random soup it self-organizes: noise → blobs → spirals ("demons") that consume the
field — the classic *demon cyclic space* (Griffeath). Endless rotating spiral waves.

**90s integer implementation.**
- Byte buffer per cell; successor test is compares, no arithmetic:
  `target = s+1; if s+1==K, target=0` — done via a 256-byte `next_state[]` table.
  Neighbor count vs threshold with `cmp/adc` chains.
- Double buffer (read old, write new); wrap edges with an index mask if the grid is a
  power of two (256×256 grid → offset arithmetic is `(i ± 1) & 0xFFFF`, `(i ± 256) & 0xFFFF`).
- K typically 14–20 states mapped directly to a hue-wheel palette; **palette rotation
  makes the spirals appear to spin even between generations**.
- Ran at half resolution (or updated alternate halves per frame) on slower CPUs;
  the effect tolerates low update rates because palette cycling covers the gaps.

**In a kaleidoscope fold.** Eerie and gorgeous: fold the *display* only (simulation on
a full grid, display through fold_map) for symmetric-but-alive spiral gardens; or fold
the *simulation* (run CCA on one wedge with mirrored boundary conditions) so spiral
waves reflect off the seams and collide with their own mirror images — standing-wave
mandalas. CCA's hue-wheel states + palette rotation are already kaleidoscope-adjacent;
this may be the best "leave it running for hours" screensaver mode of the set.

---

## 15. Interference circles

**Core math.** Two (or more) concentric ring patterns from moving centers, combined by
XOR or addition — the ring-specific case of moiré:

```
c(x,y) = ring(d1) op ring(d2),   d_i = |p − c_i(t)|,   ring(d) = (d·f) mod P  (sawtooth)
```

XOR of two ring fields yields hyperbola/ellipse interference families (constant
sum/difference of distances), exactly like water-wave or two-slit interference.

**90s integer implementation.**
- Same machinery as moiré §4: one oversized precomputed distance-byte table, two moving
  window offsets, per pixel `dist[o1+i] ^ dist[o2+i]` (XOR gives hard fringes) or
  `sat_add(dist[o1+i], dist[o2+i])` (addition + gradient palette gives soft waves).
- Ring frequency changed by shifting the distance bytes (`(d << k)` baked into
  alternate tables).
- Animating ring *phase* (waves radiating outward) = add a time byte before the mask:
  `(dist[o+i] + t) & mask` — one extra add.

**In a kaleidoscope fold.** Strong, with the same caveat as the tunnel: keep the
sources **off the fold axis**. Two orbiting sources under an N-fold become 2N mirrored
sources, and XOR fringes between a source and its own mirror generate lens-like eyes
along each seam. Radiating-phase mode folded N ways = N-fold ripple tank; visually
close to dazzle's ancestral "Stained Glass" savers.

---

## 16. XOR pattern / munching squares

**Core math.** `c(x,y,t) = (x XOR y) + t` (or `(x^y) & mask`, or `((x+t) ^ (y+t))`).
The x^y field is the Sierpinski-triangle bit pattern; animating a threshold
`(x^y) == t` gives HAKMEM's **munching squares** (PDP-1, 1962 — the oldest effect
here), where expanding/contracting square "mouths" munch across the screen. Variants:
`x AND y` (Sierpinski), `(x·y) >> k` (hyperbolic sheets), `(x² + y²) >> k` (rings —
degenerate into interference circles).

**90s integer implementation.**
- The purest integer effect in existence — the inner loop can literally be
  `mov al, bl / xor al, bh / add al, dl / stosb` (x in bl, y in bh, t in dl). No tables
  needed, though `(x^y)` per-pixel bytes were sometimes prebuilt so the frame loop is
  `screen[i] = xor_tab[i] + t` — one add per pixel, then let **palette cycling** do
  everything.
- Munching squares proper: per frame plot only the set `{(x,y): x^y == t}` — 256 pixels
  per frame on a 256×256 area. Practically free; often used as a background layer.
- Color depth via `((x^y) + t) & 0xFF` straight into a looping palette.

**In a kaleidoscope fold.** Folding a diagonal bit-lattice produces sharp crystalline
snowflake structure — the Sierpinski self-similarity means the mirrored wedges tile
into convincing fractal frost. Because the pattern is axis-aligned, use an N-fold with
N not a multiple of 4 (e.g. 6 or 10) or slowly rotate the (x,y) frame feeding the XOR
(rotozoom the coordinates via the §2 machinery) so seams don't align with the pattern's
own diagonals. Best used as a glittering backdrop layer under shadebobs/metaballs.

---

## Synthesis for dzzle1

- **Universal architecture**: every effect above reduces to *"write bytes into an index
  buffer, view through (a) a fold LUT and (b) an animated palette."* Build three shared
  assets once — `sin_tab`, polar LUTs (`angle_tab`, `radius_tab`), and `fold_map` — and
  all 16 effects plug into one pipeline; most then cost 1–3 table reads per pixel,
  faithful to how the originals ran on a 486.
- **Tier 1 (born for the fold)**: shadebobs, feedback zoom, starfield, radial copper
  rings, metaballs — accumulator/particle effects whose mirror seams create genuinely
  new structure.
- **Tier 2 (great with the radial-axis swap)**: Kefrens→spiral melt, twister→star-braid,
  fire→corona, plasma, moiré, CCA, bump-mapped mandala.
- **Tier 3 (needs off-center handling)**: tunnel, interference circles, XOR lattice —
  already symmetric; fold off-axis or rotate the source frame.
- **The killer combo**: fold-aware feedback zoom (§12) as the base loop, with any Tier 1
  effect as the stamp, palette-cycled — this is the closest single recipe to "dazzle.exe
  but 1994 demoscene."
