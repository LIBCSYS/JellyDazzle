# P85 Rose Gold

**Class** `metallic` — A narrow warm or cold hue band riding a long, steep value ramp -- the specular curve of gold, copper, steel, bronze. Chroma stays modest; the shine comes from lightness, not colour. Must include a return leg (see RULE 3) or the cyclic wrap cliffs.

**Scheme** rose gold 314-360 on a full value ramp; neutral return leg

## Mood
Polished rose gold under a single lamp: black in the recess, white on the rim, and every value between is the metal.

## Look
A narrow hue band riding a long steep value ramp, closed by a near-neutral descent so the cyclic wrap has no cliff. Sits a full 290 degrees off P45 Hammered Copper — the two warm metals are nowhere near each other.

## Pattern pairing
Specular, sheen and bevel patterns. The shine here is lightness rather than chroma, so it survives being composited under other layers.

## Swatch

![swatch](swatch.png)

Top band: the 20 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 20 | · |
| `hue_bins` | 2 | · |
| `hue_arc` | 16.1 | 0 .. 60 |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.206 | · |
| `C_lit` | 0.317 | 0.2 .. 0.6 |
| `C_sd` | 0.149 | · |
| `C_max` | 0.424 | · |
| `L_mean` | 0.486 | · |
| `L_sd` | 0.273 | 0.22 .. 0.4 |
| `L_range` | 0.893 | 0.75 .. 1.0 |
| `dark_frac` | 0.3 | · |
| `light_frac` | 0.15 | 0.08 .. 0.4 |
| `neutral_frac` | 0.4 | · |
| `max_step` | 13.03 | · |
| `step_ratio` | 1.42 | 0 .. 2.8 |

Gate: **PASS** — fit=1.00 nn=P38_nightbloom d=0.206

## Colors (20)

`010001` `070203` `23191c` `44393c` `675b5e` `8d8084` `b5a7ab` `ded0d4` `fde2f5` `edc3e4` `dca6d3` `c989c2` `b46eb0` `9d559e` `853e8a` `6c2875` `54135f` `3a0545` `200229` `09000f`
