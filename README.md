<p align="center"><img src="assets/brand/jellydazzle-logo.jpg" alt="JellyDazzle" width="820"></p>

# JellyDazzle 2.4.4

Clean-room reorganisation of JellyDazzle 2.3 (LIBCSYS/JellyDazzle) with a
proper layout, verified system-audio capture, and a rebuilt startup path.

What the built app actually contains: **24 ARMv9.2-A assembly engine modes +
201 C pattern plug-ins**, composited in up to four layers with palette windows
drawn from 180 schemes. A further 400 candidate patterns live in
`src/patterns_hold/` — they all compile, but are NOT linked into the binary.
They are kept in the tree deliberately (no work is discarded) and are reviewed
a family at a time before promotion into `src/patterns/`. Browse every routine,
with animated previews, at **https://dazzle.jelia.nyc/library/**.

### Keys

| Key | Does |
|---|---|
| **F** | fullscreen |
| **M** | audio HUD — source, bass/mid/treble, beat, BPM |
| **A** | about — version, source, library, key map |
| **Esc** | quit |

### Changes in the 2.4 line

**2.4.4** — an overlay could vanish into the layer beneath it. Each layer takes
a window into the shared palette ramp, and that offset was drawn at random with
no regard for the other live layers: measured over 232 concurrent pairs, 6.5%
sat within 5% of the same hue and the closest measured 0.0%. A spawning layer
now picks the furthest of eight candidate offsets. Worst separation is now
17.6%, with nothing under 15%.

**2.4.3** — it kept opening on the same things. The openings were genuinely
varied (26 unique routines in 30 launches), but for the first ~2 s only ONE
layer is on screen, so first impressions come from a single bag of ~77
backgrounds drawn with no memory across launches — a repeat every ten runs or
so, by the birthday paradox. The engine now remembers its last 25 openings and
refuses them. 30 launches, 30 unique openings.

**2.4.2** — the colour never stops. Palette rotation was driven entirely by
audio, so in silence hues morphed but stopped travelling. There is now an idle
floor of about one turn of the ramp every 50 s, audio stacking on top. Taken
from video of the original DAZZLE.EXE, where eight of twelve sampled frames are
the same geometry with only the colour moving.

**2.4.1** — app icon; ABOUT card on **A**; repo/library URLs moved off the
audio meter; fixed `JD_AUDIO_SRC=off` disabling the HUD as a side effect.

**2.4.0** — system-output audio capture via a Core Audio process tap (not just
the mic); strobe eliminated (layer palette windows are cyclic, so audio-driven
rotation can no longer drag a pattern across a discontinuity); fullscreen no
longer restarts the engine, and no longer crashes; probe results cached to
`~/Library/Application Support/JellyDazzle` so every launch starts with the
whole library sorted into layers.

    src/engine/    compositor.c   scheduler + layer compositor (was bridge.c)
                   routines_asm.s 24 ARM64 assembly routines (was draw.s)
                   jellydazzle.h  the plug-in contract
    src/audio/     listen.c       microphone -> bass/mid/treble/beat
    src/app/       main.c         SDL window, native-resolution render loop
    src/patterns/  NNN_name.c     pattern plug-ins, named by what they draw
    assets/        palette.bin, sintab.bin, palettes/ (sources)
    tools/         gen_palettes.py, gen_registry.sh, build_app.sh, release_app.sh
    docs/          HOW_TO_OPEN.md

Build: `make` · run: `make run` · app bundle: `make app`

Engine port verified byte-identical to 2.3.0 at the time of the port (frame
hashes at 3 sample points); the changes listed above came after.


---

## In tribute

JellyDazzle exists because of **DAZZLE.EXE**, written by **James R. Shiflett** of
Houston, Texas. He administered an NEC SX-2 supercomputer at the Houston Advanced
Research Center by day, and wrote Dazzle at night — "I'd do my administrative trick
during the day, and at night I'd sit on my computer and think up new algorithms to
add to Dazzle."

Released as $15 shareware through his own MicroTronics in 1990; picked up by Road
Scholar Software and sold at retail as *Razzle Dazzle* from late 1992. Around thirty
image-generation algorithms, mirrored into kaleidoscopic symmetry, animated almost
entirely by cycling the VGA palette rather than pushing pixels.

Its source was never released. This project is not a port and not a reverse
engineering — nothing was disassembled. It is an homage written from scratch, after
the same feeling.

Shiflett's exit splash read: *"Our Creator, evidenced by our creativity."*
