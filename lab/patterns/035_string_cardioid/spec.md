# 035 String Cardioid

## Look
Times-table string art on a circle of 180 pins: thousands of rainbow chords whose envelope morphs from cardioid through nephroid into higher epicycloids as the multiplier breathes from 2 toward 3.6. The chord colors are pinned to the rim position, so the wheel reads as a stable rainbow ring with an evolving heart.

## Math
Modular multiplication table on a circle, chords accumulated with age fade:
- chord m connects pin p = (7m mod 180) to pin k·p, on circle radius 108
- angles a1 = p·2π/180, a2 = k·p·2π/180; endpoints (R cos, R sin)
- k(m) = 2 + 1.6·(0.5 − 0.5·cos(0.0011·m))  (envelope family sweep)
- 3 chords/frame, each sampled at 44 points; weight w = exp(−age/900)
- mirror in y; hue = p/180 + 0.0003·t (rim-position rainbow)

## Integer ARM64 plan
- Pins: 180-entry precomputed (x,y) table (Q8.8). Chord endpoints are 2 table fetches; a2 needs (k·p) mod 180 — k in Q8.8, product >>8, subtract-loop mod (max 2 iterations) or reciprocal-mulhi.
- Chord rasterization: fixed-point DDA — dx = (x2−x1)/S via one division per CHORD (not per pixel), then S adds. S=44 steps, 3 chords/frame: trivial.
- Envelope sweep k: one slow BAM cosine lookup per chord.
- Hue = pin index — direct palette LUT index, no color math.
- Accumulator fade: per-frame Q16 multiply, k ≈ 65463 (tau 900 — long memory is what builds the envelope density).

## Palette pairing
Full rainbow locked to rim angle (hue = pin/180) over a warm dark amber vignette — spatially anchored color can't wash out, and the caustic envelope glows white-hot where chords bunch.

## Motion
The multiplier takes ~95 s to travel 2 → 3.6 → 2, morphing cardioid→nephroid→3-cusp smoothly. Individual chords appear 3/frame with 15 s trails — the figure evolves like slow embroidery, nothing flashes.
