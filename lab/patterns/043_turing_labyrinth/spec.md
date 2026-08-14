# 043 Turing Labyrinth

## Look
A fingerprint / brain-coral labyrinth in indigo and amber: reaction-diffusion stripes locked into a 4-fold symmetric maze, with bright seams glowing along the stripe boundaries. Color flows along the corridors while the whole maze softly breathes.

## Math
- Seed: symmetric white noise u (4-fold mirror average).
- Iterate 70×: u ← clip(12·(B₂(u) − B₈(u)), −1, 1), where B₂ = 3×3 box blur applied twice (small scale) and B₈ = six more box passes (large scale). Iterated band-pass + saturation self-organizes into labyrinth stripes (the classic RD fake, research §3 fake #1).
- Animate: uu = lerp(u, blur(u), 0.35·(0.5+0.5·sin(0.006·t))) — contrast breathing.
- Color = cospal(0.7·(0.5+0.5·uu) + 0.0012·t); ridge highlight += 0.22·(1−|uu|)².

## Integer ARM64 plan
- Pattern is precomputed once at init (or grown live at a few iterations/frame for a "self-organizing" intro): box blurs are two-pass sliding sums (adds + shifts, no muls); clip is `smax`/`smin`; ×12 is shift+add. All i16 Q12.
- Steady state stores ONE i8 field; per frame per pixel: one field read, blend with pre-blurred copy via 8-bit lerp (mul+shift), palette LUT read with a t-offset index — palette cycling does the flowing-color work for free.
- Ridge term: |u| from `abs`, squared via 8-bit mul, added before palette clamp — or baked as a second LUT dimension.

## Palette pairing
Ocean cosine palette (a≈(0.35,0.40,0.45), d=(0.60,0.75,0.90)): deep navy grooves, amber/copper stripes, pale seam-light. Two-tone families keep the maze legible.

## Motion
Hue flows along the stripes at one palette cycle per ~28 s (reads as chemicals crawling through the maze). Stripe contrast breathes on a ~17 s period. The maze itself is stationary — calm, hypnotic, zero strobe.
