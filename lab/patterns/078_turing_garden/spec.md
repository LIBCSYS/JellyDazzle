# 078 Turing Garden

## Look
Bold reaction-diffusion colonies — solid vermilion continents on cobalt seas, edged everywhere with crawling gold rims — slowly deform, split and merge like living coral viewed from above. The pattern matures from soft blobs into crisp labyrinth lobes as it "grows".

## Math
- Seed field: sum of 4 drifting sine gratings + a finer product grating, phases ∝ 0.006t.
- Turing fake (research §3, fake #1): iterate `f ← tanh(2.35·blur3(f))` k times; k ramps 10→28 with t (pattern sharpens = growth). Fixed point = ±1 domains with smooth moving boundaries.
- Color: duotone lerp between hueA(t) and hueA+0.42 by `u=(f+1)/2`; rim = `(|∂x u|+|∂y u|)^1.5` × slow pulse, added as gold; mild radial vignette.

## Integer ARM64 plan
- Field in i16 Q12 at half-res (160×120), upscale 2×. blur3 = 5-tap shift-add. tanh → 1024-entry LUT (or clamp-cubic `x(3−x²)/2` in Q12, two muls).
- k iterations per frame is the only cost: 28 passes × 19K cells ≈ 0.5M cell-ops — fine; or keep the field persistent across frames and run 1 iteration/frame with slowly drifting seed injection (true accumulate, 28× cheaper, same look).
- Rim from one-pixel differences (abs-add), gold add saturating.
- Duotone + rim via a 2D palette LUT `pal[u8_u][u8_rim]` — zero per-pixel HSV math.

## Palette pairing
Complementary pair rotating slowly together: hueA 0.44–0.60 (teal/blue) vs hueA+0.42 (vermilion/magenta), S 0.85–0.9; rims fixed warm gold `(255,217,90)` — the constant that keeps every scheme cohesive.

## Motion
Domain boundaries creep 1–2 px/s as seed phases drift; the palette pair rotates one wheel in ~35 min; rim brightness swells ±25% at 5 s. No frame-to-frame jumps — the map is a continuous function of t.
