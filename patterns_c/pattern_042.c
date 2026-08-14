/* pattern_042 — BZ Pinwheel
 * Port of lab/patterns/042_bz_pinwheel/proto.py
 * Six-fold folded spiral wavefronts: three cos layers over a kaleidoscopic
 * angle fold, gentle vignette. Pure repaint, phases precomputed per pixel
 * in a 640x480 internal buffer, bilinearly upscaled to the framebuffer. */
#include "../jellydazzle.h"
#include <math.h>

#define P42_LW 640
#define P42_LH 480
#define P42_N  (P42_LW * P42_LH)

static int16_t  p42_sin[4096];          /* Q14 sine, 4096 = full turn */
static uint16_t p42_a1[P42_N];          /* 6*tf + 0.16*r   (angle units) */
static uint16_t p42_a2[P42_N];          /* 10*tf - 0.09*r              */
static uint16_t p42_a3[P42_N];          /* 0.05*r                      */
static uint8_t  p42_vig[P42_N];         /* radial brightness           */
static uint32_t p42_low[P42_N];
static int      p42_ready = 0;

static uint8_t p42_bri[257];            /* wave -> brightness (dark troughs) */

static void p42_init(void) {
    for (int i = 0; i <= 256; i++) {
        float v = (float)i / 256.0f;
        float c = 0.5f + 0.5f * cosf(6.28318531f * (v - 0.15f));
        p42_bri[i] = (uint8_t)(255.0f * (0.42f + 0.58f * c) + 0.5f);
    }
    for (int i = 0; i < 4096; i++)
        p42_sin[i] = (int16_t)lrintf(16383.0f * sinf((float)i * 6.28318531f / 4096.0f));
    const float k = 4096.0f / 6.28318531f;
    for (int y = 0; y < P42_LH; y++) {
        float ly = ((float)y - P42_LH * 0.5f) * 0.5f;   /* lab units (320x240) */
        for (int x = 0; x < P42_LW; x++) {
            float lx = ((float)x - P42_LW * 0.5f) * 0.5f;
            float r = sqrtf(lx * lx + ly * ly);
            float th = atan2f(ly, lx);
            float m = fmodf(th, 1.04719755f);
            if (m < 0.0f) m += 1.04719755f;
            float tf = fabsf(m - 0.52359878f);
            int i = y * P42_LW + x;
            p42_a1[i] = (uint16_t)((int)lrintf((6.0f * tf + 0.16f * r) * k) & 4095);
            p42_a2[i] = (uint16_t)((int)lrintf((10.0f * tf - 0.09f * r) * k + 1.7f * k) & 4095);
            p42_a3[i] = (uint16_t)((int)lrintf((0.05f * r) * k) & 4095);
            float v = 1.0f - (r / 220.0f) * 0.30f;
            if (v < 0.0f) v = 0.0f;
            p42_vig[i] = (uint8_t)(v * 255.0f);
        }
    }
    p42_ready = 1;
}

static inline uint32_t p42_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p42_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P42_LH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fymax = ((long)(P42_LH - 1)) << 16;
        if (fy > fymax) fy = fymax;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P42_LH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p42_low + (long)y0 * P42_LW;
        const uint32_t *r1 = p42_low + (long)y1 * P42_LW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P42_LW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxmax = ((long)(P42_LW - 1)) << 16;
            if (fx > fxmax) fx = fxmax;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P42_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p42_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p42_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p42_lerp(t, b, wy);
        }
    }
}

void pattern_042(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p42_ready) p42_init();
    float t = (float)frame;
    const float k = 4096.0f / 6.28318531f;
    int ph1 = (int)(-0.020f * t * k) & 4095;
    int ph2 = (int)( 0.013f * t * k) & 4095;
    int ph3 = (int)(-0.008f * t * k) & 4095;
    int cq  = 1024;                                   /* cos = sin(+pi/2) */
    int drift = (int)(t * 0.0004f * 32768.0f) + (int)(seed & 32767u);

    for (int i = 0; i < P42_N; i++) {
        int p1 = p42_sin[(p42_a1[i] + ph1 + cq) & 4095];
        int p2 = p42_sin[(p42_a2[i] + ph2 + cq) & 4095];
        int p3 = p42_sin[(p42_a3[i] + ph3 + cq) & 4095];
        int s = (55 * p1 + 30 * p2 + 15 * p3) / 100;  /* Q14, -16384..16384 */
        int v = 8192 + (s >> 1);                      /* Q14 0..16384 */
        int idx = (((v * 9000) >> 14) + drift) & JD_PAL_MASK;
        uint32_t c = pal[idx];
        unsigned g = (p42_vig[i] * p42_bri[v >> 6]) >> 8;
        unsigned rr = ((((c >> 16) & 255) * g) >> 8);
        unsigned gg = ((((c >> 8) & 255) * g) >> 8);
        unsigned bb = (((c & 255) * g) >> 8);
        p42_low[i] = (rr << 16) | (gg << 8) | bb;
    }
    p42_blit(fb, w, h);
}
