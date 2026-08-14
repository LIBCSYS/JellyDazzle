# 060 Meteor Veil

## Look
A night sky of 300 tinted twinkling stars over a teal horizon glow; slow meteors
drift down in mirrored left/right pairs, each towing a long pastel ion trail that
keeps glowing after the head passes and evaporates over a few seconds — a curtain of
crossing glowing X arcs that never fully repeats.

## Math
Meteor k spawns at `t_k` (gaps 34–62 f) at `(x0, y0)` near the top: head
`x = x0 + dx·a`, `y = y0 + vy·a + ½·0.0035·a²` (gravity droop bends the streak).
Trail point laid at age a has been visible `ago = age - a` frames; its weight is
`e^{-ago/110}·(0.25+0.75 a/a_max)` — the trail fades from the tail end first, like a
real persistent train. Every meteor is stamped twice: `(x,y)` and `(W-1-x, y)`.

## Integer ARM64 plan
Head integration: `x += dx; y += vy; vy += g` in 8.8 fixed — three adds/frame/meteor.
Trail persistence WITHOUT per-point timestamps: plot the head into the accumulating
indexed buffer using a per-meteor 12-color ramp, and every 8 frames run a palette
decay pass that steps each trail ramp entry one notch darker (DAC-only fade, pixels
untouched). Mirror = second plot at `W-1-x`. Star twinkle = 4 shared palette entries
wobbled by a coarse sine table. Zero multiplies in steady state.

## Palette pairing
Pastel meteor hues (any hue, sat ~0.55 — rose, mint, ice blue, gold) over an
indigo→teal vertical ramp sky; stars in blue-white/gold tints. Soft, nocturne feel —
the calm closer of the set.

## Motion
A meteor crosses in ~6 s; ~2–3 visible at once with their mirror twins; trails decay
over ~4 s; stars breathe at 0.03 rad/frame. The slowest, quietest pattern of 051–060.
