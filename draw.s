// ============================================================
// draw.s — ARM64 (Apple Silicon) — step 10: COLOR SATELLITES
//
// void draw_frame(uint32_t *fb, int width, int height, int frame);
//
// From step 9 (kept): interpolated 16-bit clocks (Lsin16) — all
// motion glides; 6-scheme palette with per-pixel crossfade
// (jewels -> ember -> royal -> gilded -> ice -> spring).
// Changed on J's review:
//   - suit-morph SDF centerpiece DROPPED (read low-rez/off-look)
//   - satellite spots are now SHAPES: sat1 orbits as squares
//     (Chebyshev ring), sat2 as diamonds (L1 ring) — both turn
//     with the global spin because they live in rotated space
//   - every accent gets its OWN palette-driven color that drifts
//     with the flow and crossfades with the scheme: ring, square
//     satellites, diamond satellites, spokes — no more fixed
//     white/silver/slate. Accents are auto-lightened to pop.
//
// Register pantry (callee-saved, stacked in prologue):
//   x19 sintab | x20/x21 palette A/B | w25 fade | w27 flow |
//   w28 256-fade
// FP pantry: s1,s2 sat1 | s3 ring radius | s4/s5 spin cos/sin |
//   s6,s7 sat2 | s16 ring stash | s20 ring color |
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
    // (always scheme 0 = multicolor: guaranteed distinct hues even
    //  when the field scheme is monochrome like gilded/ember)
    lsl     w10, w3, #2                 // accent drift: 4 idx/frame
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s20, w11                    // ring color
    lsl     w10, w3, #2
    add     w10, w10, #8192
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s21, w11                    // sat1 (squares) color
    lsl     w10, w3, #2
    movz    w12, #16384
    add     w10, w10, w12
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s22, w11                    // sat2 (diamonds) color
    lsl     w10, w3, #2
    movz    w12, #24576
    add     w10, w10, w12
    and     w10, w10, #0x7FFF
    bl      Lpalmix
    fmov    s23, w11                    // spoke color

    lsr     w5, w1, #1                  // cx
    lsr     w6, w2, #1                  // cy

    mov     w3, #0                      // y (frame consumed)
Ly_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
Lx_loop:
    cmp     w4, w1
    b.ge    Lnext_row

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
    asr     w10, w10, #1
    add     w10, w10, w27
    and     w10, w10, #0x7FFF

    // ---- two-scheme palette crossfade ----
    ldr     w9,  [x20, w10, uxtw #2]    // cA (zero-extends)
    ldr     w11, [x21, w10, uxtw #2]    // cB
    and     w12, w9,  #0x00FF00FF
    and     w13, w11, #0x00FF00FF
    mul     x12, x12, x28
    mul     x13, x13, x25
    add     x12, x12, x13
    lsr     x12, x12, #8
    and     w12, w12, #0x00FF00FF       // blended R|B
    and     w9,  w9,  #0x0000FF00
    and     w11, w11, #0x0000FF00
    mul     x9,  x9,  x28
    mul     x11, x11, x25
    add     x9,  x9,  x11
    lsr     x9,  x9,  #8
    and     w9,  w9,  #0x0000FF00       // blended G
    orr     w11, w12, w9
    orr     w11, w11, #0xFF000000

    // ---- accents: each element in its own drifting color ----
    fmov    w9, s16
    cmp     w9, #64                     // breathing circle
    b.gt    Lacc_sat1
    fmov    w11, s20
Lacc_sat1:
    cmp     w7, #48                     // square satellites
    b.gt    Lacc_sat2
    fmov    w11, s21
Lacc_sat2:
    cmp     w8, #48                     // diamond satellites
    b.gt    Lacc_spoke
    fmov    w11, s22
Lacc_spoke:
    cmp     w17, #16                    // spokes
    b.gt    Lput_pixel
    fmov    w11, s23

Lput_pixel:
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       Lx_loop

Lnext_row:
    add     w3, w3, #1
    b       Ly_loop

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
    adrp    x13, palette@PAGE           // self-contained base: the
    add     x13, x13, palette@PAGEOFF   //   'mov w9,#256' upstream wipes x9
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

// ============================================================
.section __TEXT,__const
.p2align 2
sintab:
    .incbin "sintab.bin"
.p2align 2
palette:
    .incbin "palette.bin"
