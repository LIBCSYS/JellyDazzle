/* 001 kaleido_rose — dihedral kaleidoscope, fold count 4..16, rose field. */
#include "../jellydazzle.h"
#include <math.h>

#define P1_IW 320
#define P1_IH 240

static float p1_th[P1_IW * P1_IH];
static float p1_r[P1_IW * P1_IH];
static uint16_t p1_field[P1_IW * P1_IH];
static int p1_init;

static float p1_fieldval(float thf, int N, float t, float r) {
    float rose = r - 78.0f * fabsf(cosf(3.0f * thf * (float)N * 0.25f + t * 0.006f));
    return 0.50f
         + 0.28f * sinf(r * 0.055f - t * 0.012f + 2.5f * cosf(thf * 6.0f))
         + 0.22f * sinf(rose * 0.09f + t * 0.004f);
}

static void p1_upsample(uint32_t *fb, int w, int h, const uint16_t *f,
                        const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((P1_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((P1_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *r0 = f + iy * P1_IW, *r1 = r0 + P1_IW;
        uint32_t *dst = fb + (long)y * w;
        uint32_t ax = 0;
        for (int x = 0; x < w; x++) {
            int ix = (int)(ax >> 16), fx = (int)((ax >> 8) & 255);
            int a = r0[ix], b = r0[ix + 1], c = r1[ix], d = r1[ix + 1];
            int top = a + (((b - a) * fx) >> 8);
            int bot = c + (((d - c) * fx) >> 8);
            int v = top + (((bot - top) * fy) >> 8);
            dst[x] = pal[((uint32_t)v + off) & JD_PAL_MASK];
            ax += sx;
        }
    }
}

void pattern_001(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl; (void)seed;
    static const int seq[10] = {4, 6, 8, 10, 12, 14, 16, 12, 8, 6};
    if (!p1_init) {
        p1_init = 1;
        for (int y = 0; y < P1_IH; y++)
            for (int x = 0; x < P1_IW; x++) {
                float X = (float)x - P1_IW * 0.5f, Y = (float)y - P1_IH * 0.5f;
                p1_th[y * P1_IW + x] = atan2f(Y, X);
                p1_r[y * P1_IW + x] = sqrtf(X * X + Y * Y);
            }
    }
    float t = (float)frame;
    int i = (frame / 240) % 10, j = (i + 1) % 10;
    float fph = (float)(frame % 240) / 240.0f;
    float m = (fph - 0.7f) / 0.3f;
    if (m < 0.0f) m = 0.0f;
    if (m > 1.0f) m = 1.0f;
    m = m * m * (3.0f - 2.0f * m);
    float rot = t * 0.0025f;
    int Na = seq[i], Nb = seq[j];
    float wa = (float)M_PI / (float)Na, wb = (float)M_PI / (float)Nb;
    float inv2wa = 1.0f / (2.0f * wa), inv2wb = 1.0f / (2.0f * wb);
    for (int k = 0; k < P1_IW * P1_IH; k++) {
        float th = p1_th[k] + rot, r = p1_r[k];
        float ta = th - floorf(th * inv2wa) * 2.0f * wa;
        float thf = fabsf(ta - wa);
        float v = p1_fieldval(thf, Na, t, r);
        if (m > 0.0f) {
            float tb = th - floorf(th * inv2wb) * 2.0f * wb;
            float thf2 = fabsf(tb - wb);
            float v2 = p1_fieldval(thf2, Nb, t, r);
            v = v * (1.0f - m) + v2 * m;
        }
        int q = (int)(v * 3481.6f + 8192.0f);   /* v*0.85*4096, biased */
        if (q < 0) q = 0;
        if (q > 65535) q = 65535;
        p1_field[k] = (uint16_t)q;
    }
    uint32_t off = (uint32_t)((double)frame * 2.4576) & JD_PAL_MASK;
    p1_upsample(fb, w, h, p1_field, pal, off);
}
