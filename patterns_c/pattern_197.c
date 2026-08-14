/* 197 Fern Dust — an iterated function system caught between four shapes.
 * The chaos game with four affine maps: pick a map, apply it, plot, repeat, and
 * the cloud of points converges on the attractor of whichever set of maps you
 * used. Here the twelve coefficients of every map are cross-faded between four
 * complete systems — Barnsley's fern, a branching leaf, a four-corner gasket
 * and a rotating spiral — so the attractor itself morphs. Two details keep it
 * from boiling: the sequence of map choices is fixed once, so each of the forty
 * thousand points is a continuous function of the coefficients and slides
 * rather than flickers, and the framing follows a heavily damped bounding box.
 * Sparse luminous dust, thick where the invariant measure piles up. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p197_up;

#define P197_W 480
#define P197_H 360
#define P197_TAU 6.28318530717958647692f

static float p197_acc[P197_W * P197_H * 3];
static unsigned char p197_img[P197_W * P197_H * 3];
static unsigned char p197_tone[1024];
static int *p197_xm;
static int p197_xmw;
static int p197_tone_ok;
static uint32_t p197_rs = 1u;

static float p197_rf(void)
{
    p197_rs ^= p197_rs << 13; p197_rs ^= p197_rs >> 17; p197_rs ^= p197_rs << 5;
    return (float)(p197_rs >> 8) * (1.0f / 16777216.0f);
}

static void p197_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (7.00f / 1024.0f)));
        p197_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p197_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p197_col(const uint32_t *pal, float hue, float lift, float *out)
{
    uint32_t p; float r, g, b, mx;
    hue -= floorf(hue);
    p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
    r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
    mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
    out[0] = lift + (1.0f - lift) * r / mx;
    out[1] = lift + (1.0f - lift) * g / mx;
    out[2] = lift + (1.0f - lift) * b / mx;
}


static void p197_splat(float x, float y, const float *c, float w)
{
    int xi, yi; float fx, fy, w0, w1; float *p;
    if (!(x >= 0.0f) || !(y >= 0.0f)) return;
    xi = (int)x; yi = (int)y;
    if (xi >= P197_W - 1 || yi >= P197_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p197_acc + (yi * P197_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P197_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}


static float p197_tmp[P197_W * P197_H * 3];

/* 5-tap soft glow, in place. Keeps line art from aliasing when it is scaled
 * up to 1280x960 and keeps frame-to-frame motion visually continuous. */
static void p197_blur(void)
{
    int y, x, c;
    for (y = 1; y < P197_H - 1; y++)
        for (x = 1; x < P197_W - 1; x++) {
            int o = (y * P197_W + x) * 3;
            for (c = 0; c < 3; c++)
                p197_tmp[o + c] = p197_acc[o + c] * 0.52f
                    + 0.12f * (p197_acc[o + c - 3] + p197_acc[o + c + 3]
                             + p197_acc[o + c - P197_W * 3] + p197_acc[o + c + P197_W * 3]);
        }
    for (y = 1; y < P197_H - 1; y++)
        memcpy(p197_acc + (y * P197_W + 1) * 3, p197_tmp + (y * P197_W + 1) * 3,
               sizeof(float) * 3 * (P197_W - 2));
}

static void p197_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P197_W * P197_H * 3; i++) {
        int ti = (int)(p197_acc[i] * 256.0f);
        p197_img[i] = p197_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p197_xmw != w) {
        free(p197_xm);
        p197_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p197_xm[x] = (int)(((long long)x * (P197_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p197_xmw = w;
    }
    jd_up_blit(&p197_up, fb, w, h, p197_img, P197_W, P197_H);
}

#define P197_NP 100000
#define P197_BURN 26

static const float p197_sys[4][4][6] = {
  { { 0.00f, 0.00f, 0.00f, 0.16f, 0.00f, 0.00f},
    { 0.85f, 0.04f,-0.04f, 0.85f, 0.00f, 1.60f},
    { 0.20f,-0.26f, 0.23f, 0.22f, 0.00f, 1.60f},
    {-0.15f, 0.28f, 0.26f, 0.24f, 0.00f, 0.44f} },
  { { 0.05f, 0.00f, 0.00f, 0.60f, 0.00f, 0.00f},
    { 0.05f, 0.00f, 0.00f,-0.50f, 0.00f, 1.00f},
    { 0.46f,-0.32f, 0.39f, 0.61f, 0.00f, 0.60f},
    { 0.47f,-0.15f, 0.17f, 0.42f, 0.00f, 1.10f} },
  { { 0.50f, 0.00f, 0.00f, 0.50f, 0.00f, 0.00f},
    { 0.50f, 0.00f, 0.00f, 0.50f, 1.00f, 0.00f},
    { 0.50f, 0.00f, 0.00f, 0.50f, 0.50f, 0.87f},
    { 0.28f, 0.00f, 0.00f, 0.28f, 0.50f, 0.40f} },
  { { 0.62f,-0.33f, 0.33f, 0.62f, 0.10f, 0.20f},
    { 0.42f, 0.42f,-0.42f, 0.42f, 0.60f, 0.10f},
    { 0.30f, 0.50f,-0.50f, 0.30f, 0.20f, 0.70f},
    { 0.14f, 0.00f, 0.00f, 0.14f, 0.50f, 0.50f} }
};

static const float p197_prb[4][4] = {
    {0.010f, 0.850f, 0.070f, 0.070f},
    {0.044f, 0.037f, 0.593f, 0.326f},
    {0.302f, 0.302f, 0.302f, 0.094f},
    {0.409f, 0.293f, 0.282f, 0.016f}
};

static uint32_t p197_seedc = 0xFFFFFFFFu;
static float p197_h0, p197_hw, p197_ord;
static float p197_seq[P197_NP];
static float p197_px[P197_NP], p197_py[P197_NP];
static unsigned char p197_pm[P197_NP];
static float p197_bx0, p197_bx1, p197_by0, p197_by1;
static int p197_bok;
static float p197_hue[4][3];
static int p197_ordr[4];

static void p197_build(uint32_t seed)
{
    int i;
    p197_rs = seed ? seed * 2654435761u + 0x1B873593u : 0x197u;
    p197_rf(); p197_rf();
    p197_h0 = p197_rf();
    p197_hw = 0.06f + p197_rf() * 0.50f;
    for (i = 0; i < P197_NP; i++) p197_seq[i] = p197_rf();
    for (i = 0; i < 4; i++) p197_ordr[i] = i;
    for (i = 3; i > 0; i--) {                 /* shuffle the morph order */
        int j = (int)(p197_rf() * (float)(i + 1)); int tmp;
        if (j > i) j = i;
        tmp = p197_ordr[i]; p197_ordr[i] = p197_ordr[j]; p197_ordr[j] = tmp;
    }
    p197_ord = p197_rf() * 4.0f;
    p197_bok = 0;
    p197_seedc = seed;
    if (!p197_tone_ok) p197_tone_init();
}

void pattern_197(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, m[4][6], u, sc, ox, oy, ca, sa;
    float x0 = 1e30f, x1 = -1e30f, y0 = 1e30f, y1 = -1e30f, cum[4];
    float x = 0.0f, y = 0.0f;
    int i, k, ia, ib;
    (void)sl;
    if (p197_seedc != seed) p197_build(seed);
    for (i = 0; i < 4; i++)
        p197_col(pal, p197_h0 + p197_hw * ((float)i / 3.0f), 0.14f, p197_hue[i]);

    {   float g = p197_ord + t * 0.00042f;
        float gi = floorf(g);
        u = g - gi;
        u = u * u * u * (u * (u * 6.0f - 15.0f) + 10.0f);
        ia = p197_ordr[((int)gi) & 3];
        ib = p197_ordr[(((int)gi) + 1) & 3];
    }
    for (i = 0; i < 4; i++)
        for (k = 0; k < 6; k++)
            m[i][k] = p197_sys[ia][i][k] + (p197_sys[ib][i][k] - p197_sys[ia][i][k]) * u;
    {   float c = 0.0f;
        for (i = 0; i < 4; i++) {
            c += p197_prb[ia][i] + (p197_prb[ib][i] - p197_prb[ia][i]) * u;
            cum[i] = c;
        }
        cum[3] = 1.001f;
    }

    for (i = 0; i < P197_NP; i++) {
        float r = p197_seq[i];
        int mi = r < cum[0] ? 0 : r < cum[1] ? 1 : r < cum[2] ? 2 : 3;
        const float *a = m[mi];
        float nx = a[0] * x + a[1] * y + a[4];
        float ny = a[2] * x + a[3] * y + a[5];
        if (!(nx > -1e5f && nx < 1e5f && ny > -1e5f && ny < 1e5f)) { nx = 0.0f; ny = 0.0f; }
        x = nx; y = ny;
        p197_px[i] = x; p197_py[i] = y; p197_pm[i] = (unsigned char)mi;
        if (i >= P197_BURN) {
            if (x < x0) x0 = x; if (x > x1) x1 = x;
            if (y < y0) y0 = y; if (y > y1) y1 = y;
        }
    }
    if (!p197_bok) {
        p197_bx0 = x0; p197_bx1 = x1; p197_by0 = y0; p197_by1 = y1; p197_bok = 1;
    } else {
        const float k2 = 0.012f;
        p197_bx0 += (x0 - p197_bx0) * k2; p197_bx1 += (x1 - p197_bx1) * k2;
        p197_by0 += (y0 - p197_by0) * k2; p197_by1 += (y1 - p197_by1) * k2;
    }
    memset(p197_acc, 0, sizeof p197_acc);
    {
        float sx = (float)P197_W * 0.80f / (p197_bx1 - p197_bx0 + 1e-4f);
        float sy = (float)P197_H * 0.82f / (p197_by1 - p197_by0 + 1e-4f);
        sc = sx < sy ? sx : sy;
        ox = 0.5f * (p197_bx0 + p197_bx1);
        oy = 0.5f * (p197_by0 + p197_by1);
    }
    {   float rot = 0.14f * sinf(t * 0.00031f); ca = cosf(rot); sa = sinf(rot); }
    for (i = P197_BURN; i < P197_NP; i++) {
        float dx = (p197_px[i] - ox) * sc, dy = (p197_py[i] - oy) * sc;
        p197_splat(P197_W * 0.5f + dx * ca - dy * sa,
                   P197_H * 0.5f - (dx * sa + dy * ca), p197_hue[p197_pm[i]], 0.028f);
    }
    p197_blur();
    p197_blit(fb, w, h);
}
