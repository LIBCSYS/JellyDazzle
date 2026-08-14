/* 002 p4m_quilt — p4m wallpaper fold of a drifting four-wave plasma. */
#include "../jellydazzle.h"
#include <math.h>

#define P2_IW 320
#define P2_IH 240

static uint16_t p2_field[P2_IW * P2_IH];

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
    (void)sl; (void)seed;
    float t = (float)frame;
    float L = 52.0f + 18.0f * sinf(t * 0.002f);
    float invL2 = 1.0f / (2.0f * L);
    float dx = t * 0.08f, dy = t * 0.045f;
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
            float val = sinf(uu * 0.11f + t * 0.004f)
                      + sinf(vv * 0.13f - t * 0.003f)
                      + sinf((uu + vv) * 0.065f + t * 0.0025f)
                      + sinf(d * 0.17f - t * 0.005f);
            val = val * 0.125f + 0.5f;
            int q = (int)(val * 5632.0f + 4096.0f);
            if (q < 0) q = 0;
            if (q > 65535) q = 65535;
            row[x] = (uint16_t)q;
        }
    }
    uint32_t off = (uint32_t)((double)frame * 1.408) & JD_PAL_MASK;
    p2_upsample(fb, w, h, p2_field, pal, off);
}
