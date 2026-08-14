# 034 Lissajous Weave

## Look
A dense 3:4 Lissajous basket that fills the frame edge-to-edge, woven from a single saturated hue that slowly walks the color wheel; the sweeping phase makes the figure appear to tumble in 3D. Mirrored left-right so the weave reads as one symmetric tapestry.

## Math
Detuned Lissajous with phase sweep, accumulated with age fade:
- s = 0.17·n (44 substeps/frame)
- x = 118·sin(3s + delta), delta = 0.0013·n  (phase sweep = apparent rotation)
- y = 88·sin(4.0015·s)  (sub-detune: never closes, weave stays dense)
- weight w = exp(−age/450); 2-fold mirror in x
- hue = 0.0021·s + 0.0009·t, S=0.95 — hue moves slow enough that the visible window is one scheme

## Integer ARM64 plan
- Research §6 verbatim: two 32-bit sub-BAM phase accumulators. The 4.0015 detune is increment = 4.0015·base in Q16.16; delta sweep is a third tiny increment added to the x phase.
- Per point: 2 sine lookups + 2 Q15 multiplies. Cheapest pattern in the set — spend the savings on 60+ substeps for a silky line.
- Mirror: one extra store with negated x offset.
- Hue: global palette rotation (increment a palette phase per frame, index = (s>>k + phase) & 255 into an HSV-ring LUT) — the classic zero-cost color animation.

## Palette pairing
Full-spectrum HSV ring, but sampled through a window that crawls ~1 cycle per 1100 frames — each moment reads as a bold monochrome (emerald, then indigo, then amber...) with edge highlights where the curve turns. Deep navy vignette.

## Motion
Apparent 3D tumble from the delta sweep: one full apparent rotation ≈ 80 s. Trails ~7.5 s. The eye tracks slow rolling lobes — hypnotic, zero strobe.
