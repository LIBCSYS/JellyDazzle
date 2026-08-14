# 098 Gear Flower Quad

## Look
Four big toothed gear/sunflower rosettes in a 2×2 array — yellow-green discs with dark rims and dotted navy seed cores — sit on a ground of drifting diagonal blue/cyan stripes, with stepped-triangle borders along the top and bottom edges and a little diamond chain running down the center vertical. The teeth rotate slowly while the whole scheme gradually swaps figure and ground (yellow-on-blue ↔ blue-on-yellow), replicating R13 (frames d22–d24, with the melt of d25 in spirit via the swap's green midpoint).

## Math
- Rosette at each of 4 centers: tooth radius `R(θ) = 34 + 5·cos(14(θ + 0.004t))`; disc `r < R`, rim `|r−R| < 2.2`; disc shaded inner→outer by `r/36`.
- Seed core `r < 15`: dot rings where `sin(1.35r − 0.02t) > 0.35 AND sin(9θ + 0.6r) > 0.1`.
- Ground: `sin(0.32(x+y) + 0.012t)` two-color diagonal stripes; borders: 16-px bands with `(x + 5·floor(y/4)) mod 26 < 13` stair pattern.
- Figure/ground swap: every color is a lerp between two colorways with weight `0.5+0.5·sin(0.004t)` (~26 s full cycle) — the halfway state is the yellow-green wash seen in d25.

## Integer ARM64 plan
Mode-13h way: draw geometry into the index buffer with class indices (ground-A, ground-B, disc ramp 0–15, rim, core, dots, border), animate ONLY (a) the DAC for the figure/ground swap (lerp two stored palettes with a 0.8 fixed-point weight from the sine table — 256 entries/frame) and (b) redraw just the 4 rosette bounding boxes for tooth rotation. Tooth test: `r` via octagonal norm, `θ` via octant atan table; `cos(14θ+φ)` = one sine-table read; compare against r. Dots: two sine-table reads, AND. Stripes/borders precomputed. Per-frame pixel work limited to four ~72×72 boxes.

## Palette pairing
The strict d22 family: yellow, green-yellow, navy, cyan, blue — two complete colorways (yellow-figures/blue-ground and its inverse) crossfaded, so every intermediate frame stays inside the blue/yellow/green gamut.

## Motion
Teeth turn at 0.004 rad/frame (one tooth-pitch ≈ 112 frames); stripes drift diagonally; seed-dot rings ripple outward very slowly; diamond chain creeps down; palette swap breathes on a ~26 s cycle. Everything is gears-in-oil slow.
