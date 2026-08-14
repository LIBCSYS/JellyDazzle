/* 176 Bob Rosette — Amiga vector bobs strung on a 3-D Lissajous knot.
 * Ninety soft glowing balls are spaced along the curve x = sin(a u), y =
 * sin(b u + p), z = sin(c u + q) and slide along it forever; the frequency
 * triple morphs between five presets with a smoothstep, so the knot unties
 * itself into a new one every ten seconds without a single hard change. Balls
 * are sized and brightened by depth and drawn additively over a hairline
 * thread that follows the same curve, so the figure reads as a beaded wire
 * turning in space. Black everywhere else — the sparsest layer in the set. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p176_up;

#define P176_W 480
#define P176_H 360
#define P176_NB 90
#define P176_NSET 5
#define P176_TAU 6.28318530717958647692f

static float p176_acc[P176_W * P176_H * 3];
static unsigned char p176_img[P176_W * P176_H * 3];
static unsigned char p176_tone[1024];
static int *p176_xm;
static int p176_xmw;
static int p176_ready;
static uint32_t p176_seedc;
static float p176_col[64][3];
static float p176_hue0, p176_huew, p176_dir;
static float p176_set[P176_NSET][5];
static float p176_bx[P176_NB], p176_by[P176_NB], p176_bz[P176_NB];

static uint32_t p176_rs;
static float p176_rf(void)
{
    p176_rs ^= p176_rs << 13; p176_rs ^= p176_rs >> 17; p176_rs ^= p176_rs << 5;
    return (float)(p176_rs >> 8) * (1.0f / 16777216.0f);
}

static void p176_setup(uint32_t seed)
{
    int i;
    p176_rs = seed ? seed ^ 0xB0B0C1A7u : 0xB0B0C1A7u;
    p176_rf(); p176_rf();
    p176_hue0 = p176_rf();
    p176_huew = 0.20f + p176_rf() * 0.62f;
    p176_dir  = p176_rf() < 0.5f ? -1.0f : 1.0f;
    {   /* integer triples only, so every curve closes on itself */
        static const float trip[8][3] = {
            {1,2,3}, {2,3,4}, {3,2,5}, {1,3,2},
            {2,5,3}, {3,4,5}, {1,4,3}, {2,3,5}
        };
        for (i = 0; i < P176_NSET; i++) {
            int k = (int)(p176_rf() * 8.0f) & 7;
            p176_set[i][0] = trip[k][0];
            p176_set[i][1] = trip[k][1];
            p176_set[i][2] = trip[k][2];
            p176_set[i][3] = p176_rf() * P176_TAU;
            p176_set[i][4] = p176_rf() * P176_TAU;
        }
    }
    if (!p176_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.0f / 1024.0f)));
            p176_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p176_ready = 1;
    }
    p176_seedc = seed;
}

static void p176_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p176_hue0 + p176_huew * ((float)i / 63.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p176_col[i][0] = 0.10f + 0.90f * r / mx;
        p176_col[i][1] = 0.10f + 0.90f * g / mx;
        p176_col[i][2] = 0.10f + 0.90f * b / mx;
    }
}

static void p176_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= P176_W - 1 || yi >= P176_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p176_acc + (yi * P176_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P176_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p176_thread(float x0, float y0, float x1, float y1,
                        const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    if (len > 260.0f) return;
    n = (int)(len * 1.2f) + 1;
    dx /= (float)n; dy /= (float)n;
    w *= (len / (float)n + 0.3f);
    for (i = 0; i < n; i++)
        p176_splat(x0 + dx * (float)i, y0 + dy * (float)i, c, w);
}

/* a soft ball: (1 - r^2)^2 falloff, additive */
static void p176_bob(float cx, float cy, float rad, const float *c, float w)
{
    int x0 = (int)(cx - rad), x1 = (int)(cx + rad) + 1;
    int y0 = (int)(cy - rad), y1 = (int)(cy + rad) + 1;
    int x, y;
    float ir2 = 1.0f / (rad * rad);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > P176_W) x1 = P176_W;
    if (y1 > P176_H) y1 = P176_H;
    for (y = y0; y < y1; y++) {
        float dy = (float)y - cy;
        float *p = p176_acc + (y * P176_W + x0) * 3;
        for (x = x0; x < x1; x++, p += 3) {
            float dx = (float)x - cx;
            float q = 1.0f - (dx * dx + dy * dy) * ir2;
            float v;
            if (q <= 0.0f) continue;
            v = w * q * q;
            p[0] += c[0] * v; p[1] += c[1] * v; p[2] += c[2] * v;
        }
    }
}

static void p176_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P176_W * P176_H * 3; i++) {
        int ti = (int)(p176_acc[i] * 256.0f);
        p176_img[i] = p176_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p176_xmw != w) {
        free(p176_xm);
        p176_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p176_xm[x] = (int)(((long long)x * (P176_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p176_xmw = w;
    }
    jd_up_blit(&p176_up, fb, w, h, p176_img, P176_W, P176_H);
}

void pattern_176(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float m, u0;
    float ca, sa, cb, sb, sc, D, cx, cy;
    int i, s0, s1;
    (void)sl;
    if (!p176_ready || p176_seedc != seed) p176_setup(seed);
    p176_hues(pal);
    memset(p176_acc, 0, sizeof p176_acc);

    {   /* smoothstep morph between consecutive frequency presets */
        float leg = t * (1.0f / 640.0f);
        int li = (int)leg;
        float f = leg - (float)li;
        m = f < 0.62f ? 0.0f : (f - 0.62f) / 0.38f;
        m = m * m * (3.0f - 2.0f * m);
        s0 = li % P176_NSET; s1 = (li + 1) % P176_NSET;
    }
    u0 = t * 0.0016f * p176_dir;
    ca = cosf(0.50f + 0.30f * sinf(t * 0.00047f));
    sa = sinf(0.50f + 0.30f * sinf(t * 0.00047f));
    cb = cosf(t * 0.00093f * p176_dir);
    sb = sinf(t * 0.00093f * p176_dir);
    sc = (float)P176_H * 0.300f;
    D  = 3.4f;
    cx = P176_W * 0.5f; cy = P176_H * 0.5f;

    for (i = 0; i < P176_NB; i++) {
        float u = P176_TAU * (float)i / (float)P176_NB + u0;
        /* both knots are closed; lerping the POINTS (not the frequencies)
           keeps the figure closed all the way through the morph */
        float X0 = sinf(p176_set[s0][0] * u);
        float Y0 = sinf(p176_set[s0][1] * u + p176_set[s0][3]);
        float Z0 = sinf(p176_set[s0][2] * u + p176_set[s0][4]);
        float X1 = sinf(p176_set[s1][0] * u);
        float Y1 = sinf(p176_set[s1][1] * u + p176_set[s1][3]);
        float Z1 = sinf(p176_set[s1][2] * u + p176_set[s1][4]);
        float X = X0 + (X1 - X0) * m, Y = Y0 + (Y1 - Y0) * m,
              Z = Z0 + (Z1 - Z0) * m;
        float x1, y1, z1, x2, y2, z2, den;
        x2 = X * cb + Z * sb; z2 = -X * sb + Z * cb;
        y2 = Y * ca - z2 * sa; z2 = Y * sa + z2 * ca;
        x1 = x2; y1 = y2; z1 = z2;
        den = D - z1;
        if (den < 0.7f) den = 0.7f;
        p176_bx[i] = cx + x1 * sc * (D / den);
        p176_by[i] = cy + y1 * sc * (D / den);
        p176_bz[i] = z1;
    }
    for (i = 0; i < P176_NB; i++) {          /* the wire, drawn under the bobs */
        int j = (i + 1) % P176_NB;
        const float *c = p176_col[(i * 63 / P176_NB) & 63];
        p176_thread(p176_bx[i], p176_by[i], p176_bx[j], p176_by[j], c, 0.42f);
    }
    for (i = 0; i < P176_NB; i++) {
        float z = p176_bz[i];
        float dep = (z + 1.2f) * 0.45f;
        float rad = 5.0f + 11.0f * dep;
        const float *c = p176_col[(i * 63 / P176_NB) & 63];
        float pulse = 0.70f + 0.30f * sinf((float)i * 0.5f - t * 0.011f);
        p176_bob(p176_bx[i], p176_by[i], rad, c, (0.30f + 0.85f * dep) * pulse);
    }
    p176_blit(fb, w, h);
}
