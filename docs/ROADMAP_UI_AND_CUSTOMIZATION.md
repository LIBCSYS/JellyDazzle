# Roadmap: help overlay + user customisation (J, 2026-08-15)

## 1. Help overlay (near-term — next pass)
"a little question mark at the top, or some button to hover over and push to
pop up a legend, description of the app, and how to launch the monitor"

- A small ? in a corner that fades in when the mouse moves and fades out after
  ~3 s of stillness (a screensaver must not wear a permanent button).
- Click or `?` / `H` key: a legend panel drawn INTO the framebuffer (the app has
  no toolkit; the meter already proves in-frame UI works):
    JellyDazzle vX.Y   ·  what it is in two lines
    ESC quit · F full screen · M / ⌘M audio meter · ? this panel
    "It listens": mic in use / which device / privacy line
    Layers: what's on screen right now (role + name of each live layer)
    link text: dazzle.jelia.nyc · github.com/LIBCSYS/JellyDazzle
- Rendering: bitmap font baked as data (5x7 or 8x8, ~96 glyphs) — no OS text
  APIs, keeps the render path pure. Panel is a translucent dark rect + text,
  composited last, never affects the motion budget of the art beneath.
- SDL gives us mouse motion + clicks for free.

## 2. User customisation (long-term architecture — start now, ship gradually)
"a way for the user to customize how the kaleidoscope looks... json palettes
and shapes? ... images? or png?"

Principle: the engine already treats palettes and patterns as DATA — the
palette table is generated from JSON, patterns are registered plug-ins. The
job is to move that boundary from build time to run time, one layer at a time.

### 2a. Palettes — EASIEST, do first
- User folder: ~/Library/Application Support/JellyDazzle/palettes/*.json
  (same schema as assets/palettes/designed/*.json: {slug, colors:[hex]}).
- On launch: expand any user JSON into 32768-entry ramps (port the OKLab +
  Catmull-Rom expansion from tools/gen_palettes.py to C — ~200 lines) and
  append them to the scheme pool. Zero rebuild. Bad files: skip + log.
- Also accept .gpl (GIMP), .ase (Adobe) and Lospec .hex later — same pipeline.

### 2b. Images as texture sources — PNG (recommended over WMF/BMP)
- Drop a PNG in .../JellyDazzle/textures/. A "texture" pattern family samples
  it through the kaleidoscope folds, twists, and tunnels — the user's own photo
  or artwork becomes the raw material the machine kaleidoscopes.
- PNG decode: stb_image.h (single header, public domain) — no new dependency.
  Also gets JPEG for free. (WMF/EMF are Windows vector formats — skip; SVG is
  the vector option and would need a rasteriser, phase 3 at best.)
- Sampling: bilinear from a downscaled copy (e.g. 512x512) so cost is flat.
- Colour rule still applies: the palette can recolour a greyscale texture, or
  the texture's own colours can be blended toward the palette by a user dial.

### 2c. Shapes as data — HARDER, phase 3
- Patterns are compiled C for speed; the user cannot write those. Options:
  (i) parametric families exposed as JSON — pick a family (rose, spirograph,
      gear train, lightning) and set its knobs (petals, ratios, tooth counts,
      hues) in a JSON file; the engine instantiates it. Covers most "I want a
      shape like X" asks with no code.
  (ii) SVG paths rasterised to a mask -> a "stencil" pattern family
       (kaleidoscoped silhouettes). Needs a small path rasteriser.
  (iii) a tiny scripting layer (Lua) for real user routines — most powerful,
        most work; only if (i)+(ii) prove insufficient.

### 2d. Settings file + in-app dials
- ~/Library/Application Support/JellyDazzle/settings.json: reactivity strength,
  brightness cap (never above the seizure-safe ceiling), speed multiplier,
  layer count, families on/off (e.g. "no lightning"), favourite palettes,
  which audio device.
- Same panel machinery as the help overlay renders a settings page; changes
  apply live and persist.

### Sequencing
  next pass:   help overlay (1) + user palettes JSON (2a)
  after that:  PNG textures (2b) + settings file (2d)
  later:       parametric shape JSON (2c-i), then stencils (2c-ii)
Everything additive; the built-in 600 patterns and 180 palettes remain the
default experience.
