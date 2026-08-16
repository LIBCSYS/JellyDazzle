/* 179 Harmonic Lantern — a spherical-harmonic surface drawn as glowing wire.
 * The radius of the surface is r(theta,phi) = sin(a*theta)^p + cos(b*theta)^q +
 * sin(c*phi)^u + cos(d*phi)^v with even exponents, which is separable: the
 * theta half and the phi half are each evaluated once per row and once per
 * column instead of once per vertex, so a 44x88 lattice costs 132 power
 * evaluations rather than 15000. The lobes it produces are the classic
 * spherical-harmonic flowers — three-pointed, six-pointed, spiky, quilted —
 * and the eight-integer parameter word is crossfaded between presets by
 * interpolating the VERTICES, so one lantern melts into the next with no
 * intermediate garbage. Latitude rings and meridians only: additive wire that
 * lets whatever is beneath it show through the shell. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p179_up;

#define P179_W 480
#define P179_H 360
#define P179_NT 44
#define P179_NP 88
#define P179_NSET 4
#define P179_TAU 6.28318530717958647692f

static float p179_acc[P179_W * P179_H * 3];
static unsigned char p179_img[P179_W * P179_H * 3];
static unsigned char p179_tone[1024];
static int *p179_xm;
static int p179_xmw;
static int p179_ready;
static uint32_t p179_seedc;
static float p179_col[64][3];
static float p179_hue0, p179_huew, p179_dir;
static float p179_prm[P179_NSET][8];
static float p179_px[P179_NT][P179_NP], p179_py[P179_NT][P179_NP];
static float p179_pz[P179_NT][P179_NP], p179_pr[P179_NT][P179_NP];
static float p179_at[2][P179_NT], p179_bp[2][P179_NP];

static uint32_t p179_rs;
static float p179_rf(void)
{
    p179_rs ^= p179_rs << 13; p179_rs ^= p179_rs >> 17; p179_rs ^= p179_rs << 5;
    return (float)(p179_rs >> 8) * (1.0f / 16777216.0f);
}

static void p179_setup(uint32_t seed)
{
    int i, j;
    p179_rs = seed ? seed ^ 0x1A2B3C79u : 0x1A2B3C79u;
    p179_rf(); p179_rf();
    p179_hue0 = p179_rf();
    p179_huew = 0.16f + p179_rf() * 0.62f;
    p179_dir  = p179_rf() < 0.5f ? -1.0f : 1.0f;
    for (i = 0; i < P179_NSET; i++) {
        for (j = 0; j < 4; j++)
            p179_prm[i][j * 2] = (float)(1 + (int)(p179_rf() * 6.0f));   /* freq */
        for (j = 0; j < 4; j++)
            p179_prm[i][j * 2 + 1] = (float)(2 * (1 + (int)(p179_rf() * 3.0f)));
    }
    if (!p179_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.2f / 1024.0f)));
            p179_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p179_ready = 1;
    }
    p179_seedc = seed;
}

static void p179_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p179_hue0 + p179_huew * ((float)i / 63.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p179_col[i][0] = 0.12f + 0.88f * r / mx;
        p179_col[i][1] = 0.12f + 0.88f * g / mx;
        p179_col[i][2] = 0.12f + 0.88f * b / mx;
    }
}

static float p179_ipow(float x, int n)
{
    float r = 1.0f;
    while (n) { if (n & 1) r *= x; x *= x; n >>= 1; }
    return r;
}

static void p179_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= P179_W - 1 || yi >= P179_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p179_acc + (yi * P179_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P179_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p179_seg(float x0, float y0, float x1, float y1,
                     const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    if (len > 300.0f) return;
    n = (int)(len * 1.25f) + 1;
    dx /= (float)n; dy /= (float)n;
    w *= (len / (float)n + 0.3f);
    for (i = 0; i < n; i++)
        p179_splat(x0 + dx * (float)i, y0 + dy * (float)i, c, w);
}

static void p179_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P179_W * P179_H * 3; i++) {
        int ti = (int)(p179_acc[i] * 256.0f);
        p179_img[i] = p179_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p179_xmw != w) {
        free(p179_xm);
        p179_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p179_xm[x] = (int)(((long long)x * (P179_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p179_xmw = w;
    }
    jd_up_blit(&p179_up, fb, w, h, p179_img, P179_W, P179_H);
}

void pattern_179(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float m, ca, sa, cb, sb, sc, D, cx, cy, rmax;
    int i, j, k, s0, s1;
    (void)sl;
    if (!p179_ready || p179_seedc != seed) p179_setup(seed);
    p179_hues(pal);
    memset(p179_acc, 0, sizeof p179_acc);

    {
        float leg = t * (1.0f / 760.0f);
        int li = (int)leg;
        float f = leg - (float)li;
        m = f < 0.58f ? 0.0f : (f - 0.58f) / 0.42f;
        m = m * m * (3.0f - 2.0f * m);
        s0 = li % P179_NSET; s1 = (li + 1) % P179_NSET;
    }
    for (k = 0; k < 2; k++) {
        const float *P = p179_prm[k ? s1 : s0];
        for (i = 0; i < P179_NT; i++) {
            float th = (float)M_PI * ((float)i + 0.5f) / (float)P179_NT;
            p179_at[k][i] = p179_ipow(sinf(P[0] * th), (int)P[1])
                          + p179_ipow(cosf(P[2] * th), (int)P[3]);
        }
        for (j = 0; j < P179_NP; j++) {
            float ph = P179_TAU * (float)j / (float)P179_NP;
            p179_bp[k][j] = p179_ipow(sinf(P[4] * ph), (int)P[5])
                          + p179_ipow(cosf(P[6] * ph), (int)P[7]);
        }
    }
    ca = cosf(0.42f + 0.26f * sinf(t * 0.00043f));
    sa = sinf(0.42f + 0.26f * sinf(t * 0.00043f));
    cb = cosf(t * 0.00105f * p179_dir);
    sb = sinf(t * 0.00105f * p179_dir);
    sc = (float)P179_H * 0.335f;
    D  = 3.6f;
    cx = P179_W * 0.5f; cy = P179_H * 0.5f;
    rmax = 0.0f;

    for (i = 0; i < P179_NT; i++) {
        float th = (float)M_PI * ((float)i + 0.5f) / (float)P179_NT;
        float st = sinf(th), ct = cosf(th);
        for (j = 0; j < P179_NP; j++) {
            float ph = P179_TAU * (float)j / (float)P179_NP;
            float r0 = 0.30f + p179_at[0][i] + p179_bp[0][j];
            float r1 = 0.30f + p179_at[1][i] + p179_bp[1][j];
            float r = r0 + (r1 - r0) * m;
            float X = r * st * cosf(ph), Y = r * ct, Z = r * st * sinf(ph);
            float x2, y2, z2;
            x2 = X * cb + Z * sb; z2 = -X * sb + Z * cb;
            y2 = Y * ca - z2 * sa; z2 = Y * sa + z2 * ca;
            p179_px[i][j] = x2; p179_py[i][j] = y2; p179_pz[i][j] = z2;
            p179_pr[i][j] = r;
            if (r > rmax) rmax = r;
        }
    }
    if (rmax < 0.4f) rmax = 0.4f;
    {   /* normalise the lobe to a unit ball, then project it to fill the frame */
        float kn = 1.0f / rmax;
        for (i = 0; i < P179_NT; i++)
            for (j = 0; j < P179_NP; j++) {
                float zn = p179_pz[i][j] * kn;
                float den = D - zn, q;
                if (den < 0.7f) den = 0.7f;
                q = sc * (D / den);
                p179_px[i][j] = cx + p179_px[i][j] * kn * q;
                p179_py[i][j] = cy + p179_py[i][j] * kn * q;
            }
    }
    for (i = 0; i < P179_NT; i++) {
        for (j = 0; j < P179_NP; j++) {
            int j2 = (j + 1) % P179_NP;
            float dep = 0.45f + 0.55f * (p179_pz[i][j] / (rmax + 0.001f) + 1.0f) * 0.5f;
            const float *c = p179_col[(int)(p179_pr[i][j] / rmax * 58.0f) & 63];
            float wgt = 0.30f * dep;
            p179_seg(p179_px[i][j], p179_py[i][j],
                     p179_px[i][j2], p179_py[i][j2], c, wgt);
            if (i + 1 < P179_NT)
                p179_seg(p179_px[i][j], p179_py[i][j],
                         p179_px[i + 1][j], p179_py[i + 1][j], c, wgt * 0.72f);
        }
    }
    p179_blit(fb, w, h);
}
