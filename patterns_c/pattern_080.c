/* 080 Tendril Rose — two counter-rotating rose curves drawn as a mandala.
 * Port of lab/patterns/080_tendril_rose/proto.py: r(phi) = R*|sin(k*phi + drift)|^0.75
 * + wobble, sampled progressively (bright drawing head), replicated by 4
 * rotations plus a phi-mirror for 8-fold symmetry, outer violet rosette over a
 * tighter inner one in the complementary hue. 320x240 canvas, upscaled. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define SMAX 4200
#define ANGK 651.8986f              /* 4096 / 2pi */

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static uint8_t (*img)[GW][3];
static uint8_t (*bgp)[GW][3];
static uint8_t tonelut[2048];
static float huetab[1024][3];
static float sintab[4096];
static float p075[1025];
static float rr[SMAX];
static float cc[SMAX][3];
static float ww[SMAX];
static int inited;
static int up_w = -1;
static int *up_xi;
static uint8_t *up_fx;

static float lsin(float a) { return sintab[((int)(a * ANGK + 4096.5f)) & 4095]; }

/* palette hue at full saturation (proto uses S=0.95 HSV) */
static void build_huetab(const uint32_t *pal) {
    for (int i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float c[3] = { (float)((u >> 16) & 255), (float)((u >> 8) & 255), (float)(u & 255) };
        float mx = c[0] > c[1] ? c[0] : c[1]; if (c[2] > mx) mx = c[2];
        float mn = c[0] < c[1] ? c[0] : c[1]; if (c[2] < mn) mn = c[2];
        float d = mx - mn;
        if (d < 1.0f) { huetab[i][0] = huetab[i][1] = huetab[i][2] = 1.0f; continue; }
        for (int k = 0; k < 3; k++) huetab[i][k] = (c[k] - mn) / d;
    }
}
static void palcol(float hue, float sat, float val, float *out) {
    const float *c = huetab[(int)(hue * 1024.0f + 1024.0f) & 1023];
    float w = 1.0f - sat;
    out[0] = (w + sat * c[0]) * val;
    out[1] = (w + sat * c[1]) * val;
    out[2] = (w + sat * c[2]) * val;
}

static void splat(float x, float y, const float *c, float w) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - (float)xi, fy = y - (float)yi;
    float w00 = (1 - fx) * (1 - fy) * w, w10 = fx * (1 - fy) * w;
    float w01 = (1 - fx) * fy * w, w11 = fx * fy * w;
    if ((unsigned)xi < GW && (unsigned)yi < GH) {
        acc[yi][xi][0] += c[0]*w00; acc[yi][xi][1] += c[1]*w00; acc[yi][xi][2] += c[2]*w00;
    }
    if ((unsigned)(xi+1) < GW && (unsigned)yi < GH) {
        acc[yi][xi+1][0] += c[0]*w10; acc[yi][xi+1][1] += c[1]*w10; acc[yi][xi+1][2] += c[2]*w10;
    }
    if ((unsigned)xi < GW && (unsigned)(yi+1) < GH) {
        acc[yi+1][xi][0] += c[0]*w01; acc[yi+1][xi][1] += c[1]*w01; acc[yi+1][xi][2] += c[2]*w01;
    }
    if ((unsigned)(xi+1) < GW && (unsigned)(yi+1) < GH) {
        acc[yi+1][xi+1][0] += c[0]*w11; acc[yi+1][xi+1][1] += c[1]*w11; acc[yi+1][xi+1][2] += c[2]*w11;
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
        up_xi = (int *)malloc(sizeof(int) * (size_t)w);
        up_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8; if (xi > GW - 2) { xi = GW - 2; q = (GW - 1) * 256; }
            up_xi[x] = xi * 3; up_fx[x] = (uint8_t)(q & 255);
        }
        up_w = w;
    }
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
                int t0 = r0[X+k] + (((r0[X+3+k] - r0[X+k]) * fx) >> 8);
                int t1 = r1[X+k] + (((r1[X+3+k] - r1[X+k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

static void init_tables(void) {
    for (int i = 0; i < 4096; i++) sintab[i] = sinf((float)i * (6.2831853f / 4096.0f));
    for (int i = 0; i <= 1024; i++) p075[i] = powf((float)i / 1024.0f, 0.75f);
    for (int i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-((float)i / 64.0f) * 0.55f));
        tonelut[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    acc  = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    img  = malloc(GH * GW * 3);
    bgp  = malloc(GH * GW * 3);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            float dx = ((float)x - GW / 2.0f) / GW, dy = ((float)y - GH / 2.0f) / GH;
            float u = 1.0f - (dx * dx + dy * dy);
            bgp[y][x][0] = (uint8_t)(255.0f * (0.050f + 0.03f * u));
            bgp[y][x][1] = (uint8_t)(255.0f * 0.008f);
            bgp[y][x][2] = (uint8_t)(255.0f * (0.090f + 0.05f * u));
        }
}

static void layer(float t, int nvis, float R, float kpet, float wob,
                  float hue0, float dir, float vscale) {
    float dphi = 0.0075f * dir;
    float kd = t * 0.0011f * dir;
    float wd = -t * 0.009f;
    float hbase = hue0 + t * 0.0004f;
    for (int s = 0; s < nvis; s++) {
        float phi = (float)s * dphi;
        float sv = lsin(phi * kpet + kd);
        if (sv < 0.0f) sv = -sv;
        int pi = (int)(sv * 1024.0f); if (pi > 1024) pi = 1024;
        rr[s] = R * p075[pi] + wob * lsin(phi * 7.0f + wd);
        float head = 1.0f - (float)(nvis - 1 - s) / 240.0f;
        if (head < 0.0f) head = 0.0f; if (head > 1.0f) head = 1.0f;
        ww[s] = (0.5f + 1.6f * head * head) * vscale;
        float val = (0.5f + 0.5f * lsin((float)s * 0.02f + t * 0.01f)) * 0.3f + 0.6f;
        palcol(hbase + (float)s * 0.00008f, 0.95f, val, cc[s]);
    }
    float rot = t * 0.002f * dir;
    for (int k = 0; k < 4; k++) {
        float a = rot + (float)k * 1.5707963f;
        int ia = (int)(a * ANGK + 4096.5f);
        for (int s = 0; s < nvis; s++) {
            float phi = (float)s * dphi;
            int ip = (int)(phi * ANGK);
            int i1 = (ia + ip) & 4095, i2 = (ia - ip) & 4095;
            float r = rr[s];
            splat(GW / 2.0f + r * sintab[(i1 + 1024) & 4095],
                  GH / 2.0f + r * sintab[i1] * 0.85f, cc[s], ww[s]);
            splat(GW / 2.0f + r * sintab[(i2 + 1024) & 4095],
                  GH / 2.0f + r * sintab[i2] * 0.85f, cc[s], ww[s]);
        }
    }
}

void pattern_080(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; }
    build_huetab(pal);
    float t = (float)frame;
    float T = (float)sl + 60.0f;
    float hueoff = (float)((seed >> 13) & 1023) * (1.0f / 1024.0f);

    float n1 = 350.0f + T * 5.1f; if (n1 > SMAX) n1 = SMAX;
    float n2 = 240.0f + T * 3.5f; if (n2 > SMAX * 0.7f) n2 = SMAX * 0.7f;

    memset(acc, 0, sizeof(float) * GH * GW * 3);
    layer(t, (int)n1, 105.0f, 2.5f, 8.0f, 0.78f + hueoff,  1.0f, 1.0f);
    layer(t, (int)n2,  58.0f, 3.5f, 5.0f, 0.12f + hueoff, -1.0f, 0.8f);
    soften1();
    compose();
    upscale(fb, w, h);
}
