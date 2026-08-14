#!/usr/bin/env python3
"""Generate P16-P30 designed palettes: palette.json + swatch.png + spec.md each."""
import json, os, subprocess

ROOT = "/Users/exeter/dev/m5/assembly/dzzle1/lab/palettes"

# Each: (id, slug, name, scheme, mood, [(hex, role), ...], look, pairing, notes)
PALETTES = [
 ("P16", "cathedral_glass", "Cathedral Glass", "triadic (ruby / gold / sapphire) + emerald accent",
  "Jewel-toned stained glass at night — deep leaded darks with luminous ruby, gold and sapphire panes.",
  [("0d0b10","lead black"),("1b1722","lead shadow"),
   ("4a0d20","ruby deep"),("7c1230","ruby dark"),("b01c3c","ruby"),("e63a55","ruby bright"),("ff7a8a","ruby glow"),
   ("5c3a08","gold deep"),("9c6510","gold dark"),("d9960f","gold"),("ffc93c","gold bright"),("ffe9a0","gold glow"),
   ("101c4a","sapphire deep"),("1c3a8c","sapphire dark"),("2f62d9","sapphire"),("5f9dff","sapphire bright"),("aacdff","sapphire glow"),
   ("0e4d2e","emerald deep"),("1f9e5a","emerald"),("5ee08a","emerald bright"),
   ("f5f0ff","light through glass")],
  "Rich saturated jewel ramps separated by near-black leading; every hue reads against the dark ground.",
  "Best on tunnel/mandala patterns with hard cell edges — the black leading between shapes makes panes glow.",
  "Three full 5-step ramps means smooth per-hue shading with no cross-hue mud."),

 ("P17", "peacock_court", "Peacock Court", "analogous (green-teal-blue) + copper accent",
  "Peacock plumage: iridescent teals and blues fanning into green, struck through with warm copper eyespots.",
  [("04121a","abyss"),("07293a","deep water"),
   ("0a4a5c","teal deep"),("0f7280","teal"),("12a0a0","teal bright"),("3cd3c0","seafoam"),("8ff2df","seafoam glow"),
   ("123a7a","blue deep"),("1c5fc0","blue"),("4a8fe6","blue bright"),("8fc2ff","sky glow"),
   ("0d4a2a","green deep"),("1f8a48","green"),("55c46a","green bright"),("a8e88a","lime glow"),
   ("6b3014","copper deep"),("a8542a","copper"),("e08a3c","copper bright"),("ffc27a","copper glow"),
   ("f2fff4","white shimmer")],
  "Cool analogous wash that flows hue-to-hue seamlessly; copper is ~15% of usage and lands like eyespots.",
  "Pairs with flowing interference / plasma patterns — analogous ramps interpolate without ugly midpoints.",
  "Copper accent is the complement of the teal center: guaranteed pop without breaking cohesion."),

 ("P18", "aurora_veil", "Aurora Veil", "split-complement (green vs magenta + violet)",
  "Aurora borealis over a midnight sky — curtains of green light splitting into violet and magenta fringes.",
  [("050810","night sky"),("0a1226","midnight"),("101c3a","horizon blue"),
   ("0c5a3c","aurora deep"),("12996b","aurora"),("27d98a","aurora bright"),("7dffb0","aurora glow"),("ccffd9","aurora white"),
   ("2a1454","violet deep"),("4a2496","violet"),("7a44d9","violet bright"),("ab7aff","violet glow"),
   ("8a1a6a","magenta deep"),("cc2fa0","magenta"),("ff6ad5","magenta glow"),
   ("1c4a8a","sky blue"),("3f83c9","sky bright"),
   ("eef4ff","starlight")],
  "Green dominates, magenta/violet fringe it — the classic split-complement tension of real aurora photos.",
  "Made for slow vertical-curtain and sine-ribbon patterns; ramps are long enough for soft falloff.",
  "Value range runs 0x05 to 0xff so additive-looking glow works on a truly dark ground."),

 ("P19", "macaw_riot", "Macaw Riot", "triadic (scarlet / yellow-green / blue), full saturation",
  "Scarlet macaw in jungle shade — hot red and yellow wings against electric blue and lime canopy.",
  [("140806","jungle shadow"),("26100a","bark"),("0a1a10","leaf shadow"),
   ("8a1010","scarlet deep"),("cc2418","scarlet"),("ff4a2a","scarlet bright"),("ff8a5a","scarlet glow"),
   ("b87f00","amber deep"),("f2b705","amber"),("ffe14a","amber bright"),
   ("2a6a10","green deep"),("4fa524","green"),("8ad93c","lime"),("ccf07a","lime glow"),
   ("0f2a7a","blue deep"),("1c50c4","blue"),("3f8ce6","blue bright"),("85c8ff","blue glow"),
   ("12b0a0","turquoise accent"),
   ("fff8e8","feather white")],
  "Maximum-chroma triad kept cohesive by shared dark jungle grounds and matched ramp lengths.",
  "For bold hard-edged kaleidoscope patterns — big flat shapes in triad hues never clash.",
  "This is the loudest palette of the set; use where the original dazzle went full RGB-primary."),

 ("P20", "koi_pond_dusk", "Koi Pond Dusk", "analogous (blue-green water) + orange-white koi accent",
  "Looking down into a koi pond at dusk — layered teal water, lily greens, and koi flaring orange beneath.",
  [("06131c","pond deep"),("0a2433","water dark"),("10394a","water"),("1a5a66","water mid"),("2f8a86","water bright"),("5ec4ac","shallows"),("a8e8cf","surface light"),
   ("2a1440","dusk plum"),
   ("1e5a34","lily deep"),("3f8a4f","lily"),("7ec46a","lily bright"),
   ("7a2410","koi deep"),("c44514","koi rust"),("f2712a","koi orange"),("ffa04f","koi bright"),("ffd08f","koi gold"),
   ("f5f2e8","koi white")],
  "Seven-step water ramp carries the field; koi orange is the exact complement of the teal center.",
  "Ideal for ripple / interference / metaball patterns — water ramp shades smoothly, koi tones sparkle.",
  "The single plum note keeps long water gradients from feeling flat."),

 ("P21", "blacklight_lounge", "Blacklight Lounge", "double-complement (violet-yellow x pink-green), UV fluorescents",
  "A 70s blacklight poster room — deep UV violet walls with fluorescent pink, green and yellow ink glowing.",
  [("0a0514","UV black"),("160a2e","UV deep"),("241452","UV shadow"),
   ("3a1f8a","violet deep"),("5a35cc","violet"),("8a5fff","violet bright"),("bfa0ff","violet glow"),
   ("7a1060","fluoro pink deep"),("c41f9e","fluoro pink"),("ff3fd4","fluoro pink bright"),("ff8ae8","pink glow"),
   ("1f7a2a","fluoro green deep"),("3fd44f","fluoro green"),("8aff7a","green glow"),
   ("c4b800","fluoro yellow deep"),("f2ee3f","fluoro yellow"),("ffff9e","yellow glow"),
   ("e8e0ff","UV white")],
  "Everything floats on violet-black; the two complement pairs give four glow inks that all pop equally.",
  "For poster-flat shapes and slow Lissajous/spirograph line patterns — lines read as neon ink.",
  "Fluorescent tones stay >= 0x3f in their weak channel so they glow instead of buzzing."),

 ("P22", "byzantine_mosaic", "Byzantine Mosaic", "double-complement (gold-lapis x porphyry-verdigris)",
  "Gold-ground mosaic in candlelight — lapis and porphyry tesserae set in shimmering gold with verdigris bronze.",
  [("12080a","grout black"),("241408","umber shadow"),
   ("5c3c10","gold deep"),("8f6314","old gold"),("c49224","gold"),("ecc45a","gold bright"),("ffe9a8","gold gleam"),
   ("0f1c4a","lapis deep"),("1c3a8c","lapis"),("3563c4","lapis bright"),("6a95e0","lapis glow"),
   ("4a1428","porphyry deep"),("7c2148","porphyry"),("b03a6a","porphyry bright"),
   ("143c2e","verdigris deep"),("2a7a58","verdigris"),("5ab88a","verdigris bright"),
   ("f2e8d4","ivory")],
  "Warm gold field vs cool lapis, tempered by wine-purple and oxidized green — imperial and heavy.",
  "Suits tiled / truchet / mosaic-cell patterns where each cell takes one flat tessera color.",
  "Gold ramp is the longest on purpose: it is the ground, the other hues are the figures."),

 ("P23", "holi_burst", "Holi Burst", "triadic (magenta / cyan-orange / yellow) + violet-green pops",
  "Festival powder clouds mid-air — magenta, marigold and turquoise dust exploding over dusk plum shade.",
  [("1a0a1e","dusk plum"),("2e1035","shadow violet"),
   ("8a1058","magenta deep"),("cc1f7e","magenta"),("ff4aa8","magenta bright"),("ff9ad0","pink powder"),
   ("b8480a","marigold deep"),("f2751f","marigold"),("ffa84a","marigold bright"),
   ("e8c414","turmeric"),("fff06a","turmeric bright"),
   ("0c7a8a","gulal teal deep"),("18bcc4","gulal teal"),("6ae8e0","teal powder"),
   ("4a1f9e","violet deep"),("7a4ae0","violet"),("ab8aff","violet powder"),
   ("3fb83f","green pop"),
   ("fff4f8","powder white")],
  "Six powder hues at matched brightness over one shared dark — chaotic joy that still coheres.",
  "For particle / dust / radial-burst patterns; each burst takes one powder ramp, ground stays plum.",
  "All brights sit near equal luminance so no single powder dominates the frame."),

 ("P24", "obsidian_magma", "Obsidian Magma", "analogous (red-orange-gold) + steel-blue accent",
  "Cooling lava field at night — glassy purple-black obsidian cracked open over white-hot orange veins.",
  [("070608","obsidian"),("141018","obsidian sheen"),("231a24","warm black"),("3a2a33","crust"),
   ("40101c","wine crack"),
   ("5c0f0a","magma deep"),("9e1c0c","magma dark"),("e03a10","magma"),("ff6b1f","magma bright"),("ff8a54","magma hot"),("ffa03a","ember"),("ffd970","white heat"),
   ("1c2a3f","steel deep"),("3a5a7a","steel"),("7a9cb8","steel glow"),
   ("e8ddd0","ash light")],
  "One long incandescent ramp from black to near-white; steel blue is the cold complement whisper.",
  "Perfect for crack/vein/cellular patterns — obsidian fills cells, magma ramp lights the borders.",
  "The black end has four distinct warm-purple darks so shadows stay alive, not dead."),

 ("P25", "orchid_greenhouse", "Orchid Greenhouse", "analogous (purple-magenta-pink) + leaf-green accent",
  "A humid orchid house — cascades of purple and magenta blooms against dark waxy leaves, cream throats.",
  [("100a14","glasshouse dark"),("1f1226","soil shadow"),
   ("3a1f5c","purple deep"),("5c2f8a","purple"),("8a4ac0","purple bright"),("b87ae0","purple glow"),
   ("7a1454","orchid deep"),("b02585","orchid"),("e84aae","orchid bright"),("ff8ad0","orchid glow"),("ffc2e8","petal pink"),
   ("143a1f","leaf deep"),("2a7a3a","leaf"),("5cb85c","leaf bright"),("a8e08a","new leaf"),
   ("ff8a70","coral accent"),
   ("fff0c4","cream throat")],
  "Warm purple-to-pink flow (the mood the pool's pastels miss: saturated + dark-grounded florals).",
  "Great on soft blob / metaball / petal-fold patterns; green keeps large pink fields from cloying.",
  "Green is the true complement of the orchid center — small doses, high impact."),

 ("P26", "thunderhead", "Thunderhead", "split-complement (blue-grey/indigo vs amber lightning)",
  "A supercell at dusk — towering slate cloud ramps, indigo rain columns, and sudden veins of amber lightning.",
  [("0a0c12","storm black"),("151a26","cloud deep"),("232c3f","cloud dark"),("36445c","cloud"),("55688a","cloud mid"),("8399b8","cloud bright"),("bccbe0","cloud silver"),
   ("2a2260","indigo deep"),("4a3fa0","indigo"),("7a6fd9","indigo bright"),
   ("1f5c66","rain teal deep"),("3f9ea0","rain teal"),
   ("e0a01f","lightning deep"),("ffdf5c","lightning"),("fff2a8","lightning glow"),("ffffff","strike white")],
  "A seven-step neutral-blue cloud ramp does the heavy lifting; amber appears rarely and reads electric.",
  "For slow rolling-noise / cloud patterns with occasional bright filament sweeps (no strobing).",
  "The pool has no storm mood: desaturated blues WITH a hot accent is the gap this fills."),

 ("P27", "carnival_glass", "Carnival Glass", "double-complement (amber-blue x rose-green) iridescent",
  "Antique carnival glass under a lamp — oil-slick iridescence sliding amber to rose to teal to green.",
  [("140c0a","glass black"),("261410","amber shadow"),
   ("8a4a14","amber deep"),("cc7a1f","amber"),("f2a83f","amber bright"),("ffd98a","amber sheen"),
   ("8a2440","rose deep"),("cc4a70","rose"),("ff8aa0","rose sheen"),
   ("0f4a5c","teal deep"),("1f8a9e","teal"),("5cc4d4","teal sheen"),
   ("3f7a2a","green deep"),("7ab84a","green"),("c4e07a","green sheen"),
   ("5c2a7a","purple sheen deep"),("9e5cc4","purple sheen"),
   ("fce8f0","pearl highlight")],
  "Five hue families all with a bright 'sheen' step — cycle through them in order for oil-slick motion.",
  "Built for thin-film / interference-band patterns; adjacent bands take adjacent hue families.",
  "Iridescence = hue varies while value stays high; the matched sheen steps make that trivial."),

 ("P28", "absinthe_hour", "Absinthe Hour", "analogous (green-chartreuse-gold) + cherry-red accent",
  "Art-nouveau bar at midnight — louche green absinthe glow, brass fittings, one cherry-red bitters accent.",
  [("0a0f08","bar black"),("141f10","bottle deep"),("20301a","bottle green"),
   ("2f5c1f","absinthe deep"),("4a8a24","absinthe"),("74b82a","absinthe bright"),("a8dd3f","chartreuse"),("def27a","louche glow"),
   ("8a6a14","brass deep"),("c49c24","brass"),("edcc5c","brass bright"),
   ("0f4a3a","emerald deep"),("1f8a66","emerald"),
   ("8a1424","cherry deep"),("cc2a3a","cherry"),("ff5c5c","cherry bright"),
   ("f8f2d8","gaslight cream")],
  "A poison-green world warmed by brass; red is the direct complement, used at candle-flame scale.",
  "Suits swirling / marbling / fluid patterns — the long green ramp models the louche effect.",
  "Nothing in the downloaded pool owns chartreuse; this palette plants the flag."),

 ("P29", "nebula_reef", "Nebula Reef", "triadic (cyan / coral-orange / violet) bioluminescent",
  "An abyssal reef lit only by its own life — cyan plankton veils, violet coral fans, hot coral-orange polyps.",
  [("03060e","abyss"),("071226","deep water"),("0c1f3f","water shadow"),
   ("0f5c7a","cyan deep"),("14a0b8","cyan"),("2fe0e0","cyan bright"),("9afff2","plankton glow"),
   ("8a2f1a","coral deep"),("e05c24","coral"),("ff9a4a","coral bright"),
   ("3a1a6a","violet deep"),("6a35b0","violet"),("a86ae8","violet bright"),("d9aaff","violet glow"),
   ("c41f8a","magenta polyp"),("ff6ad0","magenta glow"),
   ("e8faff","biolum white")],
  "Cold cyan/violet field with warm coral counterweight — the triad keeps deep-sea from going monochrome.",
  "For drifting particle / tentacle / slow-orbit patterns; glow steps sit on very dark water grounds.",
  "Distinct from P18: this triad is cyan-anchored and warm-accented, not green-anchored."),

 ("P30", "spice_bazaar", "Spice Bazaar", "double-complement (saffron-majorelle x chili-turquoise)",
  "A Marrakech souk at golden hour — saffron and chili pyramids, majorelle-blue walls, mint tea and brass.",
  [("1a0e0a","souk shadow"),("2e180f","cedar dark"),
   ("6a1f14","chili deep"),("9e3520","chili"),("cc5c2a","paprika"),("e8834a","terracotta"),
   ("b87a10","saffron deep"),("e8a81f","saffron"),("ffd45c","saffron bright"),
   ("1f2a8a","majorelle deep"),("3547cc","majorelle"),("5c78e8","majorelle bright"),("93a8ff","majorelle glow"),
   ("0f6a66","turquoise deep"),("1fa89a","turquoise"),("62d9c4","turquoise bright"),
   ("1f5c3a","mint deep"),("a8e8c4","mint"),
   ("4a1f3f","plum stall"),
   ("f8ecd4","sunlit cream")],
  "Warm spice ramps against the famous majorelle blue — two complement pairs, market-stall energy.",
  "For patchwork / zellige-tile / radial-rug patterns; alternate warm and cool families per ring.",
  "Fills the pool's gap between 'desert' (too bleached) and 'neon' (too electric): saturated AND warm."),
]

def write_swatch(d, colors):
    cw, ch, per_row = 48, 64, 8
    n = len(colors)
    rows = (n + per_row - 1) // per_row
    W, H = per_row * cw, rows * ch
    import numpy as np
    img = np.zeros((H, W, 3), dtype=np.uint8)
    for i, (hx, _) in enumerate(colors):
        r, c = divmod(i, per_row)
        rgb = tuple(int(hx[j:j+2], 16) for j in (0, 2, 4))
        img[r*ch:(r+1)*ch, c*cw:(c+1)*cw] = rgb
    ppm = "/tmp/swatch_tmp.ppm"
    with open(ppm, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (W, H))
        f.write(img.tobytes())
    subprocess.run(["sips", "-s", "format", "png", ppm, "--out", os.path.join(d, "swatch.png")],
                   check=True, capture_output=True)

for pid, slug, name, scheme, mood, colors, look, pairing, notes in PALETTES:
    d = os.path.join(ROOT, f"{pid}_{slug}")
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, "palette.json"), "w") as f:
        json.dump({"id": pid, "slug": slug, "name": name, "scheme": scheme,
                   "mood": mood, "count": len(colors),
                   "colors": [c for c, _ in colors],
                   "roles": {c: r for c, r in colors}}, f, indent=1)
    write_swatch(d, colors)
    tbl = "\n".join(f"| `{c}` | {r} |" for c, r in colors)
    spec = f"""# {pid} {name}

## Mood
{mood}

## Scheme
{scheme}

## Look
{look}

## Colors ({len(colors)})
| Hex | Role |
|---|---|
{tbl}

## Pattern pairing
{pairing}

## Notes
{notes}
"""
    with open(os.path.join(d, "spec.md"), "w") as f:
        f.write(spec)
    print(f"{pid}_{slug}: {len(colors)} colors OK")
