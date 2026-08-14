# 039 Butterfly Lace

## Look
Temple Fay's butterfly curve drawn over and over with slowly sliding internal phases, so nested translucent butterflies of amethyst, orchid and ice-blue open their wings inside one another. X-mirroring keeps the figure bilaterally perfect while the drift blooms it outward into lace.

## Math
Butterfly curve (polar), phases drifting, accumulated with age fade:
- theta = 0.16·n (38 substeps/frame), ph = 0.0006·n
- r = 26·( e^{sin(theta + 3ph)} − 2·cos(4·theta + 5ph) + sin^5((2·theta − π)/24) )
- x = r·sin(theta), y = −r·cos(theta) + 18  (upright butterfly)
- weight w = exp(−age/420); copies: identity + x-mirror
- hue = 0.75 + 0.18·sin(0.5·theta + 2ph), S=0.80

## Integer ARM64 plan
- theta, 4·theta, and the two phase drifts: BAM16 accumulators (4·theta via 4× increment).
- e^{sin}: sin is a Q15 table lookup; exponential via a 256-entry exp LUT indexed by (sin+1)·128 — i.e. precompute exp(sin(u)) as ONE composite 256-entry Q4.12 table. The whole transcendental core collapses to a single lookup.
- sin^5 term: Q15 sine, two squarings + one multiply (x·x²·x²), >>15 each. Tiny contribution — could even be a second composite LUT on theta·(2/24) but straight multiplies are 3 ops.
- Per point: ~4 lookups + 5 multiplies. Mirror = 1 extra store with negated x.
- Hue LFO → 128-entry jewel LUT index.

## Palette pairing
Jewel arc centered on violet: indigo (H .57) ↔ pink-magenta (H .93), high value on a black-plum vignette; the exponential term's density spikes give free white-hot body/vein highlights.

## Motion
Wing phases slide one full internal cycle in ~175 s — the butterfly repeatedly folds open into a peacock-lace superposition and back. Trails ~7 s. Motion is a slow blossoming; nothing beats faster than breathing tempo.
