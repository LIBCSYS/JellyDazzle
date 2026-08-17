# JellyDazzle

A full-screen generative kaleidoscope for Apple Silicon, in the tradition of
DAZZLE.EXE — built with **AI Guided Coding for the ARMv9.2 chip for Apple
Silicon and beyond**, C, and SDL2.

What the built app contains: **24 ARMv9.2-A assembly engine modes + 610 C
pattern plug-ins**, composited four layers at a time and recoloured
continuously by a 180-scheme palette engine.

Runs on macOS 11 and later, Apple Silicon.

---

## The Dazzle method

The original DAZZLE.EXE animated by rotating the VGA DAC, not by redrawing —
the picture stayed still and the *colours* moved through it. That idea is baked
into this engine. Expensive routines draw once into a plane of palette indices
and then animate for their whole turn at one table lookup per pixel, which is
what makes fractals affordable here at all.

Credit where it is owed: DAZZLE.EXE was written by **James R. Shiflett** and
released as shareware in 1990. See `site/tribute.html`.

---

## How a frame is built

Up to four layers run at once, bottom to top:

| Layer | Job |
|---|---|
| **GROUND** | fills the screen |
| **FIELD** | covers most of it, with gaps |
| **FIGURE** | drops distinct shapes over that |
| **SPARK** | adds glints |

They blend by brightness, so dark parts of an upper layer let the one below
show through. Each layer takes a turn of its own length, then a new routine is
drawn from a shuffled bag and fades in — timing is random by design, never a
fixed loop.

The engine measures every routine at startup (coverage, motion, luma, colour
diversity) and assigns it to the layer it actually suits, rather than trusting a
label. Those measurements are cached, so only the first launch pays for it.

Two routines of the same **shape family** never run together, and layers are
pushed apart in hue, so a frame is not four versions of the same idea in the
same colour.

---

## The fractal family (2.8)

Seven routines that each speak a different visual language:

| # | Routine | What it is |
|---|---|---|
| 604 | Mandel Deep | the Mandelbrot at seven hand-picked deep-zoom regions, smooth-coloured |
| 605 | Julia Morph | a Julia set that **changes shape while you watch** — `c` interpolates through ten curated constants while the plane is re-swept fourteen scanlines a frame, forever |
| 606 | Burning Ship | one `abs()` away from the Mandelbrot, and the symmetry collapses into masts and rigging |
| 607 | Newton Basins | which root of z^n=1 each point falls to; the relaxation constant walks, so the territories spiral into each other |
| 608 | Lyapunov | not an escape fractal at all — the logistic map's stability over a two-rate plane. Markus and Hessler's "Zircon Zity" |
| 609 | Buddhabrot | records where escaping orbits *went* rather than how long they took. **Exposes over about a minute**, like a plate developing |
| 610 | Flame IFS | Draves' fractal flame as a chaos game; coefficients and variations come from the seed, so every instance is a new attractor |

Nothing in them is painted black. Interiors are coloured by orbit trap and
exteriors by trap-folded escape, so the whole frame is live colour.

They share a family, so two never run at once — several cost 3–4 ms a frame.

---

## Layout

    src/engine/    compositor.c   scheduler + layer compositor
                   routines_asm.s 24 ARM64 assembly routines
                   jellydazzle.h  the plug-in contract
    src/audio/     listen.c       system audio -> bass/mid/treble/beat
                   systap.m       Core Audio process tap
    src/app/       main.c         SDL window, native-resolution render loop
    src/patterns/  NNN_name.c     pattern plug-ins, named by what they draw
                   _*.h           shared kits (sprites, bolts, glow, veils)
    assets/        palette.bin, sintab.bin, palettes/ (sources)
    catalog/       the pattern catalogue and its command panel
    site/          landing page and tribute
    tools/         gen_palettes.py, gen_registry.sh, build_app.sh, release_app.sh
    docs/          HOW_TO_OPEN.md and design notes

Build: `make` · run: `make run` · app bundle: `make app`

Adding a pattern is dropping a `NNN_name.c` into `src/patterns/` and running
`tools/gen_registry.sh`. Nothing else needs touching.

---

## Audio

The kaleidoscope moves with whatever the Mac is playing, captured through a
Core Audio process tap (macOS 14.2+), falling back to the microphone. Audio is
analysed in memory only — never recorded, stored, or sent anywhere.

---

## Keys

| Key | Does |
|---|---|
| `F` | fullscreen |
| `M` | meter / HUD |
| `A` | about |
| `Space` / `Return` | skip the current turn |
| `Esc` / `Q` | quit |

---

## History

* **2.8** — the fractal family (604–610); nothing painted black; fractals given
  their own shape family so they never stack
* **2.7** — the whole library ships (all 400 held candidates promoted, none
  discarded); shape repetition fixed; the startup database is genuinely built
  once, with a clock and a cancel; macOS floor lifted from 26 to 11
* **2.6** — the mark; adaptive mobility, so the stillest routines get the most
  movement
* **2.5** — colour surprise: a shared ramp built from two schemes, wildcard
  overlays, luma-preserving channel rotation
* **2.4** — system-output audio capture; strobe eliminated; fullscreen no
  longer restarts the engine; probe results cached
* **2.3** — the engine this was reorganised from

---

MIT licensed. See `LICENSE`.
