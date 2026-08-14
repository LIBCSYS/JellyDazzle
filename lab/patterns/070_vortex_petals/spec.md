# 070 Vortex Petals

## Look
Six broad petal arms — moss green through chartreuse to gold and vermilion — curl into a black gravitational core, and the spiral's tightness slowly breathes: the flower winds up into a tense whirlpool, then relaxes into an open pinwheel, while ring-pulses of brighter matter stream continuously down the arms into the dark. This is the original's R16 smooth pinwheel with depth-pull added.

## Math
- `arms = sin(6(a + lr*tw/2 - t*.008))` with `lr = ln(r+3)`, tightness `tw = 2.6 + 1.1 sin(t*.005)` — log-spiral arms stay self-similar at every radius
- `pull = sin(depth*.5 + t*.42)`, `depth = 2600/(r+16)` — inward-streaming pulses
- ribs `= .3 sin(8 lr - t*.06)` — subtle concentric shading
- `field = .62 arms + .26 pull + rib` → `idx = field*100 + 128`; shade `= r/(r+30)` digs the core

## Integer ARM64 plan
- Init tables: `ang6_tab[i]` (6*angle byte), `lr_tab[i]`, `depth_tab[i]`, `shade_tab[i]`.
- Arms: `sin16[(ang6_tab[i] + ((lr_tab[i]*tw16)>>8) - rot) << 8]` — tw16 is a per-frame 8.8 scalar from the sine table; one imul per pixel.
- Pull and ribs: two more sin16 lookups on byte phases (`depth_tab*128 + p`, `lr_tab*8 - q`).
- Weighted sum by shift-adds, bias, 512-entry sat table → idx; final via `pal[idx]` x `shade` brightness LUT (256x32).
- Per-pixel budget: 1 imul, ~6 adds/shifts, 4 lookups. No trig/log/div at runtime.

## Palette pairing
Meadow-fire loop (moss → green → chartreuse → gold → vermilion → maroon). Arm crests land on gold/vermilion, troughs on moss — warm petals on cool leaves, the exact green/gold/red family of the reference pinwheel tile (f01–f11), with the maroon basin keeping contrast plush rather than neon.

## Motion
Rotation ≈ one turn every 4.5 minutes; tightness breath period ≈ 21 s (the signature move); inward pulses at 0.42 phase/frame ≈ 15 s per pulse lap. Slowest rotation of the set — the breathing does the talking.
