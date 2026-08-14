# 048 Excitable Spirals

## Look
A Greenberg–Hastings excitable medium: neon wavefronts crawl through a dark field, curling at their broken ends into greek-key spiral cores — like luminous circuitry etching itself, mirrored 4-fold. Bright leading edges trail fading refractory embers.

## Math
- Grid 160×120, K=8 states: 0 rest, 1 excited, 2..7 refractory; wrap.
- Rule: rest fires (→1) if ≥1 Moore neighbor is excited; any nonzero state increments mod K.
- Seed: sparse symmetric broken wavefronts — three excited bars each backed by a refractory bar (K/2) on one side, mirrored 4-fold. Broken ends curl into counter-rotating spiral pairs; a full-random seed gives turbulence instead, so don't.
- Precompute 380 gens, skip 100; scrub at 0.12 gen/frame, smoothstep crossfade.
- Color: age = (s−1)/(K−1); brightness = 0.22 + 0.78·(1−age)^1.2 for s>0, 0 at rest; col = dark base + cospal(0.60 + 0.25·age + 0.0005·t)·brightness.

## Integer ARM64 plan
- Same double-buffered u8 engine as 041 with a different inner loop (research §5 route 1): count excited neighbors (8 loads, `cmeq` + accumulate in NEON), fire test, else increment with wrap (add + `and` since K=8 → `& 7` — free).
- Run live: 1 CA step per ~8 frames; after settling the medium is period-K (=8 steps), so it runs forever.
- Colorize: state → (hue, brightness) via a single 8-entry table, then palette LUT with t drift; the (s−1) trail gradient is baked into the table. Crossfade as in 041.
- 2×2 upscale + cheap 3×3 smooth on the color buffer.

## Palette pairing
Electric cosine palette d=(0.55,0.70,0.85) over near-black navy: wavefronts ignite cyan-white, embers cool through violet to wine. Hue drift re-tints the circuit from copper to sapphire over ~80 s.

## Motion
Wavefronts advance one cell per 8 frames (~4 px/s) — a deliberate crawl; spiral cores rotate once per K·8 ≈ 64 frames ≈ 2 s of gentle churning, softened by the crossfade. Dark ground keeps contrast without any flash.
