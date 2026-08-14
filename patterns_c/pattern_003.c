/* 003 hex_snowfold — p6m hex lattice of D6 snowflake rosettes. */
#include "../jellydazzle.h"
#include <math.h>

#define P3_IW 320
#define P3_IH 240

static uint16_t p3_field[P3_IW * P3_IH];

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
    (void)sl; (void)seed;
    const float S3 = 1.7320508f;
    float t = (float)frame;
    const float L = 72.0f, invL = 1.0f / 72.0f;
    float ox = t * 0.04f - P3_IW * 0.5f, oy = t * 0.018f - P3_IH * 0.5f;
    float spin = t * 0.0009f;
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
                + 0.30f * sinf(r * 0.16f - t * 0.005f + 4.0f * thf)
                + 0.20f * sinf(r * 0.30f * cosf(thf * 6.0f) + t * 0.003f);
            float idx = val * 0.9f + r * 0.002f;
            int q = (int)(idx * 7168.0f + 4096.0f);
            if (q < 0) q = 0;
            if (q > 65535) q = 65535;
            row[x] = (uint16_t)q;
        }
    }
    uint32_t off = (uint32_t)((double)frame * (0.0004 * 4096.0)) & JD_PAL_MASK;
    p3_upsample(fb, w, h, p3_field, pal, off);
}
