/* 200 Lorenz Ribbon — the butterfly, drawn as a ribbon of finite width.
 * A real RK4 integration of the Lorenz system (sigma 10, rho 28, beta 8/3)
 * runs continuously into a ring buffer of ten thousand states; each frame it
 * advances only eight steps, so the curve slides forward instead of redrawing.
 * At every sample the tangent is crossed with a fixed axis to get a ribbon
 * normal, and the two edges plus a rung are drawn in 3D, which makes the sheet
 * twist and go edge-on exactly where the trajectory rolls over — the twist is
 * the geometry, not an effect. Hue follows height, brightness follows age, so
 * the leading edge glows and the tail sinks into black. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p200_up;

#define P200_W 480
#define P200_H 360
#define P200_TAU 6.28318530717958647692f

static float p200_acc[P200_W * P200_H * 3];
static unsigned char p200_img[P200_W * P200_H * 3];
static unsigned char p200_tone[1024];
static int *p200_xm;
static int p200_xmw;
static int p200_tone_ok;
static uint32_t p200_rs = 1u;

static float p200_rf(void)
{
    p200_rs ^= p200_rs << 13; p200_rs ^= p200_rs >> 17; p200_rs ^= p200_rs << 5;
    return (float)(p200_rs >> 8) * (1.0f / 16777216.0f);
}

static void p200_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (7.50f / 1024.0f)));
        p200_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p200_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p200_col(const uint32_t *pal, float hue, float lift, float *out)
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


static void p200_splat(float x, float y, const float *c, float w)
{
    int xi, yi; float fx, fy, w0, w1; float *p;
    if (!(x >= 0.0f) || !(y >= 0.0f)) return;
    xi = (int)x; yi = (int)y;
    if (xi >= P200_W - 1 || yi >= P200_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p200_acc + (yi * P200_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P200_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}


/* energy-conserving line: total deposit is w * length, so brightness does not
 * depend on how finely a curve happens to be subdivided. */
static void p200_line(float x0, float y0, float x1, float y1, const float *c, float w)
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
        p200_splat(x0 + dx * t, y0 + dy * t, c, wq);
    }
}


static float p200_tmp[P200_W * P200_H * 3];

/* 5-tap soft glow, in place. Keeps line art from aliasing when it is scaled
 * up to 1280x960 and keeps frame-to-frame motion visually continuous. */
static void p200_blur(void)
{
    int y, x, c;
    for (y = 1; y < P200_H - 1; y++)
        for (x = 1; x < P200_W - 1; x++) {
            int o = (y * P200_W + x) * 3;
            for (c = 0; c < 3; c++)
                p200_tmp[o + c] = p200_acc[o + c] * 0.52f
                    + 0.12f * (p200_acc[o + c - 3] + p200_acc[o + c + 3]
                             + p200_acc[o + c - P200_W * 3] + p200_acc[o + c + P200_W * 3]);
        }
    for (y = 1; y < P200_H - 1; y++)
        memcpy(p200_acc + (y * P200_W + 1) * 3, p200_tmp + (y * P200_W + 1) * 3,
               sizeof(float) * 3 * (P200_W - 2));
}

static void p200_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P200_W * P200_H * 3; i++) {
        int ti = (int)(p200_acc[i] * 256.0f);
        p200_img[i] = p200_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p200_xmw != w) {
        free(p200_xm);
        p200_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p200_xm[x] = (int)(((long long)x * (P200_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p200_xmw = w;
    }
    jd_up_blit(&p200_up, fb, w, h, p200_img, P200_W, P200_H);
}

#define P200_N 10000
#define P200_ADV 8
#define P200_DT 0.0042f

static uint32_t p200_seedc = 0xFFFFFFFFu;
static float p200_h0, p200_hw, p200_wid;
static float p200_px[P200_N], p200_py[P200_N], p200_pz[P200_N];
static int p200_head, p200_ready;
static float p200_x, p200_y, p200_z;
static float p200_hue[24][3];

static void p200_step(void)
{
    float x = p200_x, y = p200_y, z = p200_z, dt = P200_DT;
    float k1x, k1y, k1z, k2x, k2y, k2z, k3x, k3y, k3z, k4x, k4y, k4z;
    float ax, ay, az;
    k1x = 10.0f * (y - x);          k1y = x * (28.0f - z) - y;  k1z = x * y - 2.6666667f * z;
    ax = x + 0.5f * dt * k1x; ay = y + 0.5f * dt * k1y; az = z + 0.5f * dt * k1z;
    k2x = 10.0f * (ay - ax);        k2y = ax * (28.0f - az) - ay; k2z = ax * ay - 2.6666667f * az;
    ax = x + 0.5f * dt * k2x; ay = y + 0.5f * dt * k2y; az = z + 0.5f * dt * k2z;
    k3x = 10.0f * (ay - ax);        k3y = ax * (28.0f - az) - ay; k3z = ax * ay - 2.6666667f * az;
    ax = x + dt * k3x; ay = y + dt * k3y; az = z + dt * k3z;
    k4x = 10.0f * (ay - ax);        k4y = ax * (28.0f - az) - ay; k4z = ax * ay - 2.6666667f * az;
    p200_x = x + dt * (k1x + 2.0f * k2x + 2.0f * k3x + k4x) * (1.0f / 6.0f);
    p200_y = y + dt * (k1y + 2.0f * k2y + 2.0f * k3y + k4y) * (1.0f / 6.0f);
    p200_z = z + dt * (k1z + 2.0f * k2z + 2.0f * k3z + k4z) * (1.0f / 6.0f);
    p200_px[p200_head] = p200_x * 0.075f;
    p200_py[p200_head] = (p200_z - 25.0f) * 0.075f;
    p200_pz[p200_head] = p200_y * 0.075f;
    p200_head = (p200_head + 1) % P200_N;
}

static void p200_build(uint32_t seed)
{
    int i;
    p200_rs = seed ? seed * 2891336453u + 0x9E3779B9u : 0x200u;
    p200_rf(); p200_rf();
    p200_h0  = p200_rf();
    p200_hw  = 0.06f + p200_rf() * 0.52f;
    p200_wid = 0.055f + p200_rf() * 0.055f;
    if (!p200_ready) {
        p200_x = 0.9f; p200_y = 1.7f; p200_z = 21.0f;
        for (i = 0; i < 900; i++) p200_step();          /* onto the attractor */
        p200_head = 0;
        for (i = 0; i < P200_N; i++) p200_step();       /* prefill the ribbon */
        p200_ready = 1;
    }
    p200_seedc = seed;
    if (!p200_tone_ok) p200_tone_init();
}

void pattern_200(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, cyw, syw, cp, sp, sc, ox, oy, wid;
    int i, k;
    (void)sl;
    if (p200_seedc != seed) p200_build(seed);
    for (i = 0; i < 24; i++)
        p200_col(pal, p200_h0 + p200_hw * ((float)i / 23.0f), 0.10f, p200_hue[i]);
    for (i = 0; i < P200_ADV; i++) p200_step();
    memset(p200_acc, 0, sizeof p200_acc);

    {   float yaw = t * 0.00068f; cyw = cosf(yaw); syw = sinf(yaw); }
    {   float pit = 0.24f + 0.20f * sinf(t * 0.00029f); cp = cosf(pit); sp = sinf(pit); }
    sc  = (float)P200_H * 0.27f;
    ox  = P200_W * 0.5f; oy = P200_H * 0.5f;
    wid = p200_wid * (1.0f + 0.18f * sinf(t * 0.00043f));

    {
        float ex = 0.0f, ey = 0.0f, hx = 0.0f, hy = 0.0f;
        int have = 0;
        for (k = 2; k < P200_N - 2; k += 2) {
            int idx = (p200_head + k) % P200_N;
            int ia = (p200_head + k - 2) % P200_N, ib = (p200_head + k + 2) % P200_N;
            float X = p200_px[idx], Y = p200_py[idx], Z = p200_pz[idx];
            float tx = p200_px[ib] - p200_px[ia];
            float ty = p200_py[ib] - p200_py[ia];
            float tz = p200_pz[ib] - p200_pz[ia];
            float nx = ty * 1.0f - tz * 0.0f, ny = tz * 0.0f - tx * 1.0f, nz = 0.0f;
            float nl, age, wg;
            const float *col;
            float ax[2], ay[2];
            int e;
            nx = ty; ny = -tx; nz = 0.0f;                 /* t x (0,0,1) */
            nl = sqrtf(nx * nx + ny * ny + nz * nz);
            if (nl < 1e-5f) { have = 0; continue; }
            nx *= wid / nl; ny *= wid / nl;
            nz = tz * 0.0f;
            age = (float)k / (float)P200_N;
            wg = 0.10f + 0.72f * age * age;
            i = (int)((Y * 1.5f + 0.5f) * 23.0f);
            col = p200_hue[i < 0 ? 0 : i > 23 ? 23 : i];
            for (e = 0; e < 2; e++) {
                float s = e ? 1.0f : -1.0f;
                float px = X + nx * s, py = Y + ny * s, pz3 = Z;
                float rx = px * cyw + pz3 * syw, rz = -px * syw + pz3 * cyw, ry;
                float pz, f;
                ry = py * cp - rz * sp; rz = py * sp + rz * cp;
                pz = 3.4f + rz; if (pz < 0.5f) pz = 0.5f;
                f = 3.1f / pz;
                ax[e] = ox + rx * sc * f; ay[e] = oy + ry * sc * f;
            }
            if (have) {
                p200_line(ex, ey, ax[0], ay[0], col, wg * 0.40f);
                p200_line(hx, hy, ax[1], ay[1], col, wg * 0.40f);
            }
            p200_line(ax[0], ay[0], ax[1], ay[1], col, wg * 0.26f);
            ex = ax[0]; ey = ay[0]; hx = ax[1]; hy = ay[1]; have = 1;
        }
    }
    p200_blur();
    p200_blit(fb, w, h);
}
