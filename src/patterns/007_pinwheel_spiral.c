/* 007 pinwheel_spiral — C_N log-spiral pinwheel, arm count 6/9/12/8/10 with
 * soft crossfades, spiral pitch breathing through zero (chirality flips). */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P7_IW 320
#define P7_IH 240

static float p7_th[P7_IW * P7_IH];
static float p7_r[P7_IW * P7_IH];
static float p7_lnr[P7_IW * P7_IH];
static uint16_t p7_field[P7_IW * P7_IH];
static int p7_init;

/* 3.6.5: seed-derived look. Every appearance used to be identical (variety
 * audit, dSeed 0). The seed now picks one of two arm-count tours and where in
 * it to start, the spin direction, where in the pitch breath (chirality) we
 * begin, ripple density, and the ramp start. */
static uint32_t p7_seed = 0xFFFFFFFFu;
static int p7_set, p7_seq0;
static float p7_dir, p7_rk, p7_tph;
static uint32_t p7_poff;

static uint32_t p7_hash(uint32_t x) {          /* murmur3 finaliser */
    x ^= x >> 16; x *= 0x85ebca6bu; x ^= x >> 13; x *= 0xc2b2ae35u; x ^= x >> 16;
    return x;
}

static void p7_upsample(uint32_t *fb, int w, int h, const uint16_t *f,
                        const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((P7_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((P7_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *r0 = f + iy * P7_IW, *r1 = r0 + P7_IW;
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

void pattern_007(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    static const int seqs[2][5] = {{6, 9, 12, 8, 10}, {5, 7, 11, 4, 8}};
    if (seed != p7_seed) {
        uint32_t hs = p7_hash(seed ^ 0x9B05688Cu);
        p7_seed = seed;
        p7_set = (int)(hs & 1u);
        p7_seq0 = (int)((hs >> 1) % 5u);
        p7_dir = (hs & 0x100u) ? 1.0f : -1.0f;
        p7_rk = 0.60f + 0.90f * (float)((hs >> 9) & 255) / 255.0f;
        hs = p7_hash(hs);
        p7_tph = (float)(hs % 8400u);       /* one full pitch period */
        /* ramp start is held to the first quarter of the ramp: the field already
         * spans about a quarter of it, and the library's ramps go dark at both
         * ends, so a free offset would land some seeds in a near-black stretch */
        p7_poff = (hs >> 14) % 8192u;
    }
    const int *seq = seqs[p7_set];
    if (!p7_init) {
        p7_init = 1;
        for (int y = 0; y < P7_IH; y++)
            for (int x = 0; x < P7_IW; x++) {
                float X = (float)x - P7_IW * 0.5f, Y = (float)y - P7_IH * 0.5f;
                int k = y * P7_IW + x;
                p7_th[k] = atan2f(Y, X);
                p7_r[k] = sqrtf(X * X + Y * Y);
                p7_lnr[k] = logf(p7_r[k] + 4.0f);
            }
    }
    /* seed phase drives the slow terms only; arm crossfades keep real time */
    float t = (float)frame + p7_tph;
    int i = (frame / 300 + p7_seq0) % 5, j = (i + 1) % 5;
    float fph = (float)(frame % 300) / 300.0f;
    float m = (fph - 0.72f) / 0.28f;
    if (m < 0.0f) m = 0.0f;
    if (m > 1.0f) m = 1.0f;
    m = m * m * (3.0f - 2.0f * m);
    float im = 1.0f - m;
    /* temporal rates halved vs the lab prototype to satisfy the motion law */
    float pitch = 3.4f * sinf(t * 0.00075f);
    float spin = -t * 0.008f * p7_dir;
    const float rk = p7_rk;
    float ph2 = t * 0.0025f;
    int Na = seq[i], Nb = seq[j];
    for (int k = 0; k < P7_IW * P7_IH; k++) {
        float th = p7_th[k], r = p7_r[k];
        float base = pitch * p7_lnr[k] + spin;
        float rp = r * 0.03f * rk + ph2;
        float a = (float)Na * th + base;
        float v = 0.5f + 0.32f * sinf(a) + 0.16f * sinf(2.0f * a + rp);
        if (m > 0.0f) {
            float b = (float)Nb * th + base;
            float v2 = 0.5f + 0.32f * sinf(b) + 0.16f * sinf(2.0f * b + rp);
            v = v * im + v2 * m;
        }
        float idx = v * 0.85f + r * 0.0018f;
        int q = (int)(idx * 3584.0f + 8192.0f);
        if (q < 0) q = 0;
        if (q > 65535) q = 65535;
        p7_field[k] = (uint16_t)q;
    }
    uint32_t off = ((uint32_t)((double)frame * (0.0005 * 3584.0)) + p7_poff)
                 & JD_PAL_MASK;
    p7_upsample(fb, w, h, p7_field, pal, off);
}
