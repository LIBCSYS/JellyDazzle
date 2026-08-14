/* 023 Silk Gratings — two far off-screen ring sources make gently curved
 * gratings that beat into broad silk bands with a fine weave inside.
 * Port of lab/patterns/023_silk_gratings/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

#define P23_BLK 512              /* distance-scratch tile; stays in L1 */

static int16_t s_sin023[4096];          /* Q14 sine, full turn = 4096 */
static int s_ready023;
static int s_ia023[P23_BLK], s_ib023[P23_BLK];

static void s_init023(void) {
    if (s_ready023) return;
    for (int i = 0; i < 4096; i++)
        s_sin023[i] = (int16_t)lrintf(16383.0f *
            sinf((float)i * (float)(6.283185307179586 / 4096.0)));
    s_ready023 = 1;
}

static inline uint32_t s_shade023(uint32_t c, int v8) {
    uint32_t r = (((c >> 16) & 255u) * (uint32_t)v8) >> 8;
    uint32_t g = (((c >> 8) & 255u) * (uint32_t)v8) >> 8;
    uint32_t b = ((c & 255u) * (uint32_t)v8) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

void pattern_023(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init023();
    const float sc = (float)w / 320.0f;          /* lab was 320x240 */
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    /* the two ring sources sit ~420 lab px off-screen and sway slowly */
    const float ax = cx + (-420.0f + 60.0f * sinf(0.00085f * tt)) * sc;
    const float ay = cy + ( 120.0f * sinf(0.00070f * tt)) * sc;
    const float bx = cx + ( 420.0f - 60.0f * sinf(0.00075f * tt)) * sc;
    const float by = cy + (-120.0f * sinf(0.00065f * tt + 0.9f)) * sc;

    /* f1=0.55, f2=0.61 rad per LAB px -> LUT index per SCREEN px */
    const float K1 = 0.55f / sc * 651.8986f;
    const float K2 = 0.61f / sc * 651.8986f;
    const int ph1 = (int)( 0.0250f * tt * 651.8986f);
    const int ph2 = (int)(-0.0200f * tt * 651.8986f);
    const int phe = (int)( 0.0083f * tt * 651.8986f) + 1024;   /* +1024 => cos */
    /* park the ramp on the bright, colourful arc of the scheme */
    const int drift = 7200 + (int)(tt * 0.6f) + (int)(seed & 4095u);

    for (int y = 0; y < h; y++) {
        float dya = (float)y - ay, dyb = (float)y - by;
        float qa = dya * dya, qb = dyb * dyb;
        uint32_t *row = fb + (long)y * w;
        for (int x0 = 0; x0 < w; x0 += P23_BLK) {
        int n = w - x0; if (n > P23_BLK) n = P23_BLK;
        /* the two ring distances are the only float work; hoisting them into
         * their own tiled pass (the sine/palette lookups below block it) lets
         * clang emit 4-wide FSQRT. Same expressions, same image. */
        for (int i = 0; i < n; i++) {
            float dxa = (float)(x0 + i) - ax, dxb = (float)(x0 + i) - bx;
            s_ia023[i] = (int)(sqrtf(qa + dxa * dxa) * K1);
            s_ib023[i] = (int)(sqrtf(qb + dxb * dxb) * K2);
        }
        for (int i = 0; i < n; i++) {
            int x = x0 + i;
            int ia = s_ia023[i];
            int ib = s_ib023[i];
            int g1 = s_sin023[(ia + ph1) & 4095];
            int g2 = s_sin023[(ib + ph2) & 4095];
            int car = (g1 * g2) >> 14;                    /* fine weave, Q14 */
            int env = s_sin023[(((ia - ib) >> 1) + phe) & 4095]; /* beat bands */
            int env2 = (env * env) >> 14;                 /* 0..16384 */
            /* val = 0.22 + 0.48*(0.5+0.5c) + 0.28 env^2 */
            int v = 7537 + ((car * 3932) >> 14) + ((env2 * 4588) >> 14);
            if (v < 0) v = 0;
            if (v > 16383) v = 16383;
            int idx = drift + ((env * 2600) >> 14) + ((car * 900) >> 14);
            row[x] = s_shade023(pal[idx & JD_PAL_MASK], v >> 6);
        }
        }
    }
}
