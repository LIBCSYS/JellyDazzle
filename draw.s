// ============================================================
// draw.s — ARM64 (Apple Silicon) — step 11: MODE ENGINE
//
// void draw_frame(uint32_t *fb, int width, int height, int frame);
//
// Like the original dazzle.exe, the app now runs multiple drawing
// ROUTINES and switches between them (~34s per mode, hard cut,
// original style):
//   mode 1 — COLOR SATELLITES (step 10): calm interference field,
//            breathing ring, square + diamond satellites
//   mode 2 — THE TWIST (step 8, resurrected): angle grows with
//            radius so everything shears into spiral pinwheel
//            arms; ripple rings crawl through dense bands;
//            circle satellites
// Both modes share the step-9/10 toolkit the DOS original never
// had: interpolated 16-bit clocks (Lsin16), the 6-scheme palette
// with per-pixel crossfade, and drifting jewel-tone accents.
//
// Register pantry (callee-saved, stacked in prologue):
//   x19 sintab | x20/x21 palette A/B | w24 D32 (diamond, mode 2)
//   w25 fade | w26 twist spin Q8 | w27 flow | w28 256-fade
// FP pantry: s1,s2 sat1 | s3 ring radius | s4/s5 spin cos/sin |
//   s6,s7 sat2 | s16 in-loop ring stash | s17 twistiness |
//   s18 ripple phase | s19 mode flag | s20 ring color |
//   s21 sat1 color | s22 sat2 color | s23 spoke color
// Loop-persistent: w14 S (square field) | w15 R32
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

    adrp    x19, sintab@PAGE
    add     x19, x19, sintab@PAGEOFF

    // ---- mode select: (frame>>11) mod 7 — ~34s each, hard cut ----
    fmov    s28, w3                     // stash TRUE frame (accumulator modes)
    lsr     w9, w3, #11
    mov     w13, #7
    udiv    w10, w9, w13
    msub    w9, w10, w13, w9
    fmov    s19, w9                     // mode 0..6

    // ---- spin: phase = frame*16 (~68s/rev), interpolated ----
    lsl     w9, w3, #4
    bl      Lsin16
    fmov    s5, w12                     // sin
    lsl     w9, w3, #4
    add     w9, w9, #16384
    bl      Lsin16
    fmov    s4, w12                     // cos

    // ---- pulse: same period, offset phase, deep amplitude ----
    lsl     w9, w3, #4
    movz    w10, #21845
    add     w9, w9, w10
    bl      Lsin16
    mov     w16, w12                    // pulse sin
    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #3                // base = min*3/8
    add     w11, w10, w10, lsl #1
    lsr     w11, w11, #2                // amp = base*3/4
    mul     w16, w16, w11
    asr     w16, w16, #14               // osc
    add     w9, w10, w16                // R = base + osc
    lsl     w15, w9, #5                 // R32
    sub     w14, w10, w16               // S = base - osc
    lsl     w12, w10, #2
    fmov    s3, w12                     // satellite ring radius

    // ---- quarter-phase pulse -> D32 (mode 2 diamond field) ----
    mov     w17, w10                    // save base across the call
    lsl     w9, w3, #4
    movz    w10, #21845
    add     w9, w9, w10
    add     w9, w9, #16384
    bl      Lsin16
    mul     w12, w12, w11               // oscQ = cos * amp
    asr     w12, w12, #14
    add     w12, w17, w12               // D = base + oscQ
    lsl     w24, w12, #5                // D32

    // ---- twist controls (mode 2) ----
    lsl     w26, w3, #6                 // twist spin, Q8 idx (~17s/rev)
    lsl     w9, w3, #1                  // twistiness clock (~9min)
    bl      Lsin16
    asr     w12, w12, #11               // -8..8
    add     w12, w12, #10               // twistiness 2..18
    fmov    s17, w12
    lsr     w9, w3, #1                  // ripple phase: crawls at half rate
    fmov    s18, w9

    // ---- satellite 1: orbit=base, ~34s, smooth ----
    lsl     w9, w3, #5
    bl      Lsin16
    mov     w16, w12
    lsl     w9, w3, #5
    add     w9, w9, #16384
    bl      Lsin16
    mov     w17, w12
    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #3                // base again
    mul     w16, w16, w10
    asr     w16, w16, #14
    mul     w17, w17, w10
    asr     w17, w17, #14
    fmov    s24, w16                    // unfolded c1 (moire mode)
    fmov    s25, w17
    cmp     w16, #0
    cneg    w16, w16, lt
    cmp     w17, #0
    cneg    w17, w17, lt
    cmp     w17, w16
    csel    w13, w17, w16, ge           // fold into octant space
    csel    w12, w16, w17, ge
    fmov    s1, w13
    fmov    s2, w12

    // ---- satellite 2: orbit=base/2, counter-rotating, ~17s ----
    lsl     w9, w3, #6
    neg     w9, w9
    bl      Lsin16
    mov     w16, w12
    lsl     w9, w3, #6
    neg     w9, w9
    add     w9, w9, #16384
    bl      Lsin16
    mov     w17, w12
    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #4                // base/2
    mul     w16, w16, w10
    asr     w16, w16, #14
    mul     w17, w17, w10
    asr     w17, w17, #14
    fmov    s26, w16                    // unfolded c2 (moire mode)
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

    // ---- color scheme: pseg = frame>>10 (~17s), pair (pseg%6, +1) ----
    lsr     w9, w3, #10
    mov     w13, #6
    udiv    w10, w9, w13
    msub    w10, w10, w13, w9
    add     w11, w10, #1
    cmp     w11, #6
    csel    w11, wzr, w11, eq
    adrp    x9, palette@PAGE
    add     x9, x9, palette@PAGEOFF
    lsl     w10, w10, #17
    add     x20, x9, x10                // + schemeA*131072
    lsl     w11, w11, #17
    add     x21, x9, x11
    ubfx    w25, w3, #2, #8             // fade t 0..255
    mov     w9, #256
    sub     w28, w9, w25                // 256 - t

    lsl     w27, w3, #5                 // color flow phase

    // ---- accent colors: 4 drifting taps from the JEWELS palette ----
    lsl     w10, w3, #2                 // accent drift: 4 idx/frame
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s20, w11                    // ring color
    lsl     w10, w3, #2
    add     w10, w10, #8192
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s21, w11                    // sat1 color
    lsl     w10, w3, #2
    movz    w12, #16384
    add     w10, w10, w12
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s22, w11                    // sat2 color
    lsl     w10, w3, #2
    movz    w12, #24576
    add     w10, w10, w12
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s23, w11                    // spoke color

    lsr     w5, w1, #1                  // cx
    lsr     w6, w2, #1                  // cy

    mov     w3, #0                      // y (frame consumed)

    // ================== DISPATCH ==================
    fmov    w9, s19
    cbz     w9, L1y_loop                // 0: color satellites
    cmp     w9, #1
    b.eq    L2y_loop                    // 1: the twist
    cmp     w9, #2
    b.eq    L3y_loop                    // 2: tunnel/starburst
    cmp     w9, #3
    b.eq    L4y_loop                    // 3: moire eye
    cmp     w9, #4
    b.eq    L5start                     // 4: spirograph (accumulator)
    cmp     w9, #5
    b.eq    L6y_loop                    // 5: kaleido mirror-tile
    b       L7start                     // 6: slash canvas (accumulator)

// ============================================================
// MODE 1 — COLOR SATELLITES (step 10 routine)
// ============================================================
L1y_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
L1x_loop:
    cmp     w4, w1
    b.ge    L1next_row

    // ---- rotate (smooth spin) ----
    sub     w7, w4, w5
    sub     w8, w3, w6
    fmov    w9, s4
    fmov    w10, s5
    mul     w11, w7, w9
    mul     w12, w8, w10
    sub     w11, w11, w12
    asr     w11, w11, #14               // rx
    mul     w12, w7, w10
    madd    w12, w8, w9, w12
    asr     w12, w12, #14               // ry

    // ---- fold ----
    cmp     w11, #0
    cneg    w11, w11, lt
    cmp     w12, #0
    cneg    w12, w12, lt
    cmp     w11, w12
    csel    w9,  w11, w12, ge           // u
    csel    w10, w12, w11, ge           // v
    lsl     w17, w10, #5                // spoke term

    // ---- ring field (breathing circle) ----
    mul     w12, w9, w9
    madd    w12, w10, w10, w12
    lsl     w12, w12, #10
    scvtf   s0, w12
    fsqrt   s0, s0
    fcvtzs  w12, s0
    sub     w12, w12, w15
    cmp     w12, #0
    cneg    w12, w12, lt
    fmov    s16, w12                    // stash dc32 for accent pass

    // ---- square field ----
    sub     w13, w9, w14
    cmp     w13, #0
    cneg    w13, w13, lt                // ds

    // ---- satellite 1: SQUARE ring (Chebyshev distance) ----
    fmov    w7, s1
    sub     w7, w9, w7
    cmp     w7, #0
    cneg    w7, w7, lt
    fmov    w8, s2
    sub     w8, w10, w8
    cmp     w8, #0
    cneg    w8, w8, lt
    cmp     w7, w8
    csel    w7, w7, w8, ge              // max(|dx|,|dy|)
    lsl     w7, w7, #5
    fmov    w8, s3
    sub     w7, w7, w8
    cmp     w7, #0
    cneg    w7, w7, lt                  // dsat1: square outline

    // ---- satellite 2: DIAMOND ring (L1 distance) ----
    fmov    w8, s6
    sub     w8, w9, w8
    cmp     w8, #0
    cneg    w8, w8, lt
    fmov    w16, s7
    sub     w16, w10, w16
    cmp     w16, #0
    cneg    w16, w16, lt
    add     w8, w8, w16                 // |dx|+|dy|
    lsl     w8, w8, #5
    fmov    w16, s3
    sub     w8, w8, w16
    cmp     w8, #0
    cneg    w8, w8, lt                  // dsat2: diamond outline

    // ---- interference mix -> palette index ----
    lsl     w16, w13, #5
    add     w10, w12, w16               // rings + square field
    add     w10, w10, w7
    sub     w10, w10, w8
    sub     w10, w10, w17
    asr     w10, w10, #1                // calm density
    add     w10, w10, w27
    and     w10, w10, #0x7FFF

    // ---- two-scheme palette crossfade ----
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
    orr     w11, w12, w9
    orr     w11, w11, #0xFF000000

    // ---- accents: ring + spokes (satellite outlines removed —
    //      the satellites persist as smooth field-warps only) ----
    fmov    w9, s16
    cmp     w9, #64                     // breathing circle
    b.gt    L1acc_spoke
    fmov    w11, s20
L1acc_spoke:
    cmp     w17, #16                    // spokes
    b.gt    L1put_pixel
    fmov    w11, s23

L1put_pixel:
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       L1x_loop

L1next_row:
    add     w3, w3, #1
    b       L1y_loop

// ============================================================
// MODE 2 — THE TWIST (step 8 routine, on the step-10 toolkit)
// angle = spin + r*twistiness: the center outruns the rim and
// every field shears into spiral arms. Ripple rings crawl
// through the bands. Dense mix (no /2). Circle satellites.
// ============================================================
L2y_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
L2x_loop:
    cmp     w4, w1
    b.ge    L2next_row

    sub     w7, w4, w5                  // dx
    sub     w8, w3, w6                  // dy

    // ---- radius FIRST (rotation-invariant) ----
    mul     w12, w7, w7
    madd    w12, w8, w8, w12
    lsl     w12, w12, #10
    scvtf   s0, w12
    fsqrt   s0, s0
    fcvtzs  w12, s0                     // r32 (keep!)

    // ============ THE TWIST ============
    fmov    w9, s17                     // twistiness
    mul     w9, w12, w9
    asr     w9, w9, #4                  // Q8 angle contribution
    add     w9, w9, w26                 // + smooth spin Q8
    lsr     w9, w9, #8
    and     w9, w9, #255
    ldr     w10, [x19, w9, uxtw #2]     // sin(angle)
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w9, [x19, w9, uxtw #2]      // cos(angle)
    mul     w11, w7, w9
    mul     w13, w8, w10
    sub     w11, w11, w13
    asr     w11, w11, #14               // rx
    mul     w13, w7, w10
    madd    w13, w8, w9, w13
    asr     w13, w13, #14               // ry
    // ===================================

    // ---- fold into the octant ----
    cmp     w11, #0
    cneg    w11, w11, lt
    cmp     w13, #0
    cneg    w13, w13, lt
    cmp     w11, w13
    csel    w9,  w11, w13, ge           // u
    csel    w10, w13, w11, ge           // v

    // ---- RIPPLE: sin(r) seeds the mix accumulator ----
    lsr     w17, w12, #5
    fmov    w13, s18
    add     w17, w17, w13
    and     w17, w17, #255
    ldr     w17, [x19, w17, uxtw #2]    // -16384..16384
    asr     w16, w17, #4                // acc = ripple (-1024..1024)

    // ---- circle field (consumes r32) ----
    sub     w12, w12, w15
    cmp     w12, #0
    cneg    w12, w12, lt                // dc32
    fmov    s16, w12                    // stash for accent
    add     w16, w16, w12

    // ---- diamond field ----
    add     w13, w9, w10
    lsl     w13, w13, #5
    sub     w13, w13, w24               // - D32
    cmp     w13, #0
    cneg    w13, w13, lt
    add     w16, w16, w13

    // ---- square field ----
    sub     w13, w9, w14
    cmp     w13, #0
    cneg    w13, w13, lt
    add     w16, w16, w13, lsl #5

    // ---- spokes ----
    sub     w16, w16, w10, lsl #5

    // ---- satellite 1: circle (L2), original style ----
    fmov    w7, s1
    sub     w7, w9, w7
    fmov    w8, s2
    sub     w8, w10, w8
    mul     w7, w7, w7
    madd    w7, w8, w8, w7
    lsl     w7, w7, #10
    scvtf   s0, w7
    fsqrt   s0, s0
    fcvtzs  w7, s0
    fmov    w8, s3
    sub     w7, w7, w8
    cmp     w7, #0
    cneg    w7, w7, lt                  // dsat1
    add     w16, w16, w7

    // ---- satellite 2: circle ----
    fmov    w8, s6
    sub     w8, w9, w8
    fmov    w17, s7
    sub     w17, w10, w17
    mul     w8, w8, w8
    madd    w8, w17, w17, w8
    lsl     w8, w8, #10
    scvtf   s0, w8
    fsqrt   s0, s0
    fcvtzs  w8, s0
    fmov    w17, s3
    sub     w8, w8, w17
    cmp     w8, #0
    cneg    w8, w8, lt                  // dsat2
    sub     w16, w16, w8

    // ---- dense mix (no /2) + flow -> palette index ----
    add     w10, w16, w27
    and     w10, w10, #0x7FFF

    // ---- two-scheme palette crossfade ----
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
    orr     w11, w12, w9
    orr     w11, w11, #0xFF000000

    // ---- accent: breathing ring only ----
    fmov    w9, s16
    cmp     w9, #64
    b.gt    L2put_pixel
    fmov    w11, s20

L2put_pixel:
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       L2x_loop

L2next_row:
    add     w3, w3, #1
    b       L2y_loop


// ============================================================
// MODE 3 — TUNNEL / STARBURST (video: orange laser-fan)
// rays via octant angle param (no atan2: t = v*256/u), bands
// flow outward fast; breathing ring rides on top.
// ============================================================
L3y_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
L3x_loop:
    cmp     w4, w1
    b.ge    L3next_row
    sub     w7, w4, w5
    sub     w8, w3, w6
    fmov    w9, s4
    fmov    w10, s5
    mul     w11, w7, w9
    mul     w12, w8, w10
    sub     w11, w11, w12
    asr     w11, w11, #14
    mul     w12, w7, w10
    madd    w12, w8, w9, w12
    asr     w12, w12, #14
    cmp     w11, #0
    cneg    w11, w11, lt
    cmp     w12, #0
    cneg    w12, w12, lt
    cmp     w11, w12
    csel    w9,  w11, w12, ge           // u
    csel    w10, w12, w11, ge           // v
    mul     w12, w9, w9
    madd    w12, w10, w10, w12
    lsl     w12, w12, #10
    scvtf   s0, w12
    fsqrt   s0, s0
    fcvtzs  w12, s0                     // r32
    lsl     w13, w10, #8
    add     w16, w9, #1
    udiv    w13, w13, w16               // t = v*256/u : 0..256
    add     w13, w13, w13, lsl #1       // x3 -> 3 ray pairs/octant
    and     w13, w13, #255
    ldr     w13, [x19, w13, uxtw #2]
    asr     w13, w13, #3                // ray field
    add     w16, w12, w12, lsl #1
    lsr     w16, w16, #1                // r*1.5
    sub     w16, w16, w27, lsl #2       // bands rush outward
    add     w10, w13, w16
    sub     w13, w12, w15               // breathing ring
    cmp     w13, #0
    cneg    w13, w13, lt
    fmov    s16, w13
    add     w10, w10, w13, asr #1
    and     w10, w10, #0x7FFF
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
    orr     w11, w12, w9
    orr     w11, w11, #0xFF000000
    fmov    w9, s16
    cmp     w9, #64
    b.gt    L3put
    fmov    w11, s20
L3put:
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       L3x_loop
L3next_row:
    add     w3, w3, #1
    b       L3y_loop

// ============================================================
// MODE 4 — MOIRE EYE (video: blue/green interference eye)
// two ring systems on the counter-orbiting satellite centers;
// their beat pattern makes the crawling eye.
// ============================================================
L4y_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
L4x_loop:
    cmp     w4, w1
    b.ge    L4next_row
    sub     w7, w4, w5
    sub     w8, w3, w6
    fmov    w9, s24
    sub     w9, w7, w9
    fmov    w10, s25
    sub     w10, w8, w10
    mul     w9, w9, w9
    madd    w9, w10, w10, w9
    lsl     w9, w9, #10
    scvtf   s0, w9
    fsqrt   s0, s0
    fcvtzs  w9, s0                      // d1
    fmov    w10, s26
    sub     w10, w7, w10
    fmov    w11, s27
    sub     w11, w8, w11
    mul     w10, w10, w10
    madd    w10, w11, w11, w10
    lsl     w10, w10, #10
    scvtf   s0, w10
    fsqrt   s0, s0
    fcvtzs  w10, s0                     // d2
    add     w12, w9, w10                // elliptic rings
    sub     w13, w9, w10
    cmp     w13, #0
    cneg    w13, w13, lt                // hyperbolic beat
    add     w10, w12, w13, lsl #1
    asr     w10, w10, #2
    add     w10, w10, w27
    and     w10, w10, #0x7FFF
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
    orr     w11, w12, w9
    orr     w11, w11, #0xFF000000
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       L4x_loop
L4next_row:
    add     w3, w3, #1
    b       L4y_loop

// ============================================================
// MODE 5 — SPIROGRAPH (accumulator: the video's line-art trick)
// clears to deep ink on entry, then lays down 256 curve points
// per frame — the drawing grows for the whole ~34s segment.
// Curve constants come from the segment hash: new every visit.
// ============================================================
L5start:
    fmov    w9, s28                     // true frame
    and     w22, w9, #2047              // segment-local frame
    lsr     w23, w9, #11
    movz    w10, #0x79B1
    movk    w10, #0x9E37, lsl #16
    mul     w23, w23, w10               // segment hash
    cbnz    w22, L5draw
    mul     w9, w1, w2                  // entry: clear to deep ink
    mov     w10, #0
    movz    w11, #0x0A12
    movk    w11, #0xFF0A, lsl #16
L5clear:
    str     w11, [x0, w10, uxtw #2]
    add     w10, w10, #1
    cmp     w10, w9
    b.lt    L5clear
L5draw:
    and     w16, w23, #31
    add     w16, w16, #23               // angular step 1
    ubfx    w17, w23, #5, #6
    add     w17, w17, #97               // angular step 2
    cmp     w1, w2
    csel    w9, w1, w2, lt
    lsr     w24, w9, #2
    lsr     w25, w9, #3
    add     w24, w24, w25               // R1 = 3/8 min
    lsr     w25, w9, #3                 // R2 = 1/8 min
    mov     w26, #0
L5k:
    lsl     w9, w22, #8
    add     w21, w9, w26                // t = sl*256 + k
    mul     w9, w21, w16
    bl      Lsin16
    mov     w20, w12                    // sinA
    mul     w9, w21, w16
    add     w9, w9, #16384
    bl      Lsin16
    mul     w7, w20, w24                // x acc = sinA*R1
    mul     w8, w12, w24                // y acc = cosA*R1
    mul     w9, w21, w17
    bl      Lsin16
    madd    w7, w12, w25, w7            // + sinB*R2
    mul     w9, w21, w17
    add     w9, w9, #16384
    bl      Lsin16
    msub    w8, w12, w25, w8            // - cosB*R2
    asr     w7, w7, #14
    add     w7, w7, w5
    asr     w8, w8, #14
    add     w8, w8, w6
    lsl     w10, w21, #3
    and     w10, w10, #0x7FFF
    bl      Lpalmix                     // jewel color drifts along curve
    mov     w9, w7
    mov     w10, w8
    bl      Lplot22
    add     w26, w26, #1
    cmp     w26, #256
    b.lt    L5k
    b       Ldone

// ============================================================
// MODE 6 — KALEIDO MIRROR-TILE (video: dense mirrored tiling)
// rotated plane folded into 1024px mirrored tiles; diamond,
// cross and square-ring fields interfere inside each tile.
// ============================================================
L6y_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
L6x_loop:
    cmp     w4, w1
    b.ge    L6next_row
    sub     w7, w4, w5
    sub     w8, w3, w6
    fmov    w9, s4
    fmov    w10, s5
    mul     w11, w7, w9
    mul     w12, w8, w10
    sub     w11, w11, w12
    asr     w11, w11, #14
    mul     w12, w7, w10
    madd    w12, w8, w9, w12
    asr     w12, w12, #14
    and     w9, w11, #1023              // mirror-tile fold
    sub     w9, w9, #512
    cmp     w9, #0
    cneg    w9, w9, lt                  // tx 0..512
    and     w10, w12, #1023
    sub     w10, w10, #512
    cmp     w10, #0
    cneg    w10, w10, lt                // ty 0..512
    add     w12, w9, w10                // diamond
    sub     w13, w9, w10
    cmp     w13, #0
    cneg    w13, w13, lt                // cross
    mul     w16, w9, w9
    madd    w16, w10, w10, w16
    lsr     w16, w16, #9                // soft square-law rings
    lsl     w17, w12, #3
    add     w17, w17, w16
    sub     w17, w17, w13, lsl #2
    add     w17, w17, w27, lsl #1
    and     w10, w17, #0x7FFF
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
    orr     w11, w12, w9
    orr     w11, w11, #0xFF000000
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       L6x_loop
L6next_row:
    add     w3, w3, #1
    b       L6y_loop

// ============================================================
// MODE 7 — SLASH CANVAS (accumulator: red strokes on white)
// the video's only light-background routine. Clears to warm
// white on entry; every frame carves one jittering dark-red
// stroke. ~2000 strokes by segment end.
// ============================================================
L7start:
    fmov    w9, s28
    and     w22, w9, #2047
    cbnz    w22, L7draw
    mul     w9, w1, w2                  // entry: clear to warm white
    mov     w10, #0
    movz    w11, #0xEEF2
    movk    w11, #0xFFF2, lsl #16
L7clear:
    str     w11, [x0, w10, uxtw #2]
    add     w10, w10, #1
    cmp     w10, w9
    b.lt    L7clear
L7draw:
    fmov    w9, s28
    movz    w10, #0x79B1
    movk    w10, #0x9E37, lsl #16
    mul     w20, w9, w10                // per-frame stroke hash
    ubfx    w9, w20, #7, #10            // start anywhere on canvas
    mul     w21, w9, w1
    lsr     w21, w21, #10               // start x = h10 * w / 1024
    ubfx    w9, w20, #17, #10
    mul     w22, w9, w2
    lsr     w22, w22, #10               // start y
    ubfx    w9, w20, #27, #5
    lsl     w9, w9, #3
    and     w9, w9, #255
    ldr     w16, [x19, w9, uxtw #2]
    asr     w16, w16, #9                // step dx (Q4, ~2px/step)
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w17, [x19, w9, uxtw #2]
    asr     w17, w17, #9                // step dy
    and     w9, w20, #0x3F
    add     w9, w9, #96                 // dark red 96..159
    lsl     w11, w9, #16
    movk    w11, #0x1414
    orr     w11, w11, #0xFF000000
    lsl     w21, w21, #4                // pos in Q4
    lsl     w22, w22, #4
    mov     w26, #0
L7step:
    add     w9, w26, w26, lsl #2        // jitter walks the sine table
    add     w9, w9, w20, lsr #3
    and     w9, w9, #255
    ldr     w12, [x19, w9, uxtw #2]
    asr     w12, w12, #12               // gentle jitter: slash, not scribble
    add     w21, w21, w16
    add     w21, w21, w12
    add     w9, w9, #85
    and     w9, w9, #255
    ldr     w12, [x19, w9, uxtw #2]
    asr     w12, w12, #12
    add     w22, w22, w17
    add     w22, w22, w12
    asr     w9, w21, #4
    asr     w10, w22, #4
    bl      Lplot22
    add     w26, w26, #1
    cmp     w26, #340
    b.lt    L7step
    b       Ldone

Ldone:
    ldp     x27, x28, [sp], #16
    ldp     x25, x26, [sp], #16
    ldp     x23, x24, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x19, x20, [sp], #16
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
// out: w11 = ARGB from the jewels palette (scheme 0), lightened
// to 3/4 c + 0x404040 so it pops but keeps its hue.
// clobbers w12, x13.
// ------------------------------------------------------------
Lpalmix:
    adrp    x13, palette@PAGE           // self-contained base
    add     x13, x13, palette@PAGEOFF
    ldr     w11, [x13, w10, uxtw #2]    // jewels tap
    movz    w13, #0x7F7F
    movk    w13, #0x007F, lsl #16
    and     w12, w13, w11, lsr #1       // c/2
    lsr     w13, w13, #1
    and     w13, w13, #0x3F3F3F3F       // 0x3F3F3F
    and     w13, w13, w11, lsr #2       // c/4
    add     w11, w12, w13               // 3/4 c
    movz    w12, #0x4040
    movk    w12, #0x0040, lsl #16
    add     w11, w11, w12               // + 0x404040
    orr     w11, w11, #0xFF000000
    ret

// ------------------------------------------------------------
// Lplot22 — clipped 2x2 dot. in: w9=x, w10=y, w11=color.
// clobbers w12, w13. needs x0 fb, w1 width, w2 height.
// ------------------------------------------------------------
Lplot22:
    cmp     w9, #0
    b.lt    Lplot22_ret
    cmp     w10, #0
    b.lt    Lplot22_ret
    sub     w12, w1, #1
    cmp     w9, w12
    b.ge    Lplot22_ret
    sub     w12, w2, #1
    cmp     w10, w12
    b.ge    Lplot22_ret
    madd    w12, w10, w1, w9
    str     w11, [x0, w12, uxtw #2]
    add     w13, w12, #1
    str     w11, [x0, w13, uxtw #2]
    add     w13, w12, w1
    str     w11, [x0, w13, uxtw #2]
    add     w13, w13, #1
    str     w11, [x0, w13, uxtw #2]
Lplot22_ret:
    ret

// ============================================================
.section __TEXT,__const
.p2align 2
sintab:
    .incbin "sintab.bin"
.p2align 2
palette:
    .incbin "palette.bin"
