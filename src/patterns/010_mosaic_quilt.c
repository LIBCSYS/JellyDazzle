/* 010 mosaic_quilt — grid of 60px glazed tiles, each its own 4-fold mirror
 * kaleidoscope; odd-parity cells add the diagonal fold (p4g checkering). */
#include "../engine/jellydazzle.h"
#include <math.h>

#define PA_IW 320
#define PA_IH 240
#define PA_L  60.0f

static uint16_t pa_field[PA_IW * PA_IH];

static void pa_upsample(uint32_t *fb, int w, int h, const uint16_t *f,
                        const uint32_t *pal, uint32_t off) {
    uint32_t sx = (uint32_t)(((PA_IW - 1) << 16) - 1) / (uint32_t)(w > 1 ? w - 1 : 1);
    uint32_t sy = (uint32_t)(((PA_IH - 1) << 16) - 1) / (uint32_t)(h > 1 ? h - 1 : 1);
    for (int y = 0; y < h; y++) {
        uint32_t ay = sy * (uint32_t)y;
        int iy = (int)(ay >> 16), fy = (int)((ay >> 8) & 255);
        const uint16_t *r0 = f + iy * PA_IW, *r1 = r0 + PA_IW;
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

void pattern_010(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl; (void)seed;
    const float SCALE = 4096.0f;
    const float invL = 1.0f / PA_L;
    float t = (float)frame;
    /* temporal rates halved vs the lab prototype to satisfy the motion law */
    float drx = t * 0.05f, dry = t * 0.03f;
    float p1 = -t * 0.005f, p2 = t * 0.003f, p3 = -t * 0.0035f;
    /* drift is split into whole-cell (folded into the palette offset, so the
     * field never grows without bound) and sub-cell parts */
    float cdx = floorf(drx * invL), cdy = floorf(dry * invL);
    for (int y = 0; y < PA_IH; y++) {
        float syf = (float)y + dry;
        float gyf = floorf(syf * invL);
        float v0 = syf - gyf * PA_L - PA_L * 0.5f;
        float av = fabsf(v0);
        float gyr = gyf - cdy;
        uint16_t *row = pa_field + y * PA_IW;
        for (int x = 0; x < PA_IW; x++) {
            float sxf = (float)x + drx;
            float gxf = floorf(sxf * invL);
            float u0 = sxf - gxf * PA_L - PA_L * 0.5f;
            float au = fabsf(u0);
            float uu = au, vv = av;
            int par = ((int)(gxf + gyf)) & 1;
            if (par) {                              /* diagonal fold */
                uu = au > av ? au : av;
                vv = au > av ? av : au;
            }
            float ph = 2.5f * sinf(gxf * 1.7f + gyf * 2.3f);
            float d = uu + vv;
            float val = 0.5f
                      + 0.30f * sinf(d * 0.22f + p1 + ph)
                      + 0.18f * sinf((uu - vv) * 0.19f + p2)
                      + 0.10f * sinf(sqrtf(uu * uu + vv * vv) * 0.24f + ph + p3);
            float hue = val * 0.55f + (gxf - cdx + gyr) * 0.045f;
            int q = (int)(hue * SCALE + 2048.0f);
            if (q < 0) q = 0;
            if (q > 65535) q = 65535;
            row[x] = (uint16_t)q;
        }
    }
    /* whole-cell drift added back here, so the seam is exactly cancelled */
    double offd = (double)frame * (0.00035 * (double)SCALE)
                + (double)(cdx + cdy) * (0.045 * (double)SCALE);
    uint32_t off = (uint32_t)(long long)offd & JD_PAL_MASK;
    pa_upsample(fb, w, h, pa_field, pal, off);
}
