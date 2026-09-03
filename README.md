# JellyDazzle

**An audio-reactive kaleidoscope for Apple Silicon.** It listens to whatever your Mac is
playing and paints to it — never the same pattern, never the same colours.

An homage to **DAZZLE.EXE**, rebuilt from scratch: a hand-written ARM64 assembly engine
under a library of 610 C pattern plug-ins.

**Version 2.8.0** · macOS 11+ · Apple Silicon · MIT
· [dazzle.jelia.nyc](https://dazzle.jelia.nyc/)
· [browse every routine](https://dazzle.jelia.nyc/library/)

---

## What the built app contains

| | |
|---|---|
| **24** | ARMv9.2-A assembly engine modes |
| **610** | C pattern plug-ins |
| **634** | routines total |
| **180** | palette schemes, interpolated in OKLab |

Every routine in the tree is compiled into the shipping binary. Nothing is held back.

The engine draws once and then moves the palette, the way the original did. Patterns are
scheduled into layers by how much they move on their own, so the stillest ones get the most
transform and nothing sits static. Two routines from the same shape family never share the
screen.

## Audio

On macOS 14.2+ JellyDazzle taps the **system audio output**, so it moves with whatever is
already playing — Spotify, a browser, anything. On earlier versions it falls back to the
microphone, which is why macOS asks for permission on first launch.

Audio is turned into numbers and thrown away. It is analysed in memory to drive the current
frame and then discarded — never recorded, never stored, never transmitted. The app has no
network capability at all. Without permission the visuals simply run on internal clocks.
See the [privacy policy](https://dazzle.jelia.nyc/privacy/).

## Build it

```sh
git clone https://github.com/LIBCSYS/JellyDazzle.git
cd JellyDazzle
make          # build ./jellydazzle
make run      # build and launch
make app      # self-contained JellyDazzle.app
```

Needs only the Xcode command line tools (`xcode-select --install`).

**SDL2 ships in this repository**, built against the minimum supported macOS. That is
deliberate: a Homebrew SDL is compiled for whichever macOS the build machine happens to run
and silently pins the finished app to it — which is exactly what once made a public download
refuse to launch on anything but the newest system.

⚠️ **First launch can take 30–60 seconds** while it measures the library and probes audio
devices. It is working, not hung. The measurements are cached, so every later launch is
immediate.

## Installing a downloaded build

Current downloads are ad-hoc signed, so macOS warns that it cannot verify the app. Nothing
is wrong with it — it has not been through Apple's signing and notarisation yet. Apple
Developer enrolment under **LIBCSYSTEMS LLC** is in progress and the next release will be
signed and notarised, opening with no warning. Until then: right-click the app → **Open** →
**Open**.

## Layout

```
src/engine/    compositor.c    scheduler + layer compositor
               routines_asm.s  24 ARM64 assembly routines
               jellydazzle.h   the plug-in contract
src/audio/     listen.c        bass / mid / treble / beat
               systap.m        Core Audio system-output tap
src/app/       main.c          SDL window, native-resolution render loop
src/patterns/  NNN_name.c      pattern plug-ins, named by what they draw
assets/        palette.bin, sintab.bin, palettes/ (sources)
packaging/     JellyDazzle.entitlements (App Store sandbox)
tools/         gen_palettes.py, gen_registry.sh, build_app.sh,
               release_app.sh, build_appstore.sh
```

## In tribute

JellyDazzle exists because of **DAZZLE.EXE**, written by **James R. Shiflett** of Houston,
Texas — at night, "a sort of therapy," as he called it. MicroTronics released it as shareware
in 1990. This is an homage, not affiliated with the original.
[The full story](https://dazzle.jelia.nyc/tribute/).

## Support

Questions, bugs, or anything else: **support@libcsys.com** ·
[support page](https://dazzle.jelia.nyc/support/)

---

MIT licensed. Built by John Elia / LIBCSYSTEMS LLC.
