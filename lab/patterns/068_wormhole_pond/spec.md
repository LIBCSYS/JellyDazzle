# 068 Wormhole Pond

## Look
A pond drains into a wormhole: the throat is not round but alive, its rim slowly flexing through kidney and trefoil shapes, while rings pour down it, swells radiate back out, and a wandering rain-drop source lays interference fringes across everything. Jade, spring green and white foam over deep-blue shadow — the original's R19 ripple pond with an actual hole in it.

## Math
- throat wobble: `rr = r * (1 + .22 sin(2a + t*.012) + .12 sin(3a - t*.008))` — 2- and 3-lobe modes counter-rotating so the rim never repeats
- `depth = 2000/(rr+6)`
- field = `sin(depth*.40 - t*.35) + .8 sin(rr*.16 - t*.22) + .6 sin(r2*.20 - t*.30)` where r2 = distance to a Lissajous-drifting drop point
- `idx = field*74 + 128` → pond palette; shade `= rr/(rr+34)` sinks the throat to black

## Integer ARM64 plan
- `ang_tab`, `r_tab` per-pixel bytes at init. Wobble: `rr = r + (r * w)>>8` where `w = sin16[2*ang + p1]>>9 + sin16[3*ang - p2]>>10` — table sines on the angle byte, one imul.
- `depth = recip_tab[rr]`. Drop-source distance via incremental d^2 + isqrt table (same machinery as 067).
- Three sin16 lookups, shift-weighted sum (`+ (x*13)>>4` style), sat-bias table → idx.
- Per-frame scalars: 2 Lissajous sines. Per-pixel: adds, 2 small muls, 5 lookups. No float, no div, no trig.

## Palette pairing
Pond loop with white foam at 58% and two dark basins — crest lines render as thin foam filaments exactly where sines align, and the double dark region keeps large areas restful. Greens dominate; blue appears only in troughs, reading as depth under the surface.

## Motion
Rim flex period ≈ 17 s and 26 s (two modes beating); inflow rings 0.35 phase/frame; drop source wanders a full loop in ~3 minutes. Everything undulates; nothing pulses in place.
