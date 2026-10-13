/* 008 octa_mandala — concentric polygon rings (square/octagon/diamond norms
 * morphing) pulsing outward, dappled into beads by an 8-petal modulation. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P8_IW 320
#define P8_IH 240

static float p8_th[P8_IW * P8_IH];
static uint16_t p8_field[P8_IW * P8_IH];
static int p8_init;

/* 3.6.5: seed-derived look. Every appearance used to be identical (variety
 * audit, dSeed 0). The seed now picks the bead (petal) count, ring spacing,
 * spin and pulse direction, where in the square/diamond morph it starts,
 * and the ramp start. */
static uint32_t p8_seed = 0xFFFFFFFFu;
static float p8_pet, p8_rk, p8_dir, p8_tph;
static uint32_t p8_poff;

static uint32_t p8_hash(uint32_t x) {          /* murmur3 finaliser */
    x ^= x >> 16; x *= 0x85ebca6bu; x ^= x >> 13; x *= 0xc2b2ae35u; x ^= x >> 16;
    return x;
}

static void p8_upsample(uint32_t *fb, int w, int h, const uint16_t *f,
                        const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((P8_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((P8_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *r0 = f + iy * P8_IW, *r1 = r0 + P8_IW;
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

void pattern_008(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    static const float pets[6] = {4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 16.0f};
    if (seed != p8_seed) {
        uint32_t hs = p8_hash(seed ^ 0x1F83D9ABu);
        p8_seed = seed;
        p8_pet = pets[hs % 6u];
        p8_rk = 0.65f + 0.85f * (float)((hs >> 4) & 255) / 255.0f;
        p8_dir = (hs & 0x1000u) ? 1.0f : -1.0f;
        hs = p8_hash(hs);
        p8_tph = (float)(hs % 5236u);       /* one full morph period */
        /* ramp start is held to the first quarter of the ramp: the field already
         * spans about a quarter of it, and the library's ramps go dark at both
         * ends, so a free offset would land some seeds in a near-black stretch */
        p8_poff = (hs >> 13) % 8192u;
    }
    const float INV_S2 = 0.70710678f;
    const float pet = p8_pet, rk = p8_rk;
    if (!p8_init) {
        p8_init = 1;
        for (int y = 0; y < P8_IH; y++)
            for (int x = 0; x < P8_IW; x++) {
                float X = (float)x - P8_IW * 0.5f, Y = (float)y - P8_IH * 0.5f;
                p8_th[y * P8_IW + x] = atan2f(Y, X);
            }
    }
    float t = (float)frame + p8_tph;
    /* temporal rates halved vs the lab prototype to satisfy the motion law */
    float rot = t * 0.0008f * p8_dir;
    float c = cosf(rot), s = sinf(rot);
    float k = 0.5f + 0.5f * sinf(t * 0.0012f);
    float ik = 1.0f - k;
    float ringph = -t * 0.0055f * p8_dir;
    float petph = pet * rot;
    for (int y = 0; y < P8_IH; y++) {
        float Y = (float)y - P8_IH * 0.5f;
        float xr = (0.0f - P8_IW * 0.5f) * c - Y * s;   /* rotated coords, */
        float yr = (0.0f - P8_IW * 0.5f) * s + Y * c;   /* stepped per pixel */
        const float *throw_ = p8_th + y * P8_IW;
        uint16_t *row = p8_field + y * P8_IW;
        for (int x = 0; x < P8_IW; x++) {
            float ax = fabsf(xr), ay = fabsf(yr);
            float cheb = ax > ay ? ax : ay;
            float sum = ax + ay;
            float diam = sum * INV_S2;
            float o2 = sum * 0.58578f;
            float octn = cheb > o2 ? cheb : o2;
            float q = cheb * ik + diam * k;
            q = 0.5f * q + 0.5f * octn;
            float rings = sinf(q * 0.17f * rk + ringph);
            float petals = 0.6f + 0.4f * sinf(pet * throw_[x] + petph + q * 0.035f);
            float v = 0.5f + 0.42f * rings * petals;
            float idx = v * 0.9f + q * 0.0015f;
            int qi = (int)(idx * 3584.0f + 8192.0f);
            if (qi < 0) qi = 0;
            if (qi > 65535) qi = 65535;
            row[x] = (uint16_t)qi;
            xr += c;
            yr += s;
        }
    }
    uint32_t off = ((uint32_t)((double)frame * (0.0004 * 3584.0)) + p8_poff)
                 & JD_PAL_MASK;
    p8_upsample(fb, w, h, p8_field, pal, off);
}
