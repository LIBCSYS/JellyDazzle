# 040 Maurer Rose

## Look
A Maurer-rose walk — straight chords hopping around a sine rose at a drifting step angle — piles up into a granular mandala with concentric hue bands: magenta core, gold mid-ring, green rim. As the step angle and petal count crawl, the mandala dissolves through polygon-web phases and re-crystallizes, like sand art re-pouring itself.

## Math
Chorded rose (Maurer walk), segments accumulated with age fade:
- segment m connects rose points at θ1 = m·D(m), θ2 = (m+1)·D(m), with r = A·sin(k·θ), A = 102
- D(m) = 1.2399 + 0.12·sin(0.0005·m)  (step angle drift — the morph driver)
- k(m) = 4 + 2·sin(0.00023·m)  (petal family drift)
- 4 segments/frame, 34 samples each; weight w = exp(−age/950); mirror in y
- hue = 0.52 + 0.38·(|r1|+|r2|)/(2A) + 0.0005·t  (radius→hue: concentric bands)

## Integer ARM64 plan
- θ = m·D via accumulation: θ += D each segment (D itself updated by a slow LFO) — no multiply. k·θ via Q8.8 k times BAM θ, >>8.
- Endpoints: research §3 rose evaluation — 4 sine lookups + 3 Q15 multiplies each; reuse segment end as next segment start (1 evaluation/segment amortized).
- Chord: fixed-point DDA, one reciprocal-table multiply for the step (S constant = 34).
- Radius→hue: |r1|+|r2| is already computed; palette index = (sum·320)>>16 into a 256-entry tri-band LUT (magenta→gold→green) whose base index rotates slowly per frame.
- Mirror = second store, negated y offset. Accumulator fade k ≈ 65467 (long tau builds the granular density).

## Palette pairing
Concentric tri-band: magenta (H .90) core → gold (H .13) → spring green (H .35) rim, slowly rotating through the wheel as a set (~2 min/cycle), over a dark teal vignette. Radius-locked hue keeps every phase of the morph cohesive.

## Motion
D drifts ±0.12 rad over ~200 s — the walk sweeps through near-rational step angles, morphing dense mandala ↔ open star-polygon web. k adds a petal-count swell on ~7 min. 16-second trails make transitions feel geological, never abrupt.
