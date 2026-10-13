/* 002 p4m_quilt — p4m wallpaper fold of a drifting four-wave plasma. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P2_IW 320
#define P2_IH 240

static uint16_t p2_field[P2_IW * P2_IH];

/* 3.6.5: seed-derived look. Every appearance used to be identical (variety
 * audit, dSeed 0). The seed now picks the tile size, wave density, which way
 * the quilt drifts, the plasma's phase and where on the ramp it starts. */
static uint32_t p2_seed = 0xFFFFFFFFu;
static float p2_L0, p2_fk, p2_vx, p2_vy, p2_tph;
static uint32_t p2_poff;

static uint32_t p2_hash(uint32_t x) {          /* murmur3 finaliser */
    x ^= x >> 16; x *= 0x85ebca6bu; x ^= x >> 13; x *= 0xc2b2ae35u; x ^= x >> 16;
    return x;
}

static void p2_upsample(uint32_t *fb, int w, int h, const uint16_t *f,
                        const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((P2_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((P2_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *r0 = f + iy * P2_IW, *r1 = r0 + P2_IW;
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

void pattern_002(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (seed != p2_seed) {
        uint32_t hs = p2_hash(seed ^ 0x7F4A7C15u);
        float ang;
        p2_seed = seed;
        p2_L0 = 38.0f + 30.0f * (float)(hs & 255) / 255.0f;      /* 38..68 */
        p2_fk = 0.90f + 0.50f * (float)((hs >> 8) & 255) / 255.0f;
        /* same drift speed as before (about 0.092 px/frame), any heading */
        ang = 6.2831853f * (float)((hs >> 16) & 1023) / 1024.0f;
        p2_vx = 0.092f * cosf(ang);
        p2_vy = 0.092f * sinf(ang);
        hs = p2_hash(hs);
        p2_tph = (float)(hs % 3000u);
        /* ramp start is held to the first quarter of the ramp: the field already
         * spans about a quarter of it, and the library's ramps go dark at both
         * ends, so a free offset would land some seeds in a near-black stretch */
        p2_poff = (hs >> 12) % 8192u;
    }
    float t = (float)frame + p2_tph;
    /* breathing amplitude scales with the tile so small tiles never invert */
    float L = p2_L0 + p2_L0 * 0.34f * sinf(t * 0.002f);
    float invL2 = 1.0f / (2.0f * L);
    float dx = (float)frame * p2_vx, dy = (float)frame * p2_vy;
    const float fk = p2_fk;
    for (int y = 0; y < P2_IH; y++) {
        float sy = (float)y + dy;
        float v0 = sy - floorf(sy * invL2) * 2.0f * L;
        if (v0 >= L) v0 = 2.0f * L - v0;
        uint16_t *row = p2_field + y * P2_IW;
        for (int x = 0; x < P2_IW; x++) {
            float sx = (float)x + dx;
            float u = sx - floorf(sx * invL2) * 2.0f * L;
            if (u >= L) u = 2.0f * L - u;
            float uu = u > v0 ? u : v0;
            float vv = u > v0 ? v0 : u;
            float du = uu - L * 0.5f, dv = vv - L * 0.5f;
            float d = sqrtf(du * du + dv * dv);
            float val = sinf(uu * 0.11f * fk + t * 0.004f)
                      + sinf(vv * 0.13f * fk - t * 0.003f)
                      + sinf((uu + vv) * 0.065f * fk + t * 0.0025f)
                      + sinf(d * 0.17f * fk - t * 0.005f);
            val = val * 0.125f + 0.5f;
            int q = (int)(val * 5632.0f + 4096.0f);
            if (q < 0) q = 0;
            if (q > 65535) q = 65535;
            row[x] = (uint16_t)q;
        }
    }
    uint32_t off = ((uint32_t)((double)frame * 1.408) + p2_poff) & JD_PAL_MASK;
    p2_upsample(fb, w, h, p2_field, pal, off);
}
