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

    // ---- mode select: the C bridge (bridge.c) rolls the dice
    //      across asm modes + lab patterns and hands us ours ----
    fmov    s28, w3                     // stash TRUE frame (accumulator modes)
    adrp    x9, _g_mode@PAGE
    add     x9, x9, _g_mode@PAGEOFF
    ldr     w12, [x9]
    fmov    s19, w12                    // mode 0..23

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

    // ---- color scheme: RANDOM chained pair (mix-picked; each
    //      fade ends where the next begins — continuous, random) ----
    adrp    x9, palette@PAGE
    add     x9, x9, palette@PAGEOFF
    mov     w13, #30                    // 6 house + 24 downloaded schemes
    lsr     w16, w3, #10                // color leg p
    movz    w10, #0x79B1
    movk    w10, #0x9E37, lsl #16
    mul     w11, w16, w10
    eor     w11, w11, w11, lsr #16
    movz    w10, #0xCA6B
    movk    w10, #0x85EB, lsl #16
    mul     w11, w11, w10
    eor     w11, w11, w11, lsr #13
    udiv    w10, w11, w13
    msub    w11, w10, w13, w11          // A = mix(p) % 6
    add     w16, w16, #1
    movz    w10, #0x79B1
    movk    w10, #0x9E37, lsl #16
    mul     w12, w16, w10
    eor     w12, w12, w12, lsr #16
    movz    w10, #0xCA6B
    movk    w10, #0x85EB, lsl #16
    mul     w12, w12, w10
    eor     w12, w12, w12, lsr #13
    udiv    w10, w12, w13
    msub    w12, w10, w13, w12          // B = mix(p+1): chained
    lsl     w11, w11, #17
    add     x20, x9, x11                // + schemeA*131072
    lsl     w12, w12, #17
    add     x21, x9, x12
    lsr     w10, w3, #10                // recompute pseg for partner pair
    mov     w13, #6
    udiv    w12, w10, w13
    msub    w10, w12, w13, w10
    add     w10, w10, #3                // yin-yang partner: schemes +3
    cmp     w10, #6
    sub     w12, w10, #6
    csel    w10, w12, w10, ge
    add     w11, w10, #1
    cmp     w11, #6
    csel    w11, wzr, w11, eq
    lsl     w10, w10, #17
    add     x22, x9, x10                // opposite pair A
    lsl     w11, w11, #17
    add     x23, x9, x11                // opposite pair B
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
    // modes 0..14: per-pixel combos — transform t = m%3
    // (0 spin, 1 twist, 2 mirror-tile) x field f = m/3
    // (0 calm-interf, 1 rays, 2 moire, 3 corridor, 4 dense-ripple)
    // modes 15..20: accumulators in pairs, s31 = variant bit
    fmov    w9, s19
    cmp     w9, #15
    b.ge    Lacc_dispatch
    mov     w13, #3
    udiv    w10, w9, w13
    msub    w12, w10, w13, w9
    fmov    s29, w12                    // transform
    fmov    w12, s28
    ubfx    w12, w12, #10, #1
    orr     w10, w10, w12, lsl #3       // bit3: yin-yang duo epoch (~17s)
    fmov    s30, w10                    // field | duo
    b       Luy_loop
Lacc_dispatch:
    sub     w9, w9, #15
    cmp     w9, #6
    b.eq    L10start                    // 21: string-art fans
    cmp     w9, #7
    b.eq    L11start                    // 22: vector panels
    cmp     w9, #8
    b.eq    L12start                    // 23: fireworks
    and     w12, w9, #1
    fmov    s31, w12                    // variant
    lsr     w10, w9, #1
    cbz     w10, L5start                // 15/16: spirograph / mirrored
    cmp     w10, #1
    b.eq    L7start                     // 17/18: slash / web
    b       L8start                     // 19/20: curl garden / frost

// ============================================================
// UNIFIED PER-PIXEL ENGINE — transform stage then field stage.
// Transform leaves rx=w11, ry=w12 (signed), fold u=w9, v=w10.
// Field leaves palette index in w10, ring-stash s16 and spoke
// term w17 (0x7FFF = "no accent") for the shared accent pass.
// ============================================================
Luy_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
Lux_loop:
    cmp     w4, w1
    b.ge    Lunext_row
    sub     w7, w4, w5
    sub     w8, w3, w6

    // ---------- transform ----------
    fmov    w13, s29
    cbnz    w13, Lt_nplain
    fmov    w9, s4                      // plain smooth spin
    fmov    w10, s5
    mul     w11, w7, w9
    mul     w12, w8, w10
    sub     w11, w11, w12
    asr     w11, w11, #14
    mul     w12, w7, w10
    madd    w12, w8, w9, w12
    asr     w12, w12, #14
    b       Lt_done
Lt_nplain:
    cmp     w13, #2
    b.eq    Lt_tile
    mul     w13, w7, w7                 // TWIST: radius first
    madd    w13, w8, w8, w13
    lsl     w13, w13, #10
    scvtf   s0, w13
    fsqrt   s0, s0
    fcvtzs  w13, s0
    fmov    w9, s17                     // twistiness
    mul     w9, w13, w9
    asr     w9, w9, #4
    add     w9, w9, w26                 // + smooth spin Q8
    lsr     w9, w9, #8
    and     w9, w9, #255
    ldr     w10, [x19, w9, uxtw #2]
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w9, [x19, w9, uxtw #2]
    mul     w11, w7, w9
    mul     w12, w8, w10
    sub     w11, w11, w12
    asr     w11, w11, #14
    mul     w12, w7, w10
    madd    w12, w8, w9, w12
    asr     w12, w12, #14
    b       Lt_done
Lt_tile:
    fmov    w9, s4                      // MIRROR-TILE: rotate, then fold
    fmov    w10, s5
    mul     w11, w7, w9
    mul     w12, w8, w10
    sub     w11, w11, w12
    asr     w11, w11, #14
    mul     w12, w7, w10
    madd    w12, w8, w9, w12
    asr     w12, w12, #14
    and     w11, w11, #1023
    sub     w11, w11, #512
    and     w12, w12, #1023
    sub     w12, w12, #512
Lt_done:
    eor     w13, w11, w12               // yin-yang side: quadrant parity
    lsr     w13, w13, #31               //   (spiral-interlocked under twist)
    fmov    s31, w13
    cmp     w11, #0
    cneg    w9, w11, lt
    csel    w9, w11, w9, ge             // u = |rx|
    cmp     w12, #0
    cneg    w10, w12, lt
    csel    w10, w12, w10, ge           // v0 = |ry|
    cmp     w9, w10                     // octant fold
    csel    w13, w10, w9, ge
    csel    w9, w9, w10, ge             // u >= v
    mov     w10, w13

    // ---------- field ----------
    fmov    w13, s30
    and     w13, w13, #7                // low bits = field id
    cbz     w13, Lf_calm
    cmp     w13, #1
    b.eq    Lf_rays
    cmp     w13, #2
    b.eq    Lf_moire
    cmp     w13, #3
    b.eq    Lf_corr
    b       Lf_dense

Lf_calm:                                // calm interference + satellites
    lsl     w17, w10, #5                // spoke term (accent live)
    mul     w12, w9, w9
    madd    w12, w10, w10, w12
    lsl     w12, w12, #10
    scvtf   s0, w12
    fsqrt   s0, s0
    fcvtzs  w12, s0
    sub     w12, w12, w15
    cmp     w12, #0
    cneg    w12, w12, lt
    fmov    s16, w12                    // ring stash (accent live)
    sub     w13, w9, w14
    cmp     w13, #0
    cneg    w13, w13, lt
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
    cneg    w7, w7, lt
    fmov    w8, s6
    sub     w8, w9, w8
    fmov    w16, s7
    sub     w16, w10, w16
    mul     w8, w8, w8
    madd    w8, w16, w16, w8
    lsl     w8, w8, #10
    scvtf   s0, w8
    fsqrt   s0, s0
    fcvtzs  w8, s0
    fmov    w16, s3
    sub     w8, w8, w16
    cmp     w8, #0
    cneg    w8, w8, lt
    lsl     w16, w13, #5
    add     w10, w12, w16
    add     w10, w10, w7
    sub     w10, w10, w8
    sub     w10, w10, w17
    asr     w10, w10, #1
    add     w10, w10, w27
    b       Lf_done

Lf_rays:                                // tunnel rays + outward bands
    mul     w12, w9, w9
    madd    w12, w10, w10, w12
    lsl     w12, w12, #10
    scvtf   s0, w12
    fsqrt   s0, s0
    fcvtzs  w12, s0
    lsl     w13, w10, #8
    add     w16, w9, #1
    udiv    w13, w13, w16
    add     w13, w13, w13, lsl #1
    and     w13, w13, #255
    ldr     w13, [x19, w13, uxtw #2]
    asr     w13, w13, #3
    add     w16, w12, w12, lsl #1
    lsr     w16, w16, #1
    sub     w16, w16, w27, lsl #2
    add     w10, w13, w16
    sub     w13, w12, w15
    cmp     w13, #0
    cneg    w13, w13, lt
    fmov    s16, w13                    // breathing ring accent live
    add     w10, w10, w13, asr #1
    movz    w17, #0x7FFF                // no spoke accent
    b       Lf_done

Lf_moire:                               // two-source beat rings
    fmov    w13, s24
    sub     w13, w11, w13
    fmov    w16, s25
    sub     w16, w12, w16
    mul     w13, w13, w13
    madd    w13, w16, w16, w13
    lsl     w13, w13, #10
    scvtf   s0, w13
    fsqrt   s0, s0
    fcvtzs  w13, s0
    fmov    w16, s26
    sub     w16, w11, w16
    fmov    w17, s27
    sub     w17, w12, w17
    mul     w16, w16, w16
    madd    w17, w17, w17, w16
    lsl     w17, w17, #10
    scvtf   s0, w17
    fsqrt   s0, s0
    fcvtzs  w17, s0
    add     w16, w13, w17
    sub     w13, w13, w17
    cmp     w13, #0
    cneg    w13, w13, lt
    add     w10, w16, w13, lsl #1
    asr     w10, w10, #2
    add     w10, w10, w27
    movz    w17, #0x7FFF
    movz    w13, #0x7FFF
    fmov    s16, w13
    b       Lf_done

Lf_corr:                                // echo corridor, inward rush
    lsl     w13, w10, #1
    cmp     w9, w13
    csel    w13, w9, w13, ge
    add     w10, w13, w13, lsl #1
    lsl     w10, w10, #2
    add     w10, w10, w27, lsl #1
    lsr     w12, w12, #1
    add     w12, w12, w27, lsr #6
    and     w12, w12, #255
    ldr     w12, [x19, w12, uxtw #2]
    asr     w12, w12, #6
    add     w10, w10, w12
    movz    w17, #0x7FFF
    movz    w13, #0x7FFF
    fmov    s16, w13
    b       Lf_done

Lf_dense:                               // dense ripple interference
    mul     w12, w9, w9
    madd    w12, w10, w10, w12
    lsl     w12, w12, #10
    scvtf   s0, w12
    fsqrt   s0, s0
    fcvtzs  w12, s0                     // r32
    lsr     w17, w12, #5
    fmov    w13, s18
    add     w17, w17, w13
    and     w17, w17, #255
    ldr     w17, [x19, w17, uxtw #2]
    asr     w16, w17, #4                // ripple seed
    sub     w12, w12, w15
    cmp     w12, #0
    cneg    w12, w12, lt
    fmov    s16, w12                    // ring accent live
    add     w16, w16, w12
    add     w13, w9, w10
    lsl     w13, w13, #5
    sub     w13, w13, w24
    cmp     w13, #0
    cneg    w13, w13, lt
    add     w16, w16, w13
    sub     w13, w9, w14
    cmp     w13, #0
    cneg    w13, w13, lt
    add     w16, w16, w13, lsl #5
    sub     w16, w16, w10, lsl #5
    add     w10, w16, w27
    movz    w17, #0x7FFF
Lf_done:
    and     w10, w10, #0x7FFF

    // ---------- shared palette crossfade (yin-yang aware) ----------
    fmov    w13, s30
    tbz     w13, #3, Lpal_yin           // duo epoch off -> main pair
    fmov    w13, s31
    cbz     w13, Lpal_yin
    ldr     w9,  [x22, w10, uxtw #2]    // yang side: partner schemes
    ldr     w11, [x23, w10, uxtw #2]
    b       Lpal_go
Lpal_yin:
    ldr     w9,  [x20, w10, uxtw #2]
    ldr     w11, [x21, w10, uxtw #2]
Lpal_go:
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

    // ---------- shared accent pass ----------
    fmov    w9, s16
    cmp     w9, #64
    b.gt    Lu_spoke
    fmov    w11, s20
Lu_spoke:
    cmp     w17, #16
    b.gt    Lu_put
    fmov    w11, s23
Lu_put:
    madd    w13, w3, w1, w4
    str     w11, [x0, w13, uxtw #2]
    add     w4, w4, #1
    b       Lux_loop
Lunext_row:
    add     w3, w3, #1
    b       Luy_loop

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
    mov     w27, w5                     // curve center: full for classic,
    mov     w28, w6
    fmov    w9, s31
    cbz     w9, L5cen
    lsr     w27, w27, #1                //   quadrant for mirrored variant
    lsr     w28, w28, #1
    lsr     w24, w24, #1
    lsr     w25, w25, #1
L5cen:
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
    add     w7, w7, w27
    asr     w8, w8, #14
    add     w8, w8, w28
    lsl     w10, w21, #3
    and     w10, w10, #0x7FFF
    bl      Lpalmix                     // jewel color drifts along curve
    mov     w9, w7
    mov     w10, w8
    fmov    w12, s31
    cbz     w12, L5plain
    bl      Lplot22m                    // mirrored variant: 4 quadrants
    b       L5plotted
L5plain:
    bl      Lplot22
L5plotted:
    add     w26, w26, #1
    cmp     w26, #256
    b.lt    L5k
    b       Ldone

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
    mul     w9, w1, w2                  // entry canvas: white / deep ink
    mov     w10, #0
    movz    w11, #0xEEF2
    movk    w11, #0xFFF2, lsl #16
    fmov    w12, s31
    cbz     w12, L7clear
    movz    w11, #0x0A12                // web variant: ink canvas
    movk    w11, #0xFF0A, lsl #16
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
    fmov    w12, s31
    cbz     w12, L7colored
    lsr     w10, w20, #2                // web variant: jewel strands
    and     w10, w10, #0x7FFF
    bl      Lpalmix
L7colored:
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
    fmov    w12, s31
    cbz     w12, L7pl
    bl      Lplot22m                    // web: mirrored strands
    b       L7pld
L7pl:
    bl      Lplot22
L7pld:
    add     w26, w26, #1
    cmp     w26, #340
    b.lt    L7step
    b       Ldone


// ============================================================
// MODE 8 — CURL GARDEN (mirrored accumulator; videos A+B)
// deep-violet canvas; every frame one curling vine-stroke whose
// heading rotates as it walks (spiral curls), stamped 4-way
// mirrored via Lplot22m — the reference's cathedral symmetry.
// ============================================================
L8start:
    fmov    w9, s28
    and     w22, w9, #2047
    cbnz    w22, L8draw
    mul     w9, w1, w2                  // entry canvas: violet / frost white
    mov     w10, #0
    movz    w11, #0x0B2E
    movk    w11, #0xFF1A, lsl #16
    fmov    w12, s31
    cbz     w12, L8clear
    movz    w11, #0xF0F6                // frost variant: pale sky canvas
    movk    w11, #0xFFE8, lsl #16
L8clear:
    str     w11, [x0, w10, uxtw #2]
    add     w10, w10, #1
    cmp     w10, w9
    b.lt    L8clear
L8draw:
    fmov    w9, s28
    movz    w10, #0x79B1
    movk    w10, #0x9E37, lsl #16
    mul     w20, w9, w10                // stroke hash
    ubfx    w9, w20, #7, #10
    mul     w21, w9, w5
    lsr     w21, w21, #10               // start x in left half
    ubfx    w9, w20, #17, #10
    mul     w22, w9, w6
    lsr     w22, w22, #10               // start y in top half
    ubfx    w23, w20, #24, #8
    lsl     w23, w23, #8                // heading, Q8 table idx
    sbfx    w24, w20, #3, #4
    add     w24, w24, w24, lsl #1       // curl rate ~ -24..21 Q8/step
    lsl     w10, w20, #1
    and     w10, w10, #0x7FFF
    bl      Lpalmix                     // vine color: bright jewel
    fmov    w12, s31
    cbz     w12, L8colored
    lsr     w11, w11, #1                // frost: dark branches on pale sky
    movz    w12, #0x7F7F
    movk    w12, #0x007F, lsl #16
    and     w11, w11, w12
    orr     w11, w11, #0xFF000000
L8colored:
    lsl     w21, w21, #4                // Q4 position
    lsl     w22, w22, #4
    mov     w26, #0
L8step:
    lsr     w9, w23, #8
    and     w9, w9, #255
    ldr     w12, [x19, w9, uxtw #2]
    asr     w12, w12, #10               // dx ±16 Q4
    add     w21, w21, w12
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w12, [x19, w9, uxtw #2]
    asr     w12, w12, #10
    add     w22, w22, w12
    add     w23, w23, w24               // heading turns: the curl
    asr     w9, w21, #4
    asr     w10, w22, #4
    bl      Lplot22m                    // 4-way mirrored stamp
    add     w26, w26, #1
    cmp     w26, #300
    b.lt    L8step
    b       Ldone


// ============================================================
// MODE 21 — STRING-ART FANS (video 3: line fans, mirrored)
// two endpoints orbit on ellipses; 3 lines/frame drawn as 96
// lerped stamps, 4-way mirrored. Threads accumulate into fans.
// ============================================================
L10start:
    fmov    w9, s28
    and     w22, w9, #2047
    cbnz    w22, L10draw
    mul     w9, w1, w2
    mov     w10, #0
    movz    w11, #0x0A12
    movk    w11, #0xFF0A, lsl #16
L10clear:
    str     w11, [x0, w10, uxtw #2]
    add     w10, w10, #1
    cmp     w10, w9
    b.lt    L10clear
L10draw:
    mov     w23, #0                     // line index
L10line:
    mov     w10, #140                   // endpoint A sweep
    mul     w9, w22, w10
    mov     w10, #84
    madd    w9, w23, w10, w9
    bl      Lsin16
    mov     w20, w12
    mov     w10, #140
    mul     w9, w22, w10
    mov     w10, #84
    madd    w9, w23, w10, w9
    add     w9, w9, #16384
    bl      Lsin16
    lsr     w10, w1, #2
    mul     w20, w20, w10
    asr     w20, w20, #14
    add     w20, w20, w5                // ax
    lsr     w10, w2, #2
    mul     w12, w12, w10
    asr     w12, w12, #14
    add     w21, w12, w6                // ay
    mov     w10, #89                    // endpoint B counter-sweep
    mul     w9, w22, w10
    mov     w10, #84
    madd    w9, w23, w10, w9
    movz    w10, #21845
    add     w9, w9, w10
    bl      Lsin16
    mov     w24, w12
    mov     w10, #89
    mul     w9, w22, w10
    mov     w10, #84
    madd    w9, w23, w10, w9
    movz    w10, #21845
    add     w9, w9, w10
    add     w9, w9, #16384
    bl      Lsin16
    lsr     w10, w1, #1
    sub     w10, w10, #24
    mul     w24, w24, w10
    asr     w24, w24, #14
    add     w24, w24, w5                // bx (wide orbit)
    lsr     w10, w2, #1
    sub     w10, w10, #24
    mul     w12, w12, w10
    asr     w12, w12, #14
    add     w25, w12, w6                // by
    lsl     w10, w22, #4
    add     w10, w10, w23, lsl #10
    and     w10, w10, #0x7FFF
    bl      Lpalmix                     // thread color
    sub     w12, w24, w20               // line lerp setup, Q8
    lsl     w12, w12, #8
    mov     w13, #96
    sdiv    w16, w12, w13               // x step
    sub     w12, w25, w21
    lsl     w12, w12, #8
    sdiv    w17, w12, w13               // y step
    lsl     w20, w20, #8
    lsl     w21, w21, #8
    mov     w26, #0
L10seg:
    asr     w9, w20, #8
    asr     w10, w21, #8
    bl      Lplot22m                    // mirrored threads
    add     w20, w20, w16
    add     w21, w21, w17
    add     w26, w26, #1
    cmp     w26, #96
    b.lt    L10seg
    add     w23, w23, #1
    cmp     w23, #3
    b.lt    L10line
    b       Ldone

// ============================================================
// MODE 22 — VECTOR PANELS (video 3: bold filled geometry)
// one solid rectangle per frame in a saturated primary, stamped
// into all four mirror quadrants. Hard-edged — on purpose.
// ============================================================
L11start:
    fmov    w9, s28
    and     w22, w9, #2047
    cbnz    w22, L11draw
    mul     w9, w1, w2
    mov     w10, #0
    movz    w11, #0x0508
    movk    w11, #0xFF05, lsl #16
L11clear:
    str     w11, [x0, w10, uxtw #2]
    add     w10, w10, #1
    cmp     w10, w9
    b.lt    L11clear
L11draw:
    fmov    w9, s28
    movz    w10, #0x79B1
    movk    w10, #0x9E37, lsl #16
    mul     w20, w9, w10                // panel hash
    ubfx    w9, w20, #4, #4
    mul     w21, w9, w5
    lsr     w21, w21, #4                // x0 in left half
    ubfx    w9, w20, #12, #4
    mul     w22, w9, w6
    lsr     w22, w22, #4                // y0 in top half
    ubfx    w9, w20, #20, #6
    lsl     w16, w9, #1
    add     w16, w16, #28               // width 28..154
    ubfx    w9, w20, #26, #5
    add     w17, w9, #14                // height 14..45
    ubfx    w9, w20, #9, #2             // solid primary select
    cmp     w9, #0
    b.ne    L11c1
    movz    w11, #0x1818
    movk    w11, #0xFFE0, lsl #16       // red
    b       L11cd
L11c1:
    cmp     w9, #1
    b.ne    L11c2
    movz    w11, #0x30E8
    movk    w11, #0xFF28, lsl #16       // blue
    b       L11cd
L11c2:
    cmp     w9, #2
    b.ne    L11c3
    movz    w11, #0x38D8
    movk    w11, #0xFF90, lsl #16       // violet
    b       L11cd
L11c3:
    movz    w11, #0xE8F0
    movk    w11, #0xFFF0, lsl #16       // white
L11cd:
    mov     w23, w21                    // keep originals
    mov     w24, w22
    bl      Lfillrect                   // (x0, y0)
    sub     w21, w1, w23
    sub     w21, w21, w16               // mirror x
    mov     w22, w24
    bl      Lfillrect
    mov     w21, w23
    sub     w22, w2, w24
    sub     w22, w22, w17               // mirror y
    bl      Lfillrect
    sub     w21, w1, w23
    sub     w21, w21, w16
    sub     w22, w2, w24
    sub     w22, w22, w17
    bl      Lfillrect
    b       Ldone


// ============================================================
// MODE 23 — FIREWORKS (video 4: bursts on hot magenta)
// a new burst every 16 frames: 24 rays stamp their tips at a
// growing radius with gravity droop — the persistent canvas
// turns the tips into falling spark trails.
// ============================================================
L12start:
    fmov    w9, s28
    and     w22, w9, #2047
    cbnz    w22, L12draw
    mul     w9, w1, w2
    mov     w10, #0
    movz    w11, #0x0E86                // deep magenta night
    movk    w11, #0xFF6E, lsl #16
L12clear:
    str     w11, [x0, w10, uxtw #2]
    add     w10, w10, #1
    cmp     w10, w9
    b.lt    L12clear
L12draw:
    and     w24, w22, #15               // burst age 0..15
    fmov    w9, s28
    lsr     w9, w9, #4                  // burst serial
    movz    w10, #0x79B1
    movk    w10, #0x9E37, lsl #16
    mul     w20, w9, w10                // burst hash
    ubfx    w9, w20, #6, #10
    mul     w21, w9, w1
    lsr     w21, w21, #10               // burst x
    ubfx    w9, w20, #16, #10
    mul     w22, w9, w2
    lsr     w22, w22, #10               // burst y
    mov     w9, #7
    mul     w25, w24, w9
    add     w25, w25, #6                // radius grows with age
    lsl     w10, w20, #1
    and     w10, w10, #0x7FFF
    bl      Lpalmix                     // burst color (per burst)
    mov     w23, #0
L12ray:
    lsl     w9, w23, #8
    mov     w10, #24
    udiv    w9, w9, w10
    and     w9, w9, #255
    ldr     w16, [x19, w9, uxtw #2]
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w17, [x19, w9, uxtw #2]
    mul     w9, w16, w25
    asr     w9, w9, #14
    add     w9, w9, w21                 // tip x
    mul     w10, w17, w25
    asr     w10, w10, #14
    add     w10, w10, w22
    mul     w12, w24, w24
    lsr     w12, w12, #2
    add     w10, w10, w12               // tip y + gravity droop
    bl      Lplot22
    add     w23, w23, #1
    cmp     w23, #24
    b.lt    L12ray
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

// ------------------------------------------------------------
// Lplot22m — 4-way mirrored 2x2 stamp. in: w9=x, w10=y, w11=color.
// Stamps (x,y), (w-2-x,y), (x,h-2-y), (w-2-x,h-2-y).
// clobbers w12, w13. Preserves w11.
// ------------------------------------------------------------
Lplot22m:
    stp     x29, x30, [sp, #-16]!
    sub     sp, sp, #16
    str     w9,  [sp]
    str     w10, [sp, #8]
    bl      Lplot22
    sub     w9, w1, #2
    ldr     w12, [sp]
    sub     w9, w9, w12
    ldr     w10, [sp, #8]
    bl      Lplot22
    ldr     w9, [sp]
    sub     w10, w2, #2
    ldr     w12, [sp, #8]
    sub     w10, w10, w12
    bl      Lplot22
    sub     w9, w1, #2
    ldr     w12, [sp]
    sub     w9, w9, w12
    sub     w10, w2, #2
    ldr     w12, [sp, #8]
    sub     w10, w10, w12
    bl      Lplot22
    add     sp, sp, #16
    ldp     x29, x30, [sp], #16
    ret

// ------------------------------------------------------------
// Lfillrect — clipped solid fill. in: w21=x0, w22=y0, w16=wdt,
// w17=hgt, w11=color. clobbers w9, w10, w12, w13, w25
// (w25 is fade -- recomputed every frame; accumulator-safe only).
// ------------------------------------------------------------
Lfillrect:
    mov     w9, w22                     // row
    cmp     w9, #0
    csel    w9, wzr, w9, lt
    add     w25, w22, w17               // row limit (w25: accum-safe)
    cmp     w25, w2
    csel    w25, w2, w25, gt
Lfr_row:
    cmp     w9, w25
    b.ge    Lfr_ret
    mov     w10, w21                    // col
    cmp     w10, #0
    csel    w10, wzr, w10, lt
    add     w12, w21, w16               // col limit
    cmp     w12, w1
    csel    w12, w1, w12, gt
    madd    w13, w9, w1, w10            // base idx = row*w + colstart
Lfr_col:
    cmp     w10, w12
    b.ge    Lfr_nextrow
    str     w11, [x0, w13, uxtw #2]
    add     w13, w13, #1
    add     w10, w10, #1
    b       Lfr_col
Lfr_nextrow:
    add     w9, w9, #1
    b       Lfr_row
Lfr_ret:
    ret

// ============================================================
.section __TEXT,__const
.p2align 2
sintab:
    .incbin "sintab.bin"
.p2align 2
.globl _jd_palette
_jd_palette:
palette:
    .incbin "palette.bin"
