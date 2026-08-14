# 075 Coral Lace

## Look
Nested growth rings bloom from a drifting center like a coral polyp or lettuce edge, each new ring more ruffled than the last, sweeping the full hue wheel from magenta through gold, green and blue as it expands. The newest ring swells into place while older rings keep undulating.

## Math
- Ring j: `r(θ) = (13+4.3j)·(0.75+0.25·e) + a_j·[0.9·sin(3θ+φ1_j+d) + 0.8·sin(m_j θ−φ2_j+0.7d) + 0.4·sin(9θ+2φ1_j−0.5d)]`, amplitude `a_j = (0.6+0.55j)·e` — the differential-growth signature: crinkliness grows with radius.
- Emergence `e = clip(J(t)−j)^0.6`, ring count `J = 3 + 0.033(t+90)` capped at 26; drift `d = 0.004t·(1+0.06j)` (outer rings undulate more).
- Center on Lissajous `(12 sin 0.005t, 8 cos 0.004t)`; y squashed ×0.82.
- Hue `= 0.92 + 0.045j + 0.0004t`.

## Integer ARM64 plan
- 720 θ-samples per ring from sintab; each ring is 3 sintab reads + mul-adds per sample, all Q8. ≤ 26 rings → ~19K points/frame, trivial.
- Radius profile per ring cached and only re-phased each frame (add to phase index, re-read sintab) — no re-derivation.
- Ellipse squash = `y·210>>8`. Emergence ease from a 32-entry LUT.
- Hue → palette ring index `(j·11 + t>>4) & 255` into a full-wheel 256 ramp.

## Palette pairing
Full-spectrum wheel at S≈0.78, V≈0.9 — one ring ≈ 16 hue steps, so neighbors always harmonize; deep blue-teal vignette ground `(3,13,14)`.

## Motion
One new ring every ~30 s; rings undulate with 25–60 s periods (outer faster); center drifts on a ~21 min Lissajous; global hue rotates one wheel / ~40 min. At 26 rings the oldest fade inward and the cycle continues.
