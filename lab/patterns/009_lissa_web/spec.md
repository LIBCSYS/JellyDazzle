# 009 Lissa Web

## Look
A glowing Lissajous ribbon stamped N times around the center (N steps through 5, 7, 9, 12, 8, 6), weaving a luminous guilloche rosette on near-black — like spirograph string-art made of neon thread. The curve window crawls forward so the web perpetually re-laces itself.

## Math
- Curve: p(s) = (92·sin(3s + 0.0015t), 92·sin(4·1.0012·s)) — 3:4 Lissajous with a 0.12% detune so the figure precesses instead of closing (geometry.md §6).
- Trailing window: s ∈ [0.012t, 0.012t + 25.2], 4200 samples, weight ramps 0.15→1.0 toward the head (comet tail).
- C_N stamp: rotate all samples by 2·pi·k/N + 0.0018t for k = 0..N−1; N = SEQ[t/320].
- Render: bilinear splat into an accumulator, one 5-tap box soften, glow = 1 − exp(−0.55·b); color = cosine palette of (glow, screen radius, t); composite over a dim radial ground.

## Integer ARM64 plan
- This is a stamp routine, not a per-pixel field — exactly the original dazzle's architecture. Per frame: ~4200 curve points × N rotations; each point = 2 phase accumulators (BAM16, 32-bit sub-BAM for the 1.0012 detune, geometry.md §5) + 2 sine lookups + rotation by per-copy (c,s) — 4 multiplies/point.
- Splat: nearest-pixel add into an 8-bit count buffer with saturating `uqadd` (skip bilinear on target; at 320×240 it's invisible). Tail weight = add 1, 2, or 3 by segment — no per-point multiply.
- Persistence/decay instead of per-frame redraw: multiply buffer by 250/256 each frame (`umull`+shift per 16 pixels with NEON) and only stamp the new head segment — massively cheaper and matches the original's accumulate-then-fade feel.
- Glow curve 1−exp(−x) = 256-entry LUT on the count byte; palette = DAC indexed by that byte; box soften can be dropped on hardware (CRT/phosphor look) or done as NEON horizontal adds.

## Palette pairing
Chartreuse→amber→violet along the glow ramp on a #050a12 ground — thread hue keyed to local density plus radius, so crossings burn brighter in a different hue than single strands.

## Motion
Window advances 0.012·4200-sample lengths/frame (head crawls, tail dissolves); figure precesses via the 0.12% detune (~minutes per full precession); stamp ring rotates ~1 rev / 58 s; N changes every ~5.3 s. Dreamy, continuous, zero flicker.
