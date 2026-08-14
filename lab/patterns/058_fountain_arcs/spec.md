# 058 Fountain Arcs

## Look
A garden-sprinkler fountain at bottom center launches droplets whose full parabolic
arcs stay painted, mirrored left/right into a peacock fan; the launch angle sweeps
side to side while hue advances, layering a rainbow feather-fan of gravity arcs that
slowly dissolves as new arcs land over it.

## Math
Launch j at `t_j = 3j`: angle `α_j = π/2 + 0.72 sin(0.045j)`, speed
`s_j = 2.45 + 0.3 sin(0.11j)`, hue `0.011j`. Trajectory:
`x = vx·a`, `y = y0 - vy·a + ½·0.03·a²` with `vx = 0.62 s cosα`, `vy = s sinα`;
landing at `a = 2vy/g`. Trail = whole flight path, head-weighted `(a/a_max)^1.3`,
post-landing decay `exp(-(age-a_land)/260)`. Mirror across the vertical axis.

## Integer ARM64 plan
Textbook integer particle: per live droplet keep (x, y, vx, vy) in 8.8 fixed point;
per tick `x += vx; y += vy; vy += g` — three adds, no trig after launch (launch
vx/vy from the sine table once every 3 frames). Plot into the accumulating indexed
framebuffer; mirror plot is `(W-1-x, y)`. Arc fade-out = per-launch palette ramp
decayed by a DAC dimming pass every N frames (no pixel rewrites). ~120 live droplets
→ trivial per-frame cost; the painted arcs cost nothing to keep.

## Palette pairing
Continuous hue wheel indexed by launch number → adjacent arcs are neighbor hues,
the fan reads as a smooth spectrum; twilight blue-teal ground ramp; warm gold
emitter glow pulsing softly.

## Motion
Sweep period ~14 s paints the fan left-right-left; a droplet flies ~5 s; arcs persist
~25 s then sink into the ground color. Continuous gentle rain of color, no strobing.
