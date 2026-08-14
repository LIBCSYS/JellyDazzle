# 080 Tendril Rose

## Look
Two counter-rotating rose-curve tendrils draw themselves onto a deep violet ground with four-fold mirrored symmetry — an outer wide-petaled rosette and a tighter inner rosette in a contrasting hue, meeting like a mandala of vines. Bright drawing heads trace the petals while completed loops shimmer behind them.

## Math
- Layer: `r(φ) = R·|sin(kφ + 0.0011t)|^0.75 + w·sin(7φ − 0.009t)`, φ = 0.0075s; outer R=105,k=2.5; inner R=58,k=3.5, opposite direction.
- Progressive draw: `n_vis = min(S, 350+5.1(t+60))` samples; head glow `(1−(n−s)/240)²`.
- 4 rotations × mirror (φ→−φ) = 8-fold kaleidoscope; y squashed ×0.85; whole figure rotates ±0.002 rad/frame per layer.
- Hue = hue0 + 0.00008s + 0.0004t; value shimmers `0.6+0.3·(0.5+0.5·sin(0.02s+0.01t))`.

## Integer ARM64 plan
- r(φ) from sintab with |·| and a ^0.75 approximated by a 256-entry power LUT; per-sample cost ≈ 3 table reads + 2 muls, Q8.
- 8-fold symmetry: compute one polyline, apply 4 rotation matrices from sintab (Q14) + a sign flip for the mirror — 8 splats per sample.
- Progressive draw = prefix length counter; persistent accumulate buffer means only ~5 new samples × 8 copies drawn per frame, plus palette cycling for the shimmer on old ink.
- Head glow weight from a 256-entry ramp LUT on `n−s`.

## Palette pairing
Outer tendril violet→rose (hue0 0.78), inner gold→green (hue0 0.12) — complementary pair, both S 0.95; ground `(15,2,25)` violet vignette. Both hues drift together +1 wheel/~40 min so the scheme is always fresh but always paired.

## Motion
Drawing heads move ~5 samples/frame (~1.5 px) completing the mandala in ~12 min; layers counter-rotate one turn in ~52 min; petal wobble drifts at 12 s. On completion the figure holds and shimmers, then fades and redraws with new k values.
