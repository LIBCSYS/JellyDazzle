// ============================================================
// draw.s — ARM64 (Apple Silicon), step 8: DAZZLE MODE
//
// void draw_frame(uint32_t *fb, int width, int height, int frame);
//
// Built from frame-analysis of the real dazzle.exe reference:
//
//  1. THE TWIST — dazzle's signature. Rotation angle now grows
//     with radius: angle = spin + r * twistiness. The center
//     turns faster than the rim, shearing every shape into
//     spiral pinwheel arms. Implementation: radius is rotation-
//     invariant, so we sqrt FIRST, use r to look up sin/cos
//     from the table PER PIXEL, then rotate, then fold.
//     Twistiness itself breathes on a slow sine, so the arms
//     wind tight and unwind loose over ~2 minutes.
//
//  2. RIPPLES — a sine of the radius added to the color index:
//     concentric rings crawl outward through every band,
//     making the moiré "eye" patterns from the reference.
//
//  3. DENSITY — the /2 tamer on the interference mix is gone.
//     Bands are tight and busy, like the original.
//
//  4. VGA PALETTE — regenerated: hard-edged, fully saturated
//     primaries with white filaments (see generator).
//
// New tricks for the toolbox:
//   - per-pixel sine-table lookups (3 loads/pixel: sin, cos,
//     ripple) — the table base address lives in d20, because
//     fmov moves 64-bit x-registers into d-registers too.
//     (s20 is d20's low half — that's why stashes use s21+.)
//
// FP pantry: s1,s2 sat1 | s3 ring | s4 twist-spin | s5 twist
//   scale | s6,s7 sat2 | s16 D32 | s17 ripple phase
//   s18 diamond stash | s19 spoke stash | s21 ripple stash
//   d20 sintab address
// ============================================================

.global _draw_frame
.p2align 2

_draw_frame:
    adrp    x11, sintab@PAGE
    add     x11, x11, sintab@PAGEOFF
    fmov    d20, x11                    // table base -> FP pantry

    // ---- sine-eased pulse: ~68s, deep amplitude ----
    ubfx    w9, w3, #4, #8
    ldr     w12, [x11, w9, uxtw #2]
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w13, [x11, w9, uxtw #2]

    cmp     w1, w2
    csel    w10, w1, w2, lt
    add     w10, w10, w10, lsl #1
    lsr     w10, w10, #3                // base = min*3/8
    add     w9, w10, w10, lsl #1
    lsr     w9, w9, #2                  // amp = base*3/4
    mul     w12, w12, w9
    asr     w12, w12, #14               // osc
    mul     w13, w13, w9
    asr     w13, w13, #14               // oscQ

    add     w11, w10, w12
    lsl     w15, w11, #5                // R32
    sub     w14, w10, w12               // S
    add     w11, w10, w13
    lsl     w11, w11, #5
    fmov    s16, w11                    // D32

    lsl     w12, w10, #2
    fmov    s3, w12                     // satellite ring radius

    // ---- twist controls ----
    ubfx    w9, w3, #2, #8              // twist spin: ~17s/rev
    fmov    s4, w9
    ubfx    w9, w3, #7, #8              // twistiness clock: ~2min
    adrp    x11, sintab@PAGE
    add     x11, x11, sintab@PAGEOFF
    ldr     w12, [x11, w9, uxtw #2]     // -16384..16384
    asr     w12, w12, #11               // -8..8
    add     w12, w12, #10               // twistiness 2..18
    fmov    s5, w12

    // ---- ripple phase: crawls at half frame rate ----
    lsr     w9, w3, #1
    fmov    s17, w9

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
    // angle = spin + r * twistiness -> per-pixel sin/cos lookup
    fmov    w9, s5                      // twistiness
    mul     w9, w12, w9
    asr     w9, w9, #12
    fmov    w10, s4                     // spin
    add     w9, w9, w10
    and     w9, w9, #255
    fmov    x13, d20                    // sintab
    ldr     w10, [x13, w9, uxtw #2]     // sin(angle)
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w9, [x13, w9, uxtw #2]      // cos(angle)
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
    csel    w9, w11, w13, ge            // u
    csel    w10, w13, w11, ge           // v

    // ---- spoke + diamond stashes (r32 untouched) ----
    lsl     w11, w10, #5
    fmov    s19, w11                    // dsp32
    add     w11, w9, w10
    lsl     w11, w11, #5
    fmov    w13, s16
    sub     w11, w11, w13
    cmp     w11, #0
    cneg    w11, w11, lt
    fmov    s18, w11                    // dd32

    // ---- RIPPLE: sin(r) joins the color mix ----
    lsr     w11, w12, #5
    fmov    w13, s17
    add     w11, w11, w13
    and     w11, w11, #255
    fmov    x13, d20
    ldr     w11, [x13, w11, uxtw #2]    // -16384..16384
    asr     w11, w11, #4                // -1024..1024
    fmov    s21, w11

    // ---- circle field (consumes r32) ----
    sub     w12, w12, w15
    cmp     w12, #0
    cneg    w12, w12, lt                // dc32

    // ---- square field ----
    sub     w13, w9, w14
    cmp     w13, #0
    cneg    w13, w13, lt                // ds

    // ---- satellite fields ----
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

    // ======== DENSE INTERFERENCE MIX + RIPPLE ========
    lsl     w9, w13, #5
    add     w10, w12, w9
    fmov    w11, s18
    add     w10, w10, w11
    add     w10, w10, w7
    sub     w10, w10, w8
    fmov    w11, s19
    sub     w10, w10, w11
    fmov    w11, s21
    add     w10, w10, w11               // + ripple rings
    add     w10, w10, w16               // (no /2: full density)
    and     w10, w10, #0x7FFF
    ldr     w11, [x17, w10, uxtw #2]
    // =================================================

    cmp     w13, #1                     // square (now a pinwheel!)
    b.gt    Lcirc_line
    movz    w11, #0xD24A
    movk    w11, #0xFFFF, lsl #16
Lcirc_line:
    cmp     w12, #64
    b.gt    Ldia_line
    movz    w11, #0xE8FF
    movk    w11, #0xFFB0, lsl #16
Ldia_line:
    fmov    w9, s18
    cmp     w9, #64
    b.gt    Lsat1_line
    movz    w11, #0x44FF
    movk    w11, #0xFFCC, lsl #16
Lsat1_line:
    cmp     w7, #48
    b.gt    Lsat2_line
    movz    w11, #0xE8F0
    movk    w11, #0xFFE8, lsl #16
Lsat2_line:
    cmp     w8, #48
    b.gt    Lspoke_line
    movz    w11, #0xE8F0
    movk    w11, #0xFFE8, lsl #16
Lspoke_line:
    fmov    w9, s19
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
