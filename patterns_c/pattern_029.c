/* 029 Tartan Beat — two woven square lattices at slightly different scales and
 * opposite tilt interfere into a giant slow moire plaid with a fine weave.
 * Port of lab/patterns/029_tartan_beat/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

static int16_t s_sin029[4096];          /* Q14 sine, full turn = 4096 */
static int16_t s_tanh029[256];          /* tanh(0.9 f), f in -4..4, Q14 */
static uint16_t s_pow029[257];          /* u^1.1, u in 0..1, Q14 */
static int s_ready029;

static void s_init029(void) {
    if (s_ready029) return;
    for (int i = 0; i < 4096; i++)
        s_sin029[i] = (int16_t)lrintf(16383.0f *
            sinf((float)i * (float)(6.283185307179586 / 4096.0)));
    for (int i = 0; i < 256; i++) {
        float f = ((float)i - 127.5f) * (4.0f / 127.5f);
        s_tanh029[i] = (int16_t)lrintf(16383.0f * tanhf(0.9f * f));
    }
    for (int i = 0; i <= 256; i++)
        s_pow029[i] = (uint16_t)lrintf(16383.0f * powf((float)i / 256.0f, 1.1f));
    s_ready029 = 1;
}

static inline uint32_t s_shade029(uint32_t c, int v8) {
    uint32_t r = (((c >> 16) & 255u) * (uint32_t)v8) >> 8;
    uint32_t g = (((c >> 8) & 255u) * (uint32_t)v8) >> 8;
    uint32_t b = ((c & 255u) * (uint32_t)v8) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

void pattern_029(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init029();
    const float sc = (float)w / 320.0f;
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    /* bounded angle swings keep the moire cells large */
    const float a1 = 0.16f * sinf(0.0025f * tt);
    const float a2 = -0.14f * sinf(0.0020f * tt) + 0.10f;
    const float fr1 = 0.360f / sc * 651.8986f;
    const float fr2 = 0.395f * (1.0f + 0.05f * sinf(0.0030f * tt)) / sc * 651.8986f;
    const int p1 = (int)( 0.0125f * tt * 651.8986f);
    const int p2 = (int)(-0.0105f * tt * 651.8986f);
    const int drift = 10000 + (int)(tt * 0.5f) + (int)(seed & 2047u);

    /* four DDA accumulators (u1,v1,u2,v2) in Q6 sine-index units */
    const float c1 = cosf(a1), s1 = sinf(a1);
    const float c2 = cosf(a2), s2 = sinf(a2);
    const int du1 = (int)(fr1 * c1 * 64.0f), dv1 = (int)(fr1 * s1 * 64.0f);
    const int du2 = (int)(fr2 * c2 * 64.0f), dv2 = (int)(fr2 * s2 * 64.0f);
    const int cosb = 1024 << 6;                 /* +90 deg => cosine */

    for (int y = 0; y < h; y++) {
        float dy = (float)y - cy;
        int au1 = (int)((fr1 * (-cx * c1 - dy * s1)) * 64.0f) + cosb + (p1 << 6);
        int av1 = (int)((fr1 * (-cx * s1 + dy * c1)) * 64.0f) + cosb - (p1 << 6);
        int au2 = (int)((fr2 * (-cx * c2 - dy * s2)) * 64.0f) + cosb + (p2 << 6);
        int av2 = (int)((fr2 * (-cx * s2 + dy * c2)) * 64.0f) + cosb - (p2 << 6);
        uint32_t *row = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int g1 = s_sin029[(au1 >> 6) & 4095] + s_sin029[(av1 >> 6) & 4095];
            int g2 = s_sin029[(au2 >> 6) & 4095] + s_sin029[(av2 >> 6) & 4095];
            au1 += du1; av1 += dv1; au2 += du2; av2 += dv2;
            int f = g1 + g2;                          /* Q14, -4..4 */
            int wv = (g1 * g2) >> 16;                 /* g1 g2 /4, Q14 -1..1 */
            int af = f >= 0 ? f : -f;
            int aw = wv >= 0 ? wv : -wv;
            int pw = s_pow029[(af >> 8) > 256 ? 256 : (af >> 8)];  /* (|f|/4)^1.1 */
            /* val = 0.14 + 0.72 pw + 0.20 |w| */
            int v = 2294 + ((pw * 11796) >> 14) + ((aw * 3277) >> 14);
            if (v > 16383) v = 16383;
            int ti = (f >> 9) + 128;
            if (ti < 0) ti = 0;
            if (ti > 255) ti = 255;
            int idx = drift + ((s_tanh029[ti] * 2800) >> 14);
            row[x] = s_shade029(pal[idx & JD_PAL_MASK], v >> 6);
        }
    }
}
