# dazzle64

**An homage to DAZZLE.EXE — the magical DOS kaleidoscope — hand-written in
ARMv9.2-A assembly for Apple Silicon.**

> "I spent hours and hours staring at that magical kaleidoscope. Never the
> same pattern, never the same color scheme. It was amazing, and everything
> looked cool." — the reason this exists

Thirty years after the original DOS screensaver, this is a from-scratch
rebuild: every pixel drawn by hand-written ARM64 assembly (`draw.s`), with a
tiny SDL2 shim (`main.c`) standing in for INT 10h and a flat framebuffer.

**The pattern lab gallery: https://dazzle.jelia.nyc** — 100 numbered pattern
prototypes and 30 palettes, built as the expansion roadmap.

## What it does

- **24 drawing routines** on a shuffled wheel — interference kaleidoscopes,
  the radius-sheared *twist*, tunnels, moiré eyes, spirographs that draw
  themselves over 34 seconds, string-art fans, curl gardens, fireworks with
  gravity-drooped spark trails, and more — several reverse-engineered from
  video frames of the original
- **Really random**: avalanche-hashed mode order (no back-to-back repeats),
  random launch seed, random color-scheme pairs every leg
- **30 color schemes**: 6 house palettes + 24 community palettes from
  [Lospec](https://lospec.com) (PICO-8, NES, Apollo, resurrect-64, …)
- **All integer math**: 16-bit interpolated sine tables, fixed point,
  octagonal norms — the way 1994 would have wanted it. ~175 fps at
  1280×960 on an M-series core, single thread

## Build & run

```
brew install sdl2
make run          # builds tables (python3+numpy) + binary, launches
```

`dist/Dazzle.app` is a self-contained bundle (SDL2 inside) — copy it to any
Apple Silicon Mac and double-click. ESC quits.

## The lab

`lab/` holds a 100-entry pattern catalog — each with a runnable Python
prototype, a rendered preview, and an integer-ARM64 porting plan — plus 30
palettes and the research that produced them (frame-by-frame analysis of
original DAZZLE footage, DOS demoscene techniques, kaleidoscope math).
Patterns get promoted from the lab into `draw.s` by number.

## Versions

`builds/` archives every release: `dazzle1.0` (the first "sweet" build)
through the current wheel. `git tag j-favorite` marks the canonical one.

## Credits

- The unknown author of the original **DAZZLE.EXE** — the high-water mark
- [Lospec](https://lospec.com) and its palette artists (palette slugs are
  preserved in `reference/palettes.json` and `lab/palettes/`)
- Built with hand-written assembly, a Makefile, and stubbornness

MIT — see LICENSE.
