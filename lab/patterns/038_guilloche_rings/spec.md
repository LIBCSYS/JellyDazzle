# 038 Guilloche Rings

## Look
Five concentric engine-turned bands — gold, copper, teal, ivory, steel-blue — each a wavy 11-and-17-lobed guilloché ribbon, like the security lathework on a banknote. The bands breathe in radius and their lobe phases crawl at different speeds, so moiré interference shimmers slowly between neighbors.

## Math
Interleaved multi-ring rhodonea bands, accumulated with age fade:
- substep n is assigned ring i = n mod 5, base radius R0[i] ∈ {34,52,68,84,100} + 4·sin(0.0008·n + i)
- theta = 0.37·n; r = R0 + 9·sin(11·theta + 0.0014·(i+1)·n) + 5·sin(17·theta − 0.0009·n)
- x = r·cos(theta), y = r·sin(theta); weight w = exp(−age/380); mirror in x
- per-ring fixed hue/sat: {.11/.85, .05/.90, .48/.80, .14/.35, .58/.80}

## Integer ARM64 plan
- Ring index = 3-bit counter cycling 0..4; per-ring constants in a 5-entry struct table (R0, hue base, phase rate).
- theta BAM16 accumulator; 11·theta and 17·theta as their own accumulators (increment = 11× and 17× base) — zero angle multiplies.
- Per point: 4 sine lookups (r-wave ×2, cos, sin) + 3 Q15 multiplies.
- Radius breathing and phase crawl: slow sub-BAM LFOs, updated per frame not per point.
- Color: ring index → 5 palette LUT slices; no runtime HSV.
- Mirror deposit: 1 extra store.

## Palette pairing
Metallic engraving set on near-black bronze vignette: gold (H .11), copper (H .05), teal patina (H .48), low-sat ivory (H .14), steel blue (H .58). Fixed hue per band = instant cohesion; the ivory band acts as the neutral separator that makes the metals read.

## Motion
Each band's lobes crawl at 0.0009–0.0084 rad/frame — inner bands slightly faster, generating a slow moiré roll with a beat period over a minute. Radii breathe on ~130 s cycles. Nothing exceeds ~1 px/frame.
