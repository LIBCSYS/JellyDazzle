/* 072 Vine Waltz — climbing vines with fronds, tendril curls and blossoms.
 * Port of lab/patterns/072_vine_waltz/proto.py. Growth is driven by sl
 * (segment-local), so the hedge re-sprouts each segment. Internal 320x240
 * canvas, bilinear upscale. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define NV 14
#define SMAX 560
#define CURL 414   /* 0.74 * SMAX */

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static uint8_t (*img)[GW][3];
static uint8_t (*bgp)[GW][3];
static float sinlut[1024];
static uint8_t tonelut[2048];
static int inited;
static int last_sl = -1;
static int up_w = -1;
static int *up_xi;
static uint8_t *up_fx;

static float v_anch[NV], v_ph1[NV], v_ph2[NV], v_spd[NV], v_side[NV], v_hue0[NV];

static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs; }
static float rnd01(void) { return (float)(rnd() >> 8) * (1.0f / 16777216.0f); }

static float lsin(float a) { return sinlut[((int)(a * 162.97466f + 65536.5f)) & 1023]; }
static float lcos(float a) { return sinlut[((int)(a * 162.97466f + 65792.5f)) & 1023]; }

static float huetab[1024][3];
static void build_huetab(const uint32_t *pal) {
    for (int i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255), b = (float)(u & 255);
        float m = r > g ? r : g; if (b > m) m = b; if (m < 32.0f) m = 32.0f;
        float norm = 255.0f / m; if (norm > 2.6f) norm = 2.6f;
        norm *= (1.0f / 255.0f);
        huetab[i][0] = r * norm; huetab[i][1] = g * norm; huetab[i][2] = b * norm;
    }
}
/* proto stem hue 0.13..0.36 -> palette ramp 0.30..0.20 (gold..emerald) */
static const float *stem_col(float h) {
    float q = 0.20f + (0.30f - h) * 0.588f;
    return huetab[((int)(q * 1024.0f + 1024.0f)) & 1023];
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
            int xi = q >> 8; if (xi > GW - 2) xi = GW - 2;
            up_xi[x] = xi * 3; up_fx[x] = (uint8_t)(q & 255);
        }
        up_w = w;
    }
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8; if (yi > GH - 2) yi = GH - 2;
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
        float v = 255.0f * (1.0f - expf(-((float)i / 64.0f) * 0.6f));
        tonelut[i] = (uint8_t)(v > 255 ? 255 : v);
    }
    acc = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    img = malloc(GH * GW * 3);
    bgp = malloc(GH * GW * 3);
    for (int y = 0; y < GH; y++) {
        float g = 1.0f - (float)y / GH;
        for (int x = 0; x < GW; x++) {
            bgp[y][x][0] = (uint8_t)(255.0f * (0.10f + 0.05f * g));
            bgp[y][x][1] = (uint8_t)(255.0f * (0.02f + 0.02f * g));
            bgp[y][x][2] = (uint8_t)(255.0f * (0.13f + 0.08f * g));
        }
    }
}

void pattern_072(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; last_sl = -1; }
    if (last_sl < 0 || (sl == 0 && last_sl != 0)) {
        rs = seed ^ 0x72A5C3E1u;
        for (int i = 0; i < NV; i++) {
            v_anch[i] = 18.0f + (GW - 36.0f) * (float)i / (NV - 1) + (rnd01() * 14.0f - 7.0f);
            v_ph1[i] = rnd01() * 6.28f;
            v_ph2[i] = rnd01() * 6.28f;
            v_spd[i] = 0.75f + rnd01() * 0.55f;
            v_side[i] = (rnd() & 1) ? 1.0f : -1.0f;
            v_hue0[i] = 0.30f + (rnd01() * 0.08f - 0.04f);
        }
    }
    last_sl = sl;
    build_huetab(pal);
    float t = (float)sl;
    float T = t + 150.0f;
    memset(acc, 0, sizeof(float) * GH * GW * 3);
    for (int i = 0; i < NV; i++) {
        float fn = T * v_spd[i] * 0.95f;
        int n = fn < SMAX ? (int)fn : SMAX;
        if (n < 6) continue;
        float heading_sum = 0.0f;
        float x = v_anch[i], y = GH - 4.0f;
        int leaf_next = 26;       /* next leaf node index */
        int leaf_k = 0;           /* which leaf (for side alternation) */
        float lastx = x, lasty = y, lasth = 0.0f;
        for (int s = 0; s < n; s++) {
            float curl = s > CURL ? (float)(s - CURL) * 0.006f * v_side[i] : 0.0f;
            float turn = 0.032f * lsin((float)s * 0.05f + v_ph1[i])
                       + 0.014f * lsin((float)s * 0.013f + v_ph2[i]) + curl;
            heading_sum += turn;
            float heading = -1.5707963f + heading_sum
                + 0.07f * lsin(t * 0.014f + v_ph1[i] + (float)s * 0.004f);
            float ch = lcos(heading), sh = lsin(heading);
            x += ch * 0.72f;
            y += sh * 0.72f;
            float frac = (float)s / (float)SMAX;
            float hue = v_hue0[i] - 0.17f * frac + 0.02f * lsin((float)s * 0.05f);
            const float *c = stem_col(hue);
            float v = 0.6f + 0.4f * frac;
            float px = -sh * 0.6f, py = ch * 0.6f;
            splat(x, y, c, 1.2f * v);
            splat(x + px, y + py, c, 0.8f * v);
            splat(x - px, y - py, c, 0.8f * v);
            if (s == leaf_next) {
                float sgn = ((leaf_k & 1) == 0 ? 1.0f : -1.0f) * v_side[i];
                for (int k = 0; k < 5; k++) {
                    static const float rr[5] = { 1.5f, 3.0f, 4.5f, 6.0f, 7.2f };
                    float la = heading + sgn * (1.35f - (float)k * 0.13f);
                    float lx = x + lcos(la) * rr[k];
                    float ly = y + lsin(la) * rr[k];
                    float lw = 2.2f - (float)k * 0.35f;
                    const float *lc = stem_col(0.30f + 0.05f * lsin((float)s * 0.3f));
                    splat(lx, ly, lc, lw * 0.85f);
                }
                leaf_next += 34; leaf_k++;
            }
            lastx = x; lasty = y; lasth = heading;
        }
        (void)lasth;
        if (n > CURL + 30) {
            float pulse = 2.5f + 1.2f * lsin(t * 0.03f + v_ph2[i]);
            float qf = 0.70f + 0.04f * lsin(v_ph1[i]);
            const float *fc = huetab[((int)(qf * 1024.0f + 1024.0f)) & 1023];
            for (int a = 0; a < 9; a++) {
                float fa = (float)a * (6.28f / 9.0f);
                splat(lastx + lcos(fa) * 1.2f, lasty + lsin(fa) * 1.2f, fc, pulse);
                splat(lastx + lcos(fa) * 2.6f, lasty + lsin(fa) * 2.6f, fc, pulse);
            }
        }
    }
    soften1();
    compose();
    upscale(fb, w, h);
    (void)frame;
}
