# 072 Vine Waltz

## Look
A hedge of emerald vines climbs from the bottom of a plum-purple dusk, each stem shading green→gold as it rises, sprouting thorn-leaf fronds and finally curling into spiral tendrils tipped with glowing magenta blossoms. The whole hedge sways together like a slow waltz.

## Math
- 14 vines; heading `θ(s) = −π/2 + Σ turn(s)`, `turn = 0.032·sin(0.05s+φ1) + 0.014·sin(0.013s+φ2) + curl(s)`, `curl = 0.006·(s−s_c)·side` past s_c = 0.74·S (spiral tendril).
- Position = cumulative sum of `0.72·(cos θ, sin θ)`; sway adds `0.07·sin(0.014t+φ1+0.004s)` to θ.
- Growth: visible length `n = min(S, (t+150)·0.95·speed_i)`.
- Leaves: 5-point arcs every 34 samples, alternating sides; blossom = 2 rings of 9 dots, pulse weight `2.5+1.2·sin(0.03t)`.

## Integer ARM64 plan
- Heading in Q8 brads (256 = 2π); cumsum of sintab-built turn terms is pure adds; position steps are `costab[θ]·184>>8`.
- Growth counter n from frame counter — each frame extend each vine by 1–2 segments into a persistent accumulate buffer; sway rendered as a whole-buffer ±2px sinusoidal row shift (cheap) instead of redrawing.
- Curl term is a ramp add to θ per step — one add, no trig.
- Leaves/blossoms via small pre-rendered 8×8 sprite splats from shapes.bin.

## Palette pairing
Stem ramp H 0.34→0.13 (emerald→gold) with V rising toward tips; leaf green H 0.30; blossom magenta H 0.90; ground = vertical plum gradient `(38,10,54)→(26,5,33)`.

## Motion
Vines grow ~0.7 px/frame; hedge sways as one at ~9 s period; blossoms breathe at ~7 s. Growth loops by fading the buffer and re-sprouting every few minutes.
