/* 198 Whirl Nest — the schoolbook whirl, doubled and set spinning.
 * Take a regular n-gon and build the next one by walking a fraction f along
 * every edge; repeat sixty times and the polygons shrink and rotate by a fixed
 * angle each step, so their corners lie on a logarithmic spiral. Drawing every
 * level gives the classic whirl. This one draws two of them at once, f and
 * 1-f, which are mirror-image chiralities of the same nest, so the two spiral
 * families cross and the frame reads as a woven star. f breathes slowly, which
 * changes the pitch of both spirals together, and a brightness pulse travels
 * inward level by level. Pure outline on black — nothing but hairlines. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p198_up;

#define P198_W 480
#define P198_H 360
#define P198_TAU 6.28318530717958647692f

static float p198_acc[P198_W * P198_H * 3];
static unsigned char p198_img[P198_W * P198_H * 3];
static unsigned char p198_tone[1024];
static int *p198_xm;
static int p198_xmw;
static int p198_tone_ok;
static uint32_t p198_rs = 1u;

static float p198_rf(void)
{
    p198_rs ^= p198_rs << 13; p198_rs ^= p198_rs >> 17; p198_rs ^= p198_rs << 5;
    return (float)(p198_rs >> 8) * (1.0f / 16777216.0f);
}

static void p198_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (7.50f / 1024.0f)));
        p198_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p198_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p198_col(const uint32_t *pal, float hue, float lift, float *out)
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


static void p198_splat(float x, float y, const float *c, float w)
{
    int xi, yi; float fx, fy, w0, w1; float *p;
    if (!(x >= 0.0f) || !(y >= 0.0f)) return;
    xi = (int)x; yi = (int)y;
    if (xi >= P198_W - 1 || yi >= P198_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p198_acc + (yi * P198_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P198_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}


/* energy-conserving line: total deposit is w * length, so brightness does not
 * depend on how finely a curve happens to be subdivided. */
static void p198_line(float x0, float y0, float x1, float y1, const float *c, float w)
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
        p198_splat(x0 + dx * t, y0 + dy * t, c, wq);
    }
}


static float p198_tmp[P198_W * P198_H * 3];

/* 5-tap soft glow, in place. Keeps line art from aliasing when it is scaled
 * up to 1280x960 and keeps frame-to-frame motion visually continuous. */
static void p198_blur(void)
{
    int y, x, c;
    for (y = 1; y < P198_H - 1; y++)
        for (x = 1; x < P198_W - 1; x++) {
            int o = (y * P198_W + x) * 3;
            for (c = 0; c < 3; c++)
                p198_tmp[o + c] = p198_acc[o + c] * 0.52f
                    + 0.12f * (p198_acc[o + c - 3] + p198_acc[o + c + 3]
                             + p198_acc[o + c - P198_W * 3] + p198_acc[o + c + P198_W * 3]);
        }
    for (y = 1; y < P198_H - 1; y++)
        memcpy(p198_acc + (y * P198_W + 1) * 3, p198_tmp + (y * P198_W + 1) * 3,
               sizeof(float) * 3 * (P198_W - 2));
}

static void p198_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P198_W * P198_H * 3; i++) {
        int ti = (int)(p198_acc[i] * 256.0f);
        p198_img[i] = p198_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p198_xmw != w) {
        free(p198_xm);
        p198_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p198_xm[x] = (int)(((long long)x * (P198_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p198_xmw = w;
    }
    jd_up_blit(&p198_up, fb, w, h, p198_img, P198_W, P198_H);
}

#define P198_LV 62
#define P198_NMAX 11

static uint32_t p198_seedc = 0xFFFFFFFFu;
static float p198_h0, p198_hw, p198_f0, p198_fa, p198_spin, p198_pulse;
static int p198_n;
static float p198_hue[P198_LV][3];

static void p198_build(uint32_t seed)
{
    p198_rs = seed ? seed * 2654435761u + 0xC2B2AE35u : 0x198u;
    p198_rf(); p198_rf();
    p198_h0 = p198_rf();
    p198_hw = 0.05f + p198_rf() * 0.55f;
    p198_n  = 3 + (int)(p198_rf() * 8.0f);
    if (p198_n > P198_NMAX) p198_n = P198_NMAX;
    p198_f0 = 0.055f + p198_rf() * 0.045f;
    p198_fa = 0.028f + p198_rf() * 0.030f;
    p198_spin = (p198_rf() < 0.5f ? -1.0f : 1.0f) * (0.00055f + p198_rf() * 0.00075f);
    p198_pulse = 0.010f + p198_rf() * 0.010f;
    p198_seedc = seed;
    if (!p198_tone_ok) p198_tone_init();
}

static void p198_whirl(float f, float rot, float rad, float t, float dir)
{
    float x[P198_NMAX], y[P198_NMAX], nx[P198_NMAX], ny[P198_NMAX];
    int i, lv;
    for (i = 0; i < p198_n; i++) {
        float a = (float)i * (P198_TAU / (float)p198_n) + rot;
        x[i] = P198_W * 0.5f + rad * cosf(a);
        y[i] = P198_H * 0.5f + rad * sinf(a);
    }
    for (lv = 0; lv < P198_LV; lv++) {
        float ph = (float)lv * 0.30f - t * p198_pulse * dir;
        float wg = 0.16f + 0.30f * (0.5f + 0.5f * sinf(ph));
        const float *col = p198_hue[lv];
        for (i = 0; i < p198_n; i++) {
            int j = (i + 1) % p198_n;
            p198_line(x[i], y[i], x[j], y[j], col, wg);
            nx[i] = x[i] + (x[j] - x[i]) * f;
            ny[i] = y[i] + (y[j] - y[i]) * f;
        }
        for (i = 0; i < p198_n; i++) { x[i] = nx[i]; y[i] = ny[i]; }
    }
}

void pattern_198(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, f, rot, rad;
    int i;
    (void)sl;
    if (p198_seedc != seed) p198_build(seed);
    for (i = 0; i < P198_LV; i++)
        p198_col(pal, p198_h0 + p198_hw * ((float)i / (float)(P198_LV - 1)),
                 0.12f, p198_hue[i]);
    memset(p198_acc, 0, sizeof p198_acc);

    f   = p198_f0 + p198_fa * (0.5f + 0.5f * sinf(t * 0.00041f));
    rot = t * p198_spin;
    rad = (float)P198_H * (0.455f + 0.035f * sinf(t * 0.00027f));
    p198_whirl(f, rot, rad, t, 1.0f);
    p198_whirl(1.0f - f, -rot + 0.5f, rad, t, -1.0f);
    p198_blur();
    p198_blit(fb, w, h);
}
