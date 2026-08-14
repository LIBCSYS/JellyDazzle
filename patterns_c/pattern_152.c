/* 152 Pentagrid Lattice — a true Penrose rhomb tiling, drawn as glowing wire.
 * Built by de Bruijn's dual method: five families of parallel lines at fifth-turn
 * angles with offsets gamma_j (sum 0); every intersection of family j with
 * family k becomes a rhomb whose corners are  SUM_m K_m e_m  with K_j, K_k each
 * stepping by one. The result is aperiodic — fat and thin rhombs that never
 * tile periodically. Geometry is cached per segment; the frame only rotates it,
 * breathes the scale, and runs a slow plane wave of brightness/hue across the
 * lattice, so cells light up in drifting bands. Thin rhombs take the accent hue,
 * fat rhombs the base hue. Edge-only, dark cells: a sparse overlay layer. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p152_up;

#define CW 480
#define CH 360
#define MAXR 24000
#define GRID 5

static float p152_acc[CW * CH * 3];
static float p152_tmp[CW * CH * 3];
static unsigned char p152_img[CW * CH * 3];
static int *p152_xm;
static int p152_xmw;

static float p152_bx[MAXR], p152_by[MAXR];      /* rhomb base corner        */
static float p152_ux[MAXR], p152_uy[MAXR];      /* side vectors             */
static float p152_vx[MAXR], p152_vy[MAXR];
static unsigned char p152_thin[MAXR];
static int p152_nr;
static float p152_hue0, p152_huew, p152_wdx, p152_wdy;
static uint32_t p152_seedc;
static int p152_ready;
static unsigned char p152_tone[1024];

static uint32_t p152_rs;
static float p152_rf(void)
{
    p152_rs ^= p152_rs << 13; p152_rs ^= p152_rs >> 17; p152_rs ^= p152_rs << 5;
    return (float)(p152_rs >> 8) * (1.0f / 16777216.0f);
}

static void p152_pal3(const uint32_t *pal, float hue, float sat, float *o)
{
    uint32_t p; float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    o[0] = (1.0f - sat) + sat * r / mx;
    o[1] = (1.0f - sat) + sat * g / mx;
    o[2] = (1.0f - sat) + sat * b / mx;
}

static void p152_build(uint32_t seed)
{
    float ex[GRID], ey[GRID], gam[GRID], gs = 0.0f;
    int j, k, p, q, m, i;
    p152_rs = seed ? seed ^ 0x2E5A05u : 0x2E5A05u;
    p152_rf(); p152_rf();
    for (j = 0; j < GRID; j++) {
        float a = (float)j * (6.2831853f / GRID);
        ex[j] = cosf(a); ey[j] = sinf(a);
        gam[j] = 0.12f + p152_rf() * 0.76f;
        gs += gam[j];
    }
    for (j = 0; j < GRID; j++) gam[j] -= gs / GRID;
    p152_nr = 0;
    for (j = 0; j < GRID && p152_nr < MAXR; j++)
        for (k = j + 1; k < GRID && p152_nr < MAXR; k++) {
            float det = ex[j] * ey[k] - ey[j] * ex[k];
            if (fabsf(det) < 1e-4f) continue;
            for (p = -17; p <= 17 && p152_nr < MAXR; p++)
                for (q = -17; q <= 17 && p152_nr < MAXR; q++) {
                    float a = (float)p - gam[j], b = (float)q - gam[k];
                    float zx = (a * ey[k] - b * ey[j]) / det;
                    float zy = (b * ex[j] - a * ex[k]) / det;
                    float vx = 0.0f, vy = 0.0f, cx, cy, dsq;
                    int kk;
                    for (m = 0; m < GRID; m++) {
                        if (m == j) kk = p;
                        else if (m == k) kk = q;
                        else kk = (int)ceilf(zx * ex[m] + zy * ey[m] + gam[m]);
                        vx += (float)kk * ex[m]; vy += (float)kk * ey[m];
                    }
                    cx = vx + 0.5f * (ex[j] + ex[k]);
                    cy = vy + 0.5f * (ey[j] + ey[k]);
                    dsq = cx * cx + cy * cy;
                    if (dsq > 175.0f) continue;
                    i = p152_nr++;
                    p152_bx[i] = vx; p152_by[i] = vy;
                    p152_ux[i] = ex[j]; p152_uy[i] = ey[j];
                    p152_vx[i] = ex[k]; p152_vy[i] = ey[k];
                    p152_thin[i] = (unsigned char)(((k - j) == 2 || (k - j) == 3) ? 1 : 0);
                }
        }
    p152_hue0 = p152_rf();
    p152_huew = 0.08f + p152_rf() * 0.34f;
    {
        float a = p152_rf() * 6.2831853f;
        p152_wdx = cosf(a) * 0.30f; p152_wdy = sinf(a) * 0.30f;
    }
    if (!p152_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (5.2f / 1024.0f)));
            p152_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p152_ready = 1;
    }
    p152_seedc = seed;
}

static void p152_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= CW - 1 || yi >= CH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p152_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p152_line(float x0, float y0, float x1, float y1, const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n = (int)(len * 1.3f) + 1, i;
    float sx = dx / (float)n, sy = dy / (float)n;
    float ww = w * (len / (float)n);
    for (i = 0; i <= n; i++)
        p152_splat(x0 + sx * (float)i, y0 + sy * (float)i, c, ww);
}

static void p152_blur(void)
{
    int y, x, c;
    for (y = 1; y < CH - 1; y++)
        for (x = 1; x < CW - 1; x++) {
            int o = (y * CW + x) * 3;
            for (c = 0; c < 3; c++)
                p152_tmp[o + c] = p152_acc[o + c] * 0.52f
                    + 0.12f * (p152_acc[o + c - 3] + p152_acc[o + c + 3]
                             + p152_acc[o + c - CW * 3] + p152_acc[o + c + CW * 3]);
        }
    for (y = 1; y < CH - 1; y++)
        memcpy(p152_acc + (y * CW + 1) * 3, p152_tmp + (y * CW + 1) * 3,
               sizeof(float) * 3 * (CW - 2));
}

static void p152_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < CW * CH * 3; i++) {
        int ti = (int)(p152_acc[i] * 256.0f);
        p152_img[i] = p152_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p152_xmw != w) {
        free(p152_xm);
        p152_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p152_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p152_xmw = w;
    }
    jd_up_blit(&p152_up, fb, w, h, p152_img, CW, CH);
}

void pattern_152(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, ca, sa, sc, colF[3], colT[3];
    int i;
    (void)sl;
    if (!p152_ready || p152_seedc != seed) p152_build(seed);

    memset(p152_acc, 0, sizeof p152_acc);
    ca = t * 0.0016f;
    sc = (CW * 0.062f) * (1.0f + 0.09f * sinf(t * 0.00083f));
    sa = sinf(ca) * sc; ca = cosf(ca) * sc;
    p152_pal3(pal, p152_hue0, 0.92f, colF);
    p152_pal3(pal, p152_hue0 + p152_huew, 0.92f, colT);

    for (i = 0; i < p152_nr; i++) {
        float bx = p152_bx[i], by = p152_by[i];
        float ux = p152_ux[i], uy = p152_uy[i];
        float vx = p152_vx[i], vy = p152_vy[i];
        float cx = bx + 0.5f * (ux + vx), cy = by + 0.5f * (uy + vy);
        float ph = cx * p152_wdx + cy * p152_wdy - t * 0.010f;
        float b = 0.34f + 0.66f * (0.5f + 0.5f * sinf(ph));
        float x0, y0, x1, y1, x2, y2, x3, y3;
        const float *c = p152_thin[i] ? colT : colF;
        float wgt;
        b *= b;
        wgt = (p152_thin[i] ? 1.30f : 0.95f) * b;
        if (wgt < 0.02f) continue;
        x0 = CW * 0.5f + bx * ca - by * sa;  y0 = CH * 0.5f + bx * sa + by * ca;
        x1 = x0 + (ux * ca - uy * sa);       y1 = y0 + (ux * sa + uy * ca);
        x3 = x0 + (vx * ca - vy * sa);       y3 = y0 + (vx * sa + vy * ca);
        x2 = x1 + (vx * ca - vy * sa);       y2 = y1 + (vx * sa + vy * ca);
        if ((x0 < -40.0f && x1 < -40.0f && x2 < -40.0f) ||
            (x0 > CW + 40.0f && x1 > CW + 40.0f && x2 > CW + 40.0f) ||
            (y0 < -40.0f && y1 < -40.0f && y2 < -40.0f) ||
            (y0 > CH + 40.0f && y1 > CH + 40.0f && y2 > CH + 40.0f)) continue;
        p152_line(x0, y0, x1, y1, c, wgt);
        p152_line(x1, y1, x2, y2, c, wgt);
        p152_line(x2, y2, x3, y3, c, wgt);
        p152_line(x3, y3, x0, y0, c, wgt);
    }
    p152_blur();
    p152_blit(fb, w, h);
}
