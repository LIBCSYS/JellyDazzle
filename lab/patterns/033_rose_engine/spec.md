# 033 Rose Engine

## Look
A molten guilloché rose — crimson core rays fading through orange into a gold-green engine-turned rim — spun to 7-fold dihedral symmetry. The petal count slides continuously between 1 and 4-ish, so the medallion perpetually re-cuts itself like a rose-engine lathe.

## Math
Offset rhodonea with drifting k, accumulated with age fade:
- theta = 0.19·n (32 substeps/frame)
- r = (76 + 8·sin(0.0013·n))·cos(k·theta) + 18·sin(0.0004·n)
- k = 2.5 + 1.5·sin(0.0006·n)  (fractional petal count — the rose never closes)
- x = r·cos(theta), y = r·sin(theta); weight w = exp(−age/360)
- D7 symmetry: 7 rotations × mirror = 14 copies
- hue = 0.93 + 0.24·(r+94)/188 + 0.02·sin(0.0005·n)  → radius maps crimson→gold

## Integer ARM64 plan
- Straight from research §3: theta BAM16 accumulator; k as Q8.8, kt = (k·theta)>>8 wraps naturally; r = (A·cos_bam(kt))>>15 plus offset term. Signed r plots the opposite petal for free.
- 4 sine lookups + 3 multiplies per point; k, A, offset each driven by slow 32-bit sub-BAM LFOs (3 more lookups per SUBSTEP, amortizable per-frame).
- D7: precompute 7 sector (cos,sin) Q15 pairs; rotate-and-mirror deposit, 14 stores.
- Radius→hue: palette index = (r + 94) · 683 >> 8 (maps to 0..255), i.e. one multiply — color comes from a 256-entry crimson→gold gradient LUT, no HSV at runtime.

## Palette pairing
Fire gradient LUT: deep crimson (H .93) at center through orange to gold/green-gold (H .17) at the rim, on a near-black maroon vignette. Radius-locked hue means concentric color identity — always cohesive.

## Motion
k drifts one full petal-family sweep every ~175 s; amplitude and center offset breathe on ~80 s and ~260 s periods. The rim shimmers as new petals cut over 6-second trails. No element moves faster than ~2 px/frame.
