# 032 Harmonograph Veil

## Look
A gauzy four-pendulum harmonograph web in teal-to-violet, mirrored four ways so it reads as folded silk; the detuned second pendulum makes the veil slowly precess and re-drape. Feels like watching curtains of light settle.

## Math
Undamped two-oscillator-per-axis harmonograph; age fade plays the damping role:
- s = 0.23·n (40 substeps/frame)
- x = 74·sin(s + 0.3) + 46·sin(2.0025·s + p), p = 0.0011·n  (detune 2.0025:1 = slow precession)
- y = 62·sin(1.503·s) + 40·sin(2.996·s + 1.1 + 0.5·p)
- weight w = exp(−age/420); 4-fold mirror: deposit (±x, ±y)
- hue = 0.52 + 0.18·sin(0.0007·n) + 0.04·sin(0.11·s), S=0.80

## Integer ARM64 plan
- Four BAM16 phase accumulators with 32-bit sub-BAM increments — the 2.0025:1 detune is exactly the sub-BAM trick from research §5/§6 (increment = round(2.0025·base) in Q16.16, top 16 bits index the sine table).
- Each point: 4 sine lookups, 4 Q15 multiplies, 2 adds. No damping exp needed — fade IS the decay, one Q16 buffer multiply per frame (k ≈ 65380).
- 4-fold mirror is sign flips only: deposit at (cx±x, cy±y) — zero extra math.
- Hue band: index a 64-entry teal→violet palette slice by a slow LFO (another BAM accumulator + table lookup per point, quantized).

## Palette pairing
Analogous cool band: teal (H .48) through blue to violet (H .72), high value, moderate saturation, on a deep sea-green vignette. Works because all strokes stay within a 90-degree hue arc — overlaps blend instead of greying.

## Motion
The web precesses at ~0.0011 rad/substep phase drift — a full re-drape every ~2.5 min; the hue band swings over ~2.5 min as well. Trails last ~7 s. Everything is sub-pixel-per-frame velocity: pure drift, no strobe.
