# 014 Copper Octarings

## Look
Amiga copper bars promoted from scanlines to radius: six neon gradient bars ride independent sine paths in and out as crisp concentric octagon rings on a midnight-blue field, layering additively where they cross. The octagon frame itself rotates almost imperceptibly.

## Math
- Rotated coords: `(rx,ry) = R(0.003t)·(dx,dy)`.
- Octagonal norm: `r8 = max(|rx|, |ry|, (|rx|+|ry|)/√2)` — rings are octagons, no sqrt.
- Six bars: ring k at `pos_k = 96 + 76·sin(ω_k·t + φ_k)` with ω ∈ [0.0058, 0.0092]; profile `wgt = clip(1 − |r8 − pos_k|/17)^1.5`; each bar adds its own RGB, saturating-clipped.
- Backdrop: `blue = 30 + 16·sin(0.02·r8 − 0.01t)`.

## Integer ARM64 plan
- This is the copper-bar architecture verbatim: per frame build a **256-entry color line** `colorline[radius]` (clear, then for each bar add its gradient bytes at `pos_k` from `sin_tab` — per-*ring* work, ~6×34 byte ops with a 512-entry saturation clamp LUT, essentially free).
- Screen fill: `pix = colorline[oct_tab[i]]` — one table read per pixel. `oct_tab` is the per-pixel octagonal-norm byte, precomputed at init with `max`/`add`/`shift` only (the √2 factor is `(ax+ay)·181 >> 8`).
- Slow octagon rotation: precompute 2–4 phase-shifted `oct_tab` variants (or one table on a slightly larger buffer sampled through a rotating window) and crossfade indices — avoids per-pixel rotate math entirely.
- Saturating adds are NEON-native (`uqadd.16b`) — the whole compose vectorizes 16 pixels per instruction.

## Palette pairing
Six fixed bar hues — coral red, gold, spring green, sky blue, orchid, warm white — over a near-black midnight blue. Additive crossings produce secondary pastels automatically, so schemes recombine as bars pass through each other.

## Motion
Each ring sweeps center↔edge on its own sine period (12–18 s), all periods incommensurate. Crossings and momentary rainbow stacks happen a few times a minute; no ring ever moves faster than ~2 px/frame. The frame rotation completes an octagon-eighth in ~4 minutes.
