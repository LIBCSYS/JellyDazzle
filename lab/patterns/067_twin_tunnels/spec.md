# 067 Twin Tunnels

## Look
Eight tunnel mouths (two real, mirrored 4-fold) wander slowly around the screen, their ring systems flying in opposite directions and beating against each other into hyperbolic moire lenses; at the mirror seams the mouths kiss their own reflections. Deep-sea teal and seafoam with magenta interference blooms.

## Math
- 4-fold fold first: `fx = |x-cx|, fy = |y-cy|` — everything after is automatically kaleidoscopic
- mouths orbit in the quadrant on Lissajous paths (4 incommensurate rates, amps 20–30 px)
- `d_i = 1900/(r_i + 9)`; field `= sin(d1*.5 + t*.40) + sin(d2*.5 - t*.33) + .55 sin((r1-r2)*.09)`
- third term = classic two-source interference (constant r1-r2 curves are hyperbolas)
- `idx = field*88 + 128`; shade `= (r1/(r1+40))(r2/(r2+40))*1.35` darkens both throats

## Integer ARM64 plan
- Fold = abs on centered coords: free (2 instructions), and only one quadrant of unique work if we render the quadrant and mirror-blit with negative strides (the dazzle original's own trick).
- Distances without sqrt: incremental d^2 across a scanline (adjacent pixels differ by 2(x-ox)+1 — pure adds), then `r = isqrt_tab[d2 >> k]` byte table.
- `depth = recip_tab[r]`; three sin16 lookups + shifts; sum through a 512-entry sat/bias table → idx.
- Orbits: 4 sin16 evaluations per FRAME.
- shade as product of two byte tables indexed by r1, r2 via one 256×256 mul LUT.

## Palette pairing
Deep-sea loop (abyss→teal→seafoam→magenta→abyss). Interference antinodes land on seafoam/magenta, nodes sink to abyss blue — bright lens shapes float on a dark ground, so the eye tracks the moire, not the noise.

## Motion
Ring systems creep at 0.40 / 0.33 phase per frame in opposite senses (beat period ≈ 90 s); mouths take 2.5–4 minutes per orbit loop. Constant slow evolution, no repeats, no flashes.
