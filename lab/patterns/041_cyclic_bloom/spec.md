# 041 Cyclic Bloom

## Look
A rainbow cyclic-cellular-automaton quilt: interlocking rectilinear wavefronts and spiral cores in full-spectrum ramps, mirrored 4-fold so it reads as a woven mandala. State bands flow through each other like slow neon lava.

## Math
- Grid 160×120, K=12 states, Moore neighborhood, wrap boundaries.
- Rule: cell s adopts (s+1) mod K if any of its 8 neighbors already holds (s+1) mod K.
- Seed: random states in one quadrant, mirrored into 4-fold symmetry (rule preserves the symmetry on the torus).
- Precompute 420 generations, drop the first 120 (noisy transient); playback scrubs the history at 0.15 gen/frame with smoothstep crossfade between adjacent generations.
- Color: v = state/K + 0.0006·t into a cosine palette; 2×2 upscale + 3×3 box blur.

## Integer ARM64 plan
- u8 grid, double-buffered; rule is compare-and-store only — zero multiplies (research §4).
- Per cell: 8 loads, 8 compares against (s+1) mod K (mod via `csel`/table since K const), conditional store. NEON: `cmeq` across 16 cells at once.
- Run the CA live at ~1 update per 6–8 frames; crossfade = blend of two palette LUT reads weighted by an 8-bit frame counter (mul+shift), or skip crossfade and let palette cycling smooth it.
- Color = `palette[(state*ramp + t*drift) & 32767]` LUT; drift is the free palette-cycling trick. 2×2 block draw for chunk.

## Palette pairing
Full rainbow cosine palette (d = 0, 1/3, 2/3) — the K states ARE the hue wheel, so the cyclic rule and the cyclic palette close on each other seamlessly. Slow global hue drift on top.

## Motion
CA history scrubs forward at ~0.15 generations/frame (a wavefront crosses a band in ~7 s); after convergence the field cycles with period K so motion never stops. Global hue rotation ~28 s per full cycle. No strobing — adjacent states are adjacent hues.
