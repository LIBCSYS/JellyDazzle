# 094 Thread Web

## Look
A flat violet flood on which fine, gently-curved rainbow threads accumulate one by one, always sweeping corner-to-center through a 4-fold mirror so the whole screen slowly knots into a symmetric X/butterfly web. Rainbow wedge fans sit parked at the left/right edges and green pinwheel triangles hug the corners — replica of R4 (frames a08–a10), including the start-sparse accumulation behavior.

## Math
- Ground: constant purple flood (the routine floods before drawing — visible in a08).
- Thread i (deterministic from seed i): quadratic arc `P(s) = P0 + (P1−P0)s + swing·sin(πs)·n̂`, s∈[0,1] in 150 steps, P0 in the corner zone, P1 near center; `swing = 30·sin(0.05·i) + jitter` makes the bundle orientation precess as the web grows.
- Each point is stamped at `(x,y), (W−x,y), (x,H−y), (W−x,H−y)`.
- Thread count = `8 + 1.2·t`; hue per thread = `hash(i)`; the newest ~30 threads render brighter (fresh ink), older ones settle darker.
- Furniture: edge fans = angular stripe field `floor((dy+0.55|dx|)/7) mod 6` → 6-color rainbow, corner triangles = two half-plane tests.

## Integer ARM64 plan
Accumulation means NO per-pixel per-frame work at all: keep the framebuffer, draw only ~1–2 new polylines per frame. Curve stepping = forward differences: x,y as 16.16 fixed-point accumulators, the `sin(πs)` bow from the 16-bit sine table stepped by a constant phase per point. Plot 4 mirrored bytes per step (`W−1−x` is a NEG+ADD). Thread PRNG = 32-bit xorshift seeded by thread index. Palette: threads use indices 32–159 of a hue wheel; "fresh ink glow" = draw with bright index, engine's global palette fade ages them. Flood + furniture drawn once at routine start.

## Palette pairing
Saturated violet ground (BG never black — key to a08's look) with every thread a different full-sat hue; fans use the six primary/secondary stripes; corners muted green. Matches the purple-field/rainbow-thread reading of a08–a10 exactly.

## Motion
Pure accumulation: ~1.2 threads/frame appear, each frozen once drawn; the bundle direction drifts around the corner slowly (sin(0.05·i)) so the web's grain visibly rotates over ~500 frames. No global movement, no flicker — the image just grows richer, like watching lace being woven.
