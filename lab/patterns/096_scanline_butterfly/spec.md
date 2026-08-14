# 096 Scanline Butterfly

## Look
The whole field is closely spaced horizontal scanlines whose spacing bulges around two wing lobes, producing a big green-to-yellow moiré butterfly with a cyan diamond heart and a hot red spindle at dead center; red-and-white concentric ring ornaments occupy the corners. Replica of R9 (frames c10–c13) — the shimmer lives in the render, not the camera.

## Math
- Wing field: `wing = exp(−d/52)`, `d = √((|dx|−78)² + (1.45·dy)²)` (mirrored → both wings from one term).
- Scanlines: `L = sin(0.85·y + 30·wing + t·0.022)`; lit where `L > −0.15`, brightness = normalized L. The 30·wing phase term is what warps line spacing into interference around the lobes.
- Color: ramp keyed on `wing` — dark green → green → yellow → white at the cores; heart = diamond `|dx|/46 + |dy|/62 < 1` recolored cyan; spindle = shrinking-width column at center recolored red; all still multiplied by the line field so everything shimmers coherently.
- Corner rings: `sin(dcorner·0.55 − t·0.017)` → red band / white band thresholds inside r<46.

## Integer ARM64 plan
This is a full-repaint computed field, mode-13h style: per pixel one 16-bit sine lookup at phase `(y·K1 + wingT[x,y]·K2 + t·K3) & 0xFFFF`. The wing exponential is precomputed ONCE into an 8-bit map (it never changes) — so the inner loop is: load wing byte, add row phase + time phase, sine table, threshold, then use (wing byte, brightness byte) as index into a 256×16 shade LUT → palette index. Heart/spindle/corner regions are precomputed 2-bit region masks folded into the same LUT dimension. No per-pixel float, trig, div, or sqrt at runtime.

## Palette pairing
Green→yellow→white wing ramp on near-black, cyan heart, pure red spindle, red/white corner rings — sampled straight from c10. A cohesive complementary triad (green field / red center) like the original.

## Motion
Only the line phase moves: bands crawl upward through the wing field at 0.022 rad/frame (~4.7 s per line-pitch), which makes the interference pattern roll hypnotically while the butterfly silhouette stays put; corner rings breathe on an independent slower phase. Pure moiré drift — smooth, zero strobe.
