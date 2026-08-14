# P35 Foundry Blue

**Class** `duotone` — Exactly two hue poles, 110-180 deg apart, each with a full ramp. No third family. Reads as a two-ink print.

**Scheme** steel blue 248-256 vs amber 58-66

## Mood
Cold steel stock and the orange heat coming off it — a forge at night, two temperatures and no third opinion.

## Look
A steep blue ramp and a steep amber ramp climbing to a shared near-white, with the amber deliberately starting higher so the two inks are told apart by value as well as hue.

## Pattern pairing
Heat, diffusion and metaball patterns; anything where one field should look hotter than the other.

## Swatch

![swatch](swatch.png)

Top band: the 18 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 18 | · |
| `hue_bins` | 2 | 2 .. 4 |
| `hue_arc` | 165.024 | 110 .. 250 |
| `accent_frac` | 0.479 | · |
| `C_mean` | 0.29 | · |
| `C_lit` | 0.357 | 0.35 .. 0.9 |
| `C_sd` | 0.118 | · |
| `C_max` | 0.506 | · |
| `L_mean` | 0.458 | · |
| `L_sd` | 0.227 | · |
| `L_range` | 0.792 | 0.6 .. 1.0 |
| `dark_frac` | 0.278 | · |
| `light_frac` | 0.056 | · |
| `neutral_frac` | 0.0 | · |
| `max_step` | 15.93 | · |
| `step_ratio` | 1.63 | 0 .. 2.6 |

Gate: **PASS** — fit=1.00 nn=P04_cyberpunk-neons d=0.196

## Colors (18)

`000004` `00091a` `011f41` `05376d` `0a509f` `206ccd` `518ce3` `7facf1` `afccfb` `daa888` `d38a56` `c76c17` `a75b11` `884a0c` `6b3a07` `4f2a04` `361b02` `1e0d00`
