/* 077 Mycelium Veil — 140 hyphae creeping out of a warm centre.
 * Port of lab/patterns/077_mycelium_veil/proto.py: each filament integrates a
 * unit step whose heading is a fixed random walk plus a slow global rotation
 * and a breathing sine; growth reveals steps outward, the last 7 steps glow as
 * a tip, spore nodes pulse at fixed arc positions. 320x240 canvas, upscaled. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define NF 140
#define SMAX 260

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static uint8_t (*img)[GW][3];
static uint8_t (*bgp)[GW][3];
static uint8_t tonelut[2048];
static float huetab[1024][3];
static float sintab[4096];
static float *wand;                 /* NF * SMAX heading random walk */
static float th0[NF], spdf[NF], wph[NF], huef[NF];
static float vlut[SMAX], slut[SMAX];
static int inited;
static uint32_t last_seed = 0xFFFFFFFFu;
static int up_w = -1;
static int *up_xi;
static uint8_t *up_fx;

static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs; }
static float rnd01(void) { return (float)(rnd() >> 8) * (1.0f / 16777216.0f); }
static float gauss(void) {
    return (rnd01() + rnd01() + rnd01() + rnd01() - 2.0f) * 1.7320508f;
}
#define ANGK 651.8986f              /* 4096 / 2pi */
static float lsin(float a) { return sintab[((int)(a * ANGK + 4096.5f)) & 4095]; }

static void build_huetab(const uint32_t *pal) {
    for (int i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255), b = (float)(u & 255);
        float m = r > g ? r : g; if (b > m) m = b; if (m < 24.0f) m = 24.0f;
        float n = 1.0f / m;
        huetab[i][0] = r * n; huetab[i][1] = g * n; huetab[i][2] = b * n;
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
    for (int i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-((float)i / 64.0f) * 0.7f));
        tonelut[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    for (int s = 0; s < SMAX; s++) {
        vlut[s] = 0.5f + 0.5f * expf(-(float)s / 160.0f);
        slut[s] = (float)s * 0.15f;
    }
    acc  = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    img  = malloc(GH * GW * 3);
    bgp  = malloc(GH * GW * 3);
    wand = malloc(sizeof(float) * NF * SMAX);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            float dx = ((float)x - GW / 2.0f) / GW, dy = ((float)y - GH / 2.0f) / GH;
            float g = expf(-(dx * dx + dy * dy) * 4.0f);
            bgp[y][x][0] = (uint8_t)(255.0f * (0.060f + 0.06f * g));
            bgp[y][x][1] = (uint8_t)(255.0f * (0.025f + 0.03f * g));
            bgp[y][x][2] = (uint8_t)(255.0f * (0.020f + 0.02f * g));
        }
}

void pattern_077(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; }
    if (seed != last_seed) {
        rs = seed ^ 0x11FE1Du; rnd(); rnd();
        for (int i = 0; i < NF; i++) {
            th0[i]  = rnd01() * 6.2831853f;
            spdf[i] = 0.6f + rnd01() * 0.8f;
            wph[i]  = rnd01() * 6.2831853f;
            float hj = rnd01();
            huef[i] = (rnd01() < 0.3f) ? (0.90f + 0.05f * hj) : (0.05f + 0.07f * hj);
            float c = 0.0f;
            float *wp = wand + (size_t)i * SMAX;
            for (int s = 0; s < SMAX; s++) { c += gauss() * 0.085f; wp[s] = c; }
        }
        last_seed = seed;
    }
    build_huetab(pal);
    float t = (float)frame;
    float T = (float)sl + 130.0f;
    float rot = t * 0.0015f;
    float hueoff = (float)((seed >> 5) & 1023) * (1.0f / 1024.0f) + t * 0.0002f;
    float bph = t * 0.006f;
    float rip = -t * 0.03f;

    memset(acc, 0, sizeof(float) * GH * GW * 3);
    for (int i = 0; i < NF; i++) {
        float nf = T * 0.33f * spdf[i];
        if (nf > SMAX) nf = SMAX;
        int n = (int)nf;
        if (n < 1) continue;
        int tips = (int)(nf - 7.0f);
        const float *wp = wand + (size_t)i * SMAX;
        float base = th0[i] + rot;
        float bp = wph[i] + bph;
        float col[3], c0[3];
        palcol(huef[i] + hueoff, 0.85f, 1.0f, c0);
        palcol(huef[i] + hueoff + 0.04f, 0.6f, 1.0f, col);
        float x = GW / 2.0f, y = GH / 2.0f;
        float nodepulse = t * 0.025f + th0[i];
        for (int s = 0; s < n; s++) {
            float th = base + wp[s] + 0.25f * lsin((float)s * 0.03f + bp);
            int ai = ((int)(th * ANGK + 4096.5f));
            x += sintab[(ai + 1024) & 4095] * 1.05f;
            y += sintab[ai & 4095] * 0.85f;
            float wgt = 0.55f + 0.25f * lsin(slut[s] + rip);
            if (s > tips) wgt += 2.2f;
            float v = vlut[s];
            float cc[3] = { c0[0] * v, c0[1] * v, c0[2] * v };
            splat(x, y, cc, wgt);
            if (s >= 40 && ((s - 40) % 60) == 0)
                splat(x, y, col, 1.5f + 1.0f * lsin(nodepulse + (float)s * 0.2f));
        }
    }
    soften1();
    compose();
    upscale(fb, w, h);
}
