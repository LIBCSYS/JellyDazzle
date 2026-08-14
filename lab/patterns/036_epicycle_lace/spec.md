# 036 Epicycle Lace

## Look
Three stacked epicycles trace a scalloped, snowflake-like lace medallion with 5-fold dihedral symmetry, in cyan-blue-violet with green-gold accents as the band LFO swings. The two free epicycle phases precess at different rates, so the lace continuously re-knots itself.

## Math
Three-term Fourier curve (frequencies 1, −4, 9 — pairwise differing by 5, so the base curve itself has 5-fold symmetry that beats against the imposed D5 fold):
- theta = 0.09·n (34 substeps/frame)
- z = 56·e^{i·theta} + 34·e^{i(−4·theta + p1)} + 18·e^{i(9·theta + p2)}
- p1 = 0.0011·n, p2 = 0.0007·n + 2  (independent precession)
- weight w = exp(−age/400); D5: 5 rotations × mirror = 10 copies
- hue = 0.45 + 0.28·sin(0.07·theta + 0.0005·n)

## Integer ARM64 plan
- Three BAM16 phase accumulators; per point the −4x and 9x angles are (−4·theta + p1) computed by accumulating their own increments (−4 and 9 times the base increment) — no multiplies for angles at all.
- Per point: 6 sine-table lookups + 6 Q15 multiply-accumulates (x and y each 3 terms).
- D5 deposit: 5 precomputed Q15 sector rotations × mirror = 10 rotate-and-store, 4 multiplies each.
- Hue LFO: one more slow BAM sine → index into a 128-entry cool-band LUT.
- Frequencies chosen ≡ 1 mod 5: guarantees the raw curve already closes with 5-fold symmetry — the fold only deepens it, so seams never show.

## Palette pairing
Cool arc: cyan (H .45) ↔ violet (H .73) swinging LFO, occasional dip toward sea-green; midnight blue vignette. Accent comes free where the three epicycle terms align and density spikes to white.

## Motion
p1/p2 precession re-knots the lace with a combined period of ~150 s; the hue LFO swings over ~120 s. Scallops crawl a few pixels per second. Trails ~6.5 s.
