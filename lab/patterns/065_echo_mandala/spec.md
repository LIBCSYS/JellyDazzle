# 065 Echo Mandala

## Look
The camera-pointed-at-its-own-monitor effect, stateless: a 5-petal flower is echoed at eight nested scales, each echo rotated half a radian deeper and dimmer, and the stack dives continuously so fresh echoes bloom out of the center and the outermost drift off-screen forever. Emerald, gold, ruby and violet interlock like cloisonné.

## Math
- dive phase `z = t*0.006`, `frac = z mod 1`; echo depth `m = k - frac`, k = 0..7
- per echo: scale `= 1.9^m * 0.021`, rotation `= m*0.5 + t*0.0012`
- stamp `g = cos(6 r') * cos(5 a' + 1.5 r')` in echo-local polar coords
- weight `= 0.62^m * clip(m+1, 0, 1)` (newborn echo fades in → seamless octave shift)
- `field = sum(g*w)/sum(w)` → `idx = field*120 + 128` → jewel palette
- All quantities depend only on `m`, so the zoom is perfectly periodic-per-octave = infinite.

## Integer ARM64 plan
- This is true feedback zoom (§12 demoscene notes) done the real way at runtime: keep TWO framebuffers; per frame resample old→new with the rotozoom inner loop (16.16 du/dv, s≈0.985, dθ≈0.002 — constants per frame), remap each fetched byte through a 256-byte decay/hue-shift table, then stamp ONE flower ring (few hundred plotted pixels via sin16) at the center.
- The 8-octave closed form above is just the proto's stateless stand-in; the ARM64 build gets the identical look from the 2-buffer loop at rotozoom cost: per pixel = 2 adds (u+=du, v+=dv), 1 fetch, 1 remap fetch, 1 store. No trig, no div.
- Stamp plotting: 5-petal polar curve from sin16, radius in 8.8 fixed point.

## Palette pairing
Jewel loop (midnight→emerald→gold→ruby→violet). The decay remap in the ARM64 version steps each pixel one palette notch per generation, so trails hue-rotate as they age — echoes are literally older colors.

## Motion
Dive rate 0.006 octave/frame → one full echo generation every ~5.5 s; global spin one turn in ~90 s. Echoes emerge from the center as slow blooms; nothing pops because newborn echoes fade in over a full second.
