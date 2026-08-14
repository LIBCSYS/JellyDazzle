// ============================================================
// draw.s — ARM64 (Apple Silicon) — the good build, restored
//
// void draw_frame(uint32_t *fb, int width, int height, int frame);
//
// Sine-eased deep pulse (~68s, amplitude 3/4 of base), rigid
// slow rotation (~68s, offset from pulse), circle + square +
// quarter-phase diamond + 8-spoke star + two counter-orbiting
// satellites, smooth six-field interference at calm density,
// materials palette (jewels -> metallics -> pastels).
//
// FP pantry: s1,s2 sat1 | s3 ring | s4/s5 spin cos/sin |
//   s6,s7 sat2 | s16 D32 | s18/s19 per-pixel stash
// ============================================================

.global _draw_frame
.p2align 2

_draw_frame:
    adrp    x11, sintab@PAGE
    add     x11, x11, sintab@PAGEOFF

    // ---- sine-eased pulse: idx = (frame>>4)&255, ~68s ----
    ubfx    w9, w3, #4, #8
    ldr     w12, [x11, w9, uxtw #2]     // sin -> main osc
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w13, [x11, w9, uxtw #2]     // cos -> quarter-phase osc

    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #3                // base = min*3/8
    add     w9, w10, w10, lsl #1
    lsr     w9, w9, #2                  // amp = base*3/4  (DEEP)
    mul     w12, w12, w9
    asr     w12, w12, #14               // osc  = sin * amp
    mul     w13, w13, w9
    asr     w13, w13, #14               // oscQ = cos * amp

    add     w9, w10, w12                // R = base + osc   (w9, NOT w11 —
    lsl     w15, w9, #5                 // R32               x11 still holds sintab!)
    sub     w14, w10, w12               // S = base - osc
    add     w9, w10, w13                // D = base + oscQ
    lsl     w9, w9, #5
    fmov    s16, w9                     // D32 (diamond)

    lsl     w12, w10, #2
    fmov    s3, w12                     // satellite ring radius

    // ---- spin: ~68s, offset +85 so it never syncs w/ pulse ----
    ubfx    w9, w3, #4, #8
    add     w9, w9, #85
    and     w9, w9, #255
    ldr     w12, [x11, w9, uxtw #2]
    fmov    s5, w12                     // sin
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w12, [x11, w9, uxtw #2]
    fmov    s4, w12                     // cos

    // ---- satellite 1: orbit=base, ~34s ----
    ubfx    w9, w3, #3, #8
    ldr     w12, [x11, w9, uxtw #2]
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w9, [x11, w9, uxtw #2]
    mul     w12, w12, w10
    asr     w12, w12, #14
    mul     w9, w9, w10
    asr     w9, w9, #14
    cmp     w9, #0
    cneg    w9, w9, lt
    cmp     w12, #0
    cneg    w12, w12, lt
    cmp     w9, w12
    csel    w13, w9, w12, ge
    csel    w12, w12, w9, ge
    fmov    s1, w13
    fmov    s2, w12

    // ---- satellite 2: orbit=base/2, counter, ~17s ----
    lsr     w9, w3, #2
    neg     w9, w9
    and     w9, w9, #255
    ldr     w12, [x11, w9, uxtw #2]
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w9, [x11, w9, uxtw #2]
    lsr     w13, w10, #1
    mul     w12, w12, w13
    asr     w12, w12, #14
    mul     w9, w9, w13
    asr     w9, w9, #14
    cmp     w9, #0
    cneg    w9, w9, lt
    cmp     w12, #0
    cneg    w12, w12, lt
    cmp     w9, w12
    csel    w13, w9, w12, ge
    csel    w12, w12, w9, ge
    fmov    s6, w13
    fmov    s7, w12

    lsl     w16, w3, #5                 // color flow phase

    adrp    x17, palette@PAGE
    add     x17, x17, palette@PAGEOFF

    lsr     w5, w1, #1
    lsr     w6, w2, #1

    mov     w3, #0
Ly_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
Lx_loop:
    cmp     w4, w1
    b.ge    Lnext_row

    // ============ ROTATE, THEN FOLD ============
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
    cmp     w11, #0
    cneg    w11, w11, lt
    cmp     w12, #0
    cneg    w12, w12, lt
    cmp     w11, w12
    csel    w9, w11, w12, ge            // u
    csel    w10, w12, w11, ge           // v
    // ===========================================

    // ---- spokes: distance is simply v. Stash. ----
    lsl     w11, w10, #5
    fmov    s19, w11

    // ---- diamond: |32*(u+v) - D32|. Stash. ----
    add     w11, w9, w10
    lsl     w11, w11, #5
    fmov    w12, s16
    sub     w11, w11, w12
    cmp     w11, #0
    cneg    w11, w11, lt
    fmov    s18, w11

    // ---- circle field dc32 ----
    mul     w12, w9, w9
    madd    w12, w10, w10, w12
    lsl     w12, w12, #10
    scvtf   s0, w12
    fsqrt   s0, s0
    fcvtzs  w12, s0
    sub     w12, w12, w15
    cmp     w12, #0
    cneg    w12, w12, lt                // dc32

    // ---- square field ds (int px) ----
    sub     w13, w9, w14
    cmp     w13, #0
    cneg    w13, w13, lt                // ds

    // ---- satellite 1 field ----
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
    cneg    w7, w7, lt                  // dsat1_32

    // ---- satellite 2 field ----
    fmov    w8, s6
    sub     w8, w9, w8
    fmov    w11, s7
    sub     w11, w10, w11
    mul     w8, w8, w8
    madd    w8, w11, w11, w8
    lsl     w8, w8, #10
    scvtf   s0, w8
    fsqrt   s0, s0
    fcvtzs  w8, s0
    fmov    w11, s3
    sub     w8, w8, w11
    cmp     w8, #0
    cneg    w8, w8, lt                  // dsat2_32

    // ======== SMOOTH INTERFERENCE: six fields, calm ========
    lsl     w9, w13, #5
    add     w10, w12, w9
    fmov    w11, s18
    add     w10, w10, w11
    add     w10, w10, w7
    sub     w10, w10, w8
    fmov    w11, s19
    sub     w10, w10, w11
    asr     w10, w10, #1                // calm band density
    add     w10, w10, w16
    and     w10, w10, #0x7FFF
    ldr     w11, [x17, w10, uxtw #2]
    // =======================================================

    cmp     w13, #1                     // square: gold
    b.gt    Lcirc_line
    movz    w11, #0xD24A
    movk    w11, #0xFFFF, lsl #16
Lcirc_line:
    cmp     w12, #64                    // circle: ice
    b.gt    Ldia_line
    movz    w11, #0xE8FF
    movk    w11, #0xFFB0, lsl #16
Ldia_line:
    fmov    w9, s18                     // diamond: violet
    cmp     w9, #64
    b.gt    Lsat1_line
    movz    w11, #0x44FF
    movk    w11, #0xFFCC, lsl #16
Lsat1_line:
    cmp     w7, #48                     // satellites: silver
    b.gt    Lsat2_line
    movz    w11, #0xE8F0
    movk    w11, #0xFFE8, lsl #16
Lsat2_line:
    cmp     w8, #48
    b.gt    Lspoke_line
    movz    w11, #0xE8F0
    movk    w11, #0xFFE8, lsl #16
Lspoke_line:
    fmov    w9, s19                     // spokes: quiet slate
    cmp     w9, #16
    b.gt    Lput_pixel
    movz    w11, #0x7788
    movk    w11, #0xFF66, lsl #16

Lput_pixel:
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       Lx_loop

Lnext_row:
    add     w3, w3, #1
    b       Ly_loop
Ldone:
    ret

// ============================================================
.section __TEXT,__const
.p2align 2
sintab:
    .incbin "sintab.bin"
.p2align 2
palette:
    .incbin "palette.bin"
