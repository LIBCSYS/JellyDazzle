# 016 Shadebob Rosette

## Look
A single Lissajous shadebob whose additive trail is folded six ways: on screen, twelve synchronized bobs weave a glowing rosette whose self-crossings bloom into hot knots. Trails shade indigo → magenta → gold as they accumulate, with white-hot cores where the path overlaps itself.

## Math
- Bob path: `bx = 112·sin(0.0123τ)`, `by = 88·sin(0.0177τ + 1.1)`; the bob's polar angle is folded into the 6-fold wedge before stamping.
- Trail: for the last K=150 path samples (τ = t − 3.2j), `field += 0.30·(1 − j/K)^1.4 · exp(−d²/2σ²)`, σ = 7 px, d measured in folded wedge coords.
- Tone map: `g = tanh(field)` — crossings glow, nothing washes out.
- HSV: `hue = 0.78 − 0.62g + 0.0005t`, `val = g^0.75`, `sat = clip(1.05 − 0.35g, 0.25, 1)`.

## Integer ARM64 plan
- True accumulator, the dazzle genus itself: keep a persistent 16-bit **wedge buffer**; per frame stamp one 32×32 blob sprite (precomputed Gaussian byte mask) at the folded bob position with `uqadd`-style saturating adds — only the blob footprint is touched.
- Global decay: every 4th frame subtract 1 from the whole wedge via a 512-entry clamp LUT (or NEON `uqsub.16b` with a splat of 1) — old rosettes fade as new ones bloom, exactly xscreensaver-shadebobs style.
- Bob position: two `sin_tab` reads + 8.8 multiplies; angle fold with an integer modulo-by-wedge (wedge = 1/6 turn → multiply-shift trick, no division).
- Resolve: `pix = palette[tone_tab[wedgebuf[fold_map[i]] >> 4]]` — two table reads per pixel; `tone_tab` bakes the tanh curve; palette regenerates each frame for the hue creep.

## Palette pairing
Cold-to-hot glow ramp: near-black indigo floor → violet → magenta → gold → cream at the knots; saturation drops as value rises so cores read incandescent, never chalky.

## Motion
Lissajous periods ~8.5 s and ~5.9 s (ratio ≈ 1.44, non-repeating), trail history spans ~16 s, hue base drifts a full wheel in ~33 min. The rosette continuously re-weaves itself — new petals bloom as old ones melt back to indigo. No element moves faster than the bob (~2 px/frame).
