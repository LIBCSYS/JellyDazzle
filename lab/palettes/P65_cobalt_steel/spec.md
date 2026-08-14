# P65 Cobalt Steel

**Class** `metallic` — A narrow warm or cold hue band riding a long, steep value ramp -- the specular curve of gold, copper, steel, bronze. Chroma stays modest; the shine comes from lightness, not colour. Must include a return leg (see RULE 3) or the cyclic wrap cliffs.

**Scheme** cobalt blue 252-300 on a full value ramp; warm return leg

## Mood
Blued steel straight off the heat — cold cobalt through the body and a white edge where the light catches it.

## Look
The hot metal of the three: chroma scaled to 1.2 where Antique Bronze runs at 0.5, so C_lit is 0.45 against 0.21. Same class, same steep value ramp, twice the colour.

## Pattern pairing
Rim-light, shear and bevel patterns; also good under any pattern that wants to look manufactured rather than grown.

## Swatch

![swatch](swatch.png)

Top band: the 20 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 20 | · |
| `hue_bins` | 2 | · |
| `hue_arc` | 31.095 | 0 .. 60 |
| `accent_frac` | -0.0 | · |
| `C_mean` | 0.245 | · |
| `C_lit` | 0.445 | 0.2 .. 0.6 |
| `C_sd` | 0.221 | · |
| `C_max` | 0.6 | · |
| `L_mean` | 0.501 | · |
| `L_sd` | 0.289 | 0.22 .. 0.4 |
| `L_range` | 0.941 | 0.75 .. 1.0 |
| `dark_frac` | 0.3 | · |
| `light_frac` | 0.2 | 0.08 .. 0.4 |
| `neutral_frac` | 0.4 | · |
| `max_step` | 13.79 | · |
| `step_ratio` | 1.38 | 0 .. 2.8 |

Gate: **PASS** — fit=1.00 nn=P38_nightbloom d=0.177

## Colors (20)

`000002` `050201` `231a16` `463b38` `6c605c` `958884` `bfb2ad` `ecded9` `f9f7fe` `e0d8fc` `c5bafa` `a79df9` `887ff7` `6965e6` `4b4dd0` `2f35b6` `151e98` `011568` `010f2f` `00030e`
