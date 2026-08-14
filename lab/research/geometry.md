# Kaleidoscope & Symmetric Pattern Mathematics — Integer/Fixed-Point Reference

Research notes for dzzle1. Every section: the math, then an integer/fixed-point
implementation sketch suitable for assembly / no-FPU targets.

---

## 0. Fixed-point conventions used throughout

- **Q15** (`s0.15`): value in [-1, 1), stored in int16. `1.0 ≈ 32767`. Multiply:
  `(a*b) >> 15` (via 32-bit intermediate).
- **Q16.16**: int32, 16 fractional bits. General coordinates.
- **Q8.8**: int16, 8 fractional bits. Cheap coordinates for 8/16-bit targets.
- **BAM angles** (Binary Angular Measure): a full turn = 2^16 (or 2^8). Angle is
  a uint16; wraparound is free (natural integer overflow). `90° = 0x4000`,
  `180° = 0x8000`. All "mod 2π" operations become `& 0xFFFF` — this is the single
  biggest win of integer angle math.
- **Sine table**: 256-entry quarter-wave, Q15.
  ```
  sintab[i] = round(32767 * sin(i * (π/2) / 256)),  i = 0..255 (add entry 256 = 32767 or clamp)
  ```
  Full sine from BAM16 angle `a`:
  ```
  idx = (a >> 6) & 0x3FF          ; 1024 steps/turn
  quad = idx >> 8 ; i = idx & 255
  quad 0: +sintab[i]      quad 1: +sintab[255-i]   (or 256-i with a 257-entry table)
  quad 2: -sintab[i]      quad 3: -sintab[255-i]
  cos(a) = sin(a + 0x4000)
  ```
- **Useful Q15 constants**:
  | value | Q15 | | value | Q15 |
  |---|---|---|---|---|
  | 1/2 | 16384 | √3/2 = sin60 | 28378 |
  | √2/2 | 23170 | 1/√3 | 18919 |
  | tan(π/8) | 13573 | cos22.5 | 30274 |
  | sin22.5 | 12540 | 1/φ | 20254 |
- **isqrt** (integer square root, 32→16 bit): classic shift-subtract, 16
  iterations, no multiply:
  ```
  r=0; b=0x40000000;
  while b > x: b >>= 2
  while b: { if x >= r+b { x -= r+b; r = (r>>1)+b } else r >>= 1; b >>= 2 }
  ; r = floor(sqrt(x))
  ```
- **Magnitude without sqrt** (alpha-max-beta-min): with `mx=max(|x|,|y|)`,
  `mn=min(|x|,|y|)`:
  - cheap: `r ≈ mx + (mn>>1)` (max error ~11.8%)
  - good:  `r ≈ (31470*mx + 13035*mn) >> 15` (0.9604, 0.3978 — max error ~3.96%)
- **PRNG**: 16-bit LCG `s = s*25173 + 13849` (use high byte), or 16-bit Galois
  LFSR taps 0xB400. Needed for IFS and Truchet.

---

## 1. N-fold mirror folds (the kaleidoscope core)

### Math

A kaleidoscope with two mirrors at angle π/N generates dihedral symmetry **D_N**
(2N transforms: N rotations + N reflections). Rendering trick: instead of drawing
the pattern 2N times, **fold every screen pixel into one wedge** of angular width
π/N, then sample the source texture/pattern once:

```
θ = atan2(y, x)                      r = |(x,y)|
w = π/N                              (wedge width)
θ' = θ mod 2w
if θ' > w: θ' = 2w - θ'              (mirror fold — triangle wave in angle)
sample source at (r·cosθ', r·sinθ')
```

Equivalently: reflect the point across the mirror line nearest to it, repeatedly,
until it lies inside the wedge. Reflection across a line at angle φ through the
origin is the matrix:

```
Refl(φ) = [ cos2φ   sin2φ ]
          [ sin2φ  -cos2φ ]
```

### Integer implementation — two regimes

**Regime A: N a power of two, or divisible into 45°/22.5° — no atan at all.**
Powers-of-two folds compose from abs/swap/one fixed reflection, exact and branch-cheap:

- **N=4** (4 mirror sectors of 90°... i.e. fold to quadrant): `x=|x|; y=|y|`.
- **N=8**: fold to quadrant, then fold across the 45° diagonal:
  `x=|x|; y=|y|; if y>x swap(x,y)`.
- **N=16**: N=8 fold, then reflect across the 22.5° line if above it.
  "Above 22.5° line" test without atan: `y·cos22.5 > x·sin22.5`, i.e.
  `30274*y > 12540*x` (Q15 both sides, compare 32-bit products). Reflection uses
  Refl(22.5°) with cos45=sin45=23170:
  ```
  if 30274*y > 12540*x:
      nx = (23170*(x + y)) >> 15     ; cos45·x + sin45·y
      ny = (23170*(x - y)) >> 15     ; sin45·x − cos45·y
  ```
- **N=4 wallpaper-style square fold** (see §2) needs no trig at all.

**N=6 and N=12** fold with the same pattern using 60°/30° constants:
- N=6: fold to half-plane (`y=|y|` gives 0..180°), then fold 0..180° into 0..60°:
  - if point above 60° line (`18919*y > 32767*x`, i.e. `y/√3 > x` ... use
    `y·cos60 > x·sin60` → `16384*y > 28378*x`): reflect across 60° line,
    `Refl(60°) = [−1/2 √3/2; √3/2 1/2]`:
    `nx = (−16384*x + 28378*y)>>15; ny = (28378*x + 16384*y)>>15`
  - then if still above 60°... in practice: fold across 90° (`x=|x|` after
    rotating frame) — simplest correct sequence: `y=|y|`; if above 60° line
    reflect across it; if now above 60° again (was in 120–180°) one more
    reflection lands in wedge. **Bounded at 2 reflections.** Then mirror at 30°
    for the dihedral half: if `12540-line test` fails, reflect across 30°.
- N=12: N=6 sequence + one extra fold across the 15° line (constants
  cos15=31651, sin15=8481, Refl uses cos30=28378, sin30=16384).

**Regime B: arbitrary N (covers N=14) — BAM atan + sine table.**
General fold; works for any N, and is what you want when N animates.

1. **Integer atan2 → BAM16.** Octant reduction + polynomial on the ratio:
   ```
   ax=|x|; ay=|y|
   if ax >= ay: z = (ay << 15) / ax    ; Q15 ratio in [0,1]
                a = atan_q(z)          ; angle in [0, 0x2000] (0..45°)
   else:        z = (ax << 15) / ay
                a = 0x4000 - atan_q(z)
   fix quadrant from signs of x,y:  (x<0 → a = 0x8000 - a), (y<0 → a = -a & 0xFFFF)
   ```
   with the classic 2nd-order approximation, converted to BAM
   (π/4 rad = 0x2000 BAM; 0.273 rad ≈ 2848 BAM):
   ```
   atan_q(z) = ( z * ( 0x2000 + ((2848 * (32767 - z)) >> 15) ) ) >> 15
   ```
   Max error ≈ 0.22° — invisible in a kaleidoscope. (Alternative: 256-entry
   atan LUT on `z>>7`, or CORDIC vectoring mode which yields angle **and**
   radius·1.6468 simultaneously in ~14 add/shift iterations, `1/K = 19898` Q15.)

2. **Fold in BAM space.** Wedge = full turn / (2N)... careful: dihedral D_N
   folds the turn into `2N` wedges of `0x10000/(2N)` each, mirrored alternately:
   ```
   W    = 0x10000 / (2*N)             ; precompute per N (N=14 → W = 2340)
   t    = a mod (2W)                  ; for pow2 N this is a mask; else one
                                      ;   multiply-high trick: s = (a * (2N)) >> 16
                                      ;   t = a - s * 2W   (avoids runtime division)
   if t > W: t = 2W - t               ; triangle-wave mirror
   ```
3. **Rebuild coordinates.** Need r once: `r = isqrt(x²+y²)` or alpha-max-beta-min
   (angular fold is exact; radius error only scales sampling slightly — AMBM is
   usually fine). Then `u = (r * cos_bam(t))>>15; v = (r * sin_bam(t))>>15` and
   sample the source image/pattern at (u,v).

   **Rotation-only variant (no r needed):** compute sector `s` and fold parity;
   rotate the *original* (x,y) by `-s*2W` (sine-table rotation) and negate y on
   odd parity. One 2×2 fixed-point matrix multiply, exact radius preserved:
   ```
   c = cos_bam(s*2W); n = sin_bam(s*2W)
   u = ( x*c + y*n) >> 15
   v = (-x*n + y*c) >> 15
   if s odd: v = -v
   ```
   This is the recommended production path: 1 atan, 1 mulhi for the sector, 2
   table lookups, 4 multiplies. Per-pixel cost is constant regardless of N.

**Precomputed per-N table:** for the demoscene loop, store for each supported N
(4,6,8,12,14,16): `{ 2W, ceil(0x10000/2W) reciprocal for the mulhi sector trick,
optional 2N-entry table of (cos, sin) of sector rotations }` — then the
inner loop has zero divisions.

---

## 2. Wallpaper groups p4m, p6m, p3m1 as coordinate transforms

Wallpaper groups = kaleidoscope symmetry **plus lattice translation**. For
rendering, each is a *fold function* F(x,y) → fundamental domain; sample your
source pattern at F(x,y) and the plane tiles itself with perfect symmetry.
These three groups are exactly the ones generated purely by mirrors (kaleidoscopic
groups), so their folds are sequences of mod + reflections — cheap.

### p4m (square, *442 mirrors — the "bathroom tile" kaleidoscope)

Fundamental domain: 45-45-90 triangle, 1/8 of the square cell.
Fold = triangle-wave both axes, then diagonal fold. **No trig, no division** if
cell size L is a power of two:

```
; cell L = 1<<k, coordinates integer or Qn.8
u = x & (2L-1);  if u >= L: u = 2L-1-u      ; triangle wave (mirror-repeat)
v = y & (2L-1);  if v >= L: v = 2L-1-v
if v > u: swap(u,v)                          ; fold across diagonal
; (u,v) now in the 1/8 triangle — sample source
```
That's ~6 ops/pixel. This alone, fed with any asymmetric source (noise, texture,
plasma), produces classic tile kaleidoscopes.

### p6m (hexagonal, *632 — the "snowflake" wallpaper)

Fundamental domain: 30-60-90 triangle (1/12 of hex cell). Two-stage fold:

1. **Lattice reduction** into one rhombic cell using skewed coordinates.
   Hex basis e1=(1,0), e2=(1/2, √3/2). Inverse transform:
   ```
   ; Q15 consts: INV_SQRT3 = 18919 (1/√3), TWO_INV_SQRT3 = 37837 (Q15, use 32-bit)
   u = x - ((18919 * y) >> 15)         ; u = x − y/√3
   v = (37837 * y) >> 15               ; v = 2y/√3
   u = u mod L;  v = v mod L           ; & (L-1) for pow2 L → one rhombus
   ; back to cartesian:
   x = u + (v >> 1)
   y = (28378 * v) >> 15               ; v·(√3/2)
   ```
2. **Mirror fold** into the 30-60-90 triangle: reflect across the cell's three
   mirror lines (at 0°, 60°, 120° through the cell corner) until inside.
   Geometrically guaranteed to terminate in ≤ 3 reflections; unroll it:
   ```
   y = |y|                                        ; mirror at 0°
   if 16384*y > 28378*x: Refl60(x,y)              ; above 60° → reflect (consts §1)
   if 16384*y > 28378*x: Refl60(x,y)              ; once more covers 120..180 band
   if 28378*y > 16384*x: Refl30(x,y)              ; median mirror at 30°:
       ; Refl(30°) = [ 1/2  √3/2 ; √3/2 −1/2 ]
       ; nx = (16384x + 28378y)>>15 ; ny = (28378x − 16384y)>>15
   ```
   (Equivalent shortcut: run the §1 N=6 dihedral fold on the cell-local coords —
   same three tests.)

### p3m1 (*333 — three-fold mirrors, equilateral triangle domain)

Same lattice reduction as p6m; the fundamental domain is the **full equilateral
triangle** (twice the p6m domain). Fold sequence = p6m steps **without** the
final 30° median mirror. Practical relationship worth exploiting in the demo:
p3m1 → add one reflection → p6m; toggling one branch morphs the wallpaper
group live.

**Rule of thumb:** p4m needs 0 multiplies (pow2 cell), p6m/p3m1 need ~6
multiplies/pixel. All three are resolution-independent pure coordinate maps —
combine with any source: plasma, feedback buffer, previous frame (video-feedback
kaleidoscope).

---

## 3. Rose curves (rhodonea)

### Math
```
r(θ) = A · cos(kθ + φ)          k = p/q rational
```
- k odd integer → k petals; k even integer → 2k petals.
- k = p/q → closes after θ = qπ (p·q odd) or 2qπ; petal count p·q or 2p·q.
- Offset rose `r = A cos(kθ) + c` fattens/loops petals.

### Integer sketch
Pure phase-accumulator + sine table — the cheapest curve in this file:
```
; BAM16 theta steps dt; k as Q8.8 lets petals animate fractionally
theta += dt
kt    = (k * theta) >> 8                  ; Q8.8 k times BAM angle
r     = (A * cos_bam(kt + phi)) >> 15     ; signed! negative r is fine —
x     = cx + ((r * cos_bam(theta)) >> 15) ;   it just draws the opposite petal
y     = cy + ((r * sin_bam(theta)) >> 15)
plot(x,y)
```
4 table lookups, 3 multiplies per point. For a closed curve with rational k=p/q,
run theta over `q` full turns (or 2q if p·q even). Beautiful as the *source
pattern under the §1 fold* — rose + 14-fold mirror is a signature kaleidoscope
look.

---

## 4. Superformula (Gielis)

### Math
```
r(θ) = [ |cos(mθ/4)/a|^n2 + |sin(mθ/4)/b|^n3 ] ^ (−1/n1)
```
m controls rotational symmetry (m-fold), n1..n3 control pinching/bloating; it
subsumes circles, polygons, stars, and organic shapes. The obstacle for integers
is the **fractional power**.

### Integer sketch — two viable routes

**Route 1 (recommended): per-frame radius table.** r(θ) has period 2π/m
(and mirror symmetry inside that), so a **256-entry Q multiple-of-quarter table
covers the whole curve**. Build it once per frame (parameters animate per frame,
not per point), then the inner loop is a table-driven polar plot identical to §3.
256 evaluations/frame can even afford soft-float; but staying integer:

**Route 2: log2/exp2 fixed-point pow.** `x^p = exp2(p · log2 x)`.
- `log2` (Q16 in, Q8.8 out): `e = 31 − clz(x)`; mantissa `m = (x << (15−e))`
  normalized to Q15 in [1,2); `log2 ≈ (e<<8) + logtab[(m>>7)&0xFF]` with a
  256-entry fraction table.
- `exp2` (Q8.8 in): integer part → shift, fraction → 256-entry exp table.
- Then per table slot:
  ```
  c = |cos_bam(m*theta/4)| ; s = |sin_bam(...)|      ; Q15
  t = exp2( (n2 * log2(c_div_a)) >> 8 ) + exp2( (n3 * log2(s_div_b)) >> 8 )
  r = exp2( (−(log2 t) << 8) / n1 )                  ; the −1/n1 outer power
  ```
  Exponents as Q8.8 give plenty of shape resolution. Guard `c→0` (log of 0):
  clamp mantissa at 1 LSB; the formula's true limit is finite because the other
  term dominates.

**Cheap special cases without pow:** n exponents ∈ {1, 2, 4, 0.5}: squares are
multiplies, x^4 two multiplies, x^0.5 is `isqrt` — a surprisingly large family
of superformula shapes (polygon-ish, star-ish) needs no log/exp at all.

---

## 5. Harmonographs (damped pendulum drawings)

### Math
Two (or three) damped oscillators per axis:
```
x(t) = Σ_i A_i · sin(f_i t + φ_i) · e^(−d_i t)
y(t) = Σ_j A_j · sin(f_j t + φ_j) · e^(−d_j t)
```
Near-rational frequency ratios (e.g. f2/f1 = 2.001) give the slowly precessing
web-like figures. Damping shrinks the figure into a spiral of itself.

### Integer sketch
Everything is a phase accumulator + a decaying amplitude:
```
; per oscillator: phase p (BAM16), increment dp (BAM16/step, can be Q0.16
; sub-BAM via a 32-bit accumulator for detuning like 2.001:1),
; amplitude A (Q16.16), decay k (per-step, Q16: e.g. k = 65530 ≈ half-life ~7900 steps)
p_i += dp_i                              ; frequency = dp (free-running, wraps)
A_i  = (A_i * k_i) >> 16                 ; exponential decay ≡ e^(−dt): one mul
x    = cx + ((A1>>16)*sin_bam(p1) >> 15) + ((A2>>16)*sin_bam(p2) >> 15)
y    = cy + ((A3>>16)*sin_bam(p3) >> 15) + ((A4>>16)*sin_bam(p4) >> 15)
plot / line-to previous point
```
Key insight: `e^(−d·n) = k^n` — exponential decay in discrete time is just a
repeated Q16 multiply, no exp() ever. Detuning: make `dp` a Q16.16 added into a
32-bit phase, take the top 16 bits as BAM — gives 1/65536-turn frequency
resolution, which is what makes the figure precess slowly instead of banding.

---

## 6. Lissajous figures

Special case of §5 with no damping:
```
x = A sin(a·t + δ),  y = B sin(b·t)
```
- a:b rational p:q → closed curve with p horizontal / q vertical lobes.
- δ sweeps the figure through its family (0 → line/ellipse degenerate cases).

Integer: two BAM phase accumulators, two table lookups, two multiplies per
point. Animate δ by adding a slow increment to one phase — the classic
"rotating 3D wireframe" illusion. Sub-BAM 32-bit accumulators (as in §5) let
a:b be irrational-ish for never-closing dense figures. As a kaleidoscope source
under the §1 fold, use additive persistence (draw into an accumulation buffer).

---

## 7. Phyllotaxis spirals (sunflower / Vogel model)

### Math
```
θ_n = n · 137.50776°   (golden angle = 2π(1 − 1/φ) = 2π/φ²)
r_n = c · √n
```
Produces the parastichy spiral families (counts = consecutive Fibonacci numbers).
Any rational approximation to the golden angle causes visible spoke-banding —
the irrationality is the point.

### Integer sketch
- **Golden angle in BAM16**: `0.381966 · 65536 = 25035` (error 0.4 BAM ≈
  0.002°; for long runs use a 32-bit sub-BAM accumulator with
  `GOLDEN32 = 0.381966·2^32 = 1640531527 = 0x61C88647` — the Fibonacci-hash
  constant, not a coincidence).
  ```
  acc32 += 0x61C88647 ; theta = acc32 >> 16      ; exact-enough golden angle
  ```
- **√n incrementally, no per-point isqrt**: maintain `r` in Q16.16 with the
  Newton/derivative step `√(n+1) ≈ √n + 1/(2√n)`:
  ```
  r += (c2 << 16) / r        ; c2 = c²/2 in Q16.16; one division per point
  ```
  or division-free: keep `r` Q16.16 and `rinv ≈ 1/(2r)` updated by one
  Newton step of the reciprocal each iteration; or just isqrt(n<<16) — n is
  small (≤ few thousand dots), isqrt is 16 iterations of add/shift, fine.
- Plot: `x = cx + (r·cos_bam(theta))>>15`, same for y; dot size can grow with n.
- **Animating**: add a tiny drift to the per-step angle (25035 ± few units) —
  the pattern reorganizes through neighboring Fibonacci parastichies, the classic
  mesmerizing morph. Costs nothing.

---

## 8. IFS fractals — fern & dragon via integer affine maps

### Math
Iterated Function System: pick map i with probability p_i, apply affine map
`(x,y) ← (a_i x + b_i y + e_i,  c_i x + d_i y + f_i)`, plot after ~10 warmup
iterations ("chaos game"). Attractor is the fractal.

**Barnsley fern** (4 maps), coefficients ×256 (Q8.8) with 8-bit probability
thresholds (cumulative, compare against `rand8`):

| map | a | b | c | d | e | f | p | rand8 < |
|---|---|---|---|---|---|---|---|---|
| stem | 0 | 0 | 0 | 41 | 0 | 0 | .01 | 3 |
| main | 218 | 10 | −10 | 218 | 0 | 410 | .85 | 220 |
| left | 51 | −67 | 59 | 56 | 0 | 410 | .07 | 238 |
| right | −38 | 72 | 67 | 61 | 0 | 113 | .07 | 255 |

(e,f in Q8.8 units, i.e. 410 = 1.6. Attractor spans x∈[−2.18,2.66],
y∈[0,9.96] — scale ×24-ish into a 256-tall buffer.)

```
r = rand8()
select row by threshold
nx = (a*x + b*y) >> 8;  nx += e
ny = (c*x + d*y) >> 8;  ny += f
x = nx; y = ny
if iter > 10: plot((x*scale)>>8 + ox, oy − (y*scale)>>8)
```
4 multiplies/point; ~20–50k points fills the fern. Q8.8 is enough precision —
the attractor is self-correcting (contraction maps eat rounding error).

**Heighway dragon — exact with shifts only.** The two maps are
`f1 = R(45°)/√2` and `f2 = R(135°)/√2 + (1,0)`, and since `(1/√2)·cos45° = 1/2`
exactly, the integer forms are **pure add/shift, zero multiplies, zero error**:
```
; 50/50 on one random bit; coords Q16.16, seed anywhere in [0,1]
if bit == 0:  nx = (x − y) >> 1          ; f1
              ny = (x + y) >> 1
else:         nx = ONE − ((x + y) >> 1)  ; f2   (ONE = 1<<16)
              ny = (x − y) >> 1
```
Also works as the "dragon curve by folding": bit n of the turn sequence =
`(((n & −n) << 1) & n) != 0` — draw as a turtle with 90° turns (BAM += 0x4000)
for the line-drawing variant instead of the chaos game.

**Rendering tip for both:** accumulate hit counts in a byte buffer and map
count→palette; the density shading is what makes IFS look expensive.

---

## 9. Truchet tiles

### Math
Grid of square tiles; each tile carries one of a few *rotations/reflections* of
a single decorated motif; random orientations produce global emergent paths.
- **Smith/arc tiles** (the classic): two quarter-circle arcs of radius L/2
  centered on opposite tile corners; 2 orientations. Arcs always meet edge
  midpoints → every random field yields continuous meandering curves.
- **Diagonal tiles**: half-black triangle, 4 orientations — maze-like.
- **Multi-scale Truchet** (Carlson): subdivide random tiles into 2×2 half-size
  tiles; motifs designed so edges still match.

### Integer sketch (per-pixel, no drawing pass — ideal for a fold-fed source)
```
; tile size L = 1<<k
tx = x >> k;  ty = y >> k                ; tile coords
u  = x & (L-1);  v = y & (L-1)           ; in-tile coords
h  = hash(tx, ty)                        ; e.g. h = (tx*0x9E37 + ty*0x79B9),
                                         ;   h ^= h>>7; orientation = h & 1
if orientation: u = (L-1) − u            ; mirror = rotate the motif 90°
; arc test, radius R = L/2, half-width w, ALL in squared distances (no sqrt):
d1 = u*u + v*v                           ; corner (0,0)
d2 = (L-1-u)*(L-1-u) + (L-1-v)*(L-1-v)   ; opposite corner
in_arc = (RmW2 <= d1 <= RpW2) || (RmW2 <= d2 <= RpW2)
         ; RmW2=(R-w)², RpW2=(R+w)² precomputed
pixel = in_arc ? ink : paper
```
~6 multiplies/pixel, all 16-bit. Variations:
- extra hash bits pick among 4 motifs (arcs / crossing lines / dots) —
  Carlson-style richness for free;
- palette by `(d1 or d2)` distance → shaded ribbons;
- animate by adding frame counter into the hash (per-tile flicker) or by
  scrolling x,y before tiling (the fold in §1/§2 applied *first* gives
  kaleidoscoped Truchet — extremely good screensaver energy);
- multi-scale: if `h & 0xC0 == 0`, recurse one level with `k-1` (bounded depth 2).

---

## 10. How these compose (the dzzle pipeline)

```
per-pixel:  screen (x,y)
   → optional wallpaper fold (§2: p4m/p6m/p3m1)     [lattice symmetry]
   → optional dihedral fold (§1: N-fold BAM fold)   [kaleidoscope]
   → source sample: Truchet (§9) | rose/curve accumulation buffer (§3,5,6)
                    | phyllotaxis dots (§7) | IFS density buffer (§8)
                    | superformula r-table shape test (§4)
```
Curve generators (§3–§8) draw into an offscreen buffer once per frame; the
per-pixel path is fold + one buffer fetch. Everything above runs in 16/32-bit
integer with one 256-entry quarter-sine table, an optional 256-entry atan table,
isqrt, and a PRNG — no FPU, no runtime division except one per phyllotaxis point
(and even that is removable).
