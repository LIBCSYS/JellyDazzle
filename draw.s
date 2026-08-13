// ============================================================
// draw.s — ARM64 (Apple Silicon), step 5: THE KALEIDOSCOPE
//
// void draw_frame(uint32_t *fb, int width, int height, int frame);
//
// New machinery this step:
//
//  1. THE OCTANT FOLD — the actual mirrors. For each pixel:
//         u = |dx|, v = |dy|, then swap so u >= v.
//     Four instructions, and every pixel is mapped into ONE
//     45-degree wedge. Whatever we draw in that wedge appears
//     8 times, mirrored, on screen. This is the entire secret
//     of a kaleidoscope: don't draw 8 copies — fold the
//     LOOKUP so 8 screen regions share one pattern space.
//
//  2. THE SATELLITE — a small ring orbiting at radius=base,
//     positioned once per frame from a 256-entry Q14 sine
//     table (the classic demo sine table, at last). Because
//     its distance field is computed in FOLDED coordinates,
//     one satellite renders as 8, and as it orbits it sweeps
//     across the mirror lines and kisses its own reflections.
//
//  3. FP REGISTERS AS A SPARE POCKET — we ran out of the 18
//     usable integer registers (x18 is Apple-reserved!), so
//     the satellite's px, py, ring-radius live in s1/s2/s3.
//     fmov between w and s registers is a raw bit copy, ~free.
//
//  4. MATERIALS PALETTE — 32K entries sweeping deep jewels ->
//     metallic gold/copper/silver (high-frequency brightness
//     shimmer = the anodized look) -> pastels. Halo colors now
//     read as depth: near a line you're in deep jewel tones,
//     farther out you pass through metal into pastel mist.
//
// A nice consequence of the fold: the square test collapsed to
// |u - S| because after folding, max(|dx|,|dy|) is just u.
//
// Register map:
//   persistent: w5=cx w6=cy w14=S w15=R32 w16=phase x17=palette
//               s1=sat px  s2=sat py  s3=sat ring radius (1/32px)
//   loop:       w3=y w4=x  w7..w13 scratch  s0=sqrt scratch
// ============================================================

.global _draw_frame
.p2align 2

_draw_frame:
    // ---- triangle oscillator, period 1024 frames (~17s) ----
    and     w9, w3, #1023
    mov     w10, #512
    sub     w9, w10, w9
    cmp     w9, #0
    cneg    w9, w9, lt
    sub     w9, w9, #256        // tri (-256..+256)

    // ---- geometry: bigger this time. base = min(w,h)*5/16 ----
    cmp     w1, w2
    csel    w10, w1, w2, lt     // min(width,height)
    add     w10, w10, w10, lsl #1   // min*3
    lsr     w10, w10, #3        // base = min*3/8
    lsr     w12, w10, #2        // amp  = base/4
    mul     w9, w9, w12
    asr     w9, w9, #8          // osc (-amp..+amp)

    add     w11, w10, w9        // R = base + osc
    lsl     w15, w11, #5        // R32
    sub     w14, w10, w9        // S = base - osc

    // ---- satellite ring radius: base/8, in 1/32px -> s3 ----
    lsl     w12, w10, #2        // (base>>3)<<5 == base<<2
    fmov    s3, w12

    // ---- satellite orbit position from the sine table ----
    ubfx    w9, w3, #2, #8      // idx = (frame>>2) & 255, ~17s/orbit
    adrp    x11, sintab@PAGE
    add     x11, x11, sintab@PAGEOFF
    ldr     w12, [x11, w9, uxtw #2]     // sin, Q14
    add     w9, w9, #64
    and     w9, w9, #255
    ldr     w9, [x11, w9, uxtw #2]      // cos = sin(idx+64)
    mul     w12, w12, w10       // sin * orbit(=base)
    asr     w12, w12, #14       // py0
    mul     w9, w9, w10
    asr     w9, w9, #14         // px0
    // fold the satellite's CENTER into the octant too
    cmp     w9, #0
    cneg    w9, w9, lt
    cmp     w12, #0
    cneg    w12, w12, lt
    cmp     w9, w12
    csel    w13, w9, w12, ge    // px = max
    csel    w12, w12, w9, ge    // py = min
    fmov    s1, w13
    fmov    s2, w12

    lsl     w16, w3, #7         // palette flow phase

    adrp    x17, palette@PAGE
    add     x17, x17, palette@PAGEOFF

    lsr     w5, w1, #1          // cx
    lsr     w6, w2, #1          // cy

    mov     w3, #0
Ly_loop:
    cmp     w3, w2
    b.ge    Ldone
    mov     w4, #0
Lx_loop:
    cmp     w4, w1
    b.ge    Lnext_row

    // ================= THE FOLD =================
    sub     w7, w4, w5          // dx
    sub     w8, w3, w6          // dy
    cmp     w7, #0
    cneg    w7, w7, lt          // |dx|
    cmp     w8, #0
    cneg    w8, w8, lt          // |dy|
    cmp     w7, w8
    csel    w9, w7, w8, ge      // u = max  } one wedge,
    csel    w10, w8, w7, ge     // v = min  } mirrored x8
    // ============================================

    // ---- dc32 = |32*sqrt(u^2+v^2) - R32| (circle) ----
    mul     w12, w9, w9
    madd    w12, w10, w10, w12
    lsl     w12, w12, #10       // *1024 -> sqrt lands in 1/32px
    scvtf   s0, w12
    fsqrt   s0, s0
    fcvtzs  w12, s0
    sub     w12, w12, w15
    cmp     w12, #0
    cneg    w12, w12, lt        // dc32

    // ---- ds = |u - S| (square; fold made this one-liner) ----
    sub     w13, w9, w14
    cmp     w13, #0
    cneg    w13, w13, lt        // ds (integer px)

    // ---- dsat32: distance to satellite ring, folded space ----
    fmov    w11, s1
    sub     w11, w9, w11        // du = u - px
    fmov    w7, s2
    sub     w7, w10, w7         // dv = v - py
    mul     w11, w11, w11
    madd    w11, w7, w7, w11
    lsl     w11, w11, #10
    scvtf   s0, w11
    fsqrt   s0, s0
    fcvtzs  w11, s0             // 32 * dist to satellite center
    fmov    w7, s3
    sub     w11, w11, w7        // - ring radius
    cmp     w11, #0
    cneg    w8, w11, lt         // dsat32 -> w8

    // ---- d32 = min(dc32, ds32, dsat32) ----
    lsl     w9, w13, #5         // ds32
    cmp     w12, w9
    csel    w10, w12, w9, lt
    cmp     w8, w10
    csel    w9, w8, w10, lt     // d32

    // ---- full-window materials field: the darkness is gone.
    //      Every pixel takes a palette color from its distance
    //      to the nearest line; the gradient fills the window.
    lsl     w10, w9, #2         // d32*4 -> full palette per ~256px
    add     w10, w10, w16
    and     w10, w10, #0x7FFF
    ldr     w11, [x17, w10, uxtw #2]

Lsq_line:
    cmp     w13, #1             // square: burnished gold
    b.gt    Lcirc_line
    movz    w11, #0xD24A
    movk    w11, #0xFFFF, lsl #16
Lcirc_line:
    cmp     w12, #64            // circle: ice blue-white
    b.gt    Lsat_line
    movz    w11, #0xE8FF
    movk    w11, #0xFFB0, lsl #16
Lsat_line:
    cmp     w8, #48             // satellites: bright silver
    b.gt    Lput_pixel
    movz    w11, #0xE8F0
    movk    w11, #0xFFE8, lsl #16

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
// Q14 sine table, 256 entries (value = sin * 16384)
// ============================================================
// ============================================================
// Tables live in binary files now — .incbin pulls raw bytes
// straight into the section at assembly time. No more 4,000
// lines of hex; regenerate the .bin files to change the look.
// ============================================================
.section __TEXT,__const
.p2align 2
sintab:
    .incbin "sintab.bin"        // 256 x int32, sin*16384 (Q14)

.p2align 2
palette:
    .incbin "palette.bin"       // 32768 x ARGB8888 materials sweep
