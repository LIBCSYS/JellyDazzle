# JellyDazzle

**An audio-reactive kaleidoscope for Apple Silicon.** It listens to whatever is playing —
on your Mac, iPhone or iPad — and paints to it, never the same pattern, never the same colours.

### 🎉 Version 3.5.9 is out on the Mac — **creatures and fractals**

Eighteen animals now swim, fly, crawl and prowl through the show — jellyfish, a great whale, a
school of fish, manta rays, an octopus, seahorses, a cat, a dog, beetles, an ant trail, a starling
murmuration and more — plus ten new fractals. **638 routines** in all.
**[Free on the App Store](https://apps.apple.com/us/app/jellydazzle/id6808884433)** — Mac 3.5.9
released October 10, 2026. [iPhone and iPad](#iphone-and-ipad) are on 3.5.8, with 3.5.9 in App Review.

An homage to **DAZZLE.EXE**, rebuilt from scratch — mostly C, with an ARM64 assembly core:
24 engine modes hand-written in ARM64 assembly under a library of 614 C pattern plug-ins,
layered by a compositor written in C.

## ⬇️ Download

### **[Get JellyDazzle on the App Store — Mac 3.5.9, iPhone and iPad 3.5.8](https://apps.apple.com/us/app/jellydazzle/id6808884433)**

Free, and it updates itself. This is the current version.

**Not using the App Store?** Signed, notarised direct download — always the latest version:
[Download JellyDazzle for macOS · Apple Silicon (.zip)](https://github.com/LIBCSYS/JellyDazzle/releases/latest/download/JellyDazzle-macOS-arm64.zip)

1. Unzip.
2. Drag **JellyDazzle.app** into **Applications**.
3. Open it. That is the whole install.

Code-signed as **`Developer ID Application: LIBCSYSTEMS LLC`** and **notarised by Apple**,
with the ticket stapled so it verifies offline. No "cannot verify" dialog, no right-click
trick — download it and open it.

**macOS 11 or later · Apple Silicon (M1+) · MIT**

> **Which one do I want?** The **App Store** carries **3.5.9 for Mac** (iPhone and iPad 3.5.8,
> 3.5.9 in review) and updates itself. The direct download above is Mac 3.5.9 too, signed and
> notarised, for Macs that do not use the App Store.

> ⚠️ **First launch can take 30–60 seconds** while it measures the routine library and
> probes audio devices. It is working, not hung. The measurements are cached, so every
> later launch is immediate.

> ### 🔊 Play music first
> **JellyDazzle is audio-reactive — put something on before you watch it.** Anything playing on
> your Mac: a record, a film, a livestream. On macOS 14.2+ it listens to the *system output*, so
> it follows whatever app is making the sound. In silence it still runs, but on internal clocks —
> and you are seeing maybe half of what it does.
>
> **Nothing on hand?** Play mine — *And I Need Ur Love* by
> [The Rat's Asses](https://theratsasses.com/), featuring the guy who wrote this thing.
> **[▶ Watch on YouTube](https://www.youtube.com/watch?v=u5IyinH_3s4)** — free, no account.
> Also on [Spotify](https://open.spotify.com/track/3wRjyj2LlYPMx3cziUhb9y), though free
> accounts only get a 30-second preview there.

More at [dazzle.jelia.nyc](https://dazzle.jelia.nyc/) ·
[browse every routine](https://dazzle.jelia.nyc/library/)

## iPhone and iPad

**Version 3.5.8 is on the App Store now.** It is the same engine — the same ARM64 assembly core and
all 634 routines — running on iOS and iPadOS 15 or later.

- **Your music keeps playing.** iOS does not let one app hear another, so on a phone
  JellyDazzle listens through the microphone. Spotify or Apple Music keeps playing at full
  volume while it listens and paints.
- **Lite on battery, Full when charging.** On battery, in Low Power Mode, or when the device
  is warm it draws a softer picture at 30 frames a second; plugged in it runs at 60 with every
  layer. It switches on its own, mid-show.
- **Buttons.** **C**, **S** and **?** sit in the corner for colours, shapes and help, brighten
  when you touch the screen and fade while you watch. Tap = colours, two fingers = shapes,
  press and hold = help, three fingers = a performance readout.

The iOS port and the 3.5 changes land in this repository with the release.

---

## What the built app contains

| | |
|---|---|
| **24** | ARM64 assembly engine modes |
| **614** | C pattern plug-ins |
| **638** | routines total |
| **180** | palette schemes, interpolated in OKLab |

Every routine in the tree is compiled into the shipping binary. Nothing is held back.

The engine draws once and then moves the palette, the way the original did. Patterns are
scheduled into layers by how much they move on their own, so the stillest ones get the most
transform and nothing sits static. Two routines from the same shape family never share the
screen.

## Controls

| key | |
|---|---|
| **C** | **new colours.** The whole palette cycles to a different scheme — not the next one, the one furthest from what is on screen. |
| **S** | **new shapes.** Every routine currently drawing is retired and the stack fills again from the library. Colour is left alone. |
| **A** · **?** · **H** | help / about card — version, links, what is playing right now. Also **Help ▸ JellyDazzle Help (⌘?)** in the menu bar. |
| **M** | audio meter |
| **F** | full screen |
| **ESC** | quit |
| buttons | from 3.5.8, **C · S · ? · X** in the bottom-right corner do the same with a click |

**C** and **S** are the 3.0 feature and they split the show in two: one key changes what
it looks like, the other changes what it is doing. Either can be pressed at any time.

**C picks at random inside the extreme band.** It scores every scheme in the 180-strong
library against what is on screen, keeps the ones within 70% of the furthest, and chooses
among those — never repeating the scheme it just left. Taking the single furthest every time
looked right on paper but was deterministic, and colour distance is symmetric: from A the
furthest was B and from B the furthest was A again, so it toggled between two palettes and
the other 178 never appeared.

There is no F1 on macOS — the top row is display brightness — so help is on **⌘?** from the
Help menu, alongside the **A**, **?** and **H** keys.

Neither of them cuts. The engine has one law — nothing strobes — and a key press is not an
exemption from it: **C** crossfades to the new palette over two thirds of a second, and
**S** fades the old routines out under the same envelope that retires them normally. The
release gate measures this (`gate keys`): across seven starting seeds the worst
single-frame change after either press stayed inside the range the show already produces
on its own, and the whole cast turned over.

## Audio

On macOS 14.2+ JellyDazzle taps the **system audio output**, so it moves with whatever is
already playing — Spotify, a browser, anything. On earlier versions it falls back to the
microphone, which is why macOS asks for permission on first launch.

Audio is turned into numbers and thrown away. It is analysed in memory to drive the current
frame and then discarded — never recorded, never stored, never transmitted. The app has no
network capability at all. Without permission the visuals simply run on internal clocks.
See the [privacy policy](https://dazzle.jelia.nyc/privacy/).

## Build it yourself

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
[support page](https://support.dazzle.jelia.nyc/)

---

MIT licensed. Built by John Elia / LIBCSYSTEMS LLC.
