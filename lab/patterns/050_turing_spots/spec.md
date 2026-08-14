# 050 Turing Spots

## Look
A leopard-skin reaction-diffusion field built from two hexagonal spot lattices — big soft cells carrying small counter-rotating freckles — in blues, olives, and hot coral. Spots swell and shrink as if the pattern were re-deciding its wavelength, while a radial rainbow sweep drifts through.

## Math
- Hex lattice field: h(k, α) = [cos(k·x′) + cos(k·(x′/2 + y′·√3/2)) + cos(k·(x′/2 − y′·√3/2))]/3 with (x′,y′) = coords rotated by α.
- f1 = h(0.10, +0.0009·t) (big spots), f2 = h(0.23, −0.0006·t + 1) (freckles, counter-rotating).
- Breathing threshold thr = 0.15·sin(0.005·t); v1 = smoothstep(2.5·(f1−thr)), v2 = smoothstep(2.5·(f2+thr)) — spots grow while freckles shrink, and vice versa.
- Color = cospal(0.55·v1 + 0.28·v2 + 0.0009·R + 0.0006·t), relief × (0.70 + 0.30·v1).

## Integer ARM64 plan
- Repaint. Rotation: per frame compute Q14 (cosα, sinα) once (sintab); per pixel x′,y′ are two mul-adds — or walk the three lattice phases incrementally along each scanline (DDA: three phase accumulators per lattice, += const per pixel; zero per-pixel muls).
- Each cos = sintab read on the wrapped Q8 phase; sum three, shift.
- smoothstep+threshold = one 256-entry response LUT per lattice (rebuilt per frame as thr moves — 256 bytes, trivial).
- Radial term from the radius LUT; final palette LUT read with t offset.

## Palette pairing
Cosine palette d=(0.30,0.55,0.80): olive/teal ground, azure cell bodies, coral-red spot cores — high-chroma naturalist, nothing pastel. The radial term fans the palette across the screen so no two spots are identical.

## Motion
Lattices counter-rotate at ~2 min and ~3 min per revolution; spot-size breathing on a ~21 s period; hue sweep one cycle per ~28 s. Three incommensurate slow clocks — the field never repeats, never jumps.
