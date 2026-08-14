<p align="center">
  <a href="https://dazzle.jelia.nyc">
    <img src="lab/hero.jpg" width="900" alt="Four moments from JellyDazzle: a glowing geodesic dome, a starburst rosette, a gold kaleidoscope, a white flower on magenta">
  </a>
</p>

<h1 align="center">JellyDazzle</h1>

<p align="center">
  <b>An homage to DAZZLE.EXE — the magical DOS kaleidoscope —<br>
  rebuilt as a layering engine in ARMv9.2-A assembly and C.</b>
</p>

<p align="center">
  <i>Vibe-coded: human direction and taste, AI execution.</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-2.3.0-ffb02e?style=flat-square" alt="version 2.1.0">
  <img src="https://img.shields.io/badge/routines-225-52e0ff?style=flat-square" alt="225 routines">
  <img src="https://img.shields.io/badge/audio-reactive-ff7ad9?style=flat-square" alt="audio reactive">
  <img src="https://img.shields.io/badge/patterns-201-ff5ec8?style=flat-square" alt="201 patterns">
  <img src="https://img.shields.io/badge/palettes-180-a0e060?style=flat-square" alt="100 palettes">
  <img src="https://img.shields.io/badge/macOS-Apple%20Silicon-lightgrey?style=flat-square" alt="Apple Silicon">
  <img src="https://img.shields.io/badge/license-MIT-lightgrey?style=flat-square" alt="MIT">
</p>

<p align="center">
  <a href="https://github.com/LIBCSYS/JellyDazzle/releases/latest"><b>Download for macOS</b></a> ·
  <a href="https://dazzle.jelia.nyc"><b>Browse all the patterns</b></a> ·
  <a href="HOW_TO_OPEN.md"><b>First-launch help</b></a>
</p>

> [!IMPORTANT]
> ### 🔓 First launch: macOS will block it — 30-second fix, once
>
> macOS says **"Apple could not verify JellyDazzle is free of malware."**
> That is Gatekeeper flagging **any** app without a paid Apple notarization
> certificate. Nothing is wrong with the app — every line of its source is in
> this repo.
>
> | | |
> |---|---|
> | **1** | **Double-click** the app once and let it get refused |
> | **2** | Open **System Settings → Privacy & Security** |
> | **3** | Scroll to the bottom — there's a line: *"JellyDazzle was blocked to protect your Mac"* |
> | **4** | Click **Open Anyway** → confirm with Touch ID or your password |
> | **5** | It launches — and every launch after that is a normal double-click |
>
> **Or the one-liner**, which always works — paste in Terminal after downloading:
>
> ```
> xattr -dr com.apple.quarantine ~/Downloads/JellyDazzle.app
> ```
>
> On macOS 15+ the old "right-click → Open" trick no longer works for unsigned
> apps. A signed, notarized build is coming — then none of this is needed.
> Full walkthrough: **[HOW_TO_OPEN.md](HOW_TO_OPEN.md)**


> "I spent hours and hours staring at that magical kaleidoscope. Never the
> same pattern, never the same color scheme. It was amazing, and everything
> looked cool." — the reason this exists

Thirty years after the DOS screensaver, this is a from-scratch rebuild — and
as of v2.1 it does something the original never did: **it layers.** A routine
opens the frame, a few seconds later a second one fades in over it, then a
third, each on its own clock with its own blend mode and lifetime. What you
watch is the *combination*, and the combinations essentially never recur.

<p align="center">
  <a href="https://dazzle.jelia.nyc">
    <img src="lab/patterns_montage.png" width="900" alt="Contact sheet of the pattern library">
  </a><br>
  <i>The pattern library, one frame each — every tile is a routine that can appear in any layer.</i>
</p>

<p align="center">
  <img src="lab/jellydazzle.gif" width="520" alt="Layers fading in over each other">
  <br><i>Layers arriving over one another — the still images above are single frames of this.</i>
</p>

## It listens

**JellyDazzle hears whatever your Mac can hear** — Spotify through the speakers,
a record, a guitar in the room — and the kaleidoscope moves with it:

- **Colour rides the music.** Treble, loudness and beats slide the whole palette
  forward, so hues travel with the track rather than on a blind clock.
- **Layers surge with the bass.** The stack thickens on the low end and thins
  again as it passes.
- **New material arrives on the beat.** A downbeat pulls the next layer's entry
  forward — never later, so things land *with* the rhythm.
- **No flashing.** Brightness moves at most 1.06×, eased in over ~8 frames and
  out over ~32. The music is carried by colour and structure, deliberately not
  by luminance — a screensaver should never be a strobe.

**Full screen is native resolution.** Press `F` (or the green button) and the
engine re-renders at your display's real pixel size — Retina included — rather
than stretching a fixed buffer. Measured at 103 fps at 3456×2160 with the layer
stack running.

macOS asks for microphone permission the first time. Nothing is recorded, stored
or transmitted: audio is analysed in memory, frame by frame, and discarded. With
no microphone or in silence the values decay to zero and the engine runs on its
own clocks exactly as before.

## How the audio works

```
microphone ──► ring buffer ──► 512-point FFT ──► envelopes ──► the engine
   (SDL)        (callback       (per frame,      (fast attack,   (colour,
                 copies only)    ~30 µs)          slow release)   layers, timing)
```

**Capture.** SDL opens a real input device — explicitly by name, because the
system default is often a virtual device (Teams, Zoom, a disconnected webcam)
that delivers digital silence forever. `JD_AUDIO_DEV=n` overrides the choice.
The capture callback does nothing but copy samples into a ring buffer.

**Analysis,** once per frame: a Hann-windowed 512-point FFT, split into
**bass** (~86–260 Hz), **mid** (~350 Hz–2 kHz) and **treble** (2 kHz+), plus
overall RMS. Onsets come from **spectral flux** — the sum of positive
bin-to-bin change — compared against an adaptive running mean, with a
refractory gap so a sustained note isn't a drum roll. Beat gaps feed a running
tempo estimate.

**Smoothing.** Every value is enveloped: attack over ~2 frames, release over
~16. Visuals read them as targets, never as instant jumps. That single decision
is what makes it *sway* instead of flicker.

**What the numbers drive:**

| Signal | Effect |
|---|---|
| treble + level + beats | slides the whole palette forward — colour travels with the track |
| bass | layer weights surge 0.45×–1.6×; the stack thickens and thins |
| beat | pulls the next layer's entry forward, up to 1.5 s — never later |
| bass + beat | brightness, capped at **1.06×**, eased in over ~8 frames and out over ~32 |

The colour rotation is free at draw time: every layer's palette is stored twice
in memory, so handing a pattern `pal + offset` stays in bounds without copying
anything.

**Why brightness barely moves.** A screensaver that flashes on the beat is a
seizure risk, so the music is carried by colour and structure instead. That was
a deliberate design call, and the cap is enforced in code.

**Privacy.** Audio is analysed in memory, frame by frame, and discarded.
Nothing is recorded, stored, or transmitted. In silence — or with no microphone
at all — every value decays to zero and the engine runs on its own clocks
exactly as it always did.

## What it is

- **225 routines** — 24 hand-tuned ARM64 assembly modes plus 201 C plug-ins:
  kaleidoscope symmetries, demoscene classics (plasma, rotozoom, tunnels,
  metaballs, munching squares, copper bars), interference and moiré,
  self-drawing spirographs and harmonographs, cellular and Belousov-Zhabotinsky
  spirals, particle swarms, fireworks, organic growth, and a multicoloured
  lava lamp whose wax drifts over whatever is beneath it
- **Layered composition** — up to four slots (ground, field, figure, spark)
  with staggered entry, alpha envelopes and crossfades. Nothing hard-cuts.
- **Nothing repeats** — shuffled bags rather than dice, re-seeded every launch,
  so a routine plays once before any repeat and no two runs deal the same deck
- **180 colour schemes** — community palettes, designed harmonies, and 60 built from colour theory (duotone, monochrome-with-accent, triadic, split-complement, stark, neon-on-ink, metallic, tetradic, earth, pastel wash), expanded
  into 32,768-entry ramps in **OKLab** along a cyclic **Catmull-Rom** spline:
  no grey midpoints, no plateaus, no hairline banding
- **All integer math in the assembly core** — sine tables, fixed point,
  octagonal norms. The way 1994 would have wanted it.

## How it works

```
main.c ──► jd_frame() ──► scheduler ──► layer slots ──► compositor ──► screen
             (bridge.c)      │             │                │
                             │             │                └── alpha / max / screen
                             │             │                    blends, crossfades
                             │             └── each slot runs one routine:
                             │                 draw.s mode  or  patterns_c/pattern_NNN.c
                             └── shuffled bags, per-run entropy, role-aware picks
```

**The probe.** At startup the engine renders every routine at low resolution
and measures four things: how much of the frame it covers, how bright it is,
how fast it moves, and what it costs. From that it assigns a **role** —
*ground* (can carry a picture), *field*, *figure*, or *spark* (sparse, belongs
on top) — and the scheduler only ever draws a ground layer from routines that
actually emit light. Cost feeds an admission budget so the stack never exceeds
the frame time.

**The plug-in contract** (`jellydazzle.h`) is deliberately tiny:

```c
void pattern_NNN(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal);
```

`sl` is the segment-local frame, so accumulator patterns know when to clear
their canvas; `pal` is a 32,768-entry palette the engine has already
crossfaded for you. Drop a file in `patterns_c/`, run `tools/gen_registry.sh`,
and the scheduler picks it up.

**Run any routine on its own:** `JD_MODE=42 ./dazzle64`. Also `JD_LAYERS=n`
to cap the stack and `JD_DEBUG=1` to watch spawns and palette picks.

## Download & run (no tools needed)

Grab **JellyDazzle.app.zip** from the
[latest release](https://github.com/LIBCSYS/JellyDazzle/releases), unzip, and
double-click. **Apple Silicon Macs only.** ESC quits.


## The engine vs. the lab

The running app is a 24-routine wheel. The
[gallery](https://libcsys.github.io/JellyDazzle/) is the **expansion
roadmap** — 100 numbered prototypes waiting to be promoted into the assembly
engine, one verified port at a time. Watch the releases: each new version
names the lab numbers it absorbed.

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
Patterns get promoted from the lab into `draw.s` by number. The
[front page](https://libcsys.github.io/JellyDazzle/) is generated from
`lab/CATALOG.md` by `lab/_pagegen.py`.

## Versions

`builds/` archives every release: `dazzle1.0` (the first "sweet" build)
through the current wheel. `git tag j-favorite` marks the canonical one.

## Credits

- The unknown author of the original **DAZZLE.EXE** — the high-water mark
- [Lospec](https://lospec.com) and its palette artists (palette slugs are
  preserved in `reference/palettes.json` and `lab/palettes/`)
- **Direction, taste, and quality control:** John Elia (LIBCSystems) — who
  remembered the original, filmed it off a DOS box for reference, and threw
  out every version that didn't feel right
- **Assembly, C, and the 100-pattern lab:** Claude (Anthropic), under that
  direction — including the agent fleet that researched, prototyped, judged,
  and ported every pattern
- Built with a Makefile, an lldb session, and stubbornness

MIT — see LICENSE.
