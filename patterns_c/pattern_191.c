/* 191 Hopf Weave — the Hopf fibration, drawn as the linked circles it is.
 * Every point b on the 2-sphere owns a great circle in the 3-sphere; project
 * that circle stereographically into R3 and you get a Villarceau circle, and
 * any two of them are linked exactly once. A slowly tilting small circle of
 * base points on S2 is lifted fibre by fibre, so the screen carries a nest of
 * interlocked rings lying on a common torus that swells and thins as the base
 * latitude breathes. Rings only, black between them; a brightness wave runs
 * around each fibre so the weave keeps moving without anything jumping. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p191_up;

#define P191_W 480
#define P191_H 360
#define P191_TAU 6.28318530717958647692f

static float p191_acc[P191_W * P191_H * 3];
static unsigned char p191_img[P191_W * P191_H * 3];
static unsigned char p191_tone[1024];
static int *p191_xm;
static int p191_xmw;
static int p191_tone_ok;
static uint32_t p191_rs = 1u;

static float p191_rf(void)
{
    p191_rs ^= p191_rs << 13; p191_rs ^= p191_rs >> 17; p191_rs ^= p191_rs << 5;
    return (float)(p191_rs >> 8) * (1.0f / 16777216.0f);
}

static void p191_tone_init(void)
{
    int i;
    for (i = 0; i < 1024; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (7.50f / 1024.0f)));
        p191_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
    }
    p191_tone_ok = 1;
}

/* palette sample, brightness-normalised so dark ramp zones still read as light */
static void p191_col(const uint32_t *pal, float hue, float lift, float *out)
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


static void p191_splat(float x, float y, const float *c, float w)
{
    int xi, yi; float fx, fy, w0, w1; float *p;
    if (!(x >= 0.0f) || !(y >= 0.0f)) return;
    xi = (int)x; yi = (int)y;
    if (xi >= P191_W - 1 || yi >= P191_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p191_acc + (yi * P191_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P191_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}


/* energy-conserving line: total deposit is w * length, so brightness does not
 * depend on how finely a curve happens to be subdivided. */
static void p191_line(float x0, float y0, float x1, float y1, const float *c, float w)
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
        p191_splat(x0 + dx * t, y0 + dy * t, c, wq);
    }
}


static float p191_tmp[P191_W * P191_H * 3];

/* 5-tap soft glow, in place. Keeps line art from aliasing when it is scaled
 * up to 1280x960 and keeps frame-to-frame motion visually continuous. */
static void p191_blur(void)
{
    int y, x, c;
    for (y = 1; y < P191_H - 1; y++)
        for (x = 1; x < P191_W - 1; x++) {
            int o = (y * P191_W + x) * 3;
            for (c = 0; c < 3; c++)
                p191_tmp[o + c] = p191_acc[o + c] * 0.52f
                    + 0.12f * (p191_acc[o + c - 3] + p191_acc[o + c + 3]
                             + p191_acc[o + c - P191_W * 3] + p191_acc[o + c + P191_W * 3]);
        }
    for (y = 1; y < P191_H - 1; y++)
        memcpy(p191_acc + (y * P191_W + 1) * 3, p191_tmp + (y * P191_W + 1) * 3,
               sizeof(float) * 3 * (P191_W - 2));
}

static void p191_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P191_W * P191_H * 3; i++) {
        int ti = (int)(p191_acc[i] * 256.0f);
        p191_img[i] = p191_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p191_xmw != w) {
        free(p191_xm);
        p191_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p191_xm[x] = (int)(((long long)x * (P191_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p191_xmw = w;
    }
    jd_up_blit(&p191_up, fb, w, h, p191_img, P191_W, P191_H);
}

#define P191_NF 30
#define P191_NP 132

static uint32_t p191_seedc = 0xFFFFFFFFu;
static float p191_h0, p191_hw, p191_lat0, p191_latA, p191_spin, p191_tilt;
static float p191_fh[P191_NF][3];

static void p191_build(uint32_t seed)
{
    p191_rs = seed ? seed * 2654435761u + 0x9E3779B9u : 0x191u;
    p191_rf(); p191_rf();
    p191_h0   = p191_rf();
    p191_hw   = 0.06f + p191_rf() * 0.55f;
    p191_lat0 = 0.90f + p191_rf() * 0.50f;
    p191_latA = 0.20f + p191_rf() * 0.28f;
    p191_spin = (p191_rf() < 0.5f ? -1.0f : 1.0f) * (0.0013f + p191_rf() * 0.0015f);
    p191_tilt = 0.22f + p191_rf() * 0.38f;
    p191_seedc = seed;
    if (!p191_tone_ok) p191_tone_init();
}

void pattern_191(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float lat, ca, sa, cyw, syw, cp, sp, sc, ox, oy, sl0;
    int k, j;
    (void)sl;
    if (p191_seedc != seed) p191_build(seed);
    for (k = 0; k < P191_NF; k++)
        p191_col(pal, p191_h0 + p191_hw * ((float)k / (float)P191_NF), 0.10f, p191_fh[k]);
    memset(p191_acc, 0, sizeof p191_acc);

    lat = p191_lat0 + p191_latA * sinf(t * 0.00037f);
    sl0 = sinf(lat);
    {   float ax = p191_tilt * sinf(t * 0.00029f);
        ca = cosf(ax); sa = sinf(ax); }
    {   float yaw = t * 0.00058f;  cyw = cosf(yaw); syw = sinf(yaw); }
    {   float pit = 0.44f + 0.26f * sinf(t * 0.00023f); cp = cosf(pit); sp = sinf(pit); }
    sc = (float)P191_H * 0.235f;
    ox = P191_W * 0.5f; oy = P191_H * 0.5f;

    for (k = 0; k < P191_NF; k++) {
        float psi = (float)k * (P191_TAU / (float)P191_NF) + t * p191_spin;
        float b1 = sl0 * cosf(psi), b2 = sl0 * sinf(psi), b3 = cosf(lat);
        float ty = b2 * ca - b3 * sa, tz = b2 * sa + b3 * ca;
        float den, inv, opz, px = 0.0f, py = 0.0f;
        int have = 0;
        b2 = ty; b3 = tz;
        opz = 1.0f + b3;
        if (opz < 0.09f) continue;
        den = sqrtf(2.0f * opz);
        inv = 1.0f / den;
        for (j = 0; j <= P191_NP; j++) {
            float u = (float)j * (P191_TAU / (float)P191_NP);
            float cu = cosf(u), su = sinf(u);
            float q1 = opz * cu * inv;
            float q2 = (b1 * su - b2 * cu) * inv;
            float q3 = (b1 * cu + b2 * su) * inv;
            float q4 = opz * su * inv;
            float wd = 1.0f - q4;
            float X, Y, Z, rx, ry, rz, pz, f, sx, sy2;
            if (wd < 0.13f) { have = 0; continue; }
            X = q1 / wd; Y = q2 / wd; Z = q3 / wd;
            if (X * X + Y * Y + Z * Z > 42.0f) { have = 0; continue; }
            rx = X * cyw + Z * syw;
            rz = -X * syw + Z * cyw;
            ry = Y * cp - rz * sp;
            rz = Y * sp + rz * cp;
            pz = 6.2f + rz;
            if (pz < 0.9f) { have = 0; continue; }
            f = 5.6f / pz;
            sx = ox + rx * sc * f; sy2 = oy + ry * sc * f;
            if (have) {
                float wg = 0.42f * f * (0.42f + 0.58f *
                    (0.5f + 0.5f * sinf(u * 2.0f - t * 0.017f + (float)k * 0.7f)));
                p191_line(px, py, sx, sy2, p191_fh[k], wg);
            }
            px = sx; py = sy2; have = 1;
        }
    }
    p191_blur();
    p191_blit(fb, w, h);
}
