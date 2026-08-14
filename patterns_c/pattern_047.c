/* pattern_047 — Target Choir
 * Port of lab/patterns/047_target_choir/proto.py
 * Seven BZ pacemakers (centre + hexagon at r=85) pump target rings that
 * interfere into a rosette lace. All sources share one frequency, so the sum
 * collapses to A*cos(wt) + B*sin(wt) with A,B precomputed per pixel — the
 * frame loop is two multiplies and a palette read. 640x480, bilinear upscale. */
#include "../jellydazzle.h"
#include <math.h>

#define P47_LW 640
#define P47_LH 480
#define P47_N  (P47_LW * P47_LH)

static int16_t  p47_ca[P47_N];    /* Q12 sum of cos(0.21*d + phi)/7 */
static int16_t  p47_sa[P47_N];    /* Q12 sum of sin(0.21*d + phi)/7 */
static uint32_t p47_low[P47_N];
static uint8_t  p47_bri[256];
static int      p47_ready = 0;

static void p47_init(void) {
    float px[7], py[7], ph[7];
    px[0] = 0.0f; py[0] = 0.0f; ph[0] = 0.0f;
    for (int j = 0; j < 6; j++) {
        float a = (float)j * 1.04719755f;
        px[j + 1] = 85.0f * cosf(a);
        py[j + 1] = 85.0f * sinf(a);
        ph[j + 1] = 2.0f;
    }
    for (int y = 0; y < P47_LH; y++) {
        float ly = ((float)y - P47_LH * 0.5f) * 0.5f;
        for (int x = 0; x < P47_LW; x++) {
            float lx = ((float)x - P47_LW * 0.5f) * 0.5f;
            float sc = 0.0f, ss = 0.0f;
            for (int s = 0; s < 7; s++) {
                float dx = lx - px[s], dy = ly - py[s];
                float d = sqrtf(dx * dx + dy * dy);
                float a = d * 0.21f + ph[s];
                sc += cosf(a);
                ss += sinf(a);
            }
            int i = y * P47_LW + x;
            p47_ca[i] = (int16_t)lrintf(sc * (4096.0f / 7.0f));
            p47_sa[i] = (int16_t)lrintf(ss * (4096.0f / 7.0f));
        }
    }
    for (int i = 0; i < 256; i++)
        p47_bri[i] = (uint8_t)(255.0f * (0.45f + 0.55f * ((float)i / 255.0f)));
    p47_ready = 1;
}

static inline uint32_t p47_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p47_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P47_LH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fym = ((long)(P47_LH - 1)) << 16;
        if (fy > fym) fy = fym;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P47_LH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p47_low + (long)y0 * P47_LW;
        const uint32_t *r1 = p47_low + (long)y1 * P47_LW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P47_LW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxm = ((long)(P47_LW - 1)) << 16;
            if (fx > fxm) fx = fxm;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P47_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p47_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p47_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p47_lerp(t, b, wy);
        }
    }
}

void pattern_047(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p47_ready) p47_init();
    float t = (float)frame;
    float th = -0.024f * t;
    int cs = (int)lrintf(cosf(th) * 16384.0f);
    int sn = (int)lrintf(sinf(th) * 16384.0f);
    int drift = (int)(t * 0.0004f * 32768.0f) + (int)(seed & 32767u);

    for (int i = 0; i < P47_N; i++) {
        int wv = ((int)p47_ca[i] * cs + (int)p47_sa[i] * sn) >> 14;   /* Q12 +-4096 */
        int v = 2048 + ((wv * 45) / 100);                             /* Q12 0..4096 */
        if (v < 0) v = 0; else if (v > 4095) v = 4095;
        uint32_t c = pal[(drift + ((v * 7000) >> 12)) & JD_PAL_MASK];
        unsigned g = p47_bri[v >> 4];
        p47_low[i] = ((((((c >> 16) & 255) * g) >> 8) << 16)
                    | (((((c >> 8) & 255) * g) >> 8) << 8)
                    |  (((c & 255) * g) >> 8));
    }
    p47_blit(fb, w, h);
}
