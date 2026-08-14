# 055 Comet Carousel

## Look
Three comets on breathing circular orbits, each stamped through a 6-fold rotational
kaleidoscope: eighteen white-hot heads towing rainbow-shifting tails braid into a
slowly turning pinwheel over a blue-violet core glow. Pure flowing curves, no straight
edges.

## Math
Comet c: angle `θ = ω_c t + φ_c` (`ω_c = 0.016+0.005c`), radius
`r = 52+12c + 22 sin(0.006t + 1.7c)`. Base point `(r cosθ, 0.85 r sinθ)`. Tail =
positions at `t-j`, j = 0..149, weight `(1-j/150)^1.6`; hue drifts +0.09 along the
tail, head desaturates to white. Stamp 6 rotations by `2πm/6 + 0.0021t`, squash
y×0.92.

## Integer ARM64 plan
One sine table serves θ, the radius breathing, and the 6 stamp rotations. Only 3 new
points are computed per frame; each is pushed into a per-comet ring buffer of 150
screen-space base coords. The 6-fold stamp rotates BASE coords with two multiplies
per point using the table — 18 plots/frame plus ring-buffer redraw (or: plot into an
accumulating indexed buffer and let per-index palette decay do the tail fade, zero
redraw). Radius modulation is an add of a second, slower table walk.

## Palette pairing
Three anchors ~120° apart (e.g. coral / spring green / azure) each with a 16-step
tail ramp hue-shifted toward its neighbor; global rotation ~1 cycle / 28 s on a
blue-violet vignette ground.

## Motion
Carousel turns once ~50 s; comets lap every 6–11 s; orbits breathe in/out over ~17 s.
Three incommensurate periods → never repeats, always smooth.
