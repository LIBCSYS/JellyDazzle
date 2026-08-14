# 024 Munch Frost

## Look
The HAKMEM munching-squares lattice (x XOR y) rendered as crystalline rainbow frost:
nested diamond mouths and Sierpinski steps in full-spectrum bands that slowly rotate
and breathe in scale while color washes through the fractal. Sharp, glittering,
never noisy — the bands are wide and ordered.

## Math
Rotated, breathing coordinates: `(xi,yi) = R(0.0015t) · (p-C) · (1 + 0.22 sin(.0025t))`.
Field `v = ((⌊xi⌋ XOR ⌊yi⌋) + 0.6t) & 127`, `u = v/127`.
Hue = `u + 0.0008t` (full wheel), Val = `0.18 + 0.82 u^0.75`, Sat dips slightly at
band centers for icy highlights.

## Integer ARM64 plan
- Rotation+zoom of integer coords is the rotozoom DDA: per row, 16.16 accumulators
  (xi,yi) advanced by (du,dv) computed once per frame from the sine LUT.
- Per pixel: take high 16 bits, `EOR`, add time byte, `AND #127` — pure ALU, zero
  tables. NEON: 16 lanes of EOR/ADD/AND per iteration; this is the cheapest effect
  in the whole set.
- 128-entry rainbow palette LUT rebuilt per frame (wheel rotation + the u^0.75
  value curve baked in); per-pixel color = one `TBL` lookup.
- The +4096 bias keeps coords positive so arithmetic shift = floor (no branch).

## Palette pairing
Full HSV wheel spread across the 128 XOR bands — the Sierpinski structure keeps
the rainbow ordered into concentric diamond rings (very dazzle: "never the same
color scheme" — wheel phase means every pass through is differently keyed).
Dark band floor 18% value gives contrast so it never washes out.

## Motion
Three independent slow clocks: band cycling (+0.6/frame through a 128 cycle —
the munching mouths open/close), frame rotation (full turn ≈ 70 s), zoom breath
(period ≈ 40 s). All phase-continuous, nothing jumps or strobes.
