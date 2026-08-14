/* 075 Coral Lace — nested wavy coral rings growing outward.
 * Port of lab/patterns/075_coral_lace/proto.py: up to 26 concentric closed
 * curves (720 samples each), radius modulated by three sine harmonics with
 * per-ring phases, slight vertical squash, drifting centre, additive splat +
 * exp tonemap over a dark teal ground. 320x240 canvas, bilinear upscale. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define NTH 720
#define JMAX 26

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static uint8_t (*img)[GW][3];
static uint8_t (*bgp)[GW][3];
static uint8_t tonelut[2048];
static float huetab[1024][3];
static float sintab[4096];
static float thc[NTH], ths[NTH];
static int m3[NTH], m9[NTH], m2[NTH];
static int mm[NTH][5];           /* index steps for harmonic m = 5..9 */
static float ph1[JMAX], ph2[JMAX];
static int m2sel[JMAX];
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
    for (int i = 0; i < NTH; i++) {
        float th = (float)i * (6.2831853f / NTH);
        thc[i] = cosf(th); ths[i] = sinf(th) * 0.82f;
        m3[i] = (int)((3 * i * 4096L) / NTH) & 4095;
        m9[i] = (int)((9 * i * 4096L) / NTH) & 4095;
        m2[i] = (int)((2 * i * 4096L) / NTH) & 4095;
        for (int k = 0; k < 5; k++)
            mm[i][k] = (int)(((5 + k) * (long)i * 4096L) / NTH) & 4095;
    }
    acc  = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    img  = malloc(GH * GW * 3);
    bgp  = malloc(GH * GW * 3);
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            float dx = ((float)x - GW / 2.0f) / GW, dy = ((float)y - GH / 2.0f) / GH;
            float u = 1.0f - (dx * dx + dy * dy);
            bgp[y][x][0] = (uint8_t)(255.0f * (0.012f + 0.010f * u));
            bgp[y][x][1] = (uint8_t)(255.0f * (0.050f + 0.030f * u));
            bgp[y][x][2] = (uint8_t)(255.0f * (0.055f + 0.030f * u));
        }
}

void pattern_075(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; }
    if (seed != last_seed) {
        rs = seed ^ 0xC0FFA71u; rnd(); rnd();
        for (int j = 0; j < JMAX; j++) {
            ph1[j] = rnd01() * 6.2831853f;
            ph2[j] = rnd01() * 6.2831853f;
            m2sel[j] = j % 5;
        }
        last_seed = seed;
    }
    build_huetab(pal);
    float t = (float)frame;
    float T = (float)sl + 90.0f;
    float jf = 3.0f + T * 0.033f;
    if (jf > JMAX - 0.001f) jf = JMAX - 0.001f;
    int jv = (int)jf;
    float cx = GW / 2.0f + 12.0f * lsin(t * 0.005f);
    float cy = GH / 2.0f + 8.0f * lsin(t * 0.004f + 1.5707963f);
    float hueoff = (float)((seed >> 7) & 1023) * (1.0f / 1024.0f);

    memset(acc, 0, sizeof(float) * GH * GW * 3);
    for (int j = 0; j <= jv; j++) {
        float em = jf - (float)j;
        if (em > 1.0f) em = 1.0f; if (em < 0.0f) em = 0.0f;
        em = powf(em, 0.6f);
        float amp = (0.6f + j * 0.55f) * em;
        float drift = t * 0.004f * (1.0f + j * 0.06f);
        float r0 = (13.0f + j * 4.3f) * (0.75f + 0.25f * em);
        float v = (j == jv) ? (0.55f + 0.45f * em) : (0.9f - 0.012f * (jv - j));
        if (v < 0.35f) v = 0.35f;
        float wgt = 1.25f * ((j == jv) ? em : 1.0f);
        if (wgt < 0.002f) continue;
        float huebase = 0.92f + j * 0.045f + t * 0.0004f + hueoff;
        int k2 = m2sel[j];
        float p1 = ph1[j] + drift, p2 = -ph2[j] + drift * 0.7f;
        float p3 = ph1[j] * 2.0f - drift * 0.5f;
        int i1 = (int)(p1 * 651.8986f + 4096.5f);
        int i2 = (int)(p2 * 651.8986f + 4096.5f);
        int i3 = (int)(p3 * 651.8986f + 4096.5f);
        float prevhue = -99.0f, col[3] = {0, 0, 0};
        for (int i = 0; i < NTH; i++) {
            float r = r0 + amp * (0.9f * sintab[(m3[i] + i1) & 4095]
                                + 0.8f * sintab[(mm[i][k2] + i2) & 4095]
                                + 0.4f * sintab[(m9[i] + i3) & 4095]);
            float hue = huebase + 0.01f * sintab[m2[i]];
            if (hue - prevhue > 0.001f || prevhue - hue > 0.001f) {
                palcol(hue, 0.78f, v, col);
                prevhue = hue;
            }
            splat(cx + r * thc[i], cy + r * ths[i], col, wgt);
        }
    }
    soften1();
    compose();
    upscale(fb, w, h);
}
