# 062 Twist Corridor

## Look
Nested glowing squares recede to a dark vanishing point — a square air-shaft you are falling down — while the whole shaft slowly torques, the deep end twisting faster than the mouth so the corridor appears to wring itself like a towel. Ice-blue/white/violet bands stream past, each of the four walls tinted a step apart.

## Math
- twist: `phi(r,t) = t*0.0009*(1 + 70/(r+28))` — rotation rate increases toward center
- rotate: `rx = |dx cos phi - dy sin phi|`, `ry = |dx sin phi + dy cos phi|`
- square norm `d = max(rx, ry)` → nested-square isolines; depth `= 2400/(d+10)`
- `v = depth*1.7 + t*0.8`; wall sector = `floor((atan2(dy,dx)+phi)*2/pi + .5) mod 4`
- `idx = (v + sector*32) mod 256`; glow `= .72+.28 sin(depth*.5 + t*.06)`; shade `= d/(d+38)`

## Integer ARM64 plan
- The twist varies with r only → quantize r to 64 rings; per frame compute 64 (cos,sin) pairs from the sin16 table (8.8 fixed) — per-pixel rotate is 4 imuls + shifts using the ring's pair, indexed by a precomputed `ring_tab[i]` byte.
- `max(|rx|,|ry|)` is integer abs+cmp — the octagonal-norm family; no sqrt ever.
- `depth = recip_tab[d>>1]` (256-entry reciprocal table, byte out). `v = depth + fly` wraps free in uint8.
- Wall sector from the sign pattern of (rx', ry') before abs — 2 bit tests, no atan2.
- glow via sin16[(depth<<5)+phase]>>shift folded into a 256-entry brightness ramp; final = 2D LUT `pal[idx][bright>>3]`.

## Palette pairing
Cool loop: navy → cobalt → cyan → white → violet → navy. The 40-step sector offset puts adjacent walls two palette stops apart, so the corridor always shows a cold/hot-white contrast without ever clashing.

## Motion
Fly speed 0.7 idx/frame (band period ≈ 12 s); twist ≈ one full wring per 2–3 minutes at the mouth, faster at the throat; glow pulse period ≈ 4 s. All slow phase accumulation, zero strobe.
