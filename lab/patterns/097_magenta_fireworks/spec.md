# 097 Magenta Fireworks

## Look
Radial particle bursts pop every few frames across a hot magenta flood, each in one or two hues, their thin trails drooping under gravity and staying on screen forever — over hundreds of frames the field saturates into a dense multicolor fibrous wash, then the palette gently darkens. Replica of R11 (d11 fresh → d15 saturated → d17 dimming); deliberately the only asymmetric pattern in the set, exactly as in the original.

## Math
- Burst k spawns at `t0 = 9k`, center uniform in x, upper 2/3 in y (seeded RNG, fully deterministic).
- Particle j: angle `θ_j`, speed `v_j ∈ [0.9,2.3]`; trail point at age s: `x = cx + cosθ·v·s(1−0.004s)`, `y = cy + sinθ·v·s(1−0.004s) + 0.018·s²` (drag + gravity droop), s up to min(t−t0, 55).
- Trails are permanent (no per-frame clear); live bursts get near-white tips on the last 4 samples.
- Global dim `1 − 0.22·clip((t−620)/160)` reproduces the d17 "burning out" phase.

## Integer ARM64 plan
The natural accumulation routine: never clear, per frame extend each live burst by ONE step per particle. Particle state = (x,y) in 16.16 fixed point plus (vx,vy); update `x += vx; y += vy; vy += G; vx −= vx>>8; vy −= vy>>8` — drag as a shift, gravity as an add, no trig at runtime (launch angles come from the 16-bit sine/cosine table once at spawn). Plot one pixel per particle per frame: ~46 stores. Two palette indices per burst chosen from the hue wheel; tip glow = write index+brightRamp, let palette fade age it. Late darkening = DAC ramp-down, zero pixel work.

## Palette pairing
Hot magenta ground (the signature of d11) with burst hues drawn from pure red, cyan, yellow, blue, green, orange, white, pink — one or two per burst so each explosion reads as a colored event, not confetti soup, until natural accumulation does the mixing.

## Motion
A burst blooms over ~55 frames (≈2 s), new bursts every 9 frames — constant gentle popcorn, nothing instantaneous; trails never move once laid. Endgame is a slow 160-frame dim, not a cut.
