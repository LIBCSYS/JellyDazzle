# 017 Metaball Kiss

## Look
Four metaballs folded six ways so every ball kisses and fuses with its own reflections across the wedge seams, forming soft lobed mandalas that continuously merge and split. A glowing white-magenta rim traces the iso-contour where blobs meet; the body hue migrates slowly around the cool side of the wheel (green → cyan → violet).

## Math
- Field in folded wedge coords: `g = Σ S_i / (d_i² + 160)`, four balls on independent Lissajous paths (`ω ∈ [0.0037, 0.0079]`), each ball's position folded into the wedge before evaluation (so merges happen *before* thresholding — the seam-fusion effect).
- Soft threshold: `inside = ½ + ½·tanh(2.2(g−1))`; rim: `exp(−(g−1)²/0.030)`.
- Body: `clip(1.1(g−0.55))^1.2 · (0.72 + 0.28·sin(5·ln(g+0.2) − 0.018t))` (broad slow inner contours).
- HSV: `hue = 0.50 − 0.17·inside + 0.0006t`; rim pushes sat→0 and adds magenta (R,B boost).

## Integer ARM64 plan
- No per-pixel division ever: precompute a radial **falloff byte table** `falloff[d²>>4]` sized to the max ball radius; per pixel `sum = falloff[q1] + falloff[q2] + falloff[q3] + falloff[q4]` with saturating adds.
- `d²` by incremental deltas: along a wedge scanline, adjacent d² differs by `2(x−bx)+1` — one add per pixel per ball, no multiply (the classic 486 trick).
- Bounding boxes per ball (folded) limit work to touched pixels; the rest of the wedge decays to floor via memset of the frame's base value.
- `inside`, rim, and band shaping all collapse into **one 256-entry transfer LUT** from the summed byte → (hue-offset, sat, val) triple, rebuilt per frame in scalar code; per-pixel = 4 adds + 2 table reads. 6-fold resolve through `fold_map` as always.

## Palette pairing
Single-hue body per moment (deep sea-green shifting through cyan to violet over minutes) with a two-tone accent: white core rim flushed magenta. Monochrome-plus-accent keeps it cohesive; the slow hue voyage supplies "never the same color scheme."

## Motion
Ball orbit periods 13–28 s, all incommensurate; blobs approach a seam, bulge, fuse with their mirror twins into rings around the center, then peel apart — the signature event, occurring every 20–40 s. Inner contour bands drift at 0.018/frame. Nothing pops; fusion is a continuous soft morph.
