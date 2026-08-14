# 079 Golden Bloom

## Look
A sunflower head assembles itself floret by floret on the golden angle, rainbow seeds spiraling out from the center until they fill the frame in interlocking phyllotaxis arms. Waves of brightness ripple outward through the seed field while the whole bloom slowly turns and breathes.

## Math
- Floret n: `θ = n·2.39996 + 0.0025t`, `r = 6.4√n·(1+0.03·sin 0.01t)`, y squashed ×0.80.
- Count `N = min(900, 80+1.2(t+60))` — the bloom grows ~1 floret/frame.
- Hue `= 0.0045n + 0.0008t` (spiral rainbow arms); bloom wave `0.5+0.5·sin(1.8√n − 0.035t)` modulates size and value — rings of light travel outward.
- Newest ~60 florets are brighter and whiter (the "blooming edge").
- Dots = 3×3 weighted splat, two soften passes for glow.

## Integer ARM64 plan
- Incremental spiral: keep `(θ_n)` as an accumulator, add golden angle in brads (2.39996 rad ≈ 97.9 brads Q8) per new floret — no per-floret multiply. `√n` via incremental update or a 900-entry u8 LUT.
- Persistent accumulate: draw only the new floret + re-splat a small ring of "wave-front" florets each frame; or full repaint of 900 sprites (900 × 9 px = 8K writes — trivial either way).
- Wave/value from sintab indexed by `sqrtLUT[n]·k − 9t`; hue = `(n·73 + t) >> 4` into a 256 rainbow.
- Breathing scale = one Q8 multiply per floret radius.

## Palette pairing
Full rainbow at S 0.5–0.95, V 0.75–1.0 on a near-black moss ground `(4,10,4)` — the dark field makes every hue read; blooming edge desaturates toward white-gold.

## Motion
Growth edge advances steadily for ~11 min to full 900; brightness waves cross the bloom every ~12 s; rotation one revolution ≈ 42 min; radial breathing ±3% at 10 s. Once full, florets keep cycling color and waves — motion never stops.
