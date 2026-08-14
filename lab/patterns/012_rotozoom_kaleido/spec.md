# 012 Rotozoom Kaleido

## Look
A rotozoomed procedural tile texture seen through an 8-fold mirror: mirrored copies counter-rotate against each other at every seam exactly like a physical kaleidoscope being turned. The whole lattice slowly inhales and exhales with the zoom pulse, in dusk-purple/ember/gold sunset tones.

## Math
- Fold: 8-fold (`w = 2π/8`), wedge coords `(px,py) = (r·cos(fa), r·sin(fa))`.
- Rotozoom: `θ = 0.006t`, `k = 0.085·(1 + 0.42·sin(0.0045t))`; `u = (px·cosθ − py·sinθ)·k + 0.010t`, `v = (px·sinθ + py·cosθ)·k + 0.007t`.
- Texture: `f = sin(2.2u)·sin(2.2v) + 0.6·sin(u+v) + 0.35·sin(0.7u − 0.9v)`.
- Color: `idx = (f+1.95)/3.9 · 255` into a looping 256-entry sunset gradient LUT.

## Integer ARM64 plan
- This is the textbook 16.16 rotozoom: per frame compute `du = cosθ·k`, `dv = sinθ·k` once from `sin_tab`; per scanline of the *wedge buffer* the inner loop is `u += du; v += dv; pix = tex[((v>>8)&0xFF00)|((u>>16)&0xFF)]` — two shifts, AND, OR, load.
- Texture is a 256×256 byte map baked at init from the three-sine formula (wrap-friendly by construction); pixel byte goes straight through the 256×3 palette LUT.
- Kaleidoscope via the cheap route: rotozoom into one wedge buffer, then mirror-blit 8-fold with the precomputed `fold_map` (one indirection per screen pixel), or render a quadrant and use negative-stride copies for the 4 axis mirrors + fold LUT for the diagonals.
- Zoom pulse = one multiply per frame (`k = base + (sin_tab[t·ω]·amp >> 15)`).

## Palette pairing
Looping sunset ramp: dusk purple → magenta → ember red → gold → cream → back to purple. End equals start so the gradient tiles seamlessly across texture contours.

## Motion
Spin at 0.006 rad/frame (~17 min per revolution), zoom pulse period ~23 s, plus a slow diagonal drift across the texture so new tile features keep arriving. All three rates are incommensurate — the mandala never revisits a pose.
