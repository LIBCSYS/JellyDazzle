# 044 Majority Quilt

## Look
Bold two-tone amoeba patches — dusty rose, amber, aubergine, chartreuse — melting into and out of each other like a living pueblo quilt, mirrored 4-fold. Patch borders are inked dark so it reads as stained ceramic.

## Math
- Grid 160×120, cells ±1, seed symmetric random (4-fold mirror), wrap.
- Rule: cell ← sign of the 3×3 neighborhood sum (9 cells, ties impossible) — majority vote. Iterated, domains coarsen: blobs round off, necks pinch, islands vanish.
- Precompute 150 generations; store each as a twice-box-blurred float field (smooth in [−1,1]).
- Playback: ping-pong scrub gens 22↔149 at 0.09 gen/frame with smoothstep crossfade — coarsening runs forward then in reverse, so blobs merge, then split, forever.
- Color = cospal(0.5 + 0.32·field + 0.0005·t), darkened where |field|→0 (the borders).

## Integer ARM64 plan
- u8 0/1 grid double-buffered; vote = 3×3 sliding sum (adds only) compared to 4 — research §4 engine with a different inner loop.
- Run live instead of scrubbing: one vote pass every ~10 frames, and keep TWO smoothed i8 fields (prev, next) blending over those 10 frames — same crossfade as the proto, bounded memory.
- Smoothing = the same two-pass box-blur sliding-sum kernel as 043. Reverse phase of the ping-pong = replay from a stored ring buffer of ~32 keyframe fields (150 gens × 19200 B fits easily).
- Border darkening: brightness LUT indexed by |field| (abs + table read).

## Palette pairing
Warm cosine palette c=(1.0,0.7,0.4), d=(0.0,0.15,0.20): rose/amber vs deep plum — two clear families (one per CA sign) with a slow shared hue drift, so the quilt recolors itself over minutes.

## Motion
Blob morphing at ~0.09 generations/frame (a patch merges over ~8 s); ping-pong means no reset pop ever. Hue drifts one palette cycle in ~67 s. Everything is soft-edged and slow.
