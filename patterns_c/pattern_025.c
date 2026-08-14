/* 025 Ember Triad — three detuned wave sources on a slowly turning triangle;
 * constructive islands drift like weather over a wine->gold ripple tank.
 * Port of lab/patterns/025_ember_triad/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

static int16_t s_sin025[4096];          /* Q14 sine, full turn = 4096 */
static uint8_t s_val025[257];           /* 0.24 + 0.76 u^1.15, 0..255 */
static int s_ready025;

static void s_init025(void) {
    if (s_ready025) return;
    for (int i = 0; i < 4096; i++)
        s_sin025[i] = (int16_t)lrintf(16383.0f *
            sinf((float)i * (float)(6.283185307179586 / 4096.0)));
    for (int i = 0; i <= 256; i++) {
        float u = (float)i / 256.0f;
        int b = (int)((0.24f + 0.76f * powf(u, 1.15f)) * 255.0f + 0.5f);
        s_val025[i] = (uint8_t)(b > 255 ? 255 : b);
    }
    s_ready025 = 1;
}

static inline uint32_t s_shade025(uint32_t c, int v8) {
    uint32_t r = (((c >> 16) & 255u) * (uint32_t)v8) >> 8;
    uint32_t g = (((c >> 8) & 255u) * (uint32_t)v8) >> 8;
    uint32_t b = ((c & 255u) * (uint32_t)v8) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

void pattern_025(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init025();
    const float sc = (float)w / 320.0f;
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    static const float kk[3] = { 0.300f, 0.315f, 0.285f };   /* detuned rings */
    static const float ww[3] = { 0.032f, 0.027f, 0.037f };

    float sx[3], sy[3], ks[3];
    int ph[3];
    const float base = 0.0028f * tt;
    for (int i = 0; i < 3; i++) {
        float ang = base + (float)i * 2.0943951f;
        sx[i] = cx + 62.0f * sc * cosf(ang);
        sy[i] = cy + 46.0f * sc * sinf(ang);
        ks[i] = kk[i] / sc * 651.8986f;                 /* rad/labpx -> idx/px */
        ph[i] = (int)(-ww[i] * tt * 651.8986f);
    }

    /* park the ramp on the ember/gold arc: wine troughs -> molten crests */
    const int drift = 10800 + (int)(tt * 0.7f) + (int)(seed & 2047u)
                    + ((s_sin025[((int)(0.003f * tt * 651.8986f)) & 4095] * 900) >> 14);

    for (int y = 0; y < h; y++) {
        float dy0 = (float)y - sy[0], dy1 = (float)y - sy[1], dy2 = (float)y - sy[2];
        float q0 = dy0 * dy0, q1 = dy1 * dy1, q2 = dy2 * dy2;
        uint32_t *row = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            float ax = (float)x - sx[0], bx = (float)x - sx[1], cxx = (float)x - sx[2];
            int f = s_sin025[((int)(sqrtf(q0 + ax * ax) * ks[0]) + ph[0]) & 4095]
                  + s_sin025[((int)(sqrtf(q1 + bx * bx) * ks[1]) + ph[1]) & 4095]
                  + s_sin025[((int)(sqrtf(q2 + cxx * cxx) * ks[2]) + ph[2]) & 4095];
            f = (f * 5461) >> 14;                     /* /3 -> Q14 in -1..1 */
            /* val = 0.24 + 0.76 * clip(0.5 + 0.55 f)^1.15 */
            int q = 8192 + ((f * 9011) >> 14);
            if (q < 0) q = 0;
            if (q > 16383) q = 16383;
            int idx = drift + ((f * 1800) >> 14);
            row[x] = s_shade025(pal[idx & JD_PAL_MASK], s_val025[q >> 6]);
        }
    }
}
