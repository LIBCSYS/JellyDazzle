/* 180 Hilbert Weave — a space-filling curve refining and coarsening itself.
 * The Hilbert curves of order 4, 5 and 6 are generated once by the standard
 * d2xy bit-twiddle. To move between two orders the coarser path is resampled
 * to the finer one's point count and the two are then linearly interpolated,
 * so the thread does not jump from 1024 to 4096 segments — it grows the extra
 * detail out of itself, folding new corners into the existing ones and pulling
 * them back out again on the way down. A hue ramp runs along the curve's
 * parameter and a brightness pulse chases it, which makes the direction of
 * travel visible; the whole square turns slowly. One continuous glowing line
 * on black, from end to end. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p180_up;

#define P180_W 480
#define P180_H 360
#define P180_N4 256
#define P180_N5 1024
#define P180_N6 4096

static float p180_acc[P180_W * P180_H * 3];
static unsigned char p180_img[P180_W * P180_H * 3];
static unsigned char p180_tone[1024];
static int *p180_xm;
static int p180_xmw;
static int p180_ready;
static uint32_t p180_seedc;
static float p180_col[64][3];
static float p180_hue0, p180_huew, p180_dir;
static float p180_h4[P180_N4 * 2], p180_h5[P180_N5 * 2], p180_h6[P180_N6 * 2];
static float p180_u45[P180_N5 * 2], p180_u56[P180_N6 * 2];

static uint32_t p180_rs;
static float p180_rf(void)
{
    p180_rs ^= p180_rs << 13; p180_rs ^= p180_rs >> 17; p180_rs ^= p180_rs << 5;
    return (float)(p180_rs >> 8) * (1.0f / 16777216.0f);
}

static void p180_d2xy(int n, int d, int *xp, int *yp)
{
    int rx, ry, s, t = d, x = 0, y = 0;
    for (s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        if (ry == 0) {
            if (rx == 1) { x = s - 1 - x; y = s - 1 - y; }
            { int tmp = x; x = y; y = tmp; }
        }
        x += s * rx; y += s * ry;
        t /= 4;
    }
    *xp = x; *yp = y;
}

static void p180_build(float *out, int order)
{
    int n = 1 << order, cnt = n * n, d, x, y;
    float k = 2.0f / (float)(n - 1);
    for (d = 0; d < cnt; d++) {
        p180_d2xy(n, d, &x, &y);
        out[d * 2 + 0] = (float)x * k - 1.0f;
        out[d * 2 + 1] = (float)y * k - 1.0f;
    }
}

/* resample a path of m points onto n points, uniformly in parameter */
static void p180_resample(const float *src, int m, float *dst, int n)
{
    int i;
    float sc = (float)(m - 1) / (float)(n - 1);
    for (i = 0; i < n; i++) {
        float p = (float)i * sc;
        int k = (int)p;
        float f = p - (float)k;
        if (k >= m - 1) { k = m - 2; f = 1.0f; }
        dst[i * 2 + 0] = src[k * 2 + 0] + (src[(k + 1) * 2 + 0] - src[k * 2 + 0]) * f;
        dst[i * 2 + 1] = src[k * 2 + 1] + (src[(k + 1) * 2 + 1] - src[k * 2 + 1]) * f;
    }
}

static void p180_setup(uint32_t seed)
{
    int i;
    p180_rs = seed ? seed ^ 0x41B8E180u : 0x41B8E180u;
    p180_rf(); p180_rf();
    p180_hue0 = p180_rf();
    p180_huew = 0.20f + p180_rf() * 0.62f;
    p180_dir  = p180_rf() < 0.5f ? -1.0f : 1.0f;
    if (!p180_ready) {
        p180_build(p180_h4, 4);
        p180_build(p180_h5, 5);
        p180_build(p180_h6, 6);
        p180_resample(p180_h4, P180_N4, p180_u45, P180_N5);
        p180_resample(p180_h5, P180_N5, p180_u56, P180_N6);
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.4f / 1024.0f)));
            p180_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p180_ready = 1;
    }
    p180_seedc = seed;
}

static void p180_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p180_hue0 + p180_huew * ((float)i / 63.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p180_col[i][0] = 0.12f + 0.88f * r / mx;
        p180_col[i][1] = 0.12f + 0.88f * g / mx;
        p180_col[i][2] = 0.12f + 0.88f * b / mx;
    }
}

static void p180_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= P180_W - 1 || yi >= P180_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p180_acc + (yi * P180_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P180_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p180_seg(float x0, float y0, float x1, float y1,
                     const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    if (len > 300.0f) return;
    n = (int)(len * 1.35f) + 1;
    dx /= (float)n; dy /= (float)n;
    w *= (len / (float)n + 0.34f);
    for (i = 0; i < n; i++)
        p180_splat(x0 + dx * (float)i, y0 + dy * (float)i, c, w);
}

static void p180_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P180_W * P180_H * 3; i++) {
        int ti = (int)(p180_acc[i] * 256.0f);
        p180_img[i] = p180_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p180_xmw != w) {
        free(p180_xm);
        p180_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p180_xm[x] = (int)(((long long)x * (P180_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p180_xmw = w;
    }
    jd_up_blit(&p180_up, fb, w, h, p180_img, P180_W, P180_H);
}

void pattern_180(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    const float *A, *B;
    float m, sc, ct, st, cx, cy;
    int n, i;
    (void)sl;
    if (!p180_ready || p180_seedc != seed) p180_setup(seed);
    p180_hues(pal);
    memset(p180_acc, 0, sizeof p180_acc);

    {   /* 4 -> 5 -> 6 -> 5 -> 4, smoothstepped, 760 frames a leg */
        float leg = t * (1.0f / 760.0f);
        int li = (int)leg & 3;
        float f = leg - floorf(leg);
        f = f * f * (3.0f - 2.0f * f);
        switch (li) {
        case 0:  A = p180_u45; B = p180_h5; n = P180_N5; m = f; break;
        case 1:  A = p180_u56; B = p180_h6; n = P180_N6; m = f; break;
        case 2:  A = p180_u56; B = p180_h6; n = P180_N6; m = 1.0f - f; break;
        default: A = p180_u45; B = p180_h5; n = P180_N5; m = 1.0f - f; break;
        }
    }
    sc = (float)P180_H * 0.300f * (1.0f + 0.045f * sinf(t * 0.00069f));
    ct = cosf(t * 0.00052f * p180_dir);
    st = sinf(t * 0.00052f * p180_dir);
    cx = P180_W * 0.5f; cy = P180_H * 0.5f;

    {
        float pxs = 0.0f, pys = 0.0f;
        float phase = t * 0.0075f;
        int have = 0;
        for (i = 0; i < n; i++) {
            float ax = A[i * 2] + (B[i * 2] - A[i * 2]) * m;
            float ay = A[i * 2 + 1] + (B[i * 2 + 1] - A[i * 2 + 1]) * m;
            float X = cx + (ax * ct - ay * st) * sc;
            float Y = cy + (ax * st + ay * ct) * sc;
            float u = (float)i / (float)(n - 1);
            const float *c = p180_col[(int)(u * 63.0f) & 63];
            float pulse = 0.5f + 0.5f * sinf(u * 26.0f - phase);
            if (have)
                p180_seg(pxs, pys, X, Y, c, 0.85f + 1.45f * pulse * pulse);
            pxs = X; pys = Y; have = 1;
        }
    }
    p180_blit(fb, w, h);
}
