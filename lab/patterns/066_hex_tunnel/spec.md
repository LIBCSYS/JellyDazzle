# 066 Hex Tunnel

## Look
A honeycomb throat: crisp neon hexagon hoops recede toward a black vanishing point, drifting outward toward the viewer while the whole shaft rotates almost imperceptibly, corners catching extra glow like light on facet edges. Violet-magenta walls with cyan-white hoops — the R17 hexagon tunnel of the original footage, supercharged.

## Math
- hex norm: `d = max_j |dx cos(th + j*60deg) + dy sin(th + j*60deg)|`, j = 0,1,2 → isolines are hexagons; `th = t*0.0015`
- `depth = min(2400/(d+8), 300)`
- hoops: `ring = ((.5+.5 sin(depth*.5 + t*.45)))^3` — cubing sharpens sine bands into neon
- vertex glow: `vert = .5+.5 cos(6(a - th))`
- hue `idx = (depth*0.9 + t*0.5) mod 256`; luminance `= (.22+.78 ring)(.72+.28 vert) * d/(d+46)`

## Integer ARM64 plan
- 3 projections = 3× (imul cos + imul sin, 8.8 fixed, constants per frame) + abs + max — the classic octagonal/hex-norm trick, no sqrt/atan.
- `depth = recip_tab[d>>1]` byte. Ring: `s = sin16[(depth<<7)+ph]>>8`, sharpen by `(s*s>>8)*s>>8` (two imuls) or a 256-entry cube table.
- Vertex glow: `vert_tab[i]` precomputed per pixel for th=0 and rotated by indexing `cos6_tab[(ang6_tab[i] - rot) & 255]` — one lookup.
- Luminance combine via two 256×256 mul tables (or NEON umull on 16 pixels at a time); final color = `pal[idx]` scaled through a 256×32 brightness LUT.
- Rotation th only changes 6 per-frame constants; per-pixel work is muls/abs/max/lookups.

## Palette pairing
Neon-noir loop: deep violet → magenta → electric cyan → white → violet. Hue index rides depth, so each hoop wears a slightly different color than the one behind it — the tunnel reads as a gradient bore, never flat.

## Motion
Hoops drift toward the viewer (0.45 phase/frame ≈ 14 s per hoop cycle); shaft rotates one face width in ~70 s; hue rolls one full palette in ~17 s. Deliberately the calmest of the ten — sparse, deep, luminous.
