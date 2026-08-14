# 015 Twister Star

## Look
The demoscene twister run over radius instead of scanlines: a five-armed braid of jewel-toned ribbons (ruby, amber, emerald, sapphire) that twists sinusoidally along its length while slowly rotating. Face lighting gives each ribbon a rounded, lit-from-above 3D relief.

## Math
- Twist phase along radius: `φ(r,t) = 1.6·sin(0.028r − 0.009t) + 0.005t`.
- Braid coordinate: `v = 5a + φ` (a = pixel angle, 5 arms).
- Faces: `face = v mod 2π`, face index `fi = ⌊face/(π/2)⌋ mod 4`, in-face position `fp ∈ [0,1)`.
- Lighting: `light = sin(π·fp)^0.7` (rounded face); radial falloff `1 − r/195`; center softened by `clip(r/12)`.
- HSV: `hue = hues[fi] + 0.0006t + 0.0006r`, `sat = 0.85`, `val = light·(0.22 + 0.78·falloff)`.

## Integer ARM64 plan
- Exactly the twister's per-row trick, transposed to polar: for each radius r (a ~200-iteration loop, not per-pixel), compute `φ` from two `sin_tab` reads, then the four face-boundary *angles* `a_j = (jπ/2 − φ)/5`.
- Rasterize each visible face as an **angular arc fill** at that radius: with the polar `fold_map`/(angle,radius) index buffer laid out radius-major, an arc fill is a contiguous memset-style run of the face's shade byte — `rep stosb` energy, NEON `st1` in practice.
- Face shading: per (face, fp) from a 4×64 baked shade/hue table — no per-pixel trig or multiply; fp advances by a fixed 8.8 increment across the arc.
- Screen resolve: `pix = arcbuf[polar_index[i]]`, one indirection per pixel through the precomputed polar map.

## Palette pairing
Four fixed jewel hues (ruby 0.985, amber 0.09, emerald 0.35, sapphire 0.62) at constant high saturation; brightness carries the 3D. Hue creeps with radius and time so the arm coloring slowly migrates around the wheel as a set — always four-tone, always coherent.

## Motion
The braid twists (radial sine phase drifts at 0.009/frame) and the whole star rotates at 0.005 rad/frame (~21 min/rev). Arms appear to peristaltically ripple outward; nothing on screen exceeds ~1.5 px/frame.
