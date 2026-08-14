# 095 Ring Machine

## Look
A bold CGA-poster machine: two stacks of red concentric arcs bow outward left and right like giant parentheses, framing an "H" of blue→magenta gradient rectangles with a pulsing red crossbar and a white diamond at dead center; a red X-lattice of struts and dim green scanlines sit behind everything. Replica of R7 (frames c01–c05), restricted red/blue/white palette included.

## Math
- Arc stacks: ring fields `sin(r_c·0.55 − t·0.02) > 0.15` around centers (108,120)/(212,120), annulus 26<r<108, masked to the outward side (`(x−cx)·dir > 8`) so only "("/")" arcs survive; shading = phase-offset sine of the same argument (gradient across each ring).
- H rectangles: vertical ramp `u`, color `sin(2.4u + 0.8·sin(0.008t))` lerping blue→magenta (the roll makes the shading breathe).
- Struts: distance-to-line `| |dy| − a·|dx| |/√(1+a²) < 1.4` for slopes a ∈ {0.28,0.55,0.85} on mirrored coords → symmetric X lattice.
- Crossbar `|dy|<11` pulsing; diamond `|dx|+|dy| < 13` red, `< 6` white; scanlines `y mod 4 == 0` dim green with slow x-phase shimmer.

## Integer ARM64 plan
Ring distance via octagonal norm (`max + 3/8·min` of |x−cx|,|y−cy|) — good enough for chunky VGA arcs — then one 16-bit sine-table lookup per pixel for mask AND shade (same phase, two table reads). Strut test = multiply-free with slopes chosen as 9/32, 9/16, 27/32 (shift-add), compare against fixed threshold. Rectangles are scanline fills: per-row color computed once from the sine table, then a `memset`-style row blit — per-pixel cost zero. Scanline green rows likewise per-row. Everything else is compares on |dx|,|dy| in registers.

## Palette pairing
Deliberately restricted, as in c01–c05: pure reds (arcs, struts, bar), a blue→magenta ramp (H rectangles), white highlights, whisper of green scanlines. The discipline of NOT using rainbow here is the faithful move.

## Motion
Mechanical and slow: arc phase creeps at 0.02 rad/frame (rings appear to rotate through the stack), rectangle shading rolls on a ~780-frame sine, crossbar pulses gently, scanlines shimmer. The composition itself never moves — it feels like a machine idling.
