/* 004 mirror_truchet — D8-folded Truchet ribbon mandala over dark ground. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P4_IW 320
#define P4_IH 240

static uint8_t p4_mask[P4_IW * P4_IH];
static uint16_t p4_ink[P4_IW * P4_IH];
static uint16_t p4_gnd[P4_IW * P4_IH];
static float p4_rsc[P4_IW * P4_IH];
static int p4_init;

/* Upscale scratch. Each of the 240 source rows feeds several output rows, so
 * the horizontal half of the bilinear filter is done once per source row and
 * cached; the per-pixel work drops from three bilerps to three vertical lerps.
 * The integer maths is identical to p4_bilerp8/16, so the image is unchanged. */
static int      *p4_ux;                 /* per-column source index / weight */
static uint8_t  *p4_uf;
static uint8_t  *p4_hm[2];              /* horizontally resampled rows */
static uint16_t *p4_hg[2], *p4_hi[2];
static int p4_uw;

static void p4_freeup(void)
{
    free(p4_ux); free(p4_uf);
    free(p4_hm[0]); free(p4_hm[1]);
    free(p4_hg[0]); free(p4_hg[1]);
    free(p4_hi[0]); free(p4_hi[1]);
    p4_ux = 0; p4_uf = 0;
    p4_hm[0] = p4_hm[1] = 0; p4_hg[0] = p4_hg[1] = 0; p4_hi[0] = p4_hi[1] = 0;
}

static void p4_setupup(int w, uint32_t sxs)
{
    int x, ok;
    uint32_t ax = 0;
    if (p4_uw == w && p4_ux) return;
    p4_freeup();
    p4_uw = 0;
    p4_ux = (int *)malloc(sizeof(int) * (size_t)w);
    p4_uf = (uint8_t *)malloc((size_t)w);
    p4_hm[0] = (uint8_t *)malloc((size_t)w);
    p4_hm[1] = (uint8_t *)malloc((size_t)w);
    p4_hg[0] = (uint16_t *)malloc(sizeof(uint16_t) * (size_t)w);
    p4_hg[1] = (uint16_t *)malloc(sizeof(uint16_t) * (size_t)w);
    p4_hi[0] = (uint16_t *)malloc(sizeof(uint16_t) * (size_t)w);
    p4_hi[1] = (uint16_t *)malloc(sizeof(uint16_t) * (size_t)w);
    ok = p4_ux && p4_uf && p4_hm[0] && p4_hm[1] && p4_hg[0] && p4_hg[1]
         && p4_hi[0] && p4_hi[1];
    if (!ok) { p4_freeup(); return; }
    for (x = 0; x < w; x++) {
        p4_ux[x] = (int)(ax >> 16);
        p4_uf[x] = (uint8_t)((ax >> 8) & 255);
        ax += sxs;
    }
    p4_uw = w;
}

static void p4_hrow(uint8_t *dm, uint16_t *dg, uint16_t *di, int iy, int w)
{
    const uint8_t  *m0 = p4_mask + iy * P4_IW;
    const uint16_t *g0 = p4_gnd + iy * P4_IW;
    const uint16_t *i0 = p4_ink + iy * P4_IW;
    int x;
    for (x = 0; x < w; x++) {
        int ix = p4_ux[x], f = p4_uf[x];
        int a = m0[ix], b = m0[ix + 1];
        dm[x] = (uint8_t)(a + (((b - a) * f) >> 8));
        a = g0[ix]; b = g0[ix + 1];
        dg[x] = (uint16_t)(a + (((b - a) * f) >> 8));
        a = i0[ix]; b = i0[ix + 1];
        di[x] = (uint16_t)(a + (((b - a) * f) >> 8));
    }
}

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
    p4_setupup(w, sxs);
    int cy = -2, slot = 0;
    for (int y = 0; y < h; y++) {
        uint32_t ay = sys * (uint32_t)y;
        int iy = (int)(ay >> 16), fyq = (int)((ay >> 8) & 255);
        uint32_t *dst = fb + (long)y * w;
        if (!p4_ux) {                              /* alloc failed: direct */
            uint32_t ax = 0;
            for (int x = 0; x < w; x++) {
                int ix = (int)(ax >> 16), fxq = (int)((ax >> 8) & 255);
                int m = p4_bilerp8(p4_mask, ix, fxq, iy, fyq);
                int gi = p4_bilerp16(p4_gnd, ix, fxq, iy, fyq);
                uint32_t ground = pal[gi & JD_PAL_MASK];
                uint32_t out;
                if (m == 0) out = ground;
                else {
                    int ii = p4_bilerp16(p4_ink, ix, fxq, iy, fyq);
                    uint32_t ink = pal[((uint32_t)ii + offink) & JD_PAL_MASK];
                    uint32_t gr = (ground >> 16) & 255, gg = (ground >> 8) & 255, gb = ground & 255;
                    uint32_t ir = (ink >> 16) & 255, ig = (ink >> 8) & 255, ib = ink & 255;
                    out = 0xFF000000u
                        | ((gr + (((ir - gr) * (uint32_t)m) >> 8)) << 16)
                        | ((gg + (((ig - gg) * (uint32_t)m) >> 8)) << 8)
                        |  (gb + (((ib - gb) * (uint32_t)m) >> 8));
                }
                dst[x] = out;
                ax += sxs;
            }
            continue;
        }
        if (iy != cy) {
            if (iy == cy + 1) slot ^= 1;           /* reuse the lower row */
            else p4_hrow(p4_hm[slot], p4_hg[slot], p4_hi[slot], iy, w);
            p4_hrow(p4_hm[slot ^ 1], p4_hg[slot ^ 1], p4_hi[slot ^ 1],
                    iy + 1, w);
            cy = iy;
        }
        {
            const uint8_t  *ma = p4_hm[slot], *mb = p4_hm[slot ^ 1];
            const uint16_t *ga = p4_hg[slot], *gb2 = p4_hg[slot ^ 1];
            const uint16_t *ia = p4_hi[slot], *ib2 = p4_hi[slot ^ 1];
            for (int x = 0; x < w; x++) {
                int t0 = ma[x], m = t0 + (((mb[x] - t0) * fyq) >> 8);
                uint32_t ground, out;
                t0 = ga[x];
                ground = pal[(t0 + (((gb2[x] - t0) * fyq) >> 8)) & JD_PAL_MASK];
                if (m == 0) out = ground;
                else {
                    int ii;
                    uint32_t ink, gr, gg, gbv, ir, ig, ibv;
                    t0 = ia[x];
                    ii = t0 + (((ib2[x] - t0) * fyq) >> 8);
                    ink = pal[((uint32_t)ii + offink) & JD_PAL_MASK];
                    gr = (ground >> 16) & 255; gg = (ground >> 8) & 255; gbv = ground & 255;
                    ir = (ink >> 16) & 255; ig = (ink >> 8) & 255; ibv = ink & 255;
                    out = 0xFF000000u
                        | ((gr + (((ir - gr) * (uint32_t)m) >> 8)) << 16)
                        | ((gg + (((ig - gg) * (uint32_t)m) >> 8)) << 8)
                        |  (gbv + (((ibv - gbv) * (uint32_t)m) >> 8));
                }
                dst[x] = out;
            }
        }
    }
}
