# 020 Feedback Fractal

## Look
Fold-aware video feedback — the king of dazzle effects: a glowing orbit-blob and a breathing ring are stamped into a 7-fold wedge, and every generation of the feedback re-mirrors, twists ~0.42 rad, and shrinks toward the center, building an infinite self-similar mandala of rings-within-rings. Hue shifts with recursion depth, so the spiral of copies fans through the spectrum.

## Math
- Prototype models the steady-state of `frame' = decay·rotozoom(fold(frame)) + stamp(t)` as an explicit sum over generations g = −3…11: coords `(x,y) = R(0.42g + 0.10·sin(0.003t)·g) · fold(p) · 1.26^g`, weight `0.84^|g|` (negative g = copies larger than the screen, filling the corners).
- Stamps per generation (evaluated at lagged time `t − 22g`): orbit blob `exp(−d²/2·13²)` at a folded Lissajous point, ring `exp(−(|p|−82−26·sin(0.005t))²/2·7²)`.
- Color: generation hue `h = 0.55 + 0.075g + 0.0007t`; ring tinted `h + 0.45`.

## Integer ARM64 plan
- The real build is the honest iterative loop, cheap by construction: keep two wedge buffers; per frame (1) transform prev→next with the standard 16.16 rotozoom inner loop *through the fold_map* (scale 1/1.26, rotate −0.42 — du/dv computed once from `sin_tab`), (2) fade each fetched byte through a 256-entry `decay_tab` (same load anyway — free), (3) stamp the blob sprite (precomputed Gaussian mask, saturating adds) and the ring (radius LUT band write).
- Hue-with-age: run the buffer as (intensity, age) byte pairs, or simpler — make `decay_tab` a 256→256 *palette-index* remap that walks trails along the gradient as they age (the classic remap trick).
- Resolve: `pix = pal[wedge[fold_map[i]]]` — one indirection + palette read. Per-pixel cost ≈ rotozoom + 1; everything else is per-frame scalar.
- The per-generation twist wobble (0.10·sin) is just this frame's rotozoom angle drifting — no extra cost.

## Palette pairing
Spectral spiral: each recursion ring sits ~27° further around the hue wheel (cyan core → blue → violet → magenta → amber outer), rings accented with the complementary tint. Depth = color, so the eye reads the recursion instantly; black ground keeps it jewel-on-velvet.

## Motion
Everything is slow compounding: stamp orbit ~12 s period, ring breath ~21 s, twist wobble ~35 s, and each deeper copy lags 22 frames — so motion cascades inward like a spiral clock. The steady-state changes completely over a minute but never jumps between frames.
