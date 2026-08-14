# Palette Pool — Lospec Mining Notes

**Date:** 2026-08-13
**Source:** Lospec palette-list API (`/palette-list/load`), sorted by downloads, min 8 colors.
**Result:** 252 new palettes (8–64 colors each, avg ~22), deduped by slug against the 24 already in `reference/palettes.json`.
**Output:** `lab/research/palettes_pool.json` — array of `{slug, colors:[hex...]}` (hex lowercase, no `#`).
**Fetch script:** `lab/research/fetch_palettes.py`; per-palette tag/size/downloads metadata in `_meta.json`.

## Moods found (per tag)

| Mood / tag | Count | Character | Standouts |
|---|---|---|---|
| neon | 18 | Saturated electric hues on dark grounds; arcade/synth glow | chasm, funkyfuture-8, cyberpunk-neons, technogarten |
| sunset | 19 | Orange→magenta→purple gradients, dusk skies | sunset, sunraze, fiery-dreams, purplemorning8 |
| nature | 19 | Balanced greens/browns, soft daylight, cozy landscape sets | comfy52, hilda32, autumn-glow, aragon16 |
| pastel | 16 | Low-saturation high-value; milky, dreamy, kawaii | vanilla-milkshake, pastel-64, fairydust-8, hydrangea-11 |
| gameboy | 10 | 4-tone handheld ramps and Pokemon-era limited sets | wlk44-v2, pokemon-crystal-legacy, toybox32 |
| candy | 10 | Bubblegum brights, sugar pinks/mints, playful | cartoon-candy, daifuku-delights-24, sprinkle-cake-36 |
| horror | 9 | Desaturated, blood reds, sickly greens, oppressive darks | grim32, darkseed-16, lovecraftian-lens, survival-horror-32 |
| ocean | 9 | Teal/azure ramps, seafoam, sand accents | seafoam, azure-abyss, tropical-cone-24 |
| desert | 9 | Sun-bleached ochres, terracotta, western dust | mojave20, life-on-mars, western |
| retro | 8 | Console/computer-era sets (ZX Spectrum, N64-ish, jehkoba) | jehkoba64, zx-spectrum, antiquity16 |
| autumn | 8 | Harvest oranges, spice browns, library warmth | autumn-harvest-37, hidden-library, herbs-n-spices |
| winter | 8 | Icy blues, frost whites, dark cabin contrast | winter-wonderland, permafrost12, logcabin |
| night | 8 | Moonlit blues/purples, low-key value ranges | moonlight-15, nightsky-bricks, undernight-20 |
| earthy | 8 | Clay, moss, loam; muted organic midtones | overgrown-42, moth-27, petrichor |
| vaporwave | 7 | Pink/cyan/purple retrowave, Miami dusk | pastel-horizon, vaporsthetic, vapornes |
| gold | 7 | Gilded yellows, bronze/patina, treasure shine | golden-helmet, smooth-polished-gold, aerugo |
| metallic | 6 | Steel greys, holographic sheen, forge tones | 16-metallic, holographic-mode, ennis-blade |
| cyberpunk | 6 | Crimson/neon on near-black; city-noir | ink-crimson, cyberpunk-neon-city, neon-pulse-8 |
| space | 6 | Nebula purples, cosmic teals, star highlights | berry-nebula, nebulosa, aquaverse |
| monochrome | 6 | Single-hue or grayscale value ramps | ammo-8, grayscale-16, mariana-trench |
| cold | 5 | Chilled blue-greys, frozen accents | frozen-fire, blue-glass, cold-morning-32 |
| warm | 4 | Toasted ambers, parchment, hearth tones | toasted40, parchment-and-ink, waldgeist |
| forest | 4 | Deep green canopies, mire golds | ephemera, gold-mire, fairytale-forest |
| fantasy | 3 | Storybook saturated general-purpose sets | fantasy, ty-high-fantasy-40 |
| untagged (deep pages 2–7) | 39 | High-download general-purpose sets missed by tags | famicube, dawnbringer-32, borkfest, rust-gold-8, lux2k |

## Observations

- **Size spread:** 8–64 colors, average ~22 — plenty of small accent sets (8–12) and big general-purpose ramps (32–64).
- **Dedupe:** all 24 reference slugs excluded; no duplicate slugs within the pool (tags overlap heavily on Lospec, so cross-tag dedupe mattered — ~80 cross-tag repeats were dropped).
- **Coverage axes:** the pool now spans temperature (warm/cold), value (pastel vs horror/night), saturation (neon/candy vs earthy/monochrome), and era (gameboy/retro vs modern general-purpose).
- **Good defaults for generative work:** untagged deep-page sets (famicube, dawnbringer-32) are battle-tested general palettes; tagged sets are better as mood-specific skins.
