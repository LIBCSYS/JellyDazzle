# P68 Sea Fog

**Class** `earth` — Ochre, clay, moss, bark, stone. Warm-biased mid hues at low chroma and mid lightness -- the only class that is deliberately muted rather than dark or pale.

**Scheme** sea teal 180-235 at very low chroma, mid value only

## Mood
Fog that has not lifted off cold water: teal, grey-green, and the wet light between them.

## Look
Run as a WASH — lightness oscillates around the mid instead of climbing and returning, which reads as weathered surface rather than as layered strata. Nothing in it is dark and nothing is pale (dark_frac 0, light_frac 0): the class is deliberately muted rather than either.

## Pattern pairing
Terrain, sediment and haze patterns. Stays legible under heavy layering where a saturated palette goes muddy.

## Swatch

![swatch](swatch.png)

Top band: the 19 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 19 | · |
| `hue_bins` | 3 | · |
| `hue_arc` | 52.948 | 40 .. 160 |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.164 | · |
| `C_lit` | 0.164 | 0.12 .. 0.4 |
| `C_sd` | 0.035 | 0.0 .. 0.16 |
| `C_max` | 0.221 | · |
| `L_mean` | 0.55 | 0.38 .. 0.65 |
| `L_sd` | 0.106 | 0.12 .. 0.28 |
| `L_range` | 0.3 | · |
| `dark_frac` | 0.0 | · |
| `light_frac` | 0.0 | · |
| `neutral_frac` | 0.0 | · |
| `max_step` | 4.92 | · |
| `step_ratio` | 1.48 | 0 .. 2.4 |

Gate: **PASS** — fit=0.97 nn=P57_harbour_dusk d=0.247

## Colors (19)

`174f5e` `18525b` `1b5a5e` `206664` `2b746c` `408277` `549084` `659c91` `72a49d` `7ba8a5` `7fa6a9` `7ea0a6` `7796a0` `6c8995` `5f7a89` `4f6c7d` `3f5f72` `305669` `205163`
