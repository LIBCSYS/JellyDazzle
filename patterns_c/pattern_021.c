/* 021 Ripple Duet — two orbiting wave sources, two-slit pond interference.
 * Port of lab/patterns/021_ripple_duet/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

#define P21_BLK 512              /* distance-scratch tile; stays in L1 */

static int16_t s_sin021[4096];   /* Q14 sine, full turn = 4096 */
static int s_ready021;
static int s_d1_021[P21_BLK], s_d2_021[P21_BLK];

static void s_init021(void) {
    if (s_ready021) return;
    for (int i = 0; i < 4096; i++)
        s_sin021[i] = (int16_t)lrintf(16383.0f * sinf((float)i * (float)(6.283185307 / 4096.0)));
    s_ready021 = 1;
}

void pattern_021(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init021();
    const float sc = (float)w / 320.0f;          /* lab was 320x240 */
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    /* counter-rotating elliptical source orbits (lab radii 70,50) */
    const float a1 = 0.009f * tt;
    const float a2 = -0.007f * tt + 2.1f;
    const float x1 = cx + 70.0f * sc * cosf(a1);
    const float y1 = cy + 50.0f * sc * sinf(a1);
    const float x2 = cx + 70.0f * sc * cosf(a2);
    const float y2 = cy + 50.0f * sc * sinf(a2);

    /* k = 0.22 rad/labpx -> LUT idx per screen px: 0.22/sc * 4096/2pi */
    const int ki = (int)(0.22f / sc * 651.898f * 16.0f);   /* Q4 */
    const int ke = ki >> 1;                                /* envelope k/2 */
    const int ph1 = (int)(-0.045f * tt * 651.898f) & 4095;
    const int ph2 = (int)(-0.038f * tt * 651.898f) & 4095;
    const int drift = (int)(tt * 0.75f) + (int)(seed & 1023u) + 7000;

    /* The two source distances are the only floating-point work left, and the
     * sine/palette lookups in the body block auto-vectorisation. Splitting the
     * distances into their own tiled pass lets clang emit a 4-wide FSQRT for
     * them; the expressions are unchanged, so the image is unchanged. */
    for (int y = 0; y < h; y++) {
        float dy1 = (float)y - y1, dy2 = (float)y - y2;
        float q1 = dy1 * dy1, q2 = dy2 * dy2;
        uint32_t *row = fb + (long)y * w;
        for (int x0 = 0; x0 < w; x0 += P21_BLK) {
        int n = w - x0; if (n > P21_BLK) n = P21_BLK;
        for (int i = 0; i < n; i++) {
            float dx1 = (float)(x0 + i) - x1, dx2 = (float)(x0 + i) - x2;
            s_d1_021[i] = (int)sqrtf(q1 + dx1 * dx1);
            s_d2_021[i] = (int)sqrtf(q2 + dx2 * dx2);
        }
        for (int i = 0; i < n; i++) {
            int x = x0 + i;
            int di1 = s_d1_021[i];
            int di2 = s_d2_021[i];
            int wv1 = s_sin021[(((di1 * ki) >> 4) + ph1) & 4095];
            int wv2 = s_sin021[(((di2 * ki) >> 4) + ph2) & 4095];
            int f = wv1 + wv2;                       /* -32768..32768 ~ -2..2 */
            int env = s_sin021[((((di1 - di2) * ke) >> 4) + 1024) & 4095]; /* cos, Q14 */
            int env2 = (env * env) >> 14;            /* 0..16384 */
            /* val = 0.24 + 0.55*(0.5+0.25 f) + 0.18 env^2  (f here /16384) */
            int v = 6600 + ((f * 13) >> 6) + ((env2 * 3600) >> 14); /* Q14-ish */
            if (v < 0) v = 0; if (v > 16383) v = 16383;
            /* moderate palette span for hue (env+f tint); value scales RGB */
            int idx = drift + ((env * 1400) >> 14) + ((f * 700) >> 15);
            uint32_t c = pal[idx & JD_PAL_MASK];
            int s8 = v >> 6;                          /* 0..255 brightness */
            uint32_t r = (((c >> 16) & 255) * s8) >> 8;
            uint32_t g = (((c >> 8) & 255) * s8) >> 8;
            uint32_t b = ((c & 255) * s8) >> 8;
            row[x] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
        }
    }
}
