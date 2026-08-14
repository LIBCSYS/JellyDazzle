# 052 Orbit Weave

## Look
Seven nested electron ellipses (the c06 atom, grown up) precess at different rates so
their comet-bright heads weave a rotating rosette; each orbit keeps a faint dotted
ghost ring and twin heads on opposite sides. Cyan→blue→magenta family over near-black
with dim star dust and a pulsing gold nucleus.

## Math
Orbit i: semi-axes `A_i = 128-9i`, `B_i = 26+11i`, orientation `θ_i = i·π/7 + prec_i·t`
(`prec_i = 0.0016+0.00045i`). Head phase `φ_i = ω_i t` (`ω_i = 0.02+0.004i`); trail =
arc `[φ_i-2.6, φ_i]` sampled 230×, brightness ramping to the head, head desaturated
to white. Point: rotate `(A cos s, B sin s)` by θ, squash y×0.92, add center.

## Integer ARM64 plan
16-bit sine table drives everything: `cos s`, `sin s`, and the θ rotation are three
table lookups + two 16×16→32 multiplies per point (fixed 8.8). Trail = ring buffer of
the last 230 plotted points per orbit head — plot new head, dim old entries by palette
index decrement (colors allocated as per-orbit 16-step ramps), zero trig for the tail.
Ghost ring pre-plotted once per precession step. No divides anywhere.

## Palette pairing
Hues locked to 0.52–1.05 band (cyan→violet→rose) + gold nucleus ramp; whole band
slow-rotates (~1 cycle / 45 s) via DAC writes.

## Motion
Heads sweep an orbit in ~5 s; precession turns the whole flower once in ~1 min;
nucleus rings counter-rotate gently. Constant smooth glide, zero flicker.
