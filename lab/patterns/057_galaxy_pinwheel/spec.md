# 057 Galaxy Pinwheel

## Look
A two-armed spiral galaxy of 3000 stars seen at a tilt: molten gold-white core melting
outward through rose to blue-violet arm tips, turning with true differential rotation
(inner stars lap the outer ones) while the whole disc slowly yaws. A few foreground
cross-sparkle stars twinkle over it.

## Math
Star: radius `r = 132·u^0.72`, arm angle `θ0 = armπ + gauss·(0.35+r/90)`, spiral twist
`2.5 ln(1+r/13)`. At time t: `θ = θ0 + twist + 0.0065t/(0.35+r/60)` (differential),
disc point `(r cosθ, 0.62 r sinθ)` rotated by yaw `0.00055t`. Hue
`0.62 - 0.52 e^{-r/42}` (gold→violet), value `∝ e^{-r/75}` with per-star twinkle.

## Integer ARM64 plan
All per-star constants (r, θ0+twist, hue index, twinkle phase) precomputed at init.
Per frame each star needs `θ0 + ωr·t` where `ωr` is a per-star fixed-point angular
rate (also precomputed — differential rotation becomes one 16-bit add per star per
frame into a wrapping angle accumulator). Then 2 sine lookups + the shared yaw
rotation (2 more lookups, factored outside as a combined rotation matrix in 2.14).
3000 stars ≈ 12k multiplies/frame. Twinkle = per-star palette-index wobble from a
coarse sine table. Core halo is a static radial bitmap with breathing DAC entries.

## Palette pairing
Single 32-step radial ramp gold→amber→rose→violet→deep blue; star index chosen by
radius at init. Ramp hue-shifts very slowly (~1 cycle / 70 s). Space is near-black
blue, never grey.

## Motion
Inner arms complete a turn ~35 s, rim ~2 min — the arms visibly wind and shear but
slowly; yaw drifts the whole disc; twinkle is gentle (0.05 rad/frame). Deep-space calm.
