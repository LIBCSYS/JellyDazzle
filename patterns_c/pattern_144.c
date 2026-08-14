/* 144 Newton Filigree — the boundary of Newton's method, drawn as lace.
 * Every pixel is a starting point for the (over-)relaxed Newton iteration on
 * z^3 = c:   z <- z - a (z^3 - c) / (3 z^2).
 * Interiors converge in three or four steps and are left almost black; only
 * the fractal boundary between the three basins still has residual after
 * eight steps, and that is what is lit. The relaxation factor a breathes
 * through 0.86..1.34 and c walks around the unit circle, which drags the
 * whole filigree through a continuous family of shapes — bulbs open into
 * spirals, spirals braid, braids close again. Each basin carries its own
 * palette third, so the lace is tri-tone. Computed at 320x240, temporally
 * eased 3:1 against the previous frame (kills boundary shimmer), bilinear
 * upscale. Black between the threads: a screen/max layer. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P144_GW 320
#define P144_GH 240
#define P144_N  (P144_GW * P144_GH)

static float p144_v[P144_N];        /* eased lace intensity */
static uint8_t p144_root[P144_N];
static uint8_t p144_img[P144_N * 3];
static uint8_t p144_tone[1024];
static int p144_ready, p144_last = -1;
static int p144_uw = -1;
static int *p144_xi;
static uint8_t *p144_fx;

static void p144_init(void)
{
    for (int i = 0; i < 1024; i++) {
        float r = (float)i * (1.0f / 1024.0f);        /* residual 0..1 */
        float v = 1.0f - expf(-r * 26.0f);
        p144_tone[i] = (uint8_t)(v * 255.0f);
    }
    p144_ready = 1;
}

static void p144_upscale(uint32_t *fb, int w, int h)
{
    if (w != p144_uw) {
        free(p144_xi); free(p144_fx);
        p144_xi = (int *)malloc(sizeof(int) * (size_t)w);
        p144_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (P144_GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8;
            if (xi > P144_GW - 2) { xi = P144_GW - 2; q = (P144_GW - 1) * 256; }
            p144_xi[x] = xi * 3; p144_fx[x] = (uint8_t)(q & 255);
        }
        p144_uw = w;
    }
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (P144_GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8;
        if (yi > P144_GH - 2) { yi = P144_GH - 2; qy = (P144_GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = p144_img + (size_t)yi * P144_GW * 3;
        const uint8_t *r1 = r0 + P144_GW * 3;
        uint32_t *out = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            int X = p144_xi[x], fx = p144_fx[x], c[3];
            for (int k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16)
                   | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

void pattern_144(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    if (!p144_ready) p144_init();
    int fresh = (p144_last < 0 || (sl == 0 && p144_last != 0));
    p144_last = sl;

    const float t = (float)frame;
    const float sd = (float)(seed & 1023u) * 0.006136f;
    const float th = t * 0.00091f + sd;                    /* c walks the circle */
    const float cr = cosf(th), ci = sinf(th);
    const float a = 1.10f + 0.24f * sinf(t * 0.00047f + sd * 2.1f);
    const float zoom = 3.05f + 0.75f * sinf(t * 0.00031f + sd * 1.3f);
    const float vrot = t * 0.00043f + sd * 0.7f;
    const float ox = 0.28f * sinf(t * 0.00023f + sd), oy = 0.28f * cosf(t * 0.00019f);

    /* the three roots of z^3 = c */
    float rx[3], ry[3];
    for (int k = 0; k < 3; k++) {
        float p = (th + 6.28318531f * (float)k) * (1.0f / 3.0f);
        rx[k] = cosf(p); ry[k] = sinf(p);
    }

    const float sc = zoom / (float)P144_GH;
    const float cvr = cosf(vrot) * sc, svr = sinf(vrot) * sc;
    const float hx = (float)P144_GW * 0.5f, hy = (float)P144_GH * 0.5f;
    const float ease = fresh ? 1.0f : 0.25f;

    for (int y = 0; y < P144_GH; y++) {
        float py = (float)y - hy;
        float zx0 = -hx * cvr - py * svr + ox;
        float zy0 = -hx * svr + py * cvr + oy;
        float *vr = p144_v + (size_t)y * P144_GW;
        uint8_t *rt = p144_root + (size_t)y * P144_GW;
        for (int x = 0; x < P144_GW; x++, zx0 += cvr, zy0 += svr) {
            float zx = zx0, zy = zy0;
            for (int it = 0; it < 8; it++) {
                float x2 = zx * zx - zy * zy, y2 = 2.0f * zx * zy;        /* z^2 */
                float x3 = x2 * zx - y2 * zy, y3 = x2 * zy + y2 * zx;     /* z^3 */
                float nr = x3 - cr, ni = y3 - ci;                          /* z^3-c */
                float dr = 3.0f * x2, di = 3.0f * y2;                      /* 3z^2 */
                float q = dr * dr + di * di;
                if (q < 1e-9f) { zx += 0.7f; zy += 0.3f; continue; }
                float inv = a / q;
                zx -= (nr * dr + ni * di) * inv;
                zy -= (ni * dr - nr * di) * inv;
                if (zx > 40.0f || zx < -40.0f) zx = 0.4f;
                if (zy > 40.0f || zy < -40.0f) zy = 0.3f;
            }
            /* residual and nearest root */
            float x2 = zx * zx - zy * zy, y2 = 2.0f * zx * zy;
            float x3 = x2 * zx - y2 * zy, y3 = x2 * zy + y2 * zx;
            float er = x3 - cr, ei = y3 - ci;
            float res = er * er + ei * ei;
            if (res > 1.0f) res = 1.0f;
            int best = 0; float bd = 1e30f;
            for (int k = 0; k < 3; k++) {
                float ddx = zx - rx[k], ddy = zy - ry[k];
                float d = ddx * ddx + ddy * ddy;
                if (d < bd) { bd = d; best = k; }
            }
            rt[x] = (uint8_t)best;
            vr[x] += (res - vr[x]) * ease;
        }
    }

    const int cidx = (int)(t * 1.1f) + (int)(seed & 8191u);
    uint8_t *o = p144_img;
    for (int i = 0; i < P144_N; i++) {
        int ti = (int)(p144_v[i] * 1023.0f);
        if (ti > 1023) ti = 1023; if (ti < 0) ti = 0;
        int v8 = p144_tone[ti];
        uint32_t c = pal[(uint32_t)(cidx + p144_root[i] * 3900 + ((v8 * 1500) >> 8))
                         & JD_PAL_MASK];
        *o++ = (uint8_t)((((c >> 16) & 255u) * (uint32_t)v8) >> 8);
        *o++ = (uint8_t)((((c >> 8) & 255u) * (uint32_t)v8) >> 8);
        *o++ = (uint8_t)(((c & 255u) * (uint32_t)v8) >> 8);
    }
    p144_upscale(fb, w, h);
}
