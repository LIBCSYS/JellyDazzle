/* 182 Glass Weave — Leon Glass's random-dot moire, animated. A fixed cloud of
 * 3400 random points is superimposed on its own images under a near-identity
 * linear map  T(p) = c + (I + eps*A)(p - c);  the eye cannot see the individual
 * pairs but it locks instantly onto the flow field of A, so the dust organises
 * itself into circulation, radial spray, spirals or hyperbolic saddles with no
 * lines drawn anywhere. Four iterates T^0..T^3 are plotted per point, each a
 * step dimmer and hue-shifted, which turns each dot into a four-bead streak
 * along its own trajectory. The four entries of A are independent slow sines,
 * so the pattern walks continuously through the whole Glass family — rotation
 * -> spiral -> radial -> saddle -> back — and because the dust itself only
 * creeps (0.0006 rad/frame), nothing ever jumps. Pure sparse dust on black,
 * about 4% coverage: the lightest overlay in the set. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p182_up;

#define CW 480
#define CH 360
#define NDOT 3400
#define NIT 4

static float p182_acc[CW * CH * 3];
static unsigned char p182_img[CW * CH * 3];
static float p182_dx[NDOT], p182_dy[NDOT], p182_dr[NDOT], p182_dh[NDOT];
static unsigned char p182_tone[2048];
static int *p182_xm;
static int p182_xmw;
static float p182_hue0, p182_huew, p182_spin, p182_w0, p182_w1, p182_w2, p182_w3;
static float p182_eps;
static uint32_t p182_seedc;
static int p182_ready, p182_tabs;

static uint32_t p182_rs;
static float p182_rf(void)
{
    p182_rs ^= p182_rs << 13; p182_rs ^= p182_rs >> 17; p182_rs ^= p182_rs << 5;
    return (float)(p182_rs >> 8) * (1.0f / 16777216.0f);
}

static void p182_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p182_setup(uint32_t seed)
{
    int i;
    p182_rs = seed ? seed ^ 0x61A55Du : 0x61A55Du;
    p182_rf(); p182_rf();
    for (i = 0; i < NDOT; i++) {
        float a = p182_rf() * 6.2831853f;
        float r = sqrtf(p182_rf()) * 2.06f;
        p182_dx[i] = cosf(a) * r;
        p182_dy[i] = sinf(a) * r;
        p182_dr[i] = 0.55f + p182_rf() * 0.60f;
        p182_dh[i] = 0.5f + 0.5f * sinf(a * 2.0f + r * 1.6f) + 0.06f * p182_rf();
    }
    p182_hue0 = p182_rf();
    p182_huew = 0.06f + p182_rf() * 0.44f;
    p182_spin = (p182_rf() - 0.5f) * 0.0013f;
    p182_w0 = 0.00061f + p182_rf() * 0.00042f;
    p182_w1 = 0.00047f + p182_rf() * 0.00046f;
    p182_w2 = 0.00039f + p182_rf() * 0.00051f;
    p182_w3 = 0.00055f + p182_rf() * 0.00038f;
    p182_eps = 0.052f + p182_rf() * 0.030f;
    if (!p182_tabs) {
        for (i = 0; i < 2048; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (5.6f / 2048.0f)));
            p182_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p182_tabs = 1;
    }
    p182_ready = 1;
    p182_seedc = seed;
}

static void p182_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 1.0f || y < 1.0f || xi >= CW - 2 || yi >= CH - 2) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p182_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * wgt; w1 = fx * (1.0f - fy) * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * wgt; w1 = fx * fy * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p182_blob(float x, float y, const float *c, float wgt)
{
    p182_splat(x, y, c, wgt);
    p182_splat(x - 1.15f, y, c, wgt * 0.46f);
    p182_splat(x + 1.15f, y, c, wgt * 0.46f);
    p182_splat(x, y - 1.15f, c, wgt * 0.46f);
    p182_splat(x, y + 1.15f, c, wgt * 0.46f);
}

static void p182_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p182_xmw != w) {
        free(p182_xm);
        p182_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p182_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p182_xmw = w;
    }
    jd_up_blit(&p182_up, fb, w, h, p182_img, CW, CH);
}

void pattern_182(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, a11, a12, a21, a22, cs, sn, cx, cy, sc, e, step;
    float col[64][3], amp[NIT];
    int i, k, o;
    (void)sl;
    if (!p182_ready || p182_seedc != seed) p182_setup(seed);

    for (k = 0; k < 64; k++)
        p182_pal3(pal, p182_hue0 + p182_huew * ((float)k * (1.0f / 64.0f)),
                  0.88f, col[k]);
    for (k = 0; k < NIT; k++)
        amp[k] = (k == 0) ? 1.0f : 1.0f / (1.0f + 0.55f * (float)k);
    e = p182_eps;
    a11 = e * sinf(t * p182_w0);
    a12 = e * sinf(t * p182_w1 + 1.9f) + e * 0.9f * cosf(t * p182_w3);
    a21 = -e * 0.9f * cosf(t * p182_w3) + e * sinf(t * p182_w2 + 4.1f);
    a22 = e * sinf(t * p182_w0 + 3.3f) * 0.7f;
    step = 0.048f + 0.011f * sinf(t * 0.00067f);
    cs = cosf(t * p182_spin); sn = sinf(t * p182_spin);
    cx = CW * 0.5f + 26.0f * sinf(t * 0.00043f);
    cy = CH * 0.5f + 18.0f * cosf(t * 0.00037f);
    sc = 152.0f + 9.0f * sinf(t * 0.00051f);

    memset(p182_acc, 0, sizeof p182_acc);
    for (i = 0; i < NDOT; i++) {
        float bx = p182_dx[i], by = p182_dy[i];
        float px = bx * cs - by * sn, py = bx * sn + by * cs;
        float br = p182_dr[i];
        float hf = p182_dh[i] + t * 0.00021f;
        const float *cc = col[(int)((hf - floorf(hf)) * 63.99f)];
        for (k = 0; k < NIT; k++) {
            float vx, vy, l, fade;
            fade = 1.0f - 0.28f * (px * px + py * py);
            if (fade < 0.10f) fade = 0.10f;
            p182_blob(cx + px * sc, cy + py * sc, cc, br * amp[k] * fade);
            vx = a11 * px + a12 * py;
            vy = a21 * px + a22 * py;
            l = vx * vx + vy * vy;
            if (l > 1e-9f) {
                l = step / sqrtf(l);
                px += vx * l; py += vy * l;
            }
        }
    }
    for (o = 0; o < CW * CH * 3; o++) {
        int v = (int)(p182_acc[o] * 1450.0f);
        p182_img[o] = p182_tone[v < 0 ? 0 : v > 2047 ? 2047 : v];
    }
    p182_blit(fb, w, h);
}
