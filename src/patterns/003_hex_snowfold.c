/* 003 hex_snowfold — p6m hex lattice of D6 snowflake rosettes. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P3_IW 320
#define P3_IH 240

static uint16_t p3_field[P3_IW * P3_IH];

/* 3.6.5: seed-derived look. Every appearance used to be identical (variety
 * audit, dSeed 0). The lattice is p6m by construction, so instead of the
 * fold the seed picks cell size, ring density, petal twist, spin and drift
 * direction, and the ramp start. */
static uint32_t p3_seed = 0xFFFFFFFFu;
static float p3_L, p3_rk, p3_tw, p3_dir, p3_vx, p3_vy, p3_tph;
static uint32_t p3_poff;

static uint32_t p3_hash(uint32_t x) {          /* murmur3 finaliser */
    x ^= x >> 16; x *= 0x85ebca6bu; x ^= x >> 13; x *= 0xc2b2ae35u; x ^= x >> 16;
    return x;
}

static void p3_upsample(uint32_t *fb, int w, int h, const uint16_t *f,
                        const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((P3_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((P3_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *r0 = f + iy * P3_IW, *r1 = r0 + P3_IW;
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

void pattern_003(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (seed != p3_seed) {
        uint32_t hs = p3_hash(seed ^ 0x3C6EF372u);
        float ang;
        p3_seed = seed;
        p3_L = 50.0f + 48.0f * (float)(hs & 255) / 255.0f;       /* 50..98 */
        p3_rk = 0.90f + 0.55f * (float)((hs >> 8) & 255) / 255.0f;
        p3_tw = 2.0f + 5.0f * (float)((hs >> 16) & 255) / 255.0f; /* was 4 */
        p3_dir = (hs & 0x1000000u) ? 1.0f : -1.0f;
        hs = p3_hash(hs);
        ang = 6.2831853f * (float)(hs & 1023) / 1024.0f;
        p3_vx = 0.022f * cosf(ang);          /* original drift speed */
        p3_vy = 0.022f * sinf(ang);
        p3_tph = (float)((hs >> 10) % 3000u);
        /* ramp start is held to the first quarter of the ramp: the field already
         * spans about a quarter of it, and the library's ramps go dark at both
         * ends, so a free offset would land some seeds in a near-black stretch */
        p3_poff = p3_hash(hs) % 8192u;
    }
    const float S3 = 1.7320508f;
    float t = (float)frame + p3_tph;
    const float L = p3_L, invL = 1.0f / p3_L;
    const float rk = p3_rk, tw = p3_tw;
    float ox = (float)frame * p3_vx - P3_IW * 0.5f;
    float oy = (float)frame * p3_vy - P3_IH * 0.5f;
    float spin = t * 0.00045f * p3_dir;
    const float wg = (float)M_PI / 6.0f;
    const float inv2w = 1.0f / (2.0f * wg);
    for (int y = 0; y < P3_IH; y++) {
        float syf = (float)y + oy;
        float vlat_row = 2.0f * syf / S3;
        float u_row = -syf / S3;
        uint16_t *row = p3_field + y * P3_IW;
        for (int x = 0; x < P3_IW; x++) {
            float sxf = (float)x + ox;
            float u = sxf + u_row;
            float vl = vlat_row;
            float du = u - L * floorf(u * invL + 0.5f);
            float dv = vl - L * floorf(vl * invL + 0.5f);
            float dx = du + dv * 0.5f;
            float dy = dv * (S3 * 0.5f);
            float r = sqrtf(dx * dx + dy * dy);
            float th = atan2f(dy, dx) + spin;
            float tm = th - floorf(th * inv2w) * 2.0f * wg;
            float thf = fabsf(tm - wg);
            float val = 0.5f
                + 0.30f * sinf(r * 0.16f * rk - t * 0.0025f + tw * thf)
                + 0.20f * sinf(r * 0.30f * rk * cosf(thf * 6.0f) + t * 0.0015f);
            float idx = val * 0.9f + r * 0.002f;
            int q = (int)(idx * 5120.0f + 4096.0f);
            if (q < 0) q = 0;
            if (q > 65535) q = 65535;
            row[x] = (uint16_t)q;
        }
    }
    uint32_t off = ((uint32_t)((double)frame * (0.0004 * 4096.0)) + p3_poff)
                 & JD_PAL_MASK;
    p3_upsample(fb, w, h, p3_field, pal, off);
}
