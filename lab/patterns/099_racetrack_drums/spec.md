# 099 Racetrack Drums

## Look
The entire screen is concentric stadium/racetrack ovals — thick multicolor bands with dark seams — slowly shimmering outward from the center while their hues roll through an acid green/yellow/blue/red cycle; inside the innermost oval, two side-by-side striped "drums" roll their green→yellow→red stripe ramps vertically, and small hot-pink blobs ride the ring perimeter. Replica of R14 (frames d28–d30), including its heavy palette cycling.

## Math
- Stadium distance: `d = √(max(|dx|−64,0)² + dy²)` — exact rounded-racetrack metric.
- Bands: `band = floor((d − 0.18t)/11)`; color = `BANDS[(band + 0.02t) mod 8]`; seam where `(d − 0.18t) mod 11 < 1.8` darkened ×0.35. Two time terms = rings translate outward AND recolor, the double motion visible in d28→d30.
- Drums: rectangles `|dy|<26`, `dx∈[±8,±56]`; stripe color `DRUM[floor((y − 0.55t)/6) mod 8]` — a palindromic green→red→green ramp so rolling is seamless.
- Blobs: eight mirrored ellipses on the mid ring, brightness pulsing with per-blob phase.

## Integer ARM64 plan
Stadium metric integer-exact: `dxs = max(|dx|−64,0)` then octagonal norm `max(dxs,|dy|) + 3/8·min(...)` — a couple compares and shifts, no sqrt. Precompute the stadium distance map ONCE into a 16-bit buffer (static geometry!); per frame each pixel does `(d16 − t·k) / 11` via reciprocal multiply (11 → mul by 5958, shr 16) then two table lookups. Even cheaper, fully faithful: make band index the palette index and do ALL motion by DAC rotation — rings then march and recolor with zero pixel writes; only the drum rectangles (rows recolored per scanline from a rolling 8-entry ramp) and 8 blob fills are painted per frame.

## Palette pairing
Eight-stop acid family (lime, yellow, blue, red, teal, orange, navy, green) for the rings and a palindromic traffic-light ramp (green→yellow→red) for the drums, hot pink blobs as accents — the exact loud-but-coherent gamut of d28–d30.

## Motion
Rings glide outward one band-width every ~60 frames while the color assignment drifts one stop every 50 frames; drum stripes roll at ~0.55 px/frame (one stripe ≈ 11 frames — steady drum-roll feel, not a strobe since adjacent stripes are adjacent hues); blobs pulse on slow offset sines. Constant motion everywhere, all of it syrup-slow.
