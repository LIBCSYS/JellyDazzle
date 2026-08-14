# 047 Target Choir

## Look
Seven BZ pacemakers — one center, six on a hexagon — each pumping out target rings that interfere into a lace of crimson, teal, and gold rosettes. The whole lattice breathes as ring trains collide and cancel, like seven stones dropped forever into the same pond.

## Math
- Centers: (0,0) phase 0, plus six at radius 85, angles j·60°, shared phase 2.0 (shared phase keeps 6-fold symmetry).
- w = (1/7)·Σ cos(0.21·dᵢ − 0.024·t + φᵢ), dᵢ = distance to center i.
- v = 0.5 + 0.45·w; color = cospal(0.85·v + 0.0004·t), relief multiply 0.80 + 0.20·w.

## Integer ARM64 plan
- Classic wave-interference repaint (research §6): per source per pixel, dist from ONE shared quarter-plane radius LUT (indexed by |dx|,|dy|), then sintab[(d·k − t·s + φ) & 255]; 7 table-read pairs + adds per pixel, no muls beyond the fixed k scaling (fold k into the LUT: store d·k pre-scaled u8).
- Sum in i16, shift to Q8, palette LUT with t-offset for the hue drift.
- Relief factor folded into a 256-entry brightness table indexed by the summed wave.
- Centers are static — every per-source LUT base pointer is precomputed at init; the inner loop is pure loads/adds.

## Palette pairing
Exotic cosine palette c=(2,1,0), d=(0.50,0.20,0.25): red channel cycles twice per palette period — crimson/teal alternating rings with steady gold interference nodes. Reads chemical, not psychedelic-noisy.

## Motion
Ring trains propagate outward at ~0.024 rad/frame ≈ one wavelength per 9 s — a slow chemical crawl. Global hue drifts one palette cycle in ~80 s. Interference nodes shift smoothly; nothing blinks because all seven sources share one frequency.
