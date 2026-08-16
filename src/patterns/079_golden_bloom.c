/* 079 Golden Bloom — phyllotaxis sunflower head opening floret by floret.
 * Port of lab/patterns/079_golden_bloom/proto.py: up to 900 florets on the
 * golden-angle spiral, radius 6.4*sqrt(n) with a slow breath, a bloom wave
 * rippling outward through the head, fresh outer florets bright and pale,
 * 3x3 fat splat + 2 softens + exp tonemap. 320x240 canvas, upscaled. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define NMAX 900
#define GA 2.399963230f
#define ANGK 651.8986f              /* 4096 / 2pi */

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static uint8_t (*img)[GW][3];
static uint8_t (*bgp)[GW][3];
static uint8_t tonelut[2048];
static float huetab[1024][3];
static float sintab[4096];
static float sq[NMAX];
static int inited;
static int up_w = -1;
static int *up_xi;
static uint8_t *up_fx;

static float lsin(float a) { return sintab[((int)(a * ANGK + 4096.5f)) & 4095]; }

/* palette hue at full saturation, so the head stays vivid in every scheme */
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
    for (int i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-((float)i / 64.0f) * 0.85f));
        tonelut[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    for (int n = 0; n < NMAX; n++) sq[n] = sqrtf((float)n);
    acc  = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    img  = malloc(GH * GW * 3);
    bgp  = malloc(GH * GW * 3);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            float dx = ((float)x - GW / 2.0f) / GW, dy = ((float)y - GH / 2.0f) / GH;
            float u = 1.0f - (dx * dx + dy * dy);
            bgp[y][x][0] = (uint8_t)(255.0f * (0.015f + 0.02f * u));
            bgp[y][x][1] = (uint8_t)(255.0f * (0.040f + 0.03f * u));
            bgp[y][x][2] = (uint8_t)(255.0f * (0.015f + 0.02f * u));
        }
}

void pattern_079(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; }
    build_huetab(pal);
    float t = (float)frame;
    float T = (float)sl + 60.0f;
    float nf = 80.0f + T * 1.2f;
    if (nf > NMAX) nf = NMAX;
    int N = (int)nf;
    float rot = t * 0.0025f;
    float breathe = 6.4f * (1.0f + 0.03f * lsin(t * 0.01f));
    float wph = -t * 0.035f;
    float hueoff = (float)((seed >> 3) & 1023) * (1.0f / 1024.0f) + t * 0.0008f;

    memset(acc, 0, sizeof(float) * GH * GW * 3);
    for (int n = 0; n < N; n++) {
        float th = (float)n * GA + rot;
        int ai = (int)(th * ANGK + 4096.5f);
        float r = breathe * sq[n];
        float x = GW / 2.0f + r * sintab[(ai + 1024) & 4095];
        float y = GH / 2.0f + r * sintab[ai & 4095] * 0.80f;
        if (x < -2.0f || x > GW + 2.0f || y < -2.0f || y > GH + 2.0f) continue;
        float wave = 0.5f + 0.5f * lsin(sq[n] * 1.8f + wph);
        float young = 1.0f - (float)(N - 1 - n) / 60.0f;
        if (young < 0.0f) young = 0.0f; if (young > 1.0f) young = 1.0f;
        float sat = 0.95f - young * 0.5f; if (sat < 0.5f) sat = 0.5f;
        float val = 0.75f + 0.25f * wave + young * 0.3f;
        if (val > 1.0f) val = 1.0f;
        float col[3];
        palcol((float)n * 0.0045f + hueoff, sat, val, col);
        float wt = 2.6f + 3.6f * wave + 4.0f * young;
        splat(x, y, col, wt);
        splat(x - 1.0f, y, col, wt * 0.6f); splat(x + 1.0f, y, col, wt * 0.6f);
        splat(x, y - 1.0f, col, wt * 0.6f); splat(x, y + 1.0f, col, wt * 0.6f);
        splat(x - 1.0f, y - 1.0f, col, wt * 0.35f); splat(x + 1.0f, y - 1.0f, col, wt * 0.35f);
        splat(x - 1.0f, y + 1.0f, col, wt * 0.35f); splat(x + 1.0f, y + 1.0f, col, wt * 0.35f);
    }
    soften1();
    soften1();
    compose();
    upscale(fb, w, h);
}
