# 006 Tri Morph

## Look
A hexagonal wallpaper that morphs between two symmetry groups live: chiral pinwheels (rotation-only, p6 flavor) melt into mirrored six-point stars (p6m flavor) and back, across the whole lattice at once. The moment mirrors switch on, every swirling cell crystallizes into a snowflake.

## Math
- Same triangular-lattice reduction as 003: skew coords, delta to nearest rotocenter, cartesian (dx,dy), r, theta.
- Two folds of the same chiral source s(theta,r,t) = 0.5 + 0.30·sin(3·theta + 0.16r − 0.010t) + 0.20·sin(0.09r − 0.007t + sin(3·theta)):
  - A (chiral): theta_A = theta mod pi/3 — C6 rotation fold, keeps handedness.
  - B (mirrored): theta_B = |((theta mod pi/3)) − pi/6| — D6 mirror fold.
- Blend: m = smoothstep(0.5 + 0.5·sin(0.004t)); v = (1−m)·s(theta_A) + m·s(theta_B).
- This is the geometry.md §2 observation "p3m1 → add one reflection → p6m; toggling one branch morphs the wallpaper group live", done as a continuous fade.

## Integer ARM64 plan
- Lattice reduction identical to 003 (two Q15 skew multiplies + mulhi rounding).
- theta once per pixel (BAM atan2 poly); fold A = mask/subtract in BAM (pi/3 = 0x2AAB — use the mulhi sector trick since it's not a power of two); fold B = fold A + one triangle-wave mirror. Both folds share the sector computation, so B costs 2 extra ops over A.
- Evaluate source twice (each = 2 sine lookups + adds), blend with Q8 m: `v = vA + (((vB−vA)·m)>>8)`. m from a 256-entry smoothstep-of-sine LUT indexed by frame phase — no per-pixel cost.
- Chirality lives in the source's `sin(3·theta + ...)` term — in BAM that is just (3·theta_fold + phase) & 0xFFFF.

## Palette pairing
Crimson/teal on deep blue drifting toward gold/magenta — cosine palette d=(.85,.15,.35). Warm cells against cool grout keeps the lattice legible during the morph.

## Motion
Mirror blend full cycle ~26 s; cell swirls rotate at 0.010/frame equivalent; lattice drifts (0.08, 0.04) px/frame; palette walks at 0.0004/frame. The group-theory morph is the event; everything else idles gently.
