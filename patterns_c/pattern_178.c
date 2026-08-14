/* 178 Doyle Spiral — a hexagonal circle packing bent into a logarithmic spiral.
 * Take the triangular lattice of equal tangent circles in w-space and push it
 * through the conformal map z = exp(w). Angles are preserved, so the circles
 * stay circles and stay tangent, but each one is scaled by |z| — the packing
 * becomes a spiral of discs growing geometrically from the middle of the
 * frame. The lattice spacing is chosen as s = 2*pi/(q*sqrt3) so that q of the
 * lattice steps close exactly one turn, which makes the figure periodic under
 * a zoom of exp(s*q): drifting w along the real axis is therefore a seamless
 * endless zoom, new discs blooming at the centre while the outer ones leave.
 * Rims only, on black, so it stacks as a lattice over anything below it. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p178_up;

#define P178_W 480
#define P178_H 360
#define P178_TAU 6.28318530717958647692f

static float p178_acc[P178_W * P178_H * 3];
static unsigned char p178_img[P178_W * P178_H * 3];
static unsigned char p178_tone[1024];
static int *p178_xm;
static int p178_xmw;
static int p178_ready;
static uint32_t p178_seedc;
static float p178_col[64][3];
static float p178_hue0, p178_huew, p178_zoom, p178_rot;
static int p178_q;

static uint32_t p178_rs;
static float p178_rf(void)
{
    p178_rs ^= p178_rs << 13; p178_rs ^= p178_rs >> 17; p178_rs ^= p178_rs << 5;
    return (float)(p178_rs >> 8) * (1.0f / 16777216.0f);
}

static void p178_setup(uint32_t seed)
{
    int i;
    p178_rs = seed ? seed ^ 0xD01E5717u : 0xD01E5717u;
    p178_rf(); p178_rf();
    p178_hue0 = p178_rf();
    p178_huew = 0.14f + p178_rf() * 0.55f;
    p178_q    = 5 + (int)(p178_rf() * 5.0f);          /* 5..9 arms           */
    p178_zoom = (p178_rf() < 0.5f ? -1.0f : 1.0f) * (0.00055f + p178_rf() * 0.00055f);
    p178_rot  = (p178_rf() < 0.5f ? -1.0f : 1.0f) * (0.00030f + p178_rf() * 0.00040f);
    if (!p178_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.2f / 1024.0f)));
            p178_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p178_ready = 1;
    }
    p178_seedc = seed;
}

static void p178_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p178_hue0 + p178_huew * ((float)i / 63.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p178_col[i][0] = 0.12f + 0.88f * r / mx;
        p178_col[i][1] = 0.12f + 0.88f * g / mx;
        p178_col[i][2] = 0.12f + 0.88f * b / mx;
    }
}

static void p178_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= P178_W - 1 || yi >= P178_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p178_acc + (yi * P178_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P178_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p178_ring(float cx, float cy, float rad, const float *c, float w)
{
    int ns, i;
    float da, a, k;
    if (rad < 0.35f || rad > 1200.0f) return;
    /* skip rings entirely outside the canvas without walking them */
    if (cx + rad < 0.0f || cx - rad > (float)P178_W ||
        cy + rad < 0.0f || cy - rad > (float)P178_H) return;
    ns = (int)(rad * 8.0f) + 14;
    if (ns > 4000) ns = 4000;
    da = P178_TAU / (float)ns;
    k = w * (P178_TAU * rad) / (float)ns;
    a = 0.0f;
    for (i = 0; i < ns; i++, a += da) {
        float ca = cosf(a), sa = sinf(a);
        p178_splat(cx + rad * ca, cy + rad * sa, c, k);
        p178_splat(cx + (rad + 1.25f) * ca, cy + (rad + 1.25f) * sa, c, k * 0.38f);
        if (rad > 2.0f)
            p178_splat(cx + (rad - 1.25f) * ca, cy + (rad - 1.25f) * sa, c, k * 0.38f);
    }
}

static void p178_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P178_W * P178_H * 3; i++) {
        int ti = (int)(p178_acc[i] * 256.0f);
        p178_img[i] = p178_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p178_xmw != w) {
        free(p178_xm);
        p178_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p178_xm[x] = (int)(((long long)x * (P178_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p178_xmw = w;
    }
    jd_up_blit(&p178_up, fb, w, h, p178_img, P178_W, P178_H);
}

void pattern_178(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float s, base, rot, du, cx, cy;
    int q, n, m, mlo, mhi;
    (void)sl;
    if (!p178_ready || p178_seedc != seed) p178_setup(seed);
    p178_hues(pal);
    memset(p178_acc, 0, sizeof p178_acc);

    q    = p178_q;
    s    = P178_TAU / ((float)q * 1.7320508f);
    base = 1.0f;
    rot  = t * p178_rot;
    du   = t * p178_zoom / s;                    /* continuous log-zoom      */
    cx   = P178_W * 0.5f; cy = P178_H * 0.5f;
    mlo  = (int)floorf(logf(0.7f / base) / s) - 2;
    mhi  = (int)ceilf(logf(420.0f / base) / s) + 2;

    for (n = 0; n < 2 * q; n++) {
        float ang = (float)n * (float)M_PI / (float)q + rot;
        float ca = cosf(ang), sa = sinf(ang);
        for (m = mlo; m <= mhi; m++) {
            float u = (float)m + 0.5f * (float)n + du;
            float R = base * expf(s * u);
            float rad = R * s * 0.5f;
            float fade = 1.0f, ex;
            const float *c;
            if (R < 0.6f || R > 460.0f) continue;
            /* fade the newborn discs up out of the singularity and the huge
               ones out past the corner, so nothing pops in or out */
            if (R < 6.0f)   fade = (R - 0.6f) / 5.4f;
            if (R > 330.0f) fade = 1.0f - (R - 330.0f) / 130.0f;
            if (fade <= 0.0f) continue;
            fade = fade * fade * (3.0f - 2.0f * fade);
            ex = 0.5f + 0.5f * sinf(s * u * 2.2f - t * 0.010f + (float)n * 0.7f);
            c = p178_col[(int)(u * 5.0f + (float)n * 2.0f) & 63];
            p178_ring(cx + R * ca, cy + R * sa, rad, c,
                      fade * (0.42f + 0.72f * ex));
        }
    }
    p178_blit(fb, w, h);
}
