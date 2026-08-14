# 086 Gem Orbit

## Look
Nine faceted hexagonal gems — a big one at center, eight smaller ones on an elliptical
orbit — glide over a deep-blue pond of faint breathing rings. Each gem is cut into six
angular facets that catch light one after another as it spins, every gem a different hue.

## Math
- Orbit: gem k at `(160 + 96·cos(Ω + kπ/4), 120 + 72·sin(Ω + kπ/4))`, Ω = 0.0025·t.
- Hex gem: `d = r / (R·cos(π/6)/cos((θ mod π/3) − π/6))`, inside d < 1.
- Facet index = ⌊6·θ/2π⌋; facet brightness `0.55 + 0.35·sin(2.1·facet + 0.010·t + 1.7k)`
  — facets glint sequentially (rotating highlight).
- Hue = 0.117·k + 0.045·facet + 0.0015·t; rim = 1 − 10|d−1|.
- Ground rings: sin(0.09·r₀ − 0.007·t) at ~6% intensity.

## Integer ARM64 plan
- Ground: radial ring value from an r-LUT (octagonal norm → 8-bit ring table), dark.
- Per gem only its bounding box (≤ 56x56 px): r octagonal, θ via octant LUT;
  the hex radius function is a 64-entry Q8.8 table (θ folded mod 60°).
- Facet = high 3 bits of the folded angle — free once θ is a fixed-point byte.
- Facet brightness: 9 gems x 6 facets = 54 scalar sines per FRAME (table reads),
  stored in a tiny array; per pixel just index it. Orbit positions: 9 sin/cos table
  pairs per frame. Zero per-pixel trig.

## Palette pairing
Full cosine rainbow distributed around the ring (hue step 0.117 per gem ≈ maximally
distinct neighbors) over a desaturated navy ground — bright objects, quiet field,
strong object separation.

## Motion
Orbit revolves once ≈ 42 s; gems self-rotate at ±0.008 rad/frame (alternating);
facet highlights sweep around each gem every ~10 s; ground rings drift outward.
Slow celestial clockwork — nothing jumps.
