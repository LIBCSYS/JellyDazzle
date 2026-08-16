/* 076 Kelp Cathedral — a swaying kelp forest seen from the sea floor.
 * Port of lab/patterns/076_kelp_cathedral/proto.py: 17 anchored strands that
 * grow upward, sway with two sine harmonics scaled by rise, sprout alternating
 * fronds, glint gold at the tips; rising bubbles; water column with slow god
 * rays. 320x240 canvas, additive splat + exp tonemap, bilinear upscale. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define NS 17

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static uint8_t (*img)[GW][3];
static float (*bgf)[GW][3];
static uint8_t tonelut[2048];
static float huetab[1024][3];
static float sintab[4096];
static float anch[NS], sph[NS], spd[NS], samp[NS], shue[NS];
static uint8_t sfar[NS];
static float rowsin[GH], rowdep[GH];
static int xphase[GW];
static float sxs[GH];
static int inited;
static uint32_t last_seed = 0xFFFFFFFFu;
static int up_w = -1;
static int *up_xi;
static uint8_t *up_fx;

static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs; }
static float rnd01(void) { return (float)(rnd() >> 8) * (1.0f / 16777216.0f); }
static float lsin(float a) { return sintab[((int)(a * 651.8986f + 4096.5f)) & 4095]; }

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
    const float *b = &bgf[0][0][0];
    uint8_t *o = &img[0][0][0];
    for (int i = 0; i < GW * GH * 3; i++) {
        int ti = (int)(a[i] * 64.0f);
        if (ti > 2047) ti = 2047;
        if (ti < 0) ti = 0;
        int v = (int)(b[i] * 255.0f) + tonelut[ti];
        o[i] = (uint8_t)(v > 255 ? 255 : (v < 0 ? 0 : v));
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
        float v = 255.0f * (1.0f - expf(-((float)i / 64.0f) * 0.75f));
        tonelut[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    for (int y = 0; y < GH; y++) {
        rowsin[y] = sinf((float)y * 0.01f);
        rowdep[y] = 1.0f - (float)y / GH;
    }
    for (int x = 0; x < GW; x++)
        xphase[x] = (int)((float)x * 0.045f * 651.8986f + 4096.5f);
    acc  = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    img  = malloc(GH * GW * 3);
    bgf  = malloc(sizeof(float) * GH * GW * 3);
}

/* water column: depth gradient + slow god rays */
static void build_bg(float t) {
    for (int y = 0; y < GH; y++) {
        float up = rowdep[y];
        float u16 = powf(up, 1.6f);
        int roff = (int)((rowsin[y] - t * 0.006f) * 651.8986f);
        float b0 = 0.01f + 0.015f * up, b1 = 0.05f + 0.10f * up, b2 = 0.10f + 0.13f * up;
        for (int x = 0; x < GW; x++) {
            float s = 0.5f + 0.5f * sintab[(xphase[x] + roff) & 4095];
            s = s * s; s = s * s;
            float ray = s * u16 * 0.35f;
            bgf[y][x][0] = b0 + ray * 0.15f;
            bgf[y][x][1] = b1 + ray * 0.50f;
            bgf[y][x][2] = b2 + ray * 0.55f;
        }
    }
}

void pattern_076(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; }
    if (seed != last_seed) {
        rs = seed ^ 0x4E1B00Du; rnd(); rnd();
        for (int i = 0; i < NS; i++) {
            anch[i] = 12.0f + (float)i * ((GW - 24.0f) / (NS - 1)) + (rnd01() * 12.0f - 6.0f);
            sph[i]  = rnd01() * 6.2831853f;
            spd[i]  = 0.75f + rnd01() * 0.55f;
            samp[i] = 9.0f + rnd01() * 8.0f;
            shue[i] = (rnd01() < 0.2f) ? 0.94f : (0.26f + rnd01() * 0.12f);
            sfar[i] = rnd01() < 0.35f;
        }
        last_seed = seed;
    }
    build_huetab(pal);
    float t = (float)frame;
    float T = (float)sl + 160.0f;
    float hueoff = (float)((seed >> 11) & 1023) * (1.0f / 1024.0f);

    build_bg(t);
    memset(acc, 0, sizeof(float) * GH * GW * 3);

    for (int i = 0; i < NS; i++) {
        float dm = sfar[i] ? 0.35f : 1.0f;
        float lim = GH - (sfar[i] ? 60.0f : 16.0f);
        float Li = T * 0.35f * spd[i]; if (Li > lim) Li = lim;
        int n = (int)Li;
        if (n < 6) continue;
        float p1 = t * 0.018f * spd[i] + sph[i];
        float p2 = -t * 0.011f + sph[i] * 2.0f;
        for (int k = 0; k < n; k++) {
            float ys = (float)(GH - k);
            float rise = (GH - ys) / GH;
            float r12 = rise * sqrtf(rise); /* rise^1.5 ~ proto rise^1.2 shape */
            r12 = powf(rise, 1.2f);
            float sway = lsin(ys * 0.028f + p1) * samp[i] * r12
                       + lsin(ys * 0.011f + p2) * 4.0f * rise;
            float x = anch[i] + sway;
            sxs[k] = x;
            float v = (0.5f + 0.5f * rise) * dm;
            float col[3];
            palcol(shue[i] + hueoff + 0.05f * lsin(ys * 0.02f + sph[i]), 0.85f, v, col);
            splat(x - 0.9f, ys, col, 0.8f * dm);
            splat(x,        ys, col, 1.5f * dm);
            splat(x + 0.9f, ys, col, 0.8f * dm);
        }
        /* fronds along the strand, alternating sides */
        for (int bi = 18, q = 0; bi < n; bi += 24, q++) {
            float sgn = (q & 1) ? -1.0f : 1.0f;
            float flex = 0.6f + 0.4f * lsin(t * 0.02f + bi * 0.5f + sph[i]);
            float ys = (float)(GH - bi);
            for (int k = 0; k < 7; k++) {
                float r_ = 1.5f + 1.5f * k;
                float bw = 2.4f - k * 0.3f; if (bw < 0.25f) bw = 0.25f;
                float bh = 0.20f + 0.10f * lsin(bi * 0.3f) + 0.04f * k;
                float col[3];
                palcol(bh + hueoff, 0.9f, 0.9f * dm, col);
                splat(sxs[bi] + sgn * r_, ys - r_ * 0.55f * flex + r_ * r_ * 0.045f,
                      col, bw * dm);
            }
        }
        /* gold glint on the growing tip */
        if (!sfar[i]) {
            float glim = 2.0f + 1.2f * lsin(t * 0.025f + sph[i]);
            float col[3];
            palcol(0.13f + hueoff, 0.55f, 1.0f, col);
            for (int k = n - 3; k < n; k++)
                if (k >= 0) splat(sxs[k], (float)(GH - k), col, glim);
        }
    }
    /* bubbles */
    {
        float col[3];
        palcol(0.53f + hueoff, 0.45f, 0.9f, col);
        for (int k = 0; k < 20; k++) {
            float by = GH - fmodf(t * 0.7f + k * 53.0f, GH + 30.0f);
            float bx = fmodf(anch[k % NS] + 10.0f * lsin(t * 0.02f + k) + GW, (float)GW);
            splat(bx, by, col, 1.2f);
        }
    }
    soften1();
    compose();
    upscale(fb, w, h);
}
