/* 074 Slow Lightning — two jagged bolts creeping down a night pane.
 * Port of lab/patterns/074_slow_lightning/proto.py: static jagged polylines
 * (main spine + 7 branches each) revealed by an arclength "tip" that crawls
 * downward, bright head / fading tail, heavy soften for the glow layer,
 * exp tonemap over a vertical night gradient. 320x240 canvas, upscaled.
 * Repaint pattern; strike cycles smoothly so it re-strikes inside a segment. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GW 320
#define GH 240
#define NN 90
#define NBR 7
#define BMAXN 26
#define DENS 8
#define MAXPTS ((NN + NBR * BMAXN) * DENS + 64)

static float (*acc)[GW][3];
static float (*acc2)[GW][3];
static float (*glo)[GW][3];
static uint8_t (*img)[GW][3];
static uint8_t (*bgp)[GW][3];
static uint8_t tonelut[2048];
static float huetab[1024][3];
static int inited;
static int up_w = -1;
static int *up_xi;
static uint8_t *up_fx;

typedef struct { float x, y, a, sw; } bpt;
static bpt bolt[2][MAXPTS];
static int bn[2];
static uint32_t last_seed = 0xFFFFFFFFu;
static int have_geo;

static uint32_t rs;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs; }
static float rnd01(void) { return (float)(rnd() >> 8) * (1.0f / 16777216.0f); }
static float gauss(void) {
    return (rnd01() + rnd01() + rnd01() + rnd01() - 2.0f) * 1.7320508f;
}

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
    const float *c = huetab[(int)(hue * 1024.0f) & 1023];
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

static void blur(float (*src)[GW][3], float (*dst)[GW][3]) {
    for (int y = 0; y < GH; y++) {
        int ym = (y + GH - 1) % GH, yp = (y + 1) % GH;
        for (int x = 0; x < GW; x++) {
            int xm = (x + GW - 1) % GW, xp = (x + 1) % GW;
            for (int k = 0; k < 3; k++)
                dst[y][x][k] = src[y][x][k] * 0.5f
                    + 0.125f * (src[ym][x][k] + src[yp][x][k]
                                + src[y][xm][k] + src[y][xp][k]);
        }
    }
}

static void compose(float pulse) {
    const float *a = &acc[0][0][0];
    const float *g = &glo[0][0][0];
    const uint8_t *b = &bgp[0][0][0];
    uint8_t *o = &img[0][0][0];
    for (int i = 0; i < GW * GH * 3; i++) {
        float s = a[i] * 0.9f + g[i] * 1.6f;
        int ti = (int)(s * 64.0f);
        if (ti > 2047) ti = 2047;
        if (ti < 0) ti = 0;
        int v = b[i] + (int)(tonelut[ti] * pulse);
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
    for (int i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i / 64.0f));
        tonelut[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    acc  = malloc(sizeof(float) * GH * GW * 3);
    acc2 = malloc(sizeof(float) * GH * GW * 3);
    glo  = malloc(sizeof(float) * GH * GW * 3);
    img  = malloc(GH * GW * 3);
    bgp  = malloc(GH * GW * 3);
    for (int y = 0; y < GH; y++) {
        float f = (float)y / GH;
        uint8_t r = (uint8_t)(255.0f * (0.015f + 0.02f * f));
        uint8_t g = (uint8_t)(255.0f * (0.005f + 0.005f * f));
        uint8_t b = (uint8_t)(255.0f * (0.05f + 0.05f * f));
        for (int x = 0; x < GW; x++) { bgp[y][x][0] = r; bgp[y][x][1] = g; bgp[y][x][2] = b; }
    }
}

/* densify a polyline 4x and append to a bolt's point list */
static void emit(int b, const float *x, const float *y, const float *a, int n) {
    for (int i = 0; i < n - 1; i++)
        for (int s = 0; s < DENS; s++) {
            float f = (float)s / DENS;
            if (bn[b] >= MAXPTS) return;
            bpt *p = &bolt[b][bn[b]++];
            p->x = x[i] + (x[i+1] - x[i]) * f;
            p->y = y[i] + (y[i+1] - y[i]) * f;
            p->a = a[i] + (a[i+1] - a[i]) * f;
            p->sw = p->y * 0.02f;
        }
}

static void make_bolt(int b, float xstart) {
    float xs[NN], ys[NN], al[NN], dx[NN], sm[NN];
    for (int i = 0; i < NN; i++) {
        ys[i] = 6.0f + (float)i * ((GH + 30.0f - 6.0f) / (NN - 1));
        dx[i] = gauss() * 5.5f;
    }
    for (int i = 0; i < NN; i++) {
        float s = dx[i];
        if (i > 0) s += dx[i-1];
        if (i < NN-1) s += dx[i+1];
        sm[i] = s / 3.0f;
    }
    float c = 0.0f;
    for (int i = 0; i < NN; i++) { c += sm[i]; xs[i] = xstart + c; }
    float drift = xs[NN-1] - xstart;
    for (int i = 0; i < NN; i++)
        xs[i] -= drift * ((float)i / (NN - 1)) * 0.6f;
    al[0] = 0.0f;
    for (int i = 1; i < NN; i++)
        al[i] = al[i-1] + hypotf(xs[i] - xs[i-1], ys[i] - ys[i-1]);
    bn[b] = 0;
    emit(b, xs, ys, al, NN);
    for (int j = 0; j < NBR; j++) {
        int k = 10 + (int)(rnd01() * (NN - 35));
        int nb = 12 + (int)(rnd01() * 14);
        float ang = ((rnd() & 1) ? 1.0f : -1.0f) * (0.5f + rnd01() * 0.6f);
        float bdx[BMAXN], bdy[BMAXN], bsm[BMAXN];
        float bx[BMAXN], by[BMAXN], ba[BMAXN];
        for (int i = 0; i < nb; i++) {
            bdx[i] = cosf(ang + 1.5707963f) * 4.0f + gauss() * 3.0f;
            bdy[i] = fabsf(sinf(ang + 1.5707963f) * 4.0f + gauss() * 2.0f) + 1.5f;
        }
        for (int i = 0; i < nb; i++) {
            float s = bdx[i];
            if (i > 0) s += bdx[i-1];
            if (i < nb-1) s += bdx[i+1];
            bsm[i] = s / 3.0f;
        }
        float cx = 0.0f, cy = 0.0f;
        for (int i = 0; i < nb; i++) {
            cx += bsm[i]; cy += bdy[i];
            bx[i] = xs[k] + cx; by[i] = ys[k] + cy;
        }
        ba[0] = al[k];
        for (int i = 1; i < nb; i++)
            ba[i] = ba[i-1] + hypotf(bx[i]-bx[i-1], by[i]-by[i-1]) * 1.4f;
        emit(b, bx, by, ba, nb);
    }
}

/* smooth strike envelope over a 1024-frame cycle */
static float envel(float u) {
    if (u < 80.0f)  return 0.5f - 0.5f * cosf(u * (3.14159265f / 80.0f));
    if (u < 700.0f) return 1.0f;
    if (u < 820.0f) return 0.5f + 0.5f * cosf((u - 700.0f) * (3.14159265f / 120.0f));
    return 0.0f;
}

void pattern_074(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!inited) { init_tables(); inited = 1; }
    if (!have_geo || seed != last_seed) {
        rs = seed ^ 0x5A17E5u; rnd(); rnd();
        make_bolt(0, GW * 0.38f);
        make_bolt(1, GW * 0.66f);
        last_seed = seed; have_geo = 1;
    }
    build_huetab(pal);
    float t = (float)frame;
    memset(acc, 0, sizeof(float) * GH * GW * 3);

    const float hue0[2] = { 0.72f, 0.55f };
    const float toff[2] = { 0.0f, 260.0f };
    float swayph = t * 0.008f;
    for (int b = 0; b < 2; b++) {
        float u = fmodf((float)sl + toff[b], 1024.0f);
        float env = envel(u);
        if (env <= 0.002f) continue;
        float tip = 70.0f + u * 0.42f;
        for (int i = 0; i < bn[b]; i++) {
            const bpt *p = &bolt[b][i];
            float d = tip - p->a;
            if (d <= 0.0f) continue;
            float fade = d * (1.0f / 40.0f); if (fade > 1.0f) fade = 1.0f;
            float head = 1.0f - d * (1.0f / 26.0f); if (head < 0.0f) head = 0.0f;
            head *= head;
            float hue = hue0[b] + 0.03f * sinf(p->a * 0.02f + t * 0.004f);
            float col[3];
            palcol(hue, 0.25f + 0.3f * (1.0f - head), 1.0f, col);
            float wgt = (0.35f + 0.65f * fade) * (1.0f + 2.2f * head) * 0.8f * env;
            splat(p->x + sinf(swayph + p->sw) * 3.0f, p->y, col, wgt);
        }
    }
    /* glow = 4 blur passes of acc; acc itself gets 1 */
    blur(acc, glo); blur(glo, acc2); blur(acc2, glo); blur(glo, acc2);
    memcpy(glo, acc2, sizeof(float) * GH * GW * 3);
    blur(acc, acc2);
    { float (*tmp)[GW][3] = acc; acc = acc2; acc2 = tmp; }
    compose(0.85f + 0.15f * sinf(t * 0.02f));
    upscale(fb, w, h);
}
