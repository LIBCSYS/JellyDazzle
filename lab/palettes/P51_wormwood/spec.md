# P51 Wormwood

**Class** `mono_accent` — One hue family carries the whole frame; a single opposing accent appears in under a fifth of it. Restraint is the point.

**Scheme** wormwood olive 99-111 field; magenta glint at 315

## Mood
A distillery cellar in green half-light — everything olive, and one magenta reflection off glass somewhere near the ceiling.

## Look
Thirteen stops of a single olive family from black to pale wormwood, closed by a near-neutral descent, with two magenta anchors placed HIGH in the value ramp so they read as a glint and not as a second colour. Chroma on the accent is capped at 0.35 — at full chroma the accent takes so much of the chroma weight that the palette stops being mono_accent at all.

## Pattern pairing
Flow, plasma and interference fields. One dominant current does the work; the magenta marks where it peaks.

## Swatch

![swatch](swatch.png)

Top band: the 21 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 21 | · |
| `hue_bins` | 2 | 1 .. 3 |
| `hue_arc` | 151.536 | · |
| `accent_frac` | 0.177 | 0.04 .. 0.22 |
| `C_mean` | 0.194 | · |
| `C_lit` | 0.326 | 0.3 .. 0.75 |
| `C_sd` | 0.137 | · |
| `C_max` | 0.43 | · |
| `L_mean` | 0.443 | · |
| `L_sd` | 0.242 | · |
| `L_range` | 0.819 | 0.6 .. 1.0 |
| `dark_frac` | 0.333 | · |
| `light_frac` | 0.048 | · |
| `neutral_frac` | 0.381 | · |
| `max_step` | 21.63 | · |
| `step_ratio` | 2.36 | 0 .. 2.6 |

Gate: **PASS** — fit=1.00 nn=P71_jade_lantern d=0.211

## Colors (21)

`000000` `020200` `040300` `110e00` `211d01` `322d03` `433e05` `565109` `68640d` `7c7712` `8f8c17` `a2a11c` `b4b651` `c6ca81` `b2b2a5` `b787c3` `9a6aaf` `89887c` `616155` `3c3c31` `1b1a10`
