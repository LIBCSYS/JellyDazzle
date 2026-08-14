/* 071 Silk Currents — flow-field silk threads.
 * Port of lab/patterns/071_silk_currents/proto.py: 800 particles advected
 * 110 steps through a 4-term sine angle field, additive splats, exp tonemap.
 * Internal 320x240 canvas (same as lab), bilinear upscale to (w,h). */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define NP 800
#define NSTEP 110
#define FGX 81
#define FGY 61

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static uint8_t (*img)[GW][3];
static uint8_t (*bgp)[GW][3];
static float fld[FGY][FGX];
static float noderad[FGY][FGX];
static float sinlut[1024];
static uint8_t tonelut[2048];
static float wlut[NSTEP];
static float p_hue[NP], p_x0[NP], p_y0[NP];
static int inited;
static int last_sl = -1;
static int up_w = -1, up_h = -1;
static int *up_xi;
static uint8_t *up_fx;

static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs; }
static float rnd01(void) { return (float)(rnd() >> 8) * (1.0f / 16777216.0f); }

static float lsin(float a) { return sinlut[((int)(a * 162.97466f + 65536.5f)) & 1023]; }
static float lcos(float a) { return sinlut[((int)(a * 162.97466f + 65792.5f)) & 1023]; }

static float huetab[1024][3];
/* per-frame hue->rgb table from pal, brightness-normalized so dark ramp
 * zones still read as luminous silk (proto uses v=1 HSV). */
static void build_huetab(const uint32_t *pal) {
    for (int i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255), b = (float)(u & 255);
        float m = r > g ? r : g; if (b > m) m = b; if (m < 32.0f) m = 32.0f;
        float norm = 255.0f / m; if (norm > 2.8f) norm = 2.8f;
        norm *= (1.0f / 255.0f);
        huetab[i][0] = r * norm; huetab[i][1] = g * norm; huetab[i][2] = b * norm;
    }
}

static void splat(float x, float y, const float c[3], float w) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - (float)xi, fy = y - (float)yi;
    float w00 = (1 - fx) * (1 - fy) * w, w10 = fx * (1 - fy) * w;
    float w01 = (1 - fx) * fy * w, w11 = fx * fy * w;
    if ((unsigned)xi < GW && (unsigned)yi < GH) {
        acc[yi][xi][0] += c[0] * w00; acc[yi][xi][1] += c[1] * w00; acc[yi][xi][2] += c[2] * w00;
    }
    if ((unsigned)(xi + 1) < GW && (unsigned)yi < GH) {
        acc[yi][xi + 1][0] += c[0] * w10; acc[yi][xi + 1][1] += c[1] * w10; acc[yi][xi + 1][2] += c[2] * w10;
    }
    if ((unsigned)xi < GW && (unsigned)(yi + 1) < GH) {
        acc[yi + 1][xi][0] += c[0] * w01; acc[yi + 1][xi][1] += c[1] * w01; acc[yi + 1][xi][2] += c[2] * w01;
    }
    if ((unsigned)(xi + 1) < GW && (unsigned)(yi + 1) < GH) {
        acc[yi + 1][xi + 1][0] += c[0] * w11; acc[yi + 1][xi + 1][1] += c[1] * w11; acc[yi + 1][xi + 1][2] += c[2] * w11;
    }
}

static void soften1(void) {
    for (int y = 0; y < GH; y++) {
        int ym = (y + GH - 1) % GH, yp = (y + 1) % GH;
        for (int x = 0; x < GW; x++) {
            int xm = (x + GW - 1) % GW, xp = (x + 1) % GW;
            for (int k = 0; k < 3; k++)
                acc2[y][x][k] = acc[y][x][k] * 0.5f
                    + 0.125f * (acc[ym][x][k] + acc[yp][x][k]
                                + acc[y][xm][k] + acc[y][xp][k]);
        }
    }
    float (*tmp)[GW][3] = acc; acc = acc2; acc2 = tmp;
}

static void compose(void) {
    const float *a = &acc[0][0][0];
    const uint8_t *b = &bgp[0][0][0];
    uint8_t *o = &img[0][0][0];
    for (int i = 0; i < GW * GH * 3; i++) {
        int ti = (int)(a[i] * 64.0f);
        if (ti > 2047) ti = 2047;
        if (ti < 0) ti = 0;
        int v = b[i] + tonelut[ti];
        o[i] = (uint8_t)(v > 255 ? 255 : v);
    }
}

static void upscale(uint32_t *fb, int w, int h) {
    if (w != up_w) {
        free(up_xi); free(up_fx);
        up_xi = (int *)malloc(sizeof(int) * w);
        up_fx = (uint8_t *)malloc(w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8; if (xi > GW - 2) { xi = GW - 2; q = (GW - 1) * 256; }
            up_xi[x] = xi * 3; up_fx[x] = (uint8_t)(q & 255);
        }
        up_w = w;
    }
    up_h = h;
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8; if (yi > GH - 2) { yi = GH - 2; qy = (GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = &img[yi][0][0], *r1 = &img[yi + 1][0][0];
        uint32_t *out = fb + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            int X = up_xi[x], fx = up_fx[x];
            int c[3];
            for (int k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

static void init_tables(void) {
    for (int i = 0; i < 1024; i++) sinlut[i] = sinf((float)i * (6.2831853f / 1024.0f));
    for (int i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-((float)i / 64.0f) * 0.55f));
        tonelut[i] = (uint8_t)(v > 255 ? 255 : v);
    }
    for (int s = 0; s < NSTEP; s++) {
        float fs = (float)s / (float)NSTEP;
        wlut[s] = 0.05f + 0.55f * fs * fs;
    }
    acc = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    img = malloc(GH * GW * 3);
    bgp = malloc(GH * GW * 3);
    for (int gy = 0; gy < FGY; gy++)
        for (int gx = 0; gx < FGX; gx++)
            noderad[gy][gx] = hypotf((float)(gx * 4) - GW / 2.0f,
                                     (float)(gy * 4) - GH / 2.0f);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            float dx = ((float)x - GW / 2.0f) / GW, dy = ((float)y - GH / 2.0f) / GH;
            float r2 = dx * dx + dy * dy;
            bgp[y][x][0] = (uint8_t)(255.0f * (0.03f + 0.02f * (1 - r2)));
            bgp[y][x][1] = (uint8_t)(255.0f * (0.01f + 0.015f * (1 - r2)));
            bgp[y][x][2] = (uint8_t)(255.0f * (0.10f + 0.06f * (1 - r2)));
        }
}

void pattern_071(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; last_sl = -1; }
    if (last_sl < 0 || (sl == 0 && last_sl != 0)) {
        rs = seed ^ 0x47C0FFEEu;
        for (int i = 0; i < NP; i++) {
            p_hue[i] = rnd01();
            p_x0[i] = rnd01() * GW;
            p_y0[i] = rnd01() * GH;
        }
    }
    last_sl = sl;
    build_huetab(pal);
    float t = (float)frame;
    float T = t * 0.02f;
    float ph = T * 0.35f;
    /* angle field on 81x61 grid, 4px spacing */
    for (int gy = 0; gy < FGY; gy++)
        for (int gx = 0; gx < FGX; gx++) {
            float x = (float)(gx * 4), y = (float)(gy * 4);
            fld[gy][gx] = 1.15f * (lsin(x * 0.021f + ph)
                                   + lsin(y * 0.017f - ph * 0.7f)
                                   + lsin((x + y) * 0.011f + ph * 0.4f)
                                   + lsin(noderad[gy][gx] * 0.02f - ph * 0.5f));
        }
    memset(acc, 0, sizeof(float) * GH * GW * 3);
    for (int i = 0; i < NP; i++) {
        float lp = p_hue[i] * 6.28f;
        float x = p_x0[i] + lsin(T * 0.31f + lp) * 14.0f;
        float y = p_y0[i] + lcos(T * 0.23f + lp) * 10.0f;
        x -= floorf(x / GW) * GW;
        y -= floorf(y / GH) * GH;
        float hue = 0.52f + p_hue[i] * 0.45f + t * 0.0006f;
        for (int s = 0; s < NSTEP; s++) {
            /* bilerp angle field at (x/4, y/4) */
            float u = x * 0.25f, v = y * 0.25f;
            if (u < 0) u = 0; if (u > FGX - 1.001f) u = FGX - 1.001f;
            if (v < 0) v = 0; if (v > FGY - 1.001f) v = FGY - 1.001f;
            int ui = (int)u, vi = (int)v;
            float uf = u - ui, vf = v - vi;
            float a0 = fld[vi][ui] + (fld[vi][ui + 1] - fld[vi][ui]) * uf;
            float a1 = fld[vi + 1][ui] + (fld[vi + 1][ui + 1] - fld[vi + 1][ui]) * uf;
            float a = a0 + (a1 - a0) * vf;
            x += lcos(a) * 1.6f;
            y += lsin(a) * 1.6f;
            const float *c = huetab[((int)((hue + (float)s * 0.0012f) * 1024.0f)) & 1023];
            splat(x, y, c, wlut[s]);
        }
    }
    soften1();
    compose();
    upscale(fb, w, h);
    (void)sl;
}
