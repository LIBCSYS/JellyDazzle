# 023 Silk Gratings

## Look
Two sheets of fine diagonal thread — like layered silk — slide across each other,
and their beat pattern blooms into wide soft bands of rose, magenta and violet that
bow and scissor as the sheets' angle breathes. The fine weave shimmers inside the
broad bands (the c10-c13 "scanline moire butterfly" family, rotated free).

## Math
Two ring sources parked ~420 px off-screen left/right (slow Lissajous sway):
their ring fields `g_i = sin(f_i d_i ± phase_i)` arrive as gently CURVED gratings,
`f1=0.55`, `f2=0.61`, `d_i = |p - c_i|`. Carrier = `g1 g2` (fine weave);
envelope = `cos((f1 d1 - f2 d2)/2 + .01t)` — the classic beat term, now bowed
into arcs. Hue rides the envelope, value = carrier inside envelope².

## Integer ARM64 plan
- One oversized distance byte table (the moiré window trick) covers both far
  sources — they are just window offsets far outside the visible rect, so the
  same `dist[]` asset used by 021/022/025 serves here too.
- Both gratings = sine-LUT reads on `(dist[o_i+p]*K_i + phase_i) & 0x3FF`;
  carrier product via 8x8→16 multiply high-byte (`SQDMULH` on NEON).
- Envelope argument `(K1 d1 - K2 d2)/2` = one byte subtract (pre-scaled tables)
  feeding the same sine LUT. Total ~3 loads + 3 LUT reads + 1 mul per pixel.
- Final color via 2D LUT `pal[env_band][carrier_band]` (16x16), one `TBL` read.

## Palette pairing
Rose/orchid family: dusty plum shadow, hot rose mid, pale peach-white where
envelope and carrier both peak. Sat rises with envelope² so the beat bands glow
saturated against grey-violet gaps. Slow global hue drift (~0.0003/frame).

## Motion
Grating phases counter-slide at ~0.03 rad/frame (threads visibly crawl); the
angle between sheets swings over ~1500 frames, so the moire bands slowly scissor
open and closed — a long inhale/exhale, no strobe anywhere.
