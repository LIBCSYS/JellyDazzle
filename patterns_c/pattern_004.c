/* 004 mirror_truchet — D8-folded Truchet ribbon mandala over dark ground. */
#include "../jellydazzle.h"
#include <math.h>

#define P4_IW 320
#define P4_IH 240

static uint8_t p4_mask[P4_IW * P4_IH];
static uint16_t p4_ink[P4_IW * P4_IH];
static uint16_t p4_gnd[P4_IW * P4_IH];
static float p4_rsc[P4_IW * P4_IH];
static int p4_init;

static int p4_bilerp16(const uint16_t *f, int ix, int fx, int iy, int fy) {
    const uint16_t *r0 = f + iy * P4_IW, *r1 = r0 + P4_IW;
    int a = r0[ix], b = r0[ix + 1], c = r1[ix], d = r1[ix + 1];
    int top = a + (((b - a) * fx) >> 8);
    int bot = c + (((d - c) * fx) >> 8);
    return top + (((bot - top) * fy) >> 8);
}

static int p4_bilerp8(const uint8_t *f, int ix, int fx, int iy, int fy) {
    const uint8_t *r0 = f + iy * P4_IW, *r1 = r0 + P4_IW;
    int a = r0[ix], b = r0[ix + 1], c = r1[ix], d = r1[ix + 1];
    int top = a + (((b - a) * fx) >> 8);
    int bot = c + (((d - c) * fx) >> 8);
    return top + (((bot - top) * fy) >> 8);
}

void pattern_004(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl; (void)seed;
    if (!p4_init) {
        p4_init = 1;
        for (int y = 0; y < P4_IH; y++)
            for (int x = 0; x < P4_IW; x++) {
                float X = (float)x - P4_IW * 0.5f, Y = (float)y - P4_IH * 0.5f;
                p4_rsc[y * P4_IW + x] = sqrtf(X * X + Y * Y);
            }
    }
    float t = (float)frame;
    float rot = t * 0.0020f;
    float c = cosf(rot), s = sinf(rot);
    const float L = 42.0f, invL = 1.0f / 42.0f, R = 21.0f;
    float su = t * 0.22f, sv = t * 0.13f;
    for (int y = 0; y < P4_IH; y++) {
        float Y = (float)y - P4_IH * 0.5f;
        uint8_t *mrow = p4_mask + y * P4_IW;
        uint16_t *irow = p4_ink + y * P4_IW;
        uint16_t *grow = p4_gnd + y * P4_IW;
        const float *rrow = p4_rsc + y * P4_IW;
        for (int x = 0; x < P4_IW; x++) {
            float X = (float)x - P4_IW * 0.5f;
            float xr = X * c - Y * s, yr = X * s + Y * c;
            xr = fabsf(xr); yr = fabsf(yr);
            float fx = xr > yr ? xr : yr;
            float fy = xr > yr ? yr : xr;
            float u0 = fx + su, v0 = fy + sv;
            float txf = floorf(u0 * invL), tyf = floorf(v0 * invL);
            float u = u0 - txf * L, v = v0 - tyf * L;
            float hs = sinf(txf * 12.9898f + tyf * 78.233f) * 43758.5453f;
            hs -= floorf(hs);
            if (hs >= 0.5f) u = L - u;
            float d1 = sqrtf(u * u + v * v);
            float lu = L - u, lv = L - v;
            float d2 = sqrtf(lu * lu + lv * lv);
            float da = fabsf(d1 - R), db = fabsf(d2 - R);
            float d = da < db ? da : db;
            float rib = 1.0f - d * (1.0f / 7.0f);
            if (rib < 0.0f) rib = 0.0f;
            rib = rib * rib * (3.0f - 2.0f * rib);
            mrow[x] = (uint8_t)(rib * 255.0f);
            float hue = txf * 0.11f + tyf * 0.07f;
            hue -= floorf(hue * 0.125f) * 8.0f;      /* mod 8 keeps it bounded */
            irow[x] = (uint16_t)((int)(hue * 4096.0f) & JD_PAL_MASK);
            int g = (int)(rrow[x] * 9.2f + t * 0.3f) & 4095;
            if (g >= 2048) g = 4095 - g;
            grow[x] = (uint16_t)g;
        }
    }
    uint32_t offink = (uint32_t)((double)frame * 4.9152);
    uint32_t sxs = (uint32_t)(((P4_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sys = (uint32_t)(((P4_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sys * (uint32_t)y;
        int iy = (int)(ay >> 16), fyq = (int)((ay >> 8) & 255);
        uint32_t *dst = fb + (long)y * w;
        uint32_t ax = 0;
        for (int x = 0; x < w; x++) {
            int ix = (int)(ax >> 16), fxq = (int)((ax >> 8) & 255);
            int m = p4_bilerp8(p4_mask, ix, fxq, iy, fyq);
            int gi = p4_bilerp16(p4_gnd, ix, fxq, iy, fyq);
            uint32_t ground = pal[gi & JD_PAL_MASK];
            uint32_t out;
            if (m == 0) {
                out = ground;
            } else {
                int ii = p4_bilerp16(p4_ink, ix, fxq, iy, fyq);
                uint32_t ink = pal[((uint32_t)ii + offink) & JD_PAL_MASK];
                uint32_t gr = (ground >> 16) & 255, gg = (ground >> 8) & 255, gb = ground & 255;
                uint32_t ir = (ink >> 16) & 255, ig = (ink >> 8) & 255, ib = ink & 255;
                uint32_t orr = gr + (((ir - gr) * (uint32_t)m) >> 8);
                uint32_t og = gg + (((ig - gg) * (uint32_t)m) >> 8);
                uint32_t ob = gb + (((ib - gb) * (uint32_t)m) >> 8);
                out = 0xFF000000u | (orr << 16) | (og << 8) | ob;
            }
            dst[x] = out;
            ax += sxs;
        }
    }
}
