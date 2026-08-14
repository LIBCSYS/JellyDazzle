# 063 Spiral Zoom

## Look
Two interlocked logarithmic spirals — a 3-arm falling inward and a 2-arm slowly winding the other way — interfere into curling paisley whorls that pour endlessly into the center, forever zooming without ever repeating a frame exactly. Purple, gold and teal braid around each other like marbled ink.

## Math
- log-polar: `lr = ln(r+2)*82`, `a = atan2(dy,dx)`
- arm counts use wrap-safe coefficients `Kn = n*256/2pi` so the angle seam is invisible mod 256
- `f1 = lr*2.4 - t*1.1 + a*K3` (zoom-in, CW) ; `f2 = lr*1.1 + t*0.5 - a*K2` (zoom-out, CCW)
- `idx = (sin(f1) + sin(f2)) * 63 + 128` (period-256 sines) → palette
- shade `= r/(r+26)` sinks the singularity into darkness
- Because color depends on r only through log r, `t` translation == true infinite zoom (self-similar).

## Integer ARM64 plan
- Precompute per-pixel bytes once: `lr_tab[i]` (log-radius scaled to byte) and `ang_tab[i]`.
- Frame loop: `b1 = lr_tab[i] - z1 + arm3_tab[i]`, `b2 = (lr_tab[i]>>1) + z2 - arm2_tab[i]` — pure uint8 adds (arm3_tab = 3*ang precomputed, wraps free).
- `idx = sat( sin16[b1<<8]>>9 + sin16[b2<<8]>>9 + 128 )` using the 16-bit sine table twice; or cheaper, one 256×256 combine LUT `mix[b1][b2]` built at init.
- Shade is a per-pixel byte table; final via `pal[idx]` and a multiply-free brightness LUT.
- No log/atan/div/sqrt at runtime — everything lives in the two init tables.

## Palette pairing
Royal loop: deep purple → magenta → gold → teal → midnight → purple. Mid-tones dominate the interference sum so the whorls stay rich; gold only fires where both spirals peak, giving natural highlights along the braid lines.

## Motion
Continuous zoom-in at 0.55 idx/frame (one self-similar period ≈ 15 s), counter-spiral drifts at 0.30. The two incommensurate rates mean the interference figure never exactly recurs. Smooth, hypnotic, zero flicker.
