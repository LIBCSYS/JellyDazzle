<p align="center"><img src="assets/brand/jellydazzle-logo.jpg" alt="JellyDazzle" width="820"></p>

# JellyDazzle v2.4 — Audio

Clean-room reorganisation of JellyDazzle 2.3 (LIBCSYS/JellyDazzle) with a
proper layout, verified system-audio capture, and a rebuilt startup path.

What the built app actually contains: **24 ARMv9.2-A assembly engine
modes + 201 C pattern plug-ins**. A further 400 candidate patterns live in
`src/patterns_hold/` — they are NOT compiled into the binary. They are kept in
the tree deliberately (no work is discarded) and are being reviewed a family at
a time before promotion into `src/patterns/`. 30 of them do not currently
compile: they were written against a `_spark572.h` sprite kit that has not been
written yet, and are marked as such in the catalogue.

Changes over 2.3:
  * system-output audio capture via a Core Audio process tap (not just the mic)
  * strobe eliminated — layer palette windows are cyclic, so audio-driven
    palette rotation can no longer drag a pattern across a discontinuity
  * fullscreen no longer restarts the engine (resize keeps probe stats + bags)
  * probe results are cached to ~/Library/Application Support/JellyDazzle, so
    every launch starts with the whole library sorted into layers instead of
    spending its first ~6.5 s drawing from a near-empty pool

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
