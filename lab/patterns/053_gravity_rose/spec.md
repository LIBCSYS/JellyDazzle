# 053 Gravity Rose

## Look
Eight comet particles ride precessing Kepler ellipses around a glowing gold core, each
dragging a long fading tail; stamped through the 4-fold mirror the tails braid into a
breathing rose of rainbow ribbons on deep navy. Reads as a slow gravitational ballet.

## Math
Particle p: angle `φ = ω_p t + φ0_p` (`ω_p = 0.011+0.0016p`), apsis precession
`ψ = prec_p·t`. Radius from the conic: `r = a_p(1-e²)/(1+e·cos(φ-ψ))`, `e = 0.38`,
`a_p = 40+6.5p`. Position `(0.98 r cosφ, 0.80 r sinφ)` from center, then mirrored
(±x, ±y). Trail = positions at `t - 1.35j`, j = 0..179, weight `(1-j/180)^1.5`,
head desaturated toward white.

## Integer ARM64 plan
Sine table for cos/sin of φ and φ-ψ. The conic divide is avoided with a 256-entry
reciprocal table for `1/(1+e·cos)` in 1.15 fixed point (e·cos spans a fixed range) —
one lookup + one multiply. Trails are ring buffers of past plotted screen coords per
particle (720 entries total); mirror = two negations, no recompute. Tail fade is a
palette ramp walk (each particle owns a 16-color hue→black ramp), so drawing a frame
touches only 8 new points + ring-buffer redraw, all integer.

## Palette pairing
8 evenly spaced rainbow ramps (one per particle) + gold-white core ramp on navy
ground; all hues drift together ~1 revolution / 40 s.

## Motion
Petals sweep once ~9 s; the whole rose precesses slowly so petal crossings migrate;
tails lengthen/contract with eccentric speed changes. Silky and continuous.
