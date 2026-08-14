// ============================================================
// draw.s — ARM64 (Apple Silicon) — dazzle3.0: THE FLOW ENGINE
//
// void draw_frame(uint32_t *fb, int width, int height, int frame);
//
// J's brief: "the whole thing should flow, not jolt... different
// shapes put together randomly, with random colors, no set times."
// So: NO modes, NO segments, NO cuts. Every frame is a weighted
// blend of all five shape-fields; the weights, the twist amount,
// the tile-mirror amount, the yin-yang amount, and every size all
// DRIFT on independent slow clocks with incommensurate periods —
// combinations form and dissolve and never repeat. Colors walk a
// continuous chain of random scheme crossfades (each fade ends
// where the next begins: zero jumps, random destinations).
//
// The algebra that makes it cheap: the five fields expand into
// shared primitive terms (ring, square, satellites, spokes,
// diamond, ripple, rays, radius, moire, corridor); blending all
// fields = one per-frame coefficient per TERM, one madd per term
// per pixel. Blend-everything costs barely more than one mode.
//
// GP pantry: w14 S | w15 R32 | w24 D32 | w25 fade | w26 ripple
//   phase | w27 flow constant (pre-weighted) | w28 256-fade
//   x19 sintab | x20/x21 scheme pair | x22/x23 yang pair
// FP pantry (d8-d15 saved in prologue):
//   s1,s2 sat1 fold | s3 ring rad | s4 spin phase Q8 | s5 twistQ
//   s6,s7 sat2 fold | s8 W_ring | s9 W_square | s10 W_sat1 |
//   s11 W_sat2 | s12 W_spoke | s13 W_diamond | s14 W_ripple |
//   s15 W_ray | s17 W_rlinear | s29 w_moire | s30 w_corridor |
//   s28 tileQ|duoQ<<16 | s19 accent bits | s16 px stash |
//   s18 px moire stash | s20 ring accent color |
//   s24,s25 moire c1 | s26,s27 moire c2
// ============================================================

.global _draw_frame
.p2align 2

_draw_frame:
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    stp     x27, x28, [sp, #-16]!
    stp     d8, d9, [sp, #-16]!
    stp     d10, d11, [sp, #-16]!
    stp     d12, d13, [sp, #-16]!
    stp     d14, d15, [sp, #-16]!

    adrp    x19, sintab@PAGE
    add     x19, x19, sintab@PAGEOFF

    // ---- spin: base rotation with a breathing wobble ----
    mov     w10, #3
    mul     w9, w3, w10
    bl      Lsin16
    asr     w12, w12, #5                // wobble +-512 Q8 (~3 deg)
    mov     w10, #5
    mul     w9, w3, w10
    add     w9, w9, w12
    fmov    s4, w9                      // spin phase Q8 (~3.6 min/rev)

    // ---- twist amount: drifts 0..4600 (0 = pure spin) ----
    mov     w10, #2
    mul     w9, w3, w10
    movz    w10, #11000
    add     w9, w9, w10
    bl      Ldwell
    cmp     w12, #0
    csel    w12, wzr, w12, lt
    mul     w12, w12, w12
    lsr     w12, w12, #14               // shaped 0..16384
    mov     w10, #3000
    mul     w12, w12, w10
    lsr     w12, w12, #14
    fmov    s5, w12
    fmov    w9, s4                      // plain-rotation fast path:
    bl      Lsin16                      //   sin/cos packed into s31
    mov     w16, w12
    fmov    w9, s4
    add     w9, w9, #16384
    bl      Lsin16
    and     w16, w16, #0xFFFF
    orr     w16, w16, w12, lsl #16
    fmov    s31, w16

    // ---- tile amount & yin-yang amount, packed ----
    mov     w10, #3
    mul     w9, w3, w10
    movz    w10, #29000
    add     w9, w9, w10
    bl      Ldwell
    cmp     w12, #0
    csel    w12, wzr, w12, lt
    mul     w12, w12, w12
    lsr     w16, w12, #20               // tileQ 0..256
    mov     w10, #5
    mul     w9, w3, w10
    movz    w10, #47000
    add     w9, w9, w10
    bl      Ldwell
    cmp     w12, #0
    csel    w12, wzr, w12, lt
    mul     w12, w12, w12
    lsr     w12, w12, #20               // duoQ 0..256
    orr     w16, w16, w12, lsl #16
    fmov    s28, w16

    // ---- five field weights: staggered slow clocks ----
    mov     w10, #3
    mul     w9, w3, w10
    bl      Ldwell
    cmp     w12, #0
    csel    w12, wzr, w12, lt
    mul     w12, w12, w12
    lsr     w20, w12, #20
    add     w20, w20, #64               // w_calm 64..320 (canvas floor)
    mov     w10, #5
    mul     w9, w3, w10
    movz    w10, #13000
    add     w9, w9, w10
    bl      Ldwell
    cmp     w12, #0
    csel    w12, wzr, w12, lt
    mul     w12, w12, w12
    lsr     w21, w12, #20               // w_rays
    mov     w10, #7
    mul     w9, w3, w10
    movz    w10, #26000
    add     w9, w9, w10
    bl      Ldwell
    cmp     w12, #0
    csel    w12, wzr, w12, lt
    mul     w12, w12, w12
    lsr     w22, w12, #20               // w_moire
    mov     w10, #11
    mul     w9, w3, w10
    movz    w10, #39000
    add     w9, w9, w10
    bl      Ldwell
    cmp     w12, #0
    csel    w12, wzr, w12, lt
    mul     w12, w12, w12
    lsr     w23, w12, #20               // w_corr
    mov     w10, #13
    mul     w9, w3, w10
    movz    w10, #52000
    add     w9, w9, w10
    bl      Ldwell
    cmp     w12, #0
    csel    w12, wzr, w12, lt
    mul     w12, w12, w12
    lsr     w16, w12, #21               // w_dense (soft ceiling)

    // normalize the five to sum 256
    add     w9, w20, w21
    add     w9, w9, w22
    add     w9, w9, w23
    add     w9, w9, w16
    lsl     w10, w20, #8
    udiv    w20, w10, w9
    lsl     w10, w21, #8
    udiv    w21, w10, w9
    lsl     w10, w22, #8
    udiv    w22, w10, w9
    lsl     w10, w23, #8
    udiv    w23, w10, w9
    lsl     w10, w16, #8
    udiv    w16, w10, w9
    fmov    s29, w22                    // w_moire
    fmov    s30, w23                    // w_corr

    // ---- per-term coefficients (the algebra fold) ----
    // ---- per-term coefficients: FP ONLY (w20-23 are x20-23,
    //      the palette pointers — never park data there again) ----
    fmov    s15, w21                    // W_ray
    add     w10, w21, w21, lsl #1
    lsr     w10, w10, #1
    fmov    s17, w10                    // W_rlinear
    add     w9, w16, w20, lsr #1        // calm/dense shared base
    add     w10, w9, w21, lsr #1
    fmov    s8, w10                     // W_ring
    lsl     w10, w9, #5
    fmov    s9, w10                     // W_square == W_spoke
    fmov    s10, w9                     // W_sat
    fmov    s13, w16                    // W_dense (diamond + ripple)

    // ---- ring accent enable (raw weights) ----
    add     w10, w20, w16
    mov     w12, #0
    cmp     w10, #150
    cinc    w12, w12, gt
    fmov    s19, w12

    // ---- flow: gentle drift, net weight from RAW weights ----
    add     w17, w20, w16
    add     w17, w17, w22
    add     w17, w17, w23
    sub     w17, w17, w21, lsl #2       // rays run flow backwards
    mov     w10, #5
    mul     w27, w3, w10
    mov     w10, #3
    mul     w9, w3, w10
    bl      Lsin16
    add     w27, w27, w12, asr #5
    and     w27, w27, #0x7FFF
    mul     w27, w27, w17
    asr     w27, w27, #8                // pre-weighted flow constant

    // ---- pulse: breathing depth ----
    mov     w10, #1
    mul     w9, w3, w10
    bl      Lsin16
    asr     w12, w12, #7
    add     w12, w12, #192              // ampQ 64..320
    mov     w17, w12
    mov     w10, #4
    mul     w9, w3, w10
    bl      Lsin16
    mov     w16, w12                    // pulse sin
    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #3                // base = min*3/8
    mul     w11, w10, w17
    lsr     w11, w11, #9                // amp = base*ampQ/512
    mul     w16, w16, w11
    asr     w16, w16, #14               // osc
    add     w9, w10, w16
    lsl     w15, w9, #5                 // R32
    sub     w14, w10, w16               // S

    // D32: quarter-phase pulse (base w10 still live here)
    mov     w17, w10
    mov     w10, #4
    mul     w9, w3, w10
    add     w9, w9, #16384
    bl      Lsin16
    mul     w12, w12, w11
    asr     w12, w12, #14
    add     w12, w17, w12
    lsl     w24, w12, #5                // D32

    // ring radius: slow size drift (base recomputed after the call)
    mov     w12, #6
    mul     w9, w3, w12
    movz    w12, #61000
    add     w9, w9, w12
    bl      Lsin16
    asr     w12, w12, #6
    add     w12, w12, #768
    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #3                // base again (bl ate w10)
    mul     w12, w10, w12
    lsr     w12, w12, #8
    fmov    s3, w12                     // ~base*(2..4), drifts gently

    lsr     w26, w3, #3                 // ripple phase (calmer)

    // ---- satellites: continuous orbits, drifting radii ----
    mov     w10, #9
    mul     w9, w3, w10
    bl      Lsin16
    mov     w16, w12
    mov     w10, #9
    mul     w9, w3, w10
    add     w9, w9, #16384
    bl      Lsin16
    mov     w17, w12
    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #3
    mov     w12, #4
    mul     w9, w3, w12
    movz    w12, #7000
    add     w9, w9, w12
    bl      Lsin16
    asr     w12, w12, #6
    add     w12, w12, #640
    mul     w10, w10, w12
    lsr     w10, w10, #10               // orbit drifts ~0.4..0.9 base
    mul     w16, w16, w10
    asr     w16, w16, #14
    mul     w17, w17, w10
    asr     w17, w17, #14
    fmov    s24, w16                    // unfolded -> moire center 1
    fmov    s25, w17
    cmp     w16, #0
    cneg    w16, w16, lt
    cmp     w17, #0
    cneg    w17, w17, lt
    cmp     w17, w16
    csel    w13, w17, w16, ge
    csel    w12, w16, w17, ge
    fmov    s1, w13
    fmov    s2, w12
    mov     w10, #7
    mul     w9, w3, w10
    neg     w9, w9
    bl      Lsin16
    mov     w16, w12
    mov     w10, #7
    mul     w9, w3, w10
    neg     w9, w9
    add     w9, w9, #16384
    bl      Lsin16
    mov     w17, w12
    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #4
    mul     w16, w16, w10
    asr     w16, w16, #14
    mul     w17, w17, w10
    asr     w17, w17, #14
    fmov    s26, w16                    // moire center 2
    fmov    s27, w17
    cmp     w16, #0
    cneg    w16, w16, lt
    cmp     w17, #0
    cneg    w17, w17, lt
    cmp     w17, w16
    csel    w13, w17, w16, ge
    csel    w12, w16, w17, ge
    fmov    s6, w13
    fmov    s7, w12

    // ---- color chain: each fade ends where the next begins ----
    adrp    x9, palette@PAGE
    add     x9, x9, palette@PAGEOFF
    mov     w13, #6
    lsr     w10, w3, #11                // color leg p
    movz    w12, #0x79B1
    movk    w12, #0x9E37, lsl #16
    mul     w11, w10, w12
    lsr     w11, w11, #13
    udiv    w16, w11, w13
    msub    w11, w16, w13, w11          // A = pick(p)
    add     w10, w10, #1
    mul     w12, w10, w12
    lsr     w12, w12, #13
    udiv    w16, w12, w13
    msub    w12, w16, w13, w12          // B = pick(p+1): chained
    lsl     w11, w11, #17
    add     x20, x9, x11
    lsl     w12, w12, #17
    add     x21, x9, x12
    lsr     w10, w3, #11
    movz    w12, #0xCA6B
    movk    w12, #0x85EB, lsl #16
    mul     w11, w10, w12
    lsr     w11, w11, #13
    udiv    w16, w11, w13
    msub    w11, w16, w13, w11          // yang A
    add     w10, w10, #1
    mul     w12, w10, w12
    lsr     w12, w12, #13
    udiv    w16, w12, w13
    msub    w12, w16, w13, w12          // yang B
    lsl     w11, w11, #17
    add     x22, x9, x11
    lsl     w12, w12, #17
    add     x23, x9, x12
    ubfx    w25, w3, #3, #8             // fade 0..255 per leg
    mov     w9, #256
    sub     w28, w9, w25

    // ---- ring accent color: drifting jewel tap ----
    lsr     w10, w3, #0
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s20, w11

    lsr     w5, w1, #1                  // cx
    lsr     w6, w2, #1                  // cy
    mov     w3, #0                      // y (frame consumed)

// ============================================================
// THE ONE LOOP — transform continuum, then the term sum.
// ============================================================
Ly_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
Lx_loop:
    cmp     w4, w1
    b.ge    Lnext_row
    sub     w7, w4, w5
    sub     w8, w3, w6

    // ---- radius: octagonal norm (max + min/2), the DOS way ----
    cmp     w7, #0
    cneg    w9, w7, lt
    cmp     w8, #0
    cneg    w10, w8, lt
    cmp     w9, w10
    csel    w13, w9, w10, ge
    csel    w9, w10, w9, ge
    add     w13, w13, w9, lsr #1
    lsl     w13, w13, #5                // r32
    fmov    s16, w13

    // ---- rotate: angle = spin + r*twist (twist 0 -> pure spin) ----
    fmov    w9, s5
    cbnz    w9, Lrot_twist
    fmov    w9, s31                     // dormant twist: unpack
    sbfx    w16, w9, #0, #16            //   per-frame sin/cos
    asr     w17, w9, #16
    b       Lrot_go
Lrot_twist:
    mul     w9, w13, w9
    asr     w9, w9, #13                 // gentle shear
    fmov    w10, s4
    add     w9, w9, w10                 // angle Q8
    ubfx    w10, w9, #8, #8
    ldr     w16, [x19, w10, uxtw #2]
    add     w10, w10, #1
    and     w10, w10, #255
    ldr     w17, [x19, w10, uxtw #2]
    sub     w17, w17, w16
    and     w10, w9, #255
    mul     w17, w17, w10
    add     w16, w16, w17, asr #8       // sin, interpolated
    add     w9, w9, #16384
    ubfx    w10, w9, #8, #8
    ldr     w17, [x19, w10, uxtw #2]
    add     w10, w10, #1
    and     w10, w10, #255
    ldr     w13, [x19, w10, uxtw #2]
    sub     w13, w13, w17
    and     w10, w9, #255
    mul     w13, w13, w10
    add     w17, w17, w13, asr #8       // cos, interpolated
Lrot_go:
    mul     w11, w7, w17
    mul     w12, w8, w16
    sub     w11, w11, w12
    asr     w11, w11, #14               // rx
    mul     w12, w7, w16
    madd    w12, w8, w17, w12
    asr     w12, w12, #14               // ry

    // ---- tile morph: partial pull toward mirrored tiles ----
    fmov    w13, s28
    and     w13, w13, #0xFFFF
    cbz     w13, Ltm_done
    and     w9, w11, #1023
    sub     w9, w9, #512
    cmp     w9, #0
    cneg    w9, w9, lt
    sub     w9, w9, w11
    mul     w9, w9, w13
    add     w11, w11, w9, asr #8
    and     w9, w12, #1023
    sub     w9, w9, #512
    cmp     w9, #0
    cneg    w9, w9, lt
    sub     w9, w9, w12
    mul     w9, w9, w13
    add     w12, w12, w9, asr #8
Ltm_done:

    // ---- yin-yang side bit, packed into the r32 stash ----
    eor     w13, w11, w12
    lsr     w13, w13, #31
    fmov    w10, s16
    orr     w10, w10, w13, lsl #30
    fmov    s16, w10

    // ---- moire term (skipped when weightless) ----
    fmov    w13, s29
    cbz     w13, Lmo_skip
    fmov    w9, s24
    sub     w9, w11, w9
    cmp     w9, #0
    cneg    w9, w9, lt
    fmov    w10, s25
    sub     w10, w12, w10
    cmp     w10, #0
    cneg    w10, w10, lt
    cmp     w9, w10
    csel    w13, w9, w10, ge
    csel    w9, w10, w9, ge
    add     w9, w13, w9, lsr #1
    lsl     w9, w9, #5                  // d1 octagonal
    fmov    w10, s26
    sub     w10, w11, w10
    cmp     w10, #0
    cneg    w10, w10, lt
    fmov    w13, s27
    sub     w13, w12, w13
    cmp     w13, #0
    cneg    w13, w13, lt
    cmp     w10, w13
    csel    w17, w10, w13, ge
    csel    w10, w13, w10, ge
    add     w10, w17, w10, lsr #1
    lsl     w10, w10, #5                // d2 octagonal
    add     w13, w9, w10
    sub     w9, w9, w10
    cmp     w9, #0
    cneg    w9, w9, lt
    add     w13, w13, w9, lsl #1
    asr     w13, w13, #3                // gentle fringes
    b       Lmo_done
Lmo_skip:
    mov     w13, #0
Lmo_done:
    fmov    s18, w13

    // ---- fold ----
    cmp     w11, #0
    cneg    w9, w11, lt
    cmp     w12, #0
    cneg    w10, w12, lt
    cmp     w9, w10
    csel    w13, w10, w9, ge
    csel    w9, w9, w10, ge             // u
    mov     w10, w13                    // v

    // ================= THE TERM SUM =================
    fmov    w12, s16
    and     w12, w12, #0x3FFFFFFF       // r32
    sub     w13, w12, w15               // ring |r-R32|
    cmp     w13, #0
    cneg    w13, w13, lt
    fmov    w17, s8
    mul     w16, w13, w17               // acc = W_ring*ring
    fmov    w17, s19
    cbz     w17, Lnoring
    cmp     w13, #64
    b.gt    Lnoring
    orr     w17, w17, #4                // pixel is on the ring
    fmov    s19, w17
Lnoring:
    fmov    w17, s17                    // r linear (ray bands)
    madd    w16, w12, w17, w16
    lsr     w13, w12, #5                // ripple
    add     w13, w13, w26
    and     w13, w13, #255
    ldr     w13, [x19, w13, uxtw #2]
    asr     w13, w13, #4
    fmov    w17, s13
    madd    w16, w13, w17, w16          // W_dense
    sub     w13, w9, w14                // square |u-S|
    cmp     w13, #0
    cneg    w13, w13, lt
    fmov    w17, s9
    madd    w16, w13, w17, w16          // W_square
    add     w13, w9, w10                // diamond |32(u+v)-D32|
    lsl     w13, w13, #5
    sub     w13, w13, w24
    cmp     w13, #0
    cneg    w13, w13, lt
    fmov    w17, s13
    madd    w16, w13, w17, w16          // W_dense
    fmov    w17, s9
    msub    w16, w10, w17, w16          // spokes (subtract)
    lsl     w13, w10, #1                // corridor
    cmp     w9, w13
    csel    w13, w9, w13, ge
    add     w13, w13, w13, lsl #1
    lsl     w13, w13, #2
    fmov    w17, s30
    madd    w16, w13, w17, w16
    fmov    w17, s15                    // rays (udiv gated)
    cbz     w17, Lray_skip
    lsl     w13, w10, #8
    add     w12, w9, #1
    udiv    w13, w13, w12
    add     w13, w13, w13, lsl #1
    and     w13, w13, #255
    ldr     w13, [x19, w13, uxtw #2]
    asr     w13, w13, #3
    madd    w16, w13, w17, w16
Lray_skip:
    fmov    w13, s18                    // moire
    fmov    w17, s29
    madd    w16, w13, w17, w16
    fmov    w7, s1                      // satellite 1 (octagonal)
    sub     w7, w9, w7
    cmp     w7, #0
    cneg    w7, w7, lt
    fmov    w8, s2
    sub     w8, w10, w8
    cmp     w8, #0
    cneg    w8, w8, lt
    cmp     w7, w8
    csel    w13, w7, w8, ge
    csel    w7, w8, w7, ge
    add     w7, w13, w7, lsr #1
    lsl     w7, w7, #5
    fmov    w8, s3
    sub     w7, w7, w8
    cmp     w7, #0
    cneg    w7, w7, lt
    fmov    w17, s10
    madd    w16, w7, w17, w16           // W_sat
    fmov    w8, s6                      // satellite 2 (octagonal, sub)
    sub     w8, w9, w8
    cmp     w8, #0
    cneg    w8, w8, lt
    fmov    w13, s7
    sub     w13, w10, w13
    cmp     w13, #0
    cneg    w13, w13, lt
    cmp     w8, w13
    csel    w17, w8, w13, ge
    csel    w8, w13, w8, ge
    add     w8, w17, w8, lsr #1
    lsl     w8, w8, #5
    fmov    w13, s3
    sub     w8, w8, w13
    cmp     w8, #0
    cneg    w8, w8, lt
    fmov    w17, s10
    msub    w16, w8, w17, w16           // W_sat (subtract)
    asr     w16, w16, #8                // ---- final index ----
    add     w16, w16, w27
    and     w10, w16, #0x7FFF

    // ---- color: chained crossfade + continuous yin-yang ----
    ldr     w9,  [x20, w10, uxtw #2]
    ldr     w11, [x21, w10, uxtw #2]
    and     w12, w9,  #0x00FF00FF
    and     w13, w11, #0x00FF00FF
    mul     x12, x12, x28
    mul     x13, x13, x25
    add     x12, x12, x13
    lsr     x12, x12, #8
    and     w12, w12, #0x00FF00FF
    and     w9,  w9,  #0x0000FF00
    and     w11, w11, #0x0000FF00
    mul     x9,  x9,  x28
    mul     x11, x11, x25
    add     x9,  x9,  x11
    lsr     x9,  x9,  #8
    and     w9,  w9,  #0x0000FF00
    orr     w11, w12, w9                // main color
    fmov    w13, s28
    lsr     w13, w13, #16               // duoQ
    cbz     w13, Lduo_done
    fmov    w12, s16
    tbz     w12, #30, Lduo_done         // yang side only
    ldr     w9,  [x22, w10, uxtw #2]
    ldr     w12, [x23, w10, uxtw #2]
    and     w16, w9,  #0x00FF00FF
    and     w17, w12, #0x00FF00FF
    mul     x16, x16, x28
    mul     x17, x17, x25
    add     x16, x16, x17
    lsr     x16, x16, #8
    and     w16, w16, #0x00FF00FF
    and     w9,  w9,  #0x0000FF00
    and     w12, w12, #0x0000FF00
    mul     x9,  x9,  x28
    mul     x12, x12, x25
    add     x9,  x9,  x12
    lsr     x9,  x9,  #8
    and     w9,  w9,  #0x0000FF00
    orr     w16, w16, w9                // yang color
    mov     w9, #256
    sub     w9, w9, w13
    and     w12, w11, #0x00FF00FF
    mul     x12, x12, x9
    and     w17, w16, #0x00FF00FF
    mul     x17, x17, x13
    add     x12, x12, x17
    lsr     x12, x12, #8
    and     w12, w12, #0x00FF00FF
    and     w11, w11, #0x0000FF00
    mul     x11, x11, x9
    and     w16, w16, #0x0000FF00
    mul     x16, x16, x13
    add     x11, x11, x16
    lsr     x11, x11, #8
    and     w11, w11, #0x0000FF00
    orr     w11, w12, w11
Lduo_done:
    orr     w11, w11, #0xFF000000

    // ---- ring accent (flag set during term sum) ----
    fmov    w9, s19
    tbz     w9, #2, Lput
    and     w9, w9, #0xFFFFFFFB
    fmov    s19, w9
    fmov    w11, s20
Lput:
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       Lx_loop
Lnext_row:
    add     w3, w3, #1
    b       Ly_loop

Ldone:
    ldp     d14, d15, [sp], #16
    ldp     d12, d13, [sp], #16
    ldp     d10, d11, [sp], #16
    ldp     d8, d9, [sp], #16
    ldp     x27, x28, [sp], #16
    ldp     x25, x26, [sp], #16
    ldp     x23, x24, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x19, x20, [sp], #16
    ldp     x29, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Ldwell — time-warp then sine: phase lingers at extremes
// (hold the look ~10-15s), moves quickly between them (morph).
// p' = p - sin(p)/2, then returns sin(p'). Same ABI as Lsin16.
// ------------------------------------------------------------
Ldwell:
    stp     x29, x30, [sp, #-16]!
    mov     w29, w9
    bl      Lsin16
    sub     w9, w29, w12, asr #1
    bl      Lsin16
    ldp     x29, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Lsin16 — interpolated sine. in: w9 = 16-bit phase (wraps).
// out: w12 = Q14 sin. clobbers w10, w13. x19 = sintab.
// ------------------------------------------------------------
Lsin16:
    ubfx    w10, w9, #8, #8
    ldr     w12, [x19, w10, uxtw #2]
    add     w10, w10, #1
    and     w10, w10, #255
    ldr     w13, [x19, w10, uxtw #2]
    sub     w13, w13, w12
    and     w10, w9, #255
    mul     w13, w13, w10
    add     w12, w12, w13, asr #8
    ret

// ------------------------------------------------------------
// Lpalmix — accent color. in: w10 = palette index 0..32767.
// out: w11 = lightened jewels ARGB. clobbers w12, x13.
// ------------------------------------------------------------
Lpalmix:
    adrp    x13, palette@PAGE
    add     x13, x13, palette@PAGEOFF
    ldr     w11, [x13, w10, uxtw #2]
    movz    w13, #0x7F7F
    movk    w13, #0x007F, lsl #16
    and     w12, w13, w11, lsr #1
    lsr     w13, w13, #1
    and     w13, w13, #0x3F3F3F3F
    and     w13, w13, w11, lsr #2
    add     w11, w12, w13
    movz    w12, #0x4040
    movk    w12, #0x0040, lsl #16
    add     w11, w11, w12
    orr     w11, w11, #0xFF000000
    ret

// ============================================================
.section __TEXT,__const
.p2align 2
sintab:
    .incbin "sintab.bin"
.p2align 2
palette:
    .incbin "palette.bin"
