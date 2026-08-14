# 021 Ripple Duet

## Look
Two invisible pebbles orbit the center of a lagoon-teal pond, their expanding ring
waves overlapping into slowly turning hyperbolic fringe families. Bright cyan crests
braid over deep blue troughs while a magenta-tinted beat envelope glides through.

## Math
Sources orbit on counter-rotating ellipses:
`c1 = C + (70 cos .009t, 50 sin .009t)`, `c2 = C + (70 cos(-.007t+2.1), 50 sin(-.007t+2.1))`.
Field `f = sin(k d1 - .045t) + sin(k d2 - .038t)`, `k = 0.22`, `d_i = |p - c_i|`.
Beat envelope `env = cos(k(d1-d2)/2)` (constant-difference hyperbolas).
Hue = 0.52 + 0.10 env + 0.06 f + slow drift; Val = 0.24 + 0.55(0.5+f/4) + 0.18 env².

## Integer ARM64 plan
- One oversized (2W x 2H) distance byte table `dist[]` precomputed at init; the two
  moving sources become two window offsets into it (demoscene moiré trick) — zero
  per-pixel sqrt.
- `sin` of distance via 16-bit sine LUT indexed by `(dist[o1+i]*K + phase1) & 0x3FF`;
  two lookups + one add per pixel, 8.8 fixed point.
- `d1 - d2` for the envelope is one byte subtract feeding the same sine table.
- HSV composite baked to a 2D palette LUT `pal[env_band][f_band]` (16x64 entries)
  regenerated per frame on the CPU (4096 entries, cheap) — per pixel it is one
  table read. Palette drift = rewriting the small LUT, not the frame.

## Palette pairing
Lagoon family: deep indigo troughs, teal midtones, bright cyan crests, envelope
pushes highlights toward soft violet. Full-field slow hue drift (~0.0004/frame)
keeps the scheme mutating without ever strobing.

## Motion
Sources complete an orbit in ~700-900 frames; ring phase crawls outward at
~0.04 rad/frame (waves visibly swim, never flicker). Fringes rotate as the sources
pass each other — the whole pond breathes at beat frequency. Nothing jumps.
