# 046 RPS Trichrome

## Look
Three species — magenta, lime, azure — locked in a rock-paper-scissors chase, their territories drawn as bold six-petaled flower rings that spiral slowly inward. Domain borders are soft gradients, like ink fronts advancing through wet paper.

## Math
- Fold: tf = |mod(θ, π/3) − π/6|.
- Species fields (i = 0,1,2): fᵢ = cos(7·tf + 0.14·r − 0.010·t + 2πi/3) + 0.35·cos(0.06·r − 0.006·t + πi/3).
- Soft winner-take-all: wᵢ = softmax(4·fᵢ) — each pixel blends the three species colors by wᵢ (the RPS "dominance" picture: each phase-shifted field chases the next).
- Species colors: cospal(i/3 + 0.0005·t). Relief: multiply by 0.68 + 0.32·(0.5+0.5·max fᵢ).

## Integer ARM64 plan
- Repaint. tf, r from LUTs as in 042; each fᵢ = two sintab reads (Q14).
- Softmax → integer approximation: winner index by two compares; blend weight from the margin m = f_win − f_second via a 256-entry falloff table (soft edge, no exp/div). Two-color lerp (mul+shift) instead of full 3-way — visually identical since the third weight is negligible off the triple point.
- Species colors are 3 palette entries recomputed once per frame (scalar), drifting along the palette LUT.
- Relief factor: u8 brightness LUT indexed by f_win.

## Palette pairing
Rainbow cosine palette sampled at thirds, d=(0.10,0.40,0.70) — three maximally-separated saturated hues that share brightness, so no species ever dominates the eye; the whole triad rotates hue together (~67 s/cycle).

## Motion
Domains spiral inward at ~1 wavelength per 10 s; the radial carrier drifts at 60% of that so borders slither rather than march. Triad hue rotation is the slowest layer. Soft edges everywhere — zero flicker at the triple points.
