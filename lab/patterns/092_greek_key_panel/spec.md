# 092 Greek Key Panel

## Look
A big central panel covered edge-to-edge in interlocking greek-key maze motifs, framed by a border of blue kaleidoscope wedges with small six-lobed rosettes, while thin rays fan out horizontally behind it. The geometry is completely frozen — all motion is the palette wheel rolling underneath it, recoloring the whole scene yellow → red-on-blue → all-blue exactly as frames a02→a03→a04 show.

## Math
- The frame is an indexed image built ONCE: class map {background, panel ground, key lines, wedges, rosettes, rays}.
- Key motif: hand-drawn 12×12 meander bitmap, ×2 upscale, tiled on `(|dx| mod 24, |dy| mod 24)` — absolute coords give the mirrored-quadrant layout of a02.
- Wedge border: annular frame region, wedge on `frac(atan2·24/2π) < 0.5`.
- Rosettes: `r < 7 + 3.2·cos(6θ)` at ten fixed border sites; rays: near-horizontal angular fan `frac(ang·40/(π/2)) < 0.3`, `ang < 0.42`.
- Render: `color(class) = wheel(offset[class] + t·0.35 [+ per-pixel shading term])` on an 8-stop looping RGB wheel.

## Integer ARM64 plan
This IS the mode-13h architecture: draw the class map into an 8-bit framebuffer once at routine start, then per frame only rewrite the 256-entry DAC. Roll = `t·k` in 8.8 fixed point; each class's palette entry block is filled from a 256×3 wheel table at `(offset + roll) & 255`. Per-pixel work per frame: zero. Ray/wedge shading is baked into the index at draw time (index = class base + (x>>1 & 31)), so it rides the same rotation. No float, no trig, no div anywhere after init (init's atan2 replaced by octant lookup + 256-entry atan table).

## Palette pairing
One 8-stop full-saturation wheel (yellow→orange→red→magenta→blue→azure→cyan→green) shared by all classes with fixed phase offsets — guarantees every palette phase is a coherent scheme, reproducing the yellow phase / red-keys-on-blue phase / all-blue phase of a02-a04.

## Motion
Zero geometric motion. The palette rolls at 0.35 index/frame (~12 s for a full hue revolution); rays get an extra per-pixel phase so they appear to stream outward; wedges shade around the ring. Slow, stately recolor — nothing ever flashes because adjacent wheel stops are adjacent hues.
