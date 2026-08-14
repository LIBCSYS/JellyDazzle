# 077 Mycelium Veil

## Look
A radial web of amber and rose hyphae wanders outward from a warm-glowing heart, filaments meandering like living threads with bright growing tips and pulsing spore nodes strung along their length. Over time it becomes a dense golden veil filling the frame.

## Math
- 140 filaments; heading `θ(s) = θ0 + rot(t) + Σ gauss·0.085 + 0.25·sin(0.03s+φ+0.006t)`; position = cumulative `(1.05 cos θ, 0.85 sin θ)` from center.
- Growth `n_i = min(260, 0.33·(t+130)·spd_i)`; tips (last 7 samples) deposit ×2.2.
- Along-thread flicker weight `0.55 + 0.25·sin(0.15s − 0.03t)` (slow crawl of light).
- Spore nodes at s = 40,100,160,220 pulse `1.5+1.0·sin(0.025t+…)`.
- 30% of filaments rose (H≈0.92), rest amber (H 0.05–0.12); whole web spins at 0.0015 rad/frame.

## Integer ARM64 plan
- Wander tables: per-filament precomputed cumulative-noise heading arrays (u8 brads, LCG-generated per episode) — runtime heading = base + table + sintab breathing term: adds only.
- Persistent accumulate buffer: each frame each living filament appends ≤1 new segment (Q8 step via costab); flicker done by palette-cycling the deposit values, not redrawing.
- Tip glow = extra 3×3 splat at current head. Node pulses = 8 sprite splats with sintab-scaled weight.
- Global spin folded into θ0 offset (one add per filament per frame).

## Palette pairing
Amber ramp `(255,170,60)`-family (H 0.05–0.12) + rose accents H 0.90–0.95, both at S 0.85; dark cocoa ground `(15,6,5)` with warm center glow. Slow global hue drift +0.0002/frame.

## Motion
Tips advance ~0.35 px/frame (full veil in ~3 min); light crawls along threads (~10 s per cycle); nodes breathe at ~8 s; the web revolves once in ~70 min, then fades and re-sprouts from the heart.
