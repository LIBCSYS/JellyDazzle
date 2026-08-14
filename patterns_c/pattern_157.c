/* 157 Supershape Bloom — nested Gielis superformula outlines, morphing.
 * Each ring is r(th) = (|cos(m th/4)|^n2 + |sin(m th/4)|^n3)^(-1/n1), the
 * superformula that produces starfish, gears, rounded polygons and blobs from
 * four numbers. Two integer symmetries m1,m2 are evaluated on the same theta
 * grid and cross-faded, and n1..n3 breathe on slow incommensurate sines, so a
 * six-point star swells into a rounded heptagon and folds back without ever
 * cutting. Twelve rings, each with its own symmetry, drift speed and hue stop,
 * are normalised to their own peak radius and drawn as glowing outlines over
 * black — a sparse rosette stack that composites cleanly under MAX. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p157_up;

#define CW 480
#define CH 360
#define NRING 12
#define NTH 600

static float p157_acc[CW * CH * 3];
static float p157_tmp[CW * CH * 3];
static unsigned char p157_img[CW * CH * 3];
static unsigned char p157_tone[1024];
static int *p157_xm;
static int p157_xmw;
static float p157_rad[NTH];
static float p157_ct[NTH], p157_st[NTH];
static float p157_hue[48][3];
static float p157_hue0, p157_huew, p157_spin[NRING], p157_ph[NRING];
static int p157_m1[NRING], p157_m2[NRING];
static uint32_t p157_seedc;
static int p157_ready;

static uint32_t p157_rs;
static float p157_rf(void)
{
    p157_rs ^= p157_rs << 13; p157_rs ^= p157_rs >> 17; p157_rs ^= p157_rs << 5;
    return (float)(p157_rs >> 8) * (1.0f / 16777216.0f);
}

static void p157_setup(uint32_t seed)
{
    int i;
    p157_rs = seed ? seed ^ 0x5F00D5u : 0x5F00D5u;
    p157_rf(); p157_rf();
    p157_hue0 = p157_rf();
    p157_huew = 0.05f + p157_rf() * 0.38f;
    {
        int ma = 3 + (int)(p157_rf() * 10.0f);
        int mb = 3 + (int)(p157_rf() * 10.0f);
        if (mb == ma) mb = 3 + (ma % 10);
        for (i = 0; i < NRING; i++) {
            p157_m1[i] = (i & 1) ? mb : ma;
            p157_m2[i] = p157_m1[i];
        }
    }
    for (i = 0; i < NRING; i++) {
        p157_spin[i] = (p157_rf() - 0.5f) * 0.0052f;
        p157_ph[i] = p157_rf() * 6.2831853f;
    }
    if (!p157_ready) {
        for (i = 0; i < NTH; i++) {
            float a = (float)i * (6.2831853f / NTH);
            p157_ct[i] = cosf(a); p157_st[i] = sinf(a);
        }
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (9.0f / 1024.0f)));
            p157_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p157_ready = 1;
    }
    p157_seedc = seed;
}

static void p157_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 48; i++) {
        float hue = p157_hue0 + p157_huew * ((float)i / 47.0f);
        uint32_t p; float r, g, b, mx;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p157_hue[i][0] = 0.10f + 0.90f * r / mx;
        p157_hue[i][1] = 0.10f + 0.90f * g / mx;
        p157_hue[i][2] = 0.10f + 0.90f * b / mx;
    }
}

static void p157_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= CW - 1 || yi >= CH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p157_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p157_seg(float x0, float y0, float x1, float y1, const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    float sx, sy, ww;
    if (len > 400.0f) return;
    n = (int)(len * 1.2f) + 1;
    sx = dx / (float)n; sy = dy / (float)n;
    ww = w * (len / (float)n + 0.28f);
    for (i = 0; i < n; i++)
        p157_splat(x0 + sx * (float)i, y0 + sy * (float)i, c, ww);
}

static void p157_blur(void)
{
    int y, x, c;
    for (y = 1; y < CH - 1; y++)
        for (x = 1; x < CW - 1; x++) {
            int o = (y * CW + x) * 3;
            for (c = 0; c < 3; c++)
                p157_tmp[o + c] = p157_acc[o + c] * 0.50f
                    + 0.125f * (p157_acc[o + c - 3] + p157_acc[o + c + 3]
                              + p157_acc[o + c - CW * 3] + p157_acc[o + c + CW * 3]);
        }
    for (y = 1; y < CH - 1; y++)
        memcpy(p157_acc + (y * CW + 1) * 3, p157_tmp + (y * CW + 1) * 3,
               sizeof(float) * 3 * (CW - 2));
}

static void p157_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < CW * CH * 3; i++) {
        int ti = (int)(p157_acc[i] * 256.0f);
        p157_img[i] = p157_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p157_xmw != w) {
        free(p157_xm);
        p157_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p157_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p157_xmw = w;
    }
    jd_up_blit(&p157_up, fb, w, h, p157_img, CW, CH);
}

static float p157_super(float th, float m, float n1i, float n2, float n3)
{
    float u = m * th * 0.25f;
    float a = fabsf(cosf(u)), b = fabsf(sinf(u));
    float v;
    a = powf(a < 1e-4f ? 1e-4f : a, n2);
    b = powf(b < 1e-4f ? 1e-4f : b, n3);
    v = a + b;
    if (v < 1e-6f) v = 1e-6f;
    return powf(v, -n1i);
}

void pattern_157(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, base;
    int j, i;
    (void)sl;
    if (!p157_ready || p157_seedc != seed) p157_setup(seed);
    p157_hues(pal);
    memset(p157_acc, 0, sizeof p157_acc);
    base = (float)CH * 0.455f;

    for (j = 0; j < NRING; j++) {
        float fj = (float)j / (float)(NRING - 1);
        float n1 = 0.45f + 1.55f * (0.5f + 0.5f * sinf(t * 0.00097f + p157_ph[j]));
        float n2 = 0.55f + 2.30f * (0.5f + 0.5f * sinf(t * 0.00071f + p157_ph[j] * 1.7f));
        float n3 = 0.55f + 2.30f * (0.5f + 0.5f * sinf(t * 0.00059f + p157_ph[j] * 2.3f + 1.1f));
        float al = 0.5f - 0.5f * cosf(t * 0.00043f + p157_ph[j]);
        float mm = (float)p157_m1[j];
        float rot = t * p157_spin[j] + p157_ph[j];
        float cr = cosf(rot), sr = sinf(rot);
        float scale = base * (0.16f + 0.84f * (0.25f + 0.75f * fj));
        float mx = 1e-6f, wgt;
        float px0 = 0.0f, py0 = 0.0f;
        const float *col;
        float n1i = 1.0f / n1;
        for (i = 0; i < NTH; i++) {
            float r = p157_super((float)i * (6.2831853f / NTH), mm, n1i, n2, n3);
            if (r > 40.0f) r = 40.0f;
            p157_rad[i] = r;
            if (r > mx) mx = r;
        }
        scale /= mx;
        wgt = (0.46f + 0.62f * (0.5f + 0.5f * sinf(t * 0.0060f - fj * 3.4f))) * (0.55f + 0.45f * al);
        col = p157_hue[(int)(fj * 47.0f) & 47];
        for (i = 0; i <= NTH; i++) {
            int k = i == NTH ? 0 : i;
            float r = p157_rad[k] * scale;
            float xx = r * p157_ct[k], yy = r * p157_st[k];
            float X = CW * 0.5f + xx * cr - yy * sr;
            float Y = CH * 0.5f + xx * sr + yy * cr;
            if (i) p157_seg(px0, py0, X, Y, col, wgt);
            px0 = X; py0 = Y;
        }
    }
    p157_blur();
    p157_blit(fb, w, h);
}
