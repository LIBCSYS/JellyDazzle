# 001 Kaleido Rose

## Look
A full-screen dihedral kaleidoscope whose fold count steps through 4, 6, 8, 10, 12, 14, 16 sections, each stage crossfading into the next. Inside the wedge lives a rose-curve interference field, so every fold count reads as a different flower — rainbow petals over radial ripples.

## Math
- Fold: w = pi/N; theta' = |((theta + rot) mod 2w) - w| — the D_N triangle-wave mirror fold.
- Source: v = 0.5 + 0.28·sin(0.055·r − 0.012t + 2.5·cos(6·theta')) + 0.22·sin(0.09·(r − 78·|cos(3·theta'·N/4 + 0.006t)|) + 0.004t) — a rhodonea ring r = A|cos(k·theta)| beaten against concentric waves.
- N schedule: SEQ = [4,6,8,10,12,14,16,12,8,6], 240 frames per stage, smoothstep crossfade over the last 30% of each stage (two folds evaluated, colors lerped).
- Color: IQ cosine palette, rainbow coefficients (a=b=0.5, c=1, d=(0,.33,.67)), phase drifts +0.0006/frame.

## Integer ARM64 plan
- BAM16 angles: atan2 via octant reduction + 2nd-order polynomial (geometry.md §1 Regime B), max err 0.22 deg. Fold = one mulhi sector trick `s=(a·2N)>>16; t=a−s·2W; if t>W: t=2W−t` — zero divisions, constant cost for any N.
- r via alpha-max-beta-min `(31470·mx + 13035·mn)>>15`; all sin/cos from one 256-entry Q15 quarter-wave table.
- Rose term: one extra table lookup `|cos_bam(k·theta')|`, k as Q8.8 so petal count animates fractionally.
- Crossfade: blend two 8-bit field values `v = v1 + (((v2−v1)·m)>>8)` with m a Q8 smoothstep from a tiny 64-entry LUT.
- Palette: 256-entry DAC table rebuilt per frame from the cosine formula (256 evals/frame, not per-pixel) — per pixel is just `pal[v]`.

## Palette pairing
Full-spectrum rainbow cosine palette; slow global phase drift means no two stages repeat the same coloring — matches the client's "never the same color scheme" bar.

## Motion
Whole fold rotates at 0.0025 rad/frame (~1 rev / 42 s at 60 fps); radial waves breathe inward at 0.012/frame; fold count changes once every 4 s with a soft 1.2 s crossfade. Nothing strobes — all rates are sub-degree-per-frame.
