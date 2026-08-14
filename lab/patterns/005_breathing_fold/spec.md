# 005 Breathing Fold

## Look
A tie-dye sun mandala whose number of spokes is never constant: the fold count N glides continuously between 4 and 16, so petals split and merge like a living iris while rings pulse outward from the center. Indigo/cream/amber, hypnotic and soft.

## Math
- Continuous fold: N(t) = 10 + 6·sin(0.0028t); w = pi/N; theta' = |((theta + 0.0022t) mod 2w) − w|; a = theta'/w in [0,1] (normalized wedge coordinate, scale-free as N changes).
- Source: v = 0.5 + 0.30·sin(0.075r − 0.014t) + 0.16·sin(3pi·a + 0.028r − 0.007t) + 0.10·sin(0.02r + 6pi·a + 0.004t). Radial term dominates so the single fractional-N seam at the ±pi wrap stays invisible.
- Color: cosine palette a=(.50,.45,.42) b=(.50,.42,.40) d=(0,.12,.28) + radius-biased phase.

## Integer ARM64 plan
- BAM16 fold with runtime N: per frame compute W = 0x10000/(2N) once (N as Q8.8, one division per frame is fine) plus its Q16 reciprocal; per pixel: atan2 poly → sector via mulhi with the reciprocal → t = a − s·2W → mirror if t > W (geometry.md §1 Regime B). Constant per-pixel cost for any fractional N.
- Normalized coordinate a = t / W → multiply by the same per-frame reciprocal, keep Q15.
- r once per pixel via alpha-max-beta-min; three source sines are three quarter-wave lookups on BAM phases assembled by shifts/multiplies.
- If the seam ever bothers: production fallback = stepped N + crossfade (pattern 001 machinery); spec keeps continuous N as the signature.

## Palette pairing
Indigo-navy troughs, bone-white mids, amber/rust crests — a warm denim tie-dye triad; radius bias makes the core hot and the rim cool so the breathing reads as depth.

## Motion
N completes a full 4→16→4 breath in ~37 s; rings flow outward at 0.014/frame; whole figure rotates ~1 rev / 48 s; palette drifts at 0.0005/frame. Petal splitting is the star — slow enough to watch happen.
