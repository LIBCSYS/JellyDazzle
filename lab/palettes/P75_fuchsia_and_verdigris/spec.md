# P75 Fuchsia & Verdigris

**Class** `duotone` — Exactly two hue poles, 110-180 deg apart, each with a full ramp. No third family. Reads as a two-ink print.

**Scheme** fuchsia 312-328 vs verdigris 194-210

## Mood
Hot pink neon reflected in oxidised copper. Two inks, both loud, and no referee between them.

## Look
Fuchsia is the highest-chroma hue sRGB has (C_norm 0.95 against red's 0.79), verdigris one of the lowest — so this duotone is lopsided on purpose, and the teal side reads as the ground the pink is sitting on.

## Pattern pairing
Lattice, moire and shear patterns — two interfering families stay readable here where a full-spectrum palette turns to mud.

## Swatch

![swatch](swatch.png)

Top band: the 18 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 18 | · |
| `hue_bins` | 2 | 2 .. 4 |
| `hue_arc` | 124.449 | 110 .. 250 |
| `accent_frac` | 0.362 | · |
| `C_mean` | 0.3 | · |
| `C_lit` | 0.369 | 0.35 .. 0.9 |
| `C_sd` | 0.172 | · |
| `C_max` | 0.601 | · |
| `L_mean` | 0.496 | · |
| `L_sd` | 0.286 | · |
| `L_range` | 0.886 | 0.6 .. 1.0 |
| `dark_frac` | 0.333 | · |
| `light_frac` | 0.222 | · |
| `neutral_frac` | 0.056 | · |
| `max_step` | 13.55 | · |
| `step_ratio` | 1.22 | 0 .. 2.6 |

Gate: **PASS** — fit=1.00 nn=P10_berry-nebula d=0.196

## Colors (18)

`010003` `000101` `001111` `032e2e` `084e4e` `11706f` `1a9492` `24bbb7` `46e1da` `a8fef7` `f6e3fe` `e4affc` `c882ea` `a858d1` `8630b1` `620c89` `3a0455` `170126`
