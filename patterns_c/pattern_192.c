/* 192 Steiner Pearls — Steiner chains, rolling under a circle inversion.
 * A Steiner chain is the exact solution of "n equal circles, each tangent to
 * its neighbours and to two fixed circles": it exists only when the annulus
 * ratio satisfies sin(pi/n) = (R-r)/(R+r), which fixes the whole necklace from
 * n alone. Three concentric chains (different n, counter-rotating) are built in
 * that closed form and then pushed through an inversion whose centre orbits
 * off-axis. Inversion maps circles to circles exactly, so the chains stay
 * tangent while their sizes slide around the ring — pearls swelling on one
 * side, packing tight on the other. Rings and their inner highlights only. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p192_up;

#define P192_W 480
#define P192_H 360
#define P192_TAU 6.28318530717958647692f

static float p192_acc[P192_W * P192_H * 3];
static unsigned char p192_img[P192_W * P192_H * 3];
static unsigned char p192_tone[1024];
static int *p192_xm;
static int p192_xmw;
static int p192_tone_ok;
static uint32_t p192_rs = 1u;

static float p192_rf(void)
{
    p192_rs ^= p192_rs << 13; p192_rs ^= p192_rs >> 17; p192_rs ^= p192_rs << 5;
    return (float)(p192_rs >> 8) * (1.0f / 16777216.0f);
}

static void p192_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (7.00f / 1024.0f)));
        p192_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p192_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p192_col(const uint32_t *pal, float hue, float lift, float *out)
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


static void p192_splat(float x, float y, const float *c, float w)
{
    int xi, yi; float fx, fy, w0, w1; float *p;
    if (!(x >= 0.0f) || !(y >= 0.0f)) return;
    xi = (int)x; yi = (int)y;
    if (xi >= P192_W - 1 || yi >= P192_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p192_acc + (yi * P192_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P192_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}


/* energy-conserving line: total deposit is w * length, so brightness does not
 * depend on how finely a curve happens to be subdivided. */
static void p192_line(float x0, float y0, float x1, float y1, const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0, len, inv, wq;
    int n, i;
    len = sqrtf(dx * dx + dy * dy);
    if (!(len < 900.0f)) return;
    n = (int)len; if (n < 1) n = 1; if (n > 512) n = 512;
    inv = 1.0f / (float)n;
    wq = w * len * inv;
    if (len < 1.0f) wq = w * len;
    for (i = 0; i < n; i++) {
        float t = ((float)i + 0.5f) * inv;
        p192_splat(x0 + dx * t, y0 + dy * t, c, wq);
    }
}


static void p192_ring(float cx, float cy, float r, const float *c, float w)
{
    int ns, i; float st, px, py;
    if (!(r < 4000.0f)) return;
    if (cx + r < 0.0f || cx - r > P192_W || cy + r < 0.0f || cy - r > P192_H) return;
    if (r < 0.40f) { p192_splat(cx, cy, c, w * 2.2f); return; }
    ns = (int)(r * 1.7f) + 10; if (ns > 640) ns = 640;
    st = P192_TAU / (float)ns;
    px = cx + r; py = cy;
    for (i = 1; i <= ns; i++) {
        float a = (float)i * st;
        float nx = cx + r * cosf(a), ny = cy + r * sinf(a);
        p192_line(px, py, nx, ny, c, w);
        px = nx; py = ny;
    }
}

static float p192_tmp[P192_W * P192_H * 3];

/* 5-tap soft glow, in place. Keeps line art from aliasing when it is scaled
 * up to 1280x960 and keeps frame-to-frame motion visually continuous. */
static void p192_blur(void)
{
    int y, x, c;
    for (y = 1; y < P192_H - 1; y++)
        for (x = 1; x < P192_W - 1; x++) {
            int o = (y * P192_W + x) * 3;
            for (c = 0; c < 3; c++)
                p192_tmp[o + c] = p192_acc[o + c] * 0.52f
                    + 0.12f * (p192_acc[o + c - 3] + p192_acc[o + c + 3]
                             + p192_acc[o + c - P192_W * 3] + p192_acc[o + c + P192_W * 3]);
        }
    for (y = 1; y < P192_H - 1; y++)
        memcpy(p192_acc + (y * P192_W + 1) * 3, p192_tmp + (y * P192_W + 1) * 3,
               sizeof(float) * 3 * (P192_W - 2));
}

static void p192_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P192_W * P192_H * 3; i++) {
        int ti = (int)(p192_acc[i] * 256.0f);
        p192_img[i] = p192_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p192_xmw != w) {
        free(p192_xm);
        p192_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p192_xm[x] = (int)(((long long)x * (P192_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p192_xmw = w;
    }
    jd_up_blit(&p192_up, fb, w, h, p192_img, P192_W, P192_H);
}

#define P192_NCH 3

static uint32_t p192_seedc = 0xFFFFFFFFu;
static float p192_h0, p192_hw, p192_pd, p192_k2;
static int p192_n[P192_NCH];
static float p192_s[P192_NCH], p192_sp[P192_NCH], p192_rc[P192_NCH];
static float p192_hc[P192_NCH][32][3];

static void p192_build(uint32_t seed)
{
    int c;
    p192_rs = seed ? seed * 2246822519u + 0x85EBCA6Bu : 0x192u;
    p192_rf(); p192_rf();
    p192_h0 = p192_rf();
    p192_hw = 0.05f + p192_rf() * 0.50f;
    p192_pd = 2.40f + p192_rf() * 1.20f;
    p192_k2 = 3.10f + p192_rf() * 2.60f;
    for (c = 0; c < P192_NCH; c++) {
        p192_n[c] = 6 + (int)(p192_rf() * 12.0f);        /* 6..17 pearls */
        p192_s[c] = sinf(3.14159265f / (float)p192_n[c]);
        p192_sp[c] = ((c & 1) ? -1.0f : 1.0f) * (0.00085f + p192_rf() * 0.00115f);
        p192_rc[c] = 1.0f - 0.28f * (float)c;
    }
    p192_seedc = seed;
    if (!p192_tone_ok) p192_tone_init();
}

void pattern_192(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, Px, Py, ox, oy, scale, k2;
    int c, j;
    (void)sl;
    if (p192_seedc != seed) p192_build(seed);
    for (c = 0; c < P192_NCH; c++)
        for (j = 0; j < p192_n[c]; j++)
            p192_col(pal, p192_h0 + p192_hw * (0.62f * (float)j / (float)p192_n[c]
                                               + 0.30f * (float)c), 0.12f, p192_hc[c][j]);
    memset(p192_acc, 0, sizeof p192_acc);

    {   float pa = t * 0.00047f;
        float pd = p192_pd * (1.0f + 0.11f * sinf(t * 0.00031f));
        Px = pd * cosf(pa); Py = pd * sinf(pa); }
    k2 = p192_k2 * (1.0f + 0.05f * sinf(t * 0.00019f));
    {   /* frame on the image of the outermost bounding circle */
        float R = 1.0f + p192_s[0];
        float dd = Px * Px + Py * Py - R * R;
        float s;
        if (fabsf(dd) < 0.04f) dd = dd < 0.0f ? -0.04f : 0.04f;
        s = k2 / dd;
        ox = Px - s * Px; oy = Py - s * Py;
        scale = (float)P192_H * 0.455f / (fabsf(s) * R + 1e-4f);
    }

    for (c = 0; c < P192_NCH; c++) {
        float rc = p192_rc[c], s = p192_s[c] * rc, ring = rc;
        float rot = t * p192_sp[c];
        float st = P192_TAU / (float)p192_n[c];
        /* the two guide circles, faint */
        for (j = 0; j < 2; j++) {
            float R = (j ? ring + s : ring - s);
            float dd = Px * Px + Py * Py - R * R, sf, gx, gy, gr;
            if (fabsf(dd) < 0.02f) continue;
            sf = k2 / dd;
            gx = Px - sf * Px; gy = Py - sf * Py; gr = fabsf(sf) * R;
            p192_ring(P192_W * 0.5f + (gx - ox) * scale,
                      P192_H * 0.5f + (gy - oy) * scale, gr * scale,
                      p192_hc[c][0], 0.10f);
        }
        for (j = 0; j < p192_n[c]; j++) {
            float a = (float)j * st + rot;
            float cx = ring * cosf(a), cy = ring * sinf(a);
            float dx = cx - Px, dy = cy - Py;
            float dd = dx * dx + dy * dy - s * s;
            float sf, gx, gy, gr, rpx, wgt;
            if (fabsf(dd) < 0.012f) continue;
            sf = k2 / dd;
            gx = Px + sf * dx; gy = Py + sf * dy; gr = fabsf(sf) * s;
            gx = P192_W * 0.5f + (gx - ox) * scale;
            gy = P192_H * 0.5f + (gy - oy) * scale;
            rpx = gr * scale;
            if (rpx > 900.0f) continue;
            wgt = 0.42f + 0.30f * sinf(a * 2.0f - t * 0.011f);
            p192_ring(gx, gy, rpx, p192_hc[c][j], wgt * 1.70f);
            p192_ring(gx, gy, rpx * 0.52f, p192_hc[c][j], wgt * 0.70f);
            p192_splat(gx, gy, p192_hc[c][j], 1.6f + rpx * 0.03f);
        }
    }
    p192_blur();
    p192_blit(fb, w, h);
}
