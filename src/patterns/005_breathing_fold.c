/* 005 breathing_fold — continuous fold count N(t) = 10 + 6 sin(0.0028 t),
 * tie-dye sun mandala whose petals split and merge as N glides 4..16. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P5_IW 320
#define P5_IH 240

static float p5_th[P5_IW * P5_IH];
static float p5_r[P5_IW * P5_IH];
static uint16_t p5_field[P5_IW * P5_IH];
static int p5_init;

/* 3.6.5: seed-derived look. Every appearance used to be identical (variety
 * audit, dSeed 0). The seed now sets the fold range N breathes through, ring
 * density, which way the sun turns and its rings run, the breathing phase
 * and the ramp start. */
static uint32_t p5_seed = 0xFFFFFFFFu;
static float p5_nc, p5_na, p5_rk, p5_dir, p5_tph;
static uint32_t p5_poff;

static uint32_t p5_hash(uint32_t x) {          /* murmur3 finaliser */
    x ^= x >> 16; x *= 0x85ebca6bu; x ^= x >> 13; x *= 0xc2b2ae35u; x ^= x >> 16;
    return x;
}

static void p5_upsample(uint32_t *fb, int w, int h, const uint16_t *f,
                        const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((P5_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((P5_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *r0 = f + iy * P5_IW, *r1 = r0 + P5_IW;
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

void pattern_005(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (seed != p5_seed) {
        uint32_t hs = p5_hash(seed ^ 0xA54FF53Au);
        p5_seed = seed;
        p5_nc = 7.0f + 7.0f * (float)(hs & 255) / 255.0f;        /* 7..14 */
        p5_na = 2.0f + (p5_nc - 5.0f) * (float)((hs >> 8) & 255) / 255.0f;
        p5_rk = 0.65f + 0.75f * (float)((hs >> 16) & 255) / 255.0f;
        p5_dir = (hs & 0x1000000u) ? 1.0f : -1.0f;
        hs = p5_hash(hs);
        p5_tph = (float)(hs % 6000u);       /* a third of a breath cycle+ */
        /* ramp start is held to the first quarter of the ramp: the field already
         * spans about a quarter of it, and the library's ramps go dark at both
         * ends, so a free offset would land some seeds in a near-black stretch */
        p5_poff = (hs >> 13) % 8192u;
    }
    if (!p5_init) {
        p5_init = 1;
        for (int y = 0; y < P5_IH; y++)
            for (int x = 0; x < P5_IW; x++) {
                float X = (float)x - P5_IW * 0.5f, Y = (float)y - P5_IH * 0.5f;
                p5_th[y * P5_IW + x] = atan2f(Y, X);
                p5_r[y * P5_IW + x] = sqrtf(X * X + Y * Y);
            }
    }
    float t = (float)frame + p5_tph;
    /* temporal rates halved vs the lab prototype to satisfy the motion law */
    float N = p5_nc + p5_na * sinf(t * 0.0010f);            /* stays >= 3 */
    float wg = (float)M_PI / N;
    float inv2w = 1.0f / (2.0f * wg), invw = 1.0f / wg;
    float rot = t * 0.0006f * p5_dir;
    float ph1 = -t * 0.006f * p5_dir, ph2 = -t * 0.003f, ph3 = t * 0.0018f;
    const float rk = p5_rk;
    const float PI3 = 3.0f * (float)M_PI, PI6 = 6.0f * (float)M_PI;
    for (int k = 0; k < P5_IW * P5_IH; k++) {
        float r = p5_r[k] * rk;
        float th = p5_th[k] + rot;
        float tm = th - floorf(th * inv2w) * (2.0f * wg);
        float a = fabsf(tm - wg) * invw;           /* 0..1 wedge coordinate */
        float v = 0.5f
                + 0.30f * sinf(r * 0.075f + ph1)
                + 0.16f * sinf(a * PI3 + r * 0.028f + ph2)
                + 0.10f * sinf(r * 0.02f + a * PI6 + ph3);
        float idx = v * 0.9f + r * 0.0012f;
        int q = (int)(idx * 3072.0f + 8192.0f);
        if (q < 0) q = 0;
        if (q > 65535) q = 65535;
        p5_field[k] = (uint16_t)q;
    }
    uint32_t off = ((uint32_t)((double)frame * (0.0005 * 4096.0)) + p5_poff)
                 & JD_PAL_MASK;
    p5_upsample(fb, w, h, p5_field, pal, off);
}
