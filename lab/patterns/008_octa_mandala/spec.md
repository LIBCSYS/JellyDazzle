# 008 Octa Mandala

## Look
Concentric polygon rings — square, octagon, diamond — pulse outward from the center while the ring shape itself morphs between the three norms and an 8-petal angular modulation dapples the bands into beads. Scarlet beads on seafoam and porcelain, like a rotating ceramic plate.

## Math
- Rotated frame (0.0016t), then three polygon norms from ax=|x|, ay=|y|:
  - Chebyshev cheb = max(ax,ay) → square rings; diamond diam = (ax+ay)/√2 → 45° square; octagon octn = max(cheb, 0.5858·(ax+ay)·...) ≈ the true octagonal norm.
  - q = 0.5·octn + 0.5·(cheb·(1−k) + diam·k), k = 0.5 + 0.5·sin(0.0024t) — rings morph square→octagon→diamond→back.
- Field: v = 0.5 + 0.42·sin(0.17q − 0.011t)·(0.6 + 0.4·sin(8·theta + 8·rot)) — outward ring flow amplitude-modulated by 8 petals that counter-rotate with the frame.
- Color: cosine palette c=(2,1,1), d=(.50,.20,.25) — double-frequency red channel gives the bead/rim two-tone.

## Integer ARM64 plan
- This is the flagship "octagonal norm" pattern: cheb = max, diam = ((ax+ay)·23170)>>15, octn = max(cheb, ((ax+ay)·17734)>>15) — zero sqrt, zero trig for the rings (geometry.md §0 magnitude notes).
- Norm morph: one Q8 blend per pixel between cheb and diam, then average with octn — 2 multiplies.
- The petal term needs theta: either full BAM atan2, or exploit D8 symmetry — fold by abs/swap first, then a 64-entry atan LUT on the octant ratio (min·64/max via one small division table) is plenty at these petal frequencies.
- Ring phase 0.17q and time phase are BAM adds; two quarter-wave lookups; multiply for the AM envelope; palette via DAC.
- Frame rotation per-scanline incremental (add dx,dy per pixel) keeps the rotated ax,ay at 2 adds/pixel.

## Palette pairing
Scarlet/coral beads, deep-teal shadows, porcelain-white highs — a Wedgwood-china scheme; the doubled red frequency makes beads pop without saturating the field.

## Motion
Rings glide outward at 0.011/frame (~one ring every 3.3 s); shape morph period ~44 s; petal wheel counter-rotates at 8×0.0016 rad/frame; palette drifts slowly. Reads as steady breathing, zero strobe.
