# 013 Tunnel Bloom

## Look
A classic fly-through tunnel whose mouth orbits off-center behind a 4-fold mirror, so four (then eight, by reflection) tunnel mouths bloom, merge, and drift apart while everything rushes gently inward. Ice palette — teal through blue-violet with soft white checker highlights.

## Math
- Fold: 4-fold; wedge coords `(px,py)`.
- Orbiting center: `ox = 55·sin(0.005t)`, `oy = 40·sin(0.0037t + 1.3)`.
- Tunnel: `rr = |(px,py)−(ox,oy)| + 2`, `ang = atan2`, `v = 850/rr + 0.30t` (fly), `u = ang·8/π + 0.004t` (spin).
- Walls: `tex = sin(πu)·sin(0.32v)`; depth shade `sh = clip(rr/55)^0.6`.
- HSV: `hue = 0.52 + 0.11·sin(0.16v) + 0.05·tex + 0.00015t`, `val = sh·(0.5 + 0.5·tex)`, `sat = 0.72 − 0.25·tex·sh`.

## Integer ARM64 plan
- Classic tunnel LUTs, but built on a **buffer larger than the wedge** (e.g. 512×512 centered): `angle_tab[i]` and `depth_tab[i]` bytes precomputed once (atan2 + reciprocal at init only).
- Orbiting the center = sliding the viewport window over that big LUT (two adds per frame), zero recompute — the 1993 trick verbatim.
- Frame loop: `pix = tex[((depth_tab[j]+fly)&255)·256 + ((angle_tab[j]+rot)&255)]` — two adds, two masks, one load; `j` comes from the wedge `fold_map`.
- Depth darkening baked into the texture rows (each v-row premultiplied by its shade) so no per-pixel multiply; the texture is regenerated only if the palette scheme changes.

## Palette pairing
Ice family: deep teal → cerulean → blue-violet, checker highlights desaturating toward white. Hue drift is very slow (~0.00015/frame) so it stays in the cold family for minutes, warming imperceptibly.

## Motion
Fly speed 0.30 texture-rows/frame (slow glide, not a rush), spin 0.004 rad/frame, center orbit periods ~21 s and ~28 s. The dominant sensation is the four mirrored mouths slowly wandering and kissing at the fold axes.
