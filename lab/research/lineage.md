# Dazzle Lineage Research
Researched 2026-08-13 via web. Purpose: ground the dzzle1 rebuild in what is actually known about the original DAZZLE.EXE and its era, and harvest algorithm techniques ("algorithm gold") from its published-source cousins.

---

## 1. The original: Dazzle / Razzle Dazzle (James R. Shiflett)

**Provenance**
- Author: **James R. Shiflett**, Houston, TX — day job at the Houston Advanced Research Center (admin work around an NEC SX-2 supercomputer). Wrote Dazzle nights as "a sort of therapy."
- 1990: released as shareware by **MicroTronics** for $15 as "DAZZLE EGA/VGA SUPER-KALEIDOSCOPE SCREEN SAVER" (v5.x era; v5.22 is on Vetusware, v5.0 "DAZZLE50" and the shareware set are on archive.org).
- 1992: distribution deal with **Road Scholar Software** (Houston); retail release late 1992, renamed **Razzle Dazzle**.
- 1993: QEMM-compatibility patch; shareware sales alone averaged $1,200–1,500/week.
- 1994: **Razzle Dazzle 3D** for Windows NT 3.1 with 3D graphics + MIDI.
- 1993 spin-off: Miramar Productions + Radio Shack made a 45-minute art film *Dazzle* (score by Jonn Serrie) from Razzle Dazzle visuals — LaserDisc and VHS.
- Reception: *Compute!* — "The results are simply beautiful… I can't recommend the program too highly." Competed with After Dark and Johnny Castaway; one of the most popular DOS/Windows screensavers of the 1990s.
- Exit splash easter egg: "Our Creator, evidenced by our creativity." (Shiflett was a devout Christian.)

**Implementation (as published/documented — no source ever released)**
- Written in **Turbo C** with the image-generation cores in **x86 assembly**.
- Supported EGA, VGA, and IBM 8514/A; shipped both as a standalone DOS EXE and as a **TSR** that fired after a keyboard-idle timeout.
- Used **VGA Mode X** (planar/unchained 256-color modes, incl. 320x400) and later SVGA modes.
- Smoothness on 8088/286-class machines came from three hardware tricks:
  1. **Palette rotation** (animate the DAC, not the pixels),
  2. **VGA hardware panning** (scroll without redrawing),
  3. Mode X page flipping / planar writes.
- Roughly **~30 distinct image-generation algorithms**, hopping between them with transition effects: fade, dissolve, scroll, wrap, melt.
- Signature content: **kaleidoscopic mirrored shapes** (4-fold/8-fold symmetry drawing — plot once, reflect into all octants) and **particle simulations** — fireworks and meteor showers (Shiflett said the inspiration was an explosion scene in *Star Trek*).
- No reverse-engineering write-ups of the actual generators were found; the binary + DOSBox is the only ground truth. Best study assets: the archive.org emulated copy (45 screenshots) and starry_gulf's 12-hour capture (via Adafruit blog).

**Takeaway for dzzle1:** the authentic recipe is *cheap geometry × symmetry fold × palette-cycled color*, with the "motion" mostly living in the DAC and the CRTC, not in pixel pushes. That is exactly the aesthetic to reproduce.

---

## 2. Cousins with published source / algorithm descriptions

### Acidwarp (1992, Noah Spurrier & Mark Bilk) — SOURCE AVAILABLE
- MS-DOS, 320x200x256; **40+ patterns** plotted from 2D math formulas, then animated purely by **palette rotation**.
- **Algorithm gold:** every pattern is `pixel = f(x, y) mod 256` where f is built from polar terms — distance and angle to one or more centers, sums/products of sine LUTs — computed once into the framebuffer; all subsequent animation is DAC writes. Integer-only math via precomputed **lookup tables** (sine, atan, sqrt/distance) — no FPU needed. LUTs were sized for up to 2048x2048.
- Maintained C source: **github.com/dreamlayers/acidwarp** (SDL1/SDL2/Emscripten/WebGL ports; `-o` flag reverts to original integer-LUT mode). Pattern functions live in `lut.c` / `img.c`; palette generators + rotation in `palinit.c` / `rolnfade.c`. This is the single best readable reference for Dazzle-style static-image + cycling generators.
- Trivia: Spurrier's face in the intro is encoded pixel-by-pixel in the source.

### Cthugha (1993, Kevin "Zaph" Burfitt, Digital Aasvogel Group) — "An Oscilloscope on Acid"
- First big PC **audio-reactive** visualizer; DOS, assembly-optimized for 486, later Winamp plugin + Linux (Cthughanix); modern ports exist (cthugha-js TypeScript/PIXI, cthugha-esp).
- **Algorithm gold — the feedback triad** that MilkDrop later made famous:
  1. Draw seed content into an indexed 8-bit buffer (waveform/oscilloscope trace),
  2. **Flame/decay filter:** each pixel := average of neighbors (minus decay) → blur + fade,
  3. **Translation tables:** a precomputed per-pixel source-offset map that warps the whole buffer each frame (spirals, zooms, starfield-travel, scroll) — i.e. displacement mapping as a LUT, pure integer, no per-frame trig,
  plus **palette cycling** on top.
- Translation tables are the era's key trick for "expensive-looking" motion at 8-bit cost — directly portable to a Dazzle-style engine.

### Bomb (1994, Scott Draves) — SOURCE AVAILABLE
- One of the first interactive software artworks and arguably the first open-source artwork; "organic visual music," used for VJing. Source: **github.com/scottdraves/bomb**.
- Modes include reaction-diffusion, cellular automata, and **fractal flames** — Draves' 1991–92 invention: an IFS with non-linear variation functions, log-density rendering, and structural coloring. Flame code open-sourced 1992 → Apophysis, After Effects, Electric Sheep (1999). Math paper + code at **flam3.com**.

### Geiss → MilkDrop lineage (Ryan Geiss)
- **Geiss** (1998): CPU Winamp plugin — classic plasma/warp feedback loop, spiritual heir of Cthugha.
- **AVS** (2000, Justin Frankel/Nullsoft): modular stack of render + feedback-warp components with user math expressions — the "preset" model.
- **MilkDrop** (2001; 2.0 in 2007): same feedback-warp idea moved to GPU — per-frame/per-vertex equations (EEL language) drive a warp mesh over the previous frame, pixel shaders in 2.0; self-normalizing audio input. Lives on as Butterchurn (WebGL). geisswerks.com documents it.
- Lineage in one line: **Cthugha (decay + translation LUT) → Geiss (CPU warp) → AVS (scriptable modules) → MilkDrop (GPU warp mesh) → Butterchurn.**

### After Dark (1989 Mac, 1991 Windows — Berkeley Systems)
- By Jack Eastman & Patrick Beard. Flying Toasters (Eastman's late-night kitchen epiphany), plus a **module** architecture — dozens of pluggable displays with per-module settings sliders. Editions: More After Dark, Before Dark, Star Trek/Simpsons/Disney tie-ins. Sued by Jefferson Airplane over Flying Toasters (Berkeley won, 1994).
- Relevance: not algorithmically deep, but it defined the *product shape* (module gallery + settings) and included its own psychedelic-kaleidoscope modules. Dazzle competed directly with it.

### Pre-history / context
- 1983: **SCRNSAVE** by John Socha (of Norton Commander), published in Softalk Dec 1983 — first PC screensaver, just blanked the screen after 3 min; Socha coined "screen saver."
- Burn-in on high-contrast CRTs was the original justification; by After Dark/Dazzle the genre was entertainment.
- Deeper roots: oscilloscope Lissajous displays, and 1960s display hacks (HAKMEM-era "munching squares") as the ancestral pattern generators.
- **Mode X** itself: 320x240 (and variants like 320x400) planar 256-color VGA, popularized by Michael Abrash's 1991 Dr. Dobb's articles — the same technique Dazzle exploited.

---

## 3. Algorithm-gold cheat sheet (what to steal for dzzle1)

| Technique | Origin | Cost | Effect |
|---|---|---|---|
| Palette rotation / DAC cycling | Dazzle, Acidwarp | ~0 (256 DAC writes/frame) | Flow, pulse, hue travel on a static image |
| Polar-function image `f(r,θ) mod 256` via integer LUTs | Acidwarp | one-time fill | Endless moiré/interference/tunnel patterns |
| Symmetry fold (plot 1 point → mirror 4/8 ways) | Dazzle kaleidoscope | trivial | Kaleidoscope look, 4–8x fill speedup |
| Particle systems w/ gravity + trails | Dazzle fireworks/meteors | cheap | Organic motion contrast to geometric modes |
| Hardware panning / page flip (Mode X) | Dazzle | 2 CRTC regs | Smooth scroll with zero redraw |
| Decay/blur filter (neighbor average − k) | Cthugha | 1 pass | Glow, trails, fire |
| Translation table (precomputed per-pixel warp LUT) | Cthugha | 1 indexed copy/frame | Zoom/rotate/spiral feedback without trig |
| Warp-mesh feedback w/ per-vertex equations | Geiss/MilkDrop | GPU-era | The modern generalization of the above |
| IFS / fractal flame w/ log-density + variations | Draves (Bomb/flam3) | heavy | High-art static frames |
| Module architecture + transitions (fade/melt/dissolve) | After Dark, Dazzle | — | Product structure: N generators + switcher |

---

## Sources
- https://en.wikipedia.org/wiki/Razzle_Dazzle_(software)
- https://archive.org/details/msdos_dazzle_shareware
- https://archive.org/details/msdos_festival_DAZZLE50
- https://vetusware.com/download/DAZZLE%20EGAVGA%20SUPER-KALEIDOSCOPE%20SCREEN%20SAVER%205.22/?id=9838
- https://blog.adafruit.com/2022/04/15/12-hours-of-razzle-dazzle-an-advanced-dos-screensaver-vintagecomputing-retrocomputing-starry_gulf/
- http://eyecandyarchive.com/Razzle%20Dazzle/ (cert expired at fetch time; summary text via search index)
- https://en.wikipedia.org/wiki/Mode_X
- https://github.com/dreamlayers/acidwarp (+ README)
- https://archive.org/details/ACIDWARP
- https://en.wikipedia.org/wiki/Color_cycling
- https://shoggothox.com/blog/history-of-visualizers.html
- https://linuxgazette.net/issue17/cthugha.html
- https://github.com/delaneyparker/cthugha-js
- https://github.com/scottdraves/bomb
- https://en.wikipedia.org/wiki/Fractal_flame / https://flam3.com/
- https://en.wikipedia.org/wiki/MilkDrop / https://www.geisswerks.com/milkdrop/milkdrop.html
- https://hybridcopynet.wordpress.com/2025/02/19/the-history-of-winamps-visualizations/
- https://en.wikipedia.org/wiki/After_Dark_(software)
- https://www.theparisreview.org/blog/2025/05/20/recurring-screens/ (Socha/SCRNSAVE)
- https://en.wikipedia.org/wiki/John_Socha
