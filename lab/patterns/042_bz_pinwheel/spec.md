# 042 BZ Pinwheel

## Look
A gold/teal chemical mandala: six-fold folded spiral wavefronts rotate around a starburst core like a Belousov–Zhabotinsky dish viewed through a kaleidoscope. Petal rings breathe outward while a counter-wave crawls back in.

## Math
- Fold: tf = |mod(θ, π/3) − π/6| (6-fold mirror kaleidoscope).
- p1 = cos(6·tf + 0.16·r − 0.020·t) — outgoing folded spiral.
- p2 = cos(10·tf − 0.09·r + 0.013·t + 1.7) — finer counter-rotating layer.
- p3 = cos(0.05·r − 0.008·t) — slow radial swell.
- v = 0.5 + 0.5(0.55·p1 + 0.30·p2 + 0.15·p3); color = cospal(0.9·v + 0.0004·t); vignette 1 − 0.3·(r/220).

## Integer ARM64 plan
- Pure repaint f(x,y,t). θ and r from precomputed quarter-plane LUTs (u8 angle via octant atan table, radius via octagonal norm max+min/2−min/8 or radius LUT — research §6).
- Fold = AND/subtract/abs on the u8 angle (π/3 sector = angle & 42-ish range in 256-step angle units).
- Each cos term = one sintab[(a·tf + b·r + c·t) & 255] with Q8 integer coefficients; sum three Q14 values, shift, index the 32k palette LUT.
- Vignette folded into the palette by premultiplying a radial brightness LUT.

## Palette pairing
Cosine palette c=(1,1,0.5), d=(0.8,0.9,0.3): antique gold, deep teal, cream highlights on near-black troughs — high contrast, never washed out.

## Motion
Primary spiral turns once every ~5 min; the counter-layer drifts opposite at ~60% speed so the interference petals shimmer without strobing. Radial swell breathes on a ~13 s period; hue creeps a full palette cycle in ~80 s.
