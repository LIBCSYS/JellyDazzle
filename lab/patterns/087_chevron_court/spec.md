# 087 Chevron Court

## Look
A four-fold mirrored court of emerald-and-gold chevrons marching diagonally from every
corner toward a cream-framed violet striped core at center (the d19–d21 neon-chevron
routine slowed to a meditative pace), with teal accent V's and faint corner ray fans.

## Math
- Mirror coords u = |x−160|, v = |y−120|; chevron field on the L1 sum s = u+v.
- Interference of two stripe systems: `(½+½·sin(0.22s + 0.012t))·(½+½·sin(0.085s − 0.006t))`
  — a slow beat pattern of nested V's.
- Teal set: clamp(sin(0.15s − 0.009t) − 0.62)/0.38 (sparse ridges only).
- Rays: clamp(sin(12·atan2(v,u) + 0.008t) − 0.9)/0.1, faded in beyond s > 130.
- Core: rect u<70, v<26 with scanline ramp sin(0.9y − 0.018t); gold frame ring rect.

## Integer ARM64 plan
- Everything keys off s = |x−cx| + |y−cy|: per scanline s changes by ±1 per pixel —
  walk it incrementally, no multiplies in the inner loop.
- Both chevron systems + teal set = three reads of the shared 16-bit sine table at
  scaled s (shift-and-add scaling: 0.22 ≈ 225/1024 etc.), multiplied in 16-bit.
- Rays only outside s > 130: octant-fold angle byte → sine table; masked add.
- Core is a rectangle fill with a per-row LUT (scanline ramp precomputed per frame).
- 4-fold mirror: compute one quadrant, store, blit 3 mirrored copies.

## Palette pairing
Emerald/gold body (cosine phase .85/.20/.55 lands the working hue band on green-gold)
with teal accents and amber corner rays — the striped core sits in violet-blue behind a
cream frame, a hard complementary pop so the center reads as a distinct object.

## Motion
Fine chevrons drift inward ~0.05 px/frame, coarse set outward half that speed — the two
systems slide over each other in a slow moiré breath; core scanlines roll down gently;
rays precess. All periods > 5 s; the original's strobe is deliberately tamed.
