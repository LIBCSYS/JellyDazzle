# 091 Diamond Confetti

## Look
A single huge rhombus fills the screen: thick cyan and magenta rims, an interior packed with a 4-fold-mirrored quilt of tiny rainbow confetti cells, and faint concentric diamond outlines being born at the center and expanding to the rim. Thin cyan streaks shoot out of the left and right vertices — a 1:1 replica of reference frame a01.

## Math
- Diamond metric: `dd = |x-cx|/148 + |y-cy|/92`; interior `dd < 0.88`, rims at `0.88..0.94` (magenta) and `0.94..1.02` (cyan).
- Confetti: cell = `(|dx|//6, |dy|//6)` (absolute values give the 4-fold mirror for free); two integer hashes `h1,h2` per cell drive hue `(h1/255 + t·0.0022·(0.5+h2/255)) mod 1`, saturation and a slow value wobble.
- Expanding outlines: `ph = dd·5 − t·0.016`; ring where `frac(ph) < 0.16`, color = `floor(ph) mod 3` → cyan/magenta/green, brightness ramps with dd.
- Streaks: rows `|dy| < 3` outside the diamond, intensity `0.5+0.5·sin(|dx|·0.20 − t·0.05)`.

## Integer ARM64 plan
Diamond metric is pure adds/shifts: `dd16 = (ax·K1 + ay·K2) >> 8` with K1,K2 as 8.8 fixed-point reciprocals — no sqrt, no div. Cell hash = `(cx·73856093 xor cy·19349663) & 255`, all 32-bit multiplies. Hue drift = per-cell 16-bit phase accumulator stepped once per frame; color via a 256-entry HSV wheel table. Rings: `ph = dd16·5 − t·kv` in 8.8 fixed point, band test is a mask on the fractional byte, ring color from `(ph>>8) mod 3` lookup. Streak shimmer from the 16-bit sine table indexed by `ax<<5 − t·kk`. Zero per-pixel float/trig/div.

## Palette pairing
Full-saturation rainbow interior (the a01 confetti is unconstrained multi-hue), locked rim pair cyan-outer / magenta-inner, green as the third outline hue — exactly the rim colors visible in a01.

## Motion
Nothing translates: the diamond silhouette is rock stable. Confetti cells drift hue individually at ~0.002 cycles/frame (a slow shimmer, never a flip), outlines glide outward one rim-width every ~60 frames, rim glow pulses on a 300-frame sine, streaks ripple gently. Calm churn, no strobe.
