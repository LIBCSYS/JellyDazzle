# 025 Ember Triad

## Look
Three wave sources ride the corners of a slowly turning triangle, each ringing at a
slightly detuned frequency; where their wavefronts agree the screen ignites in
golden-orange crests, where they disagree it cools into wine-purple troughs.
A three-way ripple-tank sunset with drifting islands of constructive interference.

## Math
Sources at `C + 62·(cos, sin·0.74)(0.004t + i·2π/3)`, i = 0..2.
Field `f = (1/3) Σ sin(k_i d_i - w_i t)` with detuned `k = (0.30, 0.315, 0.285)`
and `w = (0.050, 0.042, 0.058)` — the k/w detuning makes the constructive zones
themselves migrate (spatial + temporal beats).
Hue `0.06 + 0.13 f` (wine→ember→gold), Val `0.16 + 0.84 (0.5 + 0.62 f)^1.25`.

## Integer ARM64 plan
- Same oversized distance-byte-table trick as 021/022, three window offsets.
- Three sine-LUT reads per pixel, indices `(dist[o_i+p] * K_i + phase_i) & 0x3FF`;
  the K_i multiply is a 16-bit `MUL` + shift (or three pre-scaled distance tables,
  one per K_i, making it pure loads). Sum in 16-bit lanes, `SQADD` saturating.
- Detuning lives entirely in the per-frame constants K_i / phase_i.
- Palette: 256-entry wine→gold ramp; field high byte indexes it via `TBL`.
  Hue micro-drift = rebuilding the 256-byte ramp each frame.

## Palette pairing
Sunset family locked to one arc of the wheel: deep wine (hue ~0.93 wrap) through
ember red and orange to bright gold (hue ~0.19). Saturation dips slightly at
extremes so gold crests bloom toward cream. No cold colors — a deliberate
restricted scheme, per the original's per-routine color discipline.

## Motion
The triangle turns once per ~26 s; each ring train crawls outward at its own
speed so beat islands drift around the triangle like slow weather. Value floor
16% keeps troughs readable; peaks bloom smoothly — no strobe, no flicker.
