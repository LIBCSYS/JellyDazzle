/* 194 Harmonic Shell — a wire sphere breathing in spherical-harmonic modes.
 * The radius of the mesh is r(theta,phi) = 1 + a*(w1*Y1 + w2*Y2), where each Y
 * is a real spherical harmonic P_l^m(cos theta) cos(m phi) evaluated by the
 * standard Legendre recurrence. Because P depends only on latitude and cos(m
 * phi) only on longitude, the whole 72x40 mesh is one outer product per mode,
 * which is why a full harmonic solid costs less than a plasma. The two modes
 * cross-fade; whenever one reaches zero weight it is quietly swapped for a new
 * (l,m), so the solid never repeats its shape. Drawn as latitude and longitude
 * wire only, additive, so the far side glows faintly through the near side. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p194_up;

#define P194_W 480
#define P194_H 360
#define P194_TAU 6.28318530717958647692f

static float p194_acc[P194_W * P194_H * 3];
static unsigned char p194_img[P194_W * P194_H * 3];
static unsigned char p194_tone[1024];
static int *p194_xm;
static int p194_xmw;
static int p194_tone_ok;
static uint32_t p194_rs = 1u;

static float p194_rf(void)
{
    p194_rs ^= p194_rs << 13; p194_rs ^= p194_rs >> 17; p194_rs ^= p194_rs << 5;
    return (float)(p194_rs >> 8) * (1.0f / 16777216.0f);
}

static void p194_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (7.50f / 1024.0f)));
        p194_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p194_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p194_col(const uint32_t *pal, float hue, float lift, float *out)
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


static void p194_splat(float x, float y, const float *c, float w)
{
    int xi, yi; float fx, fy, w0, w1; float *p;
    if (!(x >= 0.0f) || !(y >= 0.0f)) return;
    xi = (int)x; yi = (int)y;
    if (xi >= P194_W - 1 || yi >= P194_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p194_acc + (yi * P194_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P194_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}


/* energy-conserving line: total deposit is w * length, so brightness does not
 * depend on how finely a curve happens to be subdivided. */
static void p194_line(float x0, float y0, float x1, float y1, const float *c, float w)
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
        p194_splat(x0 + dx * t, y0 + dy * t, c, wq);
    }
}


static float p194_tmp[P194_W * P194_H * 3];

/* 5-tap soft glow, in place. Keeps line art from aliasing when it is scaled
 * up to 1280x960 and keeps frame-to-frame motion visually continuous. */
static void p194_blur(void)
{
    int y, x, c;
    for (y = 1; y < P194_H - 1; y++)
        for (x = 1; x < P194_W - 1; x++) {
            int o = (y * P194_W + x) * 3;
            for (c = 0; c < 3; c++)
                p194_tmp[o + c] = p194_acc[o + c] * 0.52f
                    + 0.12f * (p194_acc[o + c - 3] + p194_acc[o + c + 3]
                             + p194_acc[o + c - P194_W * 3] + p194_acc[o + c + P194_W * 3]);
        }
    for (y = 1; y < P194_H - 1; y++)
        memcpy(p194_acc + (y * P194_W + 1) * 3, p194_tmp + (y * P194_W + 1) * 3,
               sizeof(float) * 3 * (P194_W - 2));
}

static void p194_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P194_W * P194_H * 3; i++) {
        int ti = (int)(p194_acc[i] * 256.0f);
        p194_img[i] = p194_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p194_xmw != w) {
        free(p194_xm);
        p194_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p194_xm[x] = (int)(((long long)x * (P194_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p194_xmw = w;
    }
    jd_up_blit(&p194_up, fb, w, h, p194_img, P194_W, P194_H);
}

#define P194_NU 72
#define P194_NV 40
#define P194_NM 14

static const short p194_lm[P194_NM][2] = {
    {2,0},{2,1},{3,1},{3,2},{4,0},{4,2},{4,3},{5,1},{5,3},{5,4},{6,2},{6,4},{6,5},{7,6}
};
static uint32_t p194_seedc = 0xFFFFFFFFu;
static float p194_h0, p194_hw;
static int p194_m1, p194_m2, p194_swap;
static float p194_pl[2][P194_NV], p194_cm[2][P194_NU];
static float p194_hue[24][3];
static float p194_ct[P194_NU], p194_stt[P194_NU];
static float p194_vx[P194_NV][P194_NU], p194_vy[P194_NV][P194_NU], p194_vz[P194_NV][P194_NU];
static float p194_vw[P194_NV][P194_NU];
static short p194_vh[P194_NV][P194_NU];

static float p194_plm(int l, int m, float x)
{
    float pmm = 1.0f, pmp, pll = 0.0f;
    int i, ll;
    if (m > 0) {
        float somx2 = sqrtf((1.0f - x) * (1.0f + x)), fact = 1.0f;
        for (i = 1; i <= m; i++) { pmm *= -fact * somx2; fact += 2.0f; }
    }
    if (l == m) return pmm;
    pmp = x * (2.0f * (float)m + 1.0f) * pmm;
    if (l == m + 1) return pmp;
    for (ll = m + 2; ll <= l; ll++) {
        pll = ((2.0f * (float)ll - 1.0f) * x * pmp - ((float)(ll + m) - 1.0f) * pmm)
              / (float)(ll - m);
        pmm = pmp; pmp = pll;
    }
    return pll;
}

static void p194_mode(int slot, int idx)
{
    int l = p194_lm[idx][0], m = p194_lm[idx][1], i;
    float mx = 1e-4f;
    for (i = 0; i < P194_NV; i++) {
        float th = ((float)i + 0.5f) * (3.14159265f / (float)P194_NV);
        p194_pl[slot][i] = p194_plm(l, m, cosf(th));
        if (fabsf(p194_pl[slot][i]) > mx) mx = fabsf(p194_pl[slot][i]);
    }
    for (i = 0; i < P194_NV; i++) p194_pl[slot][i] /= mx;
    for (i = 0; i < P194_NU; i++)
        p194_cm[slot][i] = cosf((float)m * (float)i * (P194_TAU / (float)P194_NU));
}

static void p194_build(uint32_t seed)
{
    int i;
    p194_rs = seed ? seed * 3266489917u + 0x27D4EB2Fu : 0x194u;
    p194_rf(); p194_rf();
    p194_h0 = p194_rf();
    p194_hw = 0.07f + p194_rf() * 0.50f;
    p194_m1 = (int)(p194_rf() * P194_NM) % P194_NM;
    p194_m2 = (int)(p194_rf() * P194_NM) % P194_NM;
    p194_mode(0, p194_m1); p194_mode(1, p194_m2);
    p194_swap = 0;
    for (i = 0; i < P194_NU; i++) {
        float a = (float)i * (P194_TAU / (float)P194_NU);
        p194_ct[i] = cosf(a); p194_stt[i] = sinf(a);
    }
    p194_seedc = seed;
    if (!p194_tone_ok) p194_tone_init();
}

void pattern_194(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, w1, w2, amp, cyw, syw, cp, sp, sc, ox, oy;
    int u, v, i;
    (void)sl;
    if (p194_seedc != seed) p194_build(seed);
    for (i = 0; i < 24; i++)
        p194_col(pal, p194_h0 + p194_hw * ((float)i / 23.0f), 0.14f, p194_hue[i]);
    memset(p194_acc, 0, sizeof p194_acc);

    w1 = 0.5f + 0.5f * cosf(t * 0.00036f);
    w2 = 1.0f - w1;
    if (w2 < 0.02f && p194_swap == 0) {
        p194_m2 = (p194_m2 + 1 + (int)(p194_rf() * 5.0f)) % P194_NM;
        p194_mode(1, p194_m2); p194_swap = 1;
    } else if (w1 < 0.02f && p194_swap == 1) {
        p194_m1 = (p194_m1 + 1 + (int)(p194_rf() * 5.0f)) % P194_NM;
        p194_mode(0, p194_m1); p194_swap = 0;
    }
    amp = 0.30f + 0.13f * sinf(t * 0.00051f);
    {   float yaw = t * 0.00088f; cyw = cosf(yaw); syw = sinf(yaw); }
    {   float pit = 0.30f * sinf(t * 0.00033f); cp = cosf(pit); sp = sinf(pit); }
    sc = (float)P194_H * 0.335f;
    ox = P194_W * 0.5f; oy = P194_H * 0.5f;

    for (v = 0; v < P194_NV; v++) {
        float th = ((float)v + 0.5f) * (3.14159265f / (float)P194_NV);
        float stv = sinf(th), ctv = cosf(th);
        for (u = 0; u < P194_NU; u++) {
            float d = w1 * p194_pl[0][v] * p194_cm[0][u]
                    + w2 * p194_pl[1][v] * p194_cm[1][u];
            float r = 1.0f + amp * d;
            float X = r * stv * p194_ct[u], Y = r * ctv, Z = r * stv * p194_stt[u];
            float rx = X * cyw + Z * syw, rz = -X * syw + Z * cyw, ry;
            float pz, f;
            ry = Y * cp - rz * sp; rz = Y * sp + rz * cp;
            pz = 3.7f + rz;
            if (pz < 0.5f) pz = 0.5f;
            f = 3.4f / pz;
            p194_vx[v][u] = ox + rx * sc * f;
            p194_vy[v][u] = oy + ry * sc * f;
            p194_vz[v][u] = rz;
            p194_vw[v][u] = (0.34f + 0.66f * (0.5f - 0.5f * rz)) * f;
            i = (int)((d * 0.5f + 0.5f) * 23.0f);
            p194_vh[v][u] = (short)(i < 0 ? 0 : i > 23 ? 23 : i);
        }
    }
    for (v = 0; v < P194_NV; v++)
        for (u = 0; u < P194_NU; u++) {
            int u1 = (u + 1) % P194_NU;
            float wg = 0.19f * (p194_vw[v][u] + p194_vw[v][u1]);
            p194_line(p194_vx[v][u], p194_vy[v][u], p194_vx[v][u1], p194_vy[v][u1],
                      p194_hue[p194_vh[v][u]], wg);
            if (v + 1 < P194_NV) {
                wg = 0.13f * (p194_vw[v][u] + p194_vw[v + 1][u]);
                p194_line(p194_vx[v][u], p194_vy[v][u], p194_vx[v + 1][u], p194_vy[v + 1][u],
                          p194_hue[p194_vh[v][u]], wg);
            }
        }
    p194_blur();
    p194_blit(fb, w, h);
}
