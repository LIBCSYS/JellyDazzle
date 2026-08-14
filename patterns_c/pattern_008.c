/* 008 octa_mandala — concentric polygon rings (square/octagon/diamond norms
 * morphing) pulsing outward, dappled into beads by an 8-petal modulation. */
#include "../jellydazzle.h"
#include <math.h>

#define P8_IW 320
#define P8_IH 240

static float p8_th[P8_IW * P8_IH];
static uint16_t p8_field[P8_IW * P8_IH];
static int p8_init;

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
    (void)sl; (void)seed;
    const float INV_S2 = 0.70710678f;
    if (!p8_init) {
        p8_init = 1;
        for (int y = 0; y < P8_IH; y++)
            for (int x = 0; x < P8_IW; x++) {
                float X = (float)x - P8_IW * 0.5f, Y = (float)y - P8_IH * 0.5f;
                p8_th[y * P8_IW + x] = atan2f(Y, X);
            }
    }
    float t = (float)frame;
    /* temporal rates halved vs the lab prototype to satisfy the motion law */
    float rot = t * 0.0008f;
    float c = cosf(rot), s = sinf(rot);
    float k = 0.5f + 0.5f * sinf(t * 0.0012f);
    float ik = 1.0f - k;
    float ringph = -t * 0.0055f;
    float petph = 8.0f * rot;
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
            float rings = sinf(q * 0.17f + ringph);
            float petals = 0.6f + 0.4f * sinf(8.0f * throw_[x] + petph + q * 0.035f);
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
    uint32_t off = (uint32_t)((double)frame * (0.0004 * 3584.0)) & JD_PAL_MASK;
    p8_upsample(fb, w, h, p8_field, pal, off);
}
