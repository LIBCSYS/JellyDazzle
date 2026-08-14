# Generative-art techniques → integer-math 2D screensaver patterns

Research notes for dazzle64. Framing assumptions throughout:

- **Machine model:** per-pixel asm loop over an 800×600 (or half-res 400×300 upscaled) buffer, integer/fixed-point only (Q8–Q16), 256-entry Q14 `sintab`, 32768-entry palette LUT, optional persistent side buffers.
- **Two rendering families:**
  - **Repaint** — every frame recomputes every pixel from a formula `f(x, y, t)`. Stateless, resolution-independent, trivially restartable. Plasma-family.
  - **Accumulate** — a persistent state buffer (particle trails, CA grid, chemical field) evolves frame to frame. Richer, more "alive," but needs init/reset logic and can die or saturate.
- Palette cycling is free extra motion for any repaint technique: keep the field static-ish and rotate the palette index offset.

---

## 1. Flow fields (particles advected by a vector field)

**What it looks like in motion.** Thousands of particles drifting along invisible currents — like iron filings, silk threads, or wind maps. With fading trails it becomes flowing hair / river-delta imagery. The gold standard of "modern generative art" (Tyler Hobbs et al.).

**Cheap integer version.**
- Vector field = angle per cell: `angle = f(x>>4, y>>4, t)` where `f` sums 2–3 `sintab` terms (i.e., a plasma *is* the angle field). No Perlin needed — sine-sum fields curve beautifully.
- Store field as a coarse grid (e.g., 50×38 cells of `u8` angle) recomputed once per frame — negligible cost.
- Particles: `x, y` in Q8 fixed point. Step: `a = field[cell]; x += costab[a] >> k; y += sintab[a] >> k`. Plot a pixel (or 2×2 blob) at `(x>>8, y>>8)`.
- Respawn particles that exit the screen or sit in a sink (random reseed keeps density even).
- 5–10K particles is nothing at 60 fps even in scalar asm.

**Repaint or accumulate?** **Accumulate.** Draw particles into a persistent buffer; each frame multiply the whole buffer by ~0.96 (integer: `v -= v>>5`) for fading comet trails. Color = palette[trail intensity] or palette[particle age].

**Verdict: top pick.** Cheapest spectacular technique on the list; the field generator reuses the existing sintab/plasma machinery directly.

---

## 2. Curl noise (table-based)

**What it looks like in motion.** Flow field, but the flow is *divergence-free* — no sources or sinks, so particles never bunch up or drain away. Looks like smoke, ink in water, turbulent swirls that conserve density forever. Time-varying field gives roiling fluid without any fluid solver.

**Cheap integer version.**
- Curl of scalar potential ψ: `vx = dψ/dy, vy = -dψ/dx`. With a table-based ψ this is just finite differences: `vx = ψ[x][y+1] - ψ[x][y-1]; vy = ψ[x-1][y] - ψ[x+1][y]`.
- ψ = the same sine-sum plasma field (coarse grid, i16, recomputed per frame with a slow `t`). Finite differences of a smooth integer field are perfectly adequate — no gradients of real Perlin noise required.
- Scale velocities into Q8 and advect particles exactly as in §1.
- Animate by drifting the plasma phases; add a second ψ octave (finer grid, smaller amplitude) for turbulence.

**Repaint or accumulate?** **Accumulate** (particles + fading trails). The divergence-free property is exactly what keeps a screensaver alive for hours — density stays uniform, nothing collapses into attractors.

**Verdict: the "pro" upgrade of §1** for ~20 extra instructions per particle. Do §1 first, switch the field to curl-of-ψ, and observe that it stops forming clumps.

---

## 3. Reaction–diffusion (Gray–Scott) — and cheap fakes

**What it looks like in motion.** Living coral / leopard spots / fingerprint labyrinths that grow, split, and merge. Spots divide like cells; stripes crawl. Genuinely mesmerizing, genuinely expensive.

**Real thing, integer version.**
- Two fields U, V per cell (Q12 in i16 works; classic demos have done it in 8.8). Per cell per step: a 3×3 Laplacian on each field, one multiply `U·V²`, feed/kill terms. ~20 ops × 2 fields × N cells, and it needs **several iterations per frame** to move at watchable speed.
- Feasible at 200×150 with 2–4 iterations/frame; a stress test at full res. Parameter choice is touchy — many (f, k) pairs just die or saturate. Needs auto-reseed watchdog (if variance of V collapses, splat new seeds).

**Cheap fakes (often good enough on a screensaver wall):**
1. **Blur + sharpen + threshold loop:** buffer → box blur (two-pass, integer) → contrast stretch around 128 (`v = clamp(128 + ((v-128)*3)>>1)`) → repeat. Converges to RD-style labyrinth stripes from any noisy seed. One field, no multiplies beyond a shift-add. This is the classic demoscene "Turing pattern" cheat.
2. **Cyclic CA (§4)** gives RD-like spirals for a fraction of the cost.
3. **Static fake:** thresholded sum of ~8 random-direction sine gratings looks like frozen RD stripes; animate phases for crawling stripes. Pure repaint, zero state.

**Repaint or accumulate?** Real RD and fake #1: **accumulate** (the pattern *is* the state). Fake #3: **repaint**.

**Verdict:** skip real Gray–Scott initially; fake #1 delivers 80% of the look with ~10% of the cost and only one byte-per-cell of state.

---

## 4. Cyclic cellular automata (CCA)

**What it looks like in motion.** Starts as colored static → curdles into blobs → blobs organize into rotating spirals and expanding "demon" wavefronts that eat each other forever. Self-organizing, never repeats exactly, runs indefinitely — screensaver-perfect.

**Cheap integer version.**
- Grid of `u8` states 0..N-1 (N = 12–16 states maps directly onto a palette ramp).
- Rule per cell: if any of 4 (von Neumann) or 8 (Moore) neighbors has state `(mine+1) mod N`, adopt that state. That is: a few compares and a conditional store. **No multiplies at all.** The cheapest technique on this list.
- Double-buffer the grid. Half-res grid (400×300), each cell drawn as 2×2 pixels, keeps it fast and chunky-retro.
- Variants: extended range-2 neighborhoods and thresholds ("Cyclic Demons") give bigger, slower, more majestic spirals.
- Endgame handling: CCA converges to stable spiral cores after a few minutes — fine to leave, or perturb with occasional random splats.

**Repaint or accumulate?** **Accumulate** (the grid is the state). Map state → palette index; palette-cycle on top for extra shimmer.

**Verdict: best effort-to-payoff ratio** of any accumulate technique. An afternoon of asm, guaranteed hypnotic.

---

## 5. Belousov–Zhabotinsky spirals (excitable media)

**What it looks like in motion.** Chemical-oscillator look: expanding target rings and pinwheeling spirals with sharp wavefronts, like the famous petri-dish films. Similar family to CCA spirals but smoother and more organic.

**Cheap integer version — two routes:**
1. **3-state excitable CA (Greenberg–Hastings):** cells are resting / excited / refractory (with `u8` timers). Resting cell fires if ≥ T excited neighbors; excited → refractory → counts down → resting. Compare-and-store only, CCA-cost. Classic spirals from a single broken-wave seed.
2. **Continuous "hodgepodge machine" / averaged BZ:** cell value `u8`, per step: neighbor average (sum of 8, shift) plus excitation increment, wrap at 255. One add-heavy pass, no multiplies. Gives the smooth-shaded BZ film look rather than hard CA bands.
- Route 2's wrap-at-255 maps perfectly onto a cyclic palette — the wrap *is* the wavefront.

**Repaint or accumulate?** **Accumulate.**

**Verdict:** implement as a rule-variant of the §4 engine (same grid, same double-buffer, different inner loop). Two screensaver modes for one infrastructure.

---

## 6. Wave interference

**What it looks like in motion.** Overlapping ripple rings from several moving sources — moiré-like breathing interference lattices. The classic "ripple plasma."

**Cheap integer version.**
- Per pixel: `v = Σ sintab[(dist_i(x,y) × freq + t·speed_i) & 255]` over 3–5 sources.
- Distance without sqrt:
  - **Precomputed radius LUT:** one quarter-plane `u8 dist[400×300]` table (index by |dx|,|dy|), built once at init (or shipped like `shapes.bin`). Per-source per-pixel cost collapses to one table read + one sintab read.
  - Or octagonal approximation `max + (min>>1) - (min>>3)` (≈2% error, invisible here).
- Move the sources along Lissajous paths (sintab again). Interference pattern churn comes free.
- Layer on top: the existing SDF `shapes.bin` can *be* a source — waves radiating from a heart outline (`sintab[(sdf(x,y)×k + t) & 255]`) is a signature dazzle move.

**Repaint or accumulate?** **Repaint** — pure `f(x,y,t)`. Alternatively the accumulate version (2-buffer water ripple: `new = ((sum4 neighbors)>>1) - old`, damped) gives *impulse-driven* rain-on-water instead; both are cheap, and they're different effects worth having as separate modes.

**Verdict:** lowest-risk item; a direct sibling of the current plasma loop.

---

## 7. Domain warping

**What it looks like in motion.** Take any pattern `p(x,y)` and evaluate `p(x + d1(x,y,t), y + d2(x,y,t))` — the pattern melts, smears, and folds like marbled ink or heat shimmer. Iterated twice (warp the warp) it produces the flowing organic sheets Inigo Quilez popularized. Motion = drifting the warp phases: the whole image kneads itself.

**Cheap integer version.**
- `d1, d2` = small sine-sums: `d1 = (sintab[(x·a + t) & 255] + sintab[(y·b - t) & 255]) >> k` — displacement in pixels (keep |d| ≤ 32).
- Base pattern `p`: XOR/AND of coordinates, checkerboard, radial rings, the SDF shape tables, or another plasma. Even a boring base becomes organic after one warp; two warp layers ≈ liquid marble.
- All Q8 math; total ~6 sintab lookups per pixel for a double warp. Compute at half res and 2× upscale if needed.
- Bonus: warping the *coordinates fed into the kaleidoscope fold* melts the kaleidoscope itself.

**Repaint or accumulate?** **Repaint.** (Accumulate variant: warp last frame's buffer instead of a formula — gives infinite feedback-smear, the classic demoscene "rotozoom feedback tunnel." Watch out for value drift/saturation; add slight decay.)

**Verdict: highest visual payoff per instruction for the repaint family.** Should be an early add — it upgrades every existing pattern including the kaleidoscope.

---

## 8. Voronoi / Worley cells

**What it looks like in motion.** Cellular / cracked-mud / stained-glass / bubble-foam partitions. Animated by moving the seed points: cells slide, borders slither, cells swallow each other. Worley F2−F1 gives glowing cell walls.

**Integer feasible? Yes, three ways, cheapest first:**
1. **Grid-bucketed seeds (standard Worley):** drop one jittered seed per 32×32 cell block; per pixel check the 3×3 neighboring blocks' seeds (9 candidates), track min and 2nd-min **squared** distance (`dx² + dy²`, i32 — no sqrt ever). Color by `F1` (bubbles), seed id (flat stained-glass), or `F2−F1` (walls). ~9 mul-adds per pixel; multiplies are cheap on Apple Silicon. Fully repaint, seeds move on Lissajous paths.
2. **Manhattan/Chebyshev metric:** replaces the squares with abs/max — diamond- or square-shaped cells, distinctly crystalline look, even cheaper.
3. **Jump Flooding Algorithm:** full-screen nearest-seed map in log(n) buffer passes — overkill here, only needed for hundreds of free-roaming seeds.

**Repaint or accumulate?** **Repaint** (method 1–2). Palette-map F1 distance and cycle it: cells pulse from their centers.

**Verdict: fully integer feasible** — squared distances are the trick; nobody needs the root. Method 1 is a solid mid-cost mode, and `F2−F1` walls + palette cycling is a distinctive look the plasma family can't produce.

---

## 9. Differential growth

**What it looks like in motion.** A closed loop of points that repels itself, grows by inserting points, and folds into ever-denser brain-coral / lettuce-edge meanders. Mesmerizing as a *drawn history* — the trail it leaves looks like carved relief.

**Cheap integer version.**
- Polyline of nodes in Q8 (`x, y` i32). Per step per node: attract to neighbors (spring, shift-based), repel from nearby nodes (the costly part), small noise jitter; insert a node when a segment stretches past a threshold; cap total nodes.
- Repulsion needs neighbor lookup: bucket nodes into a coarse grid (like §8) and only test the 3×3 surrounding cells. With that, a few thousand nodes at 60 fps is fine. Squared-distance compares only.
- Draw with the Bresenham/line kernel presumably already in `draw.s`.
- Failure mode: the curve fills the screen and stops reading as growth → fade the trail buffer slowly and reset the curve every few minutes, or grow inside an SDF shape mask (heart that fills with coral = very dazzle).

**Repaint or accumulate?** **Accumulate** — the whole appeal is the deposit-over-time trail. Non-fading buffer for engraving look, or slow decay for perpetual mode.

**Verdict:** medium complexity (dynamic point set, insertion, spatial hashing) but a unique organic signature nothing else on this list produces. Tier-2 backlog.

---

## 10. Space colonization (vines / venation)

**What it looks like in motion.** Branches creep across the screen toward scattered invisible "attractor" points, ramify like ivy, lightning, or leaf veins, and terminate when the food is consumed. Watching it colonize a shape (again: SDF heart full of vines) is the moneymaker.

**Cheap integer version.**
- Scatter 500–2000 attractor points (inside an SDF mask for shaped growth). Nodes = tree of branch tips (x, y in Q8, parent index).
- Per step: each attractor finds its nearest node within radius R (coarse-grid buckets again, squared distances); each node influenced by ≥1 attractor grows a fixed-length step toward the average direction — normalize with one LUT: `atan2`-by-octant table or a reciprocal-sqrt-free trick (step = dominant axis + half minor axis, close enough at 2–4 px steps). Kill attractors within kill-radius of a node.
- Cost is bursty early, then decays as attractors die; totally fine at these scales.
- Motion payoff is the *drawing*, so it's inherently a timed episode: colonize → hold → fade out → reseed new shape. That episodic structure fits a screensaver's scene-rotation model well.
- Bonus pass: age-based line thickness (redraw parent paths thicker) turns twigs into trunks.

**Repaint or accumulate?** **Accumulate** — branch segments are drawn permanently as they appear; fade the whole buffer only during scene transitions.

**Verdict:** most "choreographed" technique here; medium effort, high wow, natural fit with `shapes.bin` masks.

---

## Cross-cutting summary table

| # | Technique | Motion character | Integer trick | Mode | Cost | Priority |
|---|---|---|---|---|---|---|
| 1 | Flow field | silk-thread currents | plasma-as-angle-field, Q8 particles | accumulate | low | **A** |
| 2 | Curl noise | non-clumping smoke | finite-diff curl of plasma ψ | accumulate | low | **A** (after 1) |
| 3 | Reaction–diffusion | dividing spots/stripes | fake: blur+sharpen+threshold loop | accumulate | med (fake: low) | B (fake), C (real) |
| 4 | Cyclic CA | self-organizing spirals | compare/adopt rule, u8 grid, no muls | accumulate | very low | **A** |
| 5 | BZ spirals | chemical pinwheels | 3-state excitable CA or wrap-add hodgepodge | accumulate | very low | B (shares §4 engine) |
| 6 | Wave interference | breathing ripple moiré | radius LUT / octagonal dist + sintab | repaint | very low | **A** |
| 7 | Domain warping | melting marble | sine-sum coordinate offsets, Q8 | repaint | low | **A** |
| 8 | Voronoi/Worley | sliding stained glass | squared distances, 3×3 seed buckets | repaint | med | B |
| 9 | Differential growth | brain-coral meander | Q8 polyline + spatial hash repulsion | accumulate | med-high | C |
| 10 | Space colonization | creeping vines | nearest-attractor via buckets, LUT normalize | accumulate | med | B |

**Shared infrastructure worth building once:**
- **Coarse-grid field buffer** (u8/i16 per 16×16 block) — powers 1, 2, and the warp fields of 7.
- **Trail buffer with shift-based decay** (`v -= v>>5`) — powers 1, 2, 9, 10.
- **Double-buffered u8 cell grid** — powers 3, 4, 5 (three modes, one engine, different inner loops).
- **Quarter-plane radius LUT** — powers 6, 8, and any future radial effect; generate in `gen_tables.py` alongside the existing bins.
- **Squared-distance discipline** — never take a square root anywhere in this codebase.
