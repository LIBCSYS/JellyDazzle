# 051 Firework Garden

## Look
Classic dazzle fireworks (footage R11): radial particle bursts with gravity-drooped
trails accumulating over a deep plum gradient, each burst a single saturated hue with
white-hot heads. Old bursts slowly sink into the ground color instead of strobing away.

## Math
Burst k born at time `b_k` at `(x_k, y_k)`, hue `h_k`. Particle i: angle `θ_i`,
speed `s_i`. Position at age a: `x = x_k + s_i cos θ_i · a`,
`y = y_k + 0.85 s_i sin θ_i · a + ½ g a²` with `g = 0.0105`. Trail = the full swept
path (ages 0..min(age, LIFE)), brightness `∝ (a/a_max)^1.7`, whole-burst decay
`exp(-(age-LIFE)/150)` after burnout.

## Integer ARM64 plan
Per burst store origin, hue, per-particle (cosθ, sinθ, speed) as 8.8 fixed point from
a 256-entry 16-bit sine table. Each tick advance every live particle one step:
`x += vx; y += vy; vy += g` in 8.8 — no trig at draw time, gravity is one add.
Accumulation is free: plot into the indexed framebuffer and never clear; burnout
fade = palette-ramp remap of that burst's color range toward the ground color
(DAC writes, no pixel touches). Head sparkle = plot color 255 (white) at current pos.

## Palette pairing
Ground: plum→indigo vertical ramp (2 DAC ranges). Each burst grabs one of 12
pre-built 16-entry ramps (hue → white at head). Rotate ramps slowly for shimmer.

## Motion
2–3 bursts visible at once, each expanding ~3 s then freezing as a drooped flower;
20 recent bursts persist. Nothing blinks — only slow expansion, slow decay,
slow palette drift. Fully asymmetric, like the original.
