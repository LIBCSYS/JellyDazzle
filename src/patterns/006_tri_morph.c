/* 006 tri_morph — triangular lattice whose cells morph between a chiral C6
 * pinwheel fold (p6) and a mirrored D6 snowflake fold (p6m) and back. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P6_IW 320
#define P6_IH 240

static uint16_t p6_field[P6_IW * P6_IH];

static void p6_upsample(uint32_t *fb, int w, int h, const uint16_t *f,
                        const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((P6_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((P6_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *r0 = f + iy * P6_IW, *r1 = r0 + P6_IW;
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

void pattern_006(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl; (void)seed;
    const float S3 = 1.7320508f;
    const float L = 78.0f, invL = 1.0f / 78.0f;
    const float W2 = (float)M_PI / 3.0f, invW2 = 3.0f / (float)M_PI;
    const float WH = (float)M_PI / 6.0f;
    float t = (float)frame;
    /* temporal rates halved vs the lab prototype to satisfy the motion law */
    float ox = t * 0.04f - P6_IW * 0.5f, oy = t * 0.02f - P6_IH * 0.5f;
    float pa = -t * 0.005f, pb = -t * 0.0035f;
    float m = 0.5f + 0.5f * sinf(t * 0.002f);
    m = m * m * (3.0f - 2.0f * m);
    float im = 1.0f - m;
    for (int y = 0; y < P6_IH; y++) {
        float syf = (float)y + oy;
        float vlat = 2.0f * syf / S3;
        float urow = -syf / S3;
        float dv = vlat - L * floorf(vlat * invL + 0.5f);
        float dyc = dv * (S3 * 0.5f);
        float dxb = dv * 0.5f;
        uint16_t *row = p6_field + y * P6_IW;
        for (int x = 0; x < P6_IW; x++) {
            float u = (float)x + ox + urow;
            float du = u - L * floorf(u * invL + 0.5f);
            float dx = du + dxb;
            float r = sqrtf(dx * dx + dyc * dyc);
            float th = atan2f(dyc, dx);
            float thA = th - floorf(th * invW2) * W2;      /* C6 rotation fold */
            float thB = fabsf(thA - WH);                   /* D6 mirror fold   */
            float rr = r * 0.16f + pa, rs = r * 0.09f + pb;
            float vA = 0.5f + 0.30f * sinf(3.0f * thA + rr)
                            + 0.20f * sinf(rs + sinf(thA * 3.0f));
            float vB = 0.5f + 0.30f * sinf(3.0f * thB + rr)
                            + 0.20f * sinf(rs + sinf(thB * 3.0f));
            float val = vA * im + vB * m;
            float idx = val * 0.9f + r * 0.0015f;
            int q = (int)(idx * 4096.0f + 8192.0f);
            if (q < 0) q = 0;
            if (q > 65535) q = 65535;
            row[x] = (uint16_t)q;
        }
    }
    uint32_t off = (uint32_t)((double)frame * (0.0004 * 4096.0)) & JD_PAL_MASK;
    p6_upsample(fb, w, h, p6_field, pal, off);
}
