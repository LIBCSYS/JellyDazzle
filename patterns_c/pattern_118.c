/* 118 Harmonic Lantern — Bourke's spherical-harmonic surface, spun slowly.
 * The radius of the surface is r(phi,theta) = |sin(m0 phi)|^2 + |cos(m1 phi)|^3
 * + |sin(m2 theta)|^2 + |cos(m3 theta)|^3, which separates: the phi terms are a
 * per-row table, the theta terms a per-column table, so the whole 420x240
 * sample grid costs two adds per point. The four frequencies are integers (any
 * other value tears the surface at the wrap) and each one cross-fades to a new
 * integer on its own slow clock, so the solid melts continuously from a
 * six-lobed clover into a spiked urchin into a pinched torus without ever
 * cutting. Points are yawed, pitched, perspective-projected and deposited
 * additively, so the lobes glow where the surface folds toward the eye. Bright
 * lobes on black — an overlay.
 */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
static jd_up p118_up;

#define P118_LW 640
#define P118_LH 480
#define P118_NPH 240
#define P118_NTH 420
#define P118_TAU 6.28318530717959f

static float p118_acc[P118_LW * P118_LH * 3];
static float p118_tmp[P118_LW * P118_LH * 3];
static unsigned char p118_img[P118_LW * P118_LH * 3];
static float p118_ramp[256][3];
static float p118_sin[1024];
static int p118_tab;
static int p118_cur[4] = { 3, 2, 5, 2 }, p118_nxt[4] = { 4, 3, 2, 6 };
static float p118_mix[4];
static int p118_last = -1;
static uint32_t p118_rs = 0xB16B00B5u;

static uint32_t p118_rnd(void)
{
    p118_rs ^= p118_rs << 13; p118_rs ^= p118_rs >> 17; p118_rs ^= p118_rs << 5;
    return p118_rs;
}

static void p118_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p118_ramp[i][0] = r / mx; p118_ramp[i][1] = g / mx; p118_ramp[i][2] = b / mx;
    }
}

static float p118_s(float a)               /* sin from the table, any angle */
{
    int i = (int)(a * (1024.0f / P118_TAU)) & 1023;
    return p118_sin[i];
}
static float p118_c(float a) { return p118_s(a + P118_TAU * 0.25f); }

static void p118_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1, w2, w3; float *p;
    if (xi < 0 || yi < 0 || xi >= P118_LW - 1 || yi >= P118_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    w2 = (1.0f - fx) * fy * w;         w3 = fx * fy * w;
    p = p118_acc + ((size_t)yi * P118_LW + xi) * 3;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P118_LW * 3;
    p[0] += c[0] * w2; p[1] += c[1] * w2; p[2] += c[2] * w2;
    p[3] += c[0] * w3; p[4] += c[1] * w3; p[5] += c[2] * w3;
}

static void p118_blit(uint32_t *fb, int w, int h)
{
    int i, x, y, c, n = P118_LW * P118_LH * 3;
    for (y = 0; y < P118_LH; y++) {
        const float *s = p118_acc + (size_t)y * P118_LW * 3;
        float *d = p118_tmp + (size_t)y * P118_LW * 3;
        for (x = 0; x < P118_LW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < P118_LW - 1 ? x + 1 : P118_LW - 1;
            for (c = 0; c < 3; c++)
                d[x * 3 + c] = 0.27f * (s[xm * 3 + c] + s[xp * 3 + c]) +
                               0.46f * s[x * 3 + c];
        }
    }
    for (x = 0; x < P118_LW; x++)
        for (y = 0; y < P118_LH; y++) {
            int ym = y > 0 ? y - 1 : 0, yp = y < P118_LH - 1 ? y + 1 : P118_LH - 1;
            for (c = 0; c < 3; c++) {
                size_t o = (size_t)x * 3 + (size_t)c;
                float v = 0.27f * (p118_tmp[(size_t)ym * P118_LW * 3 + o] +
                                   p118_tmp[(size_t)yp * P118_LW * 3 + o]) +
                          0.46f * p118_tmp[(size_t)y * P118_LW * 3 + o];
                p118_acc[(size_t)y * P118_LW * 3 + o] += 0.85f * v;
            }
        }
    for (i = 0; i < n; i++) {
        float cc = p118_acc[i], v = 255.0f * cc / (0.85f + cc);
        p118_img[i] = v <= 0.0f ? 0 : v >= 255.0f ? 255 : (unsigned char)v;
    }
    jd_up_blit(&p118_up, fb, w, h, p118_img, P118_LW, P118_LH);
}

/* |sin(m a)|^e for e = 2 or 3, from the table */
static float p118_term(float a, int m, int e, int useCos)
{
    float v = useCos ? p118_c((float)m * a) : p118_s((float)m * a);
    if (v < 0.0f) v = -v;
    return e == 2 ? v * v : v * v * v;
}

void pattern_118(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    static float rowR[P118_NPH], rowS[P118_NPH], rowC[P118_NPH];
    static float colR[P118_NTH], colS[P118_NTH], colC[P118_NTH];
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    float yaw, pit, cy_, sy_, cp, spn, S, cx, cyy;
    int i, j, hbase;
    (void)sl;

    if (!p118_tab) {
        for (i = 0; i < 1024; i++)
            p118_sin[i] = sinf((float)i * P118_TAU / 1024.0f);
        p118_tab = 1;
    }
    p118_ramp_build(pal);
    memset(p118_acc, 0, sizeof p118_acc);

    /* the two frequency PAIRS cross-fade on their own slow clocks; both members
     * of a pair must swap on the same frame or the surface would jump */
    if (p118_last != frame) {
        p118_last = frame;
        for (i = 0; i < 4; i += 2) {
            p118_mix[i] += (i == 0) ? 0.0021f : 0.0016f;
            if (p118_mix[i] >= 1.0f) {
                p118_mix[i] = 0.0f;
                p118_cur[i] = p118_nxt[i];
                p118_cur[i + 1] = p118_nxt[i + 1];
                p118_nxt[i] = 2 + (int)(p118_rnd() % 6u);
                p118_nxt[i + 1] = 2 + (int)(p118_rnd() % 6u);
            }
        }
    }

    for (j = 0; j < P118_NPH; j++) {
        float ph = 3.14159265f * ((float)j + 0.5f) / (float)P118_NPH;
        float a = p118_term(ph, p118_cur[0], 2, 0) + p118_term(ph, p118_cur[1], 3, 1);
        float b = p118_term(ph, p118_nxt[0], 2, 0) + p118_term(ph, p118_nxt[1], 3, 1);
        rowR[j] = a + (b - a) * p118_mix[0];
        rowS[j] = p118_s(ph); rowC[j] = p118_c(ph);
    }
    for (i = 0; i < P118_NTH; i++) {
        float th = P118_TAU * (float)i / (float)P118_NTH;
        float a = p118_term(th, p118_cur[2], 2, 0) + p118_term(th, p118_cur[3], 3, 1);
        float b = p118_term(th, p118_nxt[2], 2, 0) + p118_term(th, p118_nxt[3], 3, 1);
        colR[i] = a + (b - a) * p118_mix[2];
        colS[i] = p118_s(th); colC[i] = p118_c(th);
    }

    yaw = 0.00082f * t + sp;
    pit = 0.38f + 0.30f * sinf(0.00041f * t);
    cy_ = cosf(yaw); sy_ = sinf(yaw);
    cp  = cosf(pit); spn = sinf(pit);
    S   = (float)P118_LH * (0.105f + 0.008f * sinf(0.00063f * t));
    cx  = (float)P118_LW * 0.5f;
    cyy = (float)P118_LH * 0.5f;
    hbase = (int)(t * 0.043f + sp * 30.0f);

    for (j = 0; j < P118_NPH; j++) {
        float sph = rowS[j], cph = rowC[j], rr0 = rowR[j];
        for (i = 0; i < P118_NTH; i++) {
            float r = rr0 + colR[i];
            float X = r * sph * colC[i];
            float Y = r * cph;
            float Z = r * sph * colS[i];
            float x1 = X * cy_ - Z * sy_;
            float z1 = X * sy_ + Z * cy_;
            float y2 = Y * cp - z1 * spn;
            float z2 = Y * spn + z1 * cp;
            float d = 4.6f + z2;
            float k, sx, sy2, bw;
            int hi;
            if (d < 0.6f) continue;
            k = 4.6f / d;
            sx = cx + x1 * S * k;
            sy2 = cyy - y2 * S * k;
            bw = 0.17f * k * k;
            hi = (hbase + (int)(r * 46.0f) + (int)(z2 * 9.0f)) & 255;
            p118_splat(sx, sy2, p118_ramp[hi], bw);
        }
    }
    p118_blit(fb, w, h);
}
