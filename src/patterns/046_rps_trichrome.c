/* pattern_046 — RPS Trichrome
 * Port of lab/patterns/046_rps_trichrome/proto.py
 * Three phase-shifted wave fields over a 6-fold angle fold chase each other;
 * a soft winner-take-all (margin falloff instead of exp/div) paints three
 * palette-separated species colors with soft ink-front borders and relief.
 * Repaint, 640x480 internal buffer, bilinear upscale. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P46_LW 640
#define P46_LH 480
#define P46_N  (P46_LW * P46_LH)

static int16_t  p46_sin[4096];
static uint16_t p46_pa[P46_N];      /* 7*tf + 0.14*r  (angle units) */
static uint16_t p46_pb[P46_N];      /* 0.06*r                       */
static uint32_t p46_low[P46_N];
static uint8_t  p46_mix[256];       /* softmax-ish margin falloff   */
static int      p46_ready = 0;

static void p46_init(void) {
    for (int i = 0; i < 4096; i++)
        p46_sin[i] = (int16_t)lrintf(16383.0f * sinf((float)i * 6.28318531f / 4096.0f));
    for (int i = 0; i < 256; i++) {
        float m = (float)i * (1.6f / 256.0f);
        p46_mix[i] = (uint8_t)(255.0f / (1.0f + expf(2.2f * m)) + 0.5f);
    }
    const float k = 4096.0f / 6.28318531f;
    for (int y = 0; y < P46_LH; y++) {
        float ly = ((float)y - P46_LH * 0.5f) * 0.5f;
        for (int x = 0; x < P46_LW; x++) {
            float lx = ((float)x - P46_LW * 0.5f) * 0.5f;
            float r = sqrtf(lx * lx + ly * ly);
            float th = atan2f(ly, lx);
            float m = fmodf(th, 1.04719755f);
            if (m < 0.0f) m += 1.04719755f;
            float tf = fabsf(m - 0.52359878f);
            int i = y * P46_LW + x;
            p46_pa[i] = (uint16_t)((int)lrintf((7.0f * tf + 0.14f * r) * k) & 4095);
            p46_pb[i] = (uint16_t)((int)lrintf((0.06f * r) * k) & 4095);
        }
    }
    p46_ready = 1;
}

static inline uint32_t p46_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p46_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P46_LH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fym = ((long)(P46_LH - 1)) << 16;
        if (fy > fym) fy = fym;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P46_LH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p46_low + (long)y0 * P46_LW;
        const uint32_t *r1 = p46_low + (long)y1 * P46_LW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P46_LW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxm = ((long)(P46_LW - 1)) << 16;
            if (fx > fxm) fx = fxm;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P46_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p46_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p46_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p46_lerp(t, b, wy);
        }
    }
}

void pattern_046(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p46_ready) p46_init();
    float t = (float)frame;
    const float k = 4096.0f / 6.28318531f;
    int base_a = ((int)(-0.006f * t * k) + 1024) & 4095;   /* +1024 -> cos */
    int base_b = ((int)(-0.0035f * t * k) + 1024) & 4095;
    int off_a[3], off_b[3];
    uint32_t col[3];
    int drift = (int)(t * 0.00018f * 32768.0f) + (int)(seed & 32767u);
    for (int i = 0; i < 3; i++) {
        off_a[i] = (base_a + (int)(i * 4096 / 3)) & 4095;   /* 2*pi*i/3 */
        off_b[i] = (base_b + (int)(i * 4096 / 6)) & 4095;   /* pi*i/3   */
        col[i] = pal[(drift + i * 10923) & JD_PAL_MASK];
    }

    for (int i = 0; i < P46_N; i++) {
        int pa = p46_pa[i], pb = p46_pb[i];
        int f0 = p46_sin[(pa + off_a[0]) & 4095] + ((p46_sin[(pb + off_b[0]) & 4095] * 45) >> 7);
        int f1 = p46_sin[(pa + off_a[1]) & 4095] + ((p46_sin[(pb + off_b[1]) & 4095] * 45) >> 7);
        int f2 = p46_sin[(pa + off_a[2]) & 4095] + ((p46_sin[(pb + off_b[2]) & 4095] * 45) >> 7);
        int wi, top, sec;
        if (f0 >= f1) {
            if (f0 >= f2) { wi = 0; top = f0; sec = f1 > f2 ? f1 : f2; }
            else          { wi = 2; top = f2; sec = f0; }
        } else {
            if (f1 >= f2) { wi = 1; top = f1; sec = f0 > f2 ? f0 : f2; }
            else          { wi = 2; top = f2; sec = f1; }
        }
        int si = (wi == 0) ? ((f1 >= f2) ? 1 : 2)
               : (wi == 1) ? ((f0 >= f2) ? 0 : 2)
                           : ((f0 >= f1) ? 0 : 1);
        int marg = (top - sec) >> 6;                  /* Q14 -> 0..~400 */
        if (marg > 255) marg = 255;
        uint32_t c = p46_lerp(col[wi], col[si], p46_mix[marg]);
        int rel = 174 + ((top * 82) >> 14);           /* 0.68 + 0.32*(.5+.5top) */
        if (rel < 0) rel = 0; else if (rel > 255) rel = 255;
        p46_low[i] = ((((((c >> 16) & 255) * rel) >> 8) << 16)
                    | (((((c >> 8) & 255) * rel) >> 8) << 8)
                    |  (((c & 255) * rel) >> 8));
    }
    p46_blit(fb, w, h);
}
