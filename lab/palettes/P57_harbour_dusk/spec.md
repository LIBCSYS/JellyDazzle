# P57 Harbour Dusk

**Class** `analogous` — Neighbouring hues only -- a continuous slice of the wheel, no more than a third of a turn. Warm and cool instances are separate palettes and the diversity gate keeps them apart.

**Scheme** sea green 140 -> teal -> steel blue 250

## Mood
The hour after sunset over water: green low on the horizon going teal, then steel blue overhead.

## Look
The same builder as Iris Morning run at the opposite tone — ground at L 0.05, chroma scaled to 0.6, dark_frac 0.36. Same class, same construction, and the two share neither hue zone nor weather.

## Pattern pairing
Ripple, caustic and slow-wave patterns; anything where brightness rather than hue should carry the story.

## Swatch

![swatch](swatch.png)

Top band: the 25 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 25 | · |
| `hue_bins` | 4 | 3 .. 5 |
| `hue_arc` | 83.099 | 50 .. 125 |
| `accent_frac` | 0.288 | 0.0 .. 0.32 |
| `C_mean` | 0.182 | · |
| `C_lit` | 0.221 | 0.3 .. 0.85 |
| `C_sd` | 0.064 | · |
| `C_max` | 0.301 | · |
| `L_mean` | 0.424 | · |
| `L_sd` | 0.242 | · |
| `L_range` | 0.801 | 0.6 .. 1.0 |
| `dark_frac` | 0.36 | · |
| `light_frac` | 0.04 | · |
| `neutral_frac` | 0.04 | · |
| `max_step` | 12.05 | · |
| `step_ratio` | 1.81 | 0 .. 2.4 |

Gate: **PASS** — fit=0.91 nn=P86_verdigris_steel d=0.182

## Colors (25)

`000100` `000400` `000500` `002111` `004132` `02635a` `268688` `48a9b8` `6fccec` `c0d3e8` `a4c2da` `88b2ca` `6aa2b9` `4993a7` `1d8492` `0f7378` `0b615f` `075048` `05473d` `043c30` `023124` `012719` `011d0f` `001306` `000a02`
