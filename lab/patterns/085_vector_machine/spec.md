# 085 Vector Machine

## Look
A bold CGA-poster machine on black (the c01–c05 "H-frame" reference): giant red concentric
arc stacks open left and right like parentheses, a blue-to-magenta gradient "H" of panels
sits center, red struts form an X lattice behind, and a white diamond breathes at dead center.

## Math
- Struts: distance to lines `y = tan(α)x` for α ∈ {±0.55, ±1.05}, intensity 1 − d/2.
- Arc stacks: centers at x = ±175; rings `½+½·sin(0.26·r − 0.010·t·sx)` masked to
  55 < r < 170 and the inward half-plane — the two stacks creep in opposite phase.
- H bars: |x∓40| < 20, |y| < 72 with rolling ramp `g = frac((y+70)/140 + 0.004·t)`;
  crossbar |y| < 13, |x| < 40 with horizontal ramp.
- Center diamond: manhattan |x|+|y| vs radius 14 + 3·sin(0.02·t).

## Integer ARM64 plan
- Struts: incremental line rasterization — per scanline the strut center x moves by a
  fixed 8.8 delta; paint a fixed-width span (no per-pixel distance).
- Arcs: octagonal-norm r from the two centers (only inside their half-screen boxes),
  ring value from the 16-bit sine table at (r·k − t·k2); mask via two compares.
- Bars/crossbar: pure rectangle fills with a per-row ramp index (add per row, not per
  pixel); ramp rolls by bumping a start index each frame — classic copper-bar tech.
- Diamond: |x|+|y| via adds; band + fill via compares. Whole frame is spans + LUTs.

## Palette pairing
Deliberately restricted duotone: pure reds vs blue→magenta ramp + white accents on
black — the palette-discipline routine of the set (contrast against the rainbow
siblings). DAC cycling rolls only the magenta ramp.

## Motion
Arc rings creep inward/outward at 0.010 phase/frame in mirrored phase; bar gradients
roll at 0.004/frame (full cycle ~4 s, smooth not strobing); center diamond breathes on a
5 s period. Mechanical, slow, deliberate.
