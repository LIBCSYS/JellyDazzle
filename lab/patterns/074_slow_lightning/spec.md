# 074 Slow Lightning

## Look
Two Lichtenberg bolts — one violet, one electric blue — crawl down a near-black sky in slow motion, forking into ragged branches wrapped in soft neon glow. The channels sway gently like held plasma filaments and breathe in brightness rather than strobing.

## Math
- Main path: 90 nodes, `x = x0 + Σ smooth3(gauss)` with a linear pull keeping it vertical; 7 branches fork at random nodes with their own smoothed walks; every node carries arclength `al`.
- Growth: visible where `al < tip(t) = 70 + 0.42·(t+offset)`; head highlight `(1−(tip−al)/26)²`, tail fade over 40 px.
- Sway: `x += 3·sin(0.008t + 0.02y)` applied to all points.
- Glow = 4-pass 5-tap blur of the core deposit; brightness pulse `0.85+0.15·sin(0.02t)`.

## Integer ARM64 plan
- Bolt geometry generated once per episode with an LCG (`seed = episode#`), stored as i16 node arrays + Q8 arclengths — growth is a prefix draw by arclength threshold, one compare per segment.
- Segment densify = fixed 4× lerp (shifts). Sway = per-scanline x-offset table from sintab, applied at blit time.
- Glow: draw core into buffer A; buffer B = box-blurred A (two-pass, running-sum, integer); final = palette[A·2 + B].
- Head/tail weights from a 64-entry ramp LUT indexed by `tip−al` (Q8).

## Palette pairing
Bolt A hue 0.72 (violet), bolt B hue 0.55 (cyan-blue); cores desaturate to near-white; ground vertical `(4,1,13)→(9,3,26)`. Glow inherits bolt hue at ~40% value.

## Motion
Tips advance ~0.4 px/frame (a full strike takes ~25 s); the two bolts are phase-offset so one is always growing. Sway period ~13 s; pulse period ~5 s, ±15% only — never a strobe. Completed bolts fade over ~8 s while a new seed begins.
