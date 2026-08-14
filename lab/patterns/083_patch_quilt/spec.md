# 083 Patch Quilt

## Look
A stitched quilt of 48-px patches, each patch owning ONE distinct motif — nested diamonds,
target rings, cross bands, or a woven checker — in its own hue, separated by dark sashing
with glowing corner buttons. Reads like the R5 mirrored-stamp collage frozen into fabric.

## Math
- Patch id: `h = frac(sin(127.1·gx + 311.7·gy)·43758.5453)`; motif = ⌊4h⌋; phase = 2πh.
- Motifs: sin of (|u|+|v|) manhattan rings, √(u²+v²) rings, min(|u|,|v|) cross bands,
  and sin(u)·sin(v) weave — all phase-animated with ±0.010–0.016·t.
- Hue = 0.9·h + 0.22·field + 0.0012·t through cosine palette.
- Sashing where distance-to-cell-edge < 2.5 px; buttons where both |‖u‖−L/2| and
  |‖v‖−L/2| < 5.

## Integer ARM64 plan
- Cell coords by bit-masking (L = 48 → use 32 or 64 in the real build for shift/AND).
- h from a tiny integer hash (gx·73 ^ gy·151, xorshift, take low byte).
- All four motif fields use only |u|,|v| adds, one octagonal-norm r, and 16-bit sine
  table lookups; select motif with a jump table per cell (motif chosen per CELL, so
  branch once per cell row-run, not per pixel).
- Per-cell hue offset + global palette rotation via DAC LUT.

## Palette pairing
Each patch pulls a stable hue from its hash across a full cosine rainbow; motif field
modulates ±0.22 around it so every patch is internally shaded but distinct from its
neighbors. Sashing is near-black indigo so objects separate hard.

## Motion
Each motif breathes along its own axis (rings out, diamonds in, weave shimmer) at
~0.012–0.016 phase/frame with per-patch phase offsets — the quilt ripples patch by patch,
never in lockstep. Global hue drift completes a rainbow in ~14 s. No strobe.
