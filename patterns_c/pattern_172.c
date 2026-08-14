/* 172 Villarceau Weave — the fourth family of circles on a torus.
 * Slice a torus with a plane that touches it twice and the section is not an
 * oval but two perfect circles, tilted out of the equator by asin(r/R). This
 * draws that family: the base circle p(t) = (r + R cos t, sqrt(R^2-r^2) sin t,
 * r sin t) — verified to satisfy the torus equation exactly — spun into N
 * copies about the axis, once for each chirality, so the two oblique families
 * cross and the torus appears woven out of rings that are individually flat
 * and collectively curved. The whole basket turns slowly in 3D with the tube
 * radius breathing; nothing but glowing wire on black. */
#include "../jellydazzle.h"
#include "jd_up.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p172_up;

#define P172_W 480
#define P172_H 360
#define P172_NR 22
#define P172_NS 132

static float p172_acc[P172_W * P172_H * 3];
static unsigned char p172_img[P172_W * P172_H * 3];
static unsigned char p172_tone[1024];
static int *p172_xm;
static int p172_xmw;
static int p172_ready;
static uint32_t p172_seedc;
static float p172_col[64][3];
static float p172_hue0, p172_huew, p172_rr, p172_tilt, p172_dir;

static uint32_t p172_rs;
static float p172_rf(void)
{
    p172_rs ^= p172_rs << 13; p172_rs ^= p172_rs >> 17; p172_rs ^= p172_rs << 5;
    return (float)(p172_rs >> 8) * (1.0f / 16777216.0f);
}

static void p172_setup(uint32_t seed)
{
    int i;
    p172_rs = seed ? seed ^ 0x7A11EAC0u : 0x7A11EAC0u;
    p172_rf(); p172_rf();
    p172_hue0 = p172_rf();
    p172_huew = 0.10f + p172_rf() * 0.45f;
    p172_rr   = 0.30f + p172_rf() * 0.20f;        /* tube radius / R         */
    p172_tilt = 0.30f + p172_rf() * 0.55f;
    p172_dir  = p172_rf() < 0.5f ? -1.0f : 1.0f;
    if (!p172_ready) {
        for (i = 0; i < 1024; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (4.4f / 1024.0f)));
            p172_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p172_ready = 1;
    }
    p172_seedc = seed;
}

static void p172_hues(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 64; i++) {
        float hue = p172_hue0 + p172_huew * ((float)i / 63.0f);
        float r, g, b, mx;
        uint32_t p;
        hue -= floorf(hue);
        p = pal[(int)(hue * 32767.0f) & JD_PAL_MASK];
        r = (float)((p >> 16) & 255); g = (float)((p >> 8) & 255); b = (float)(p & 255);
        mx = r > g ? r : g; if (b > mx) mx = b; if (mx < 1.0f) mx = 1.0f;
        p172_col[i][0] = 0.12f + 0.88f * r / mx;
        p172_col[i][1] = 0.12f + 0.88f * g / mx;
        p172_col[i][2] = 0.12f + 0.88f * b / mx;
    }
}

static void p172_splat(float x, float y, const float *c, float w)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 0.0f || y < 0.0f || xi >= P172_W - 1 || yi >= P172_H - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p172_acc + (yi * P172_W + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * w; w1 = fx * (1.0f - fy) * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += P172_W * 3;
    w0 = (1.0f - fx) * fy * w; w1 = fx * fy * w;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p172_seg(float x0, float y0, float x1, float y1,
                     const float *c, float w)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    int n, i;
    float sx, sy, ww;
    if (len > 200.0f) return;
    n = (int)(len * 1.3f) + 1;
    sx = dx / (float)n; sy = dy / (float)n;
    ww = w * (len / (float)n + 0.30f);
    for (i = 0; i < n; i++)
        p172_splat(x0 + sx * (float)i, y0 + sy * (float)i, c, ww);
}

static void p172_blit(uint32_t *fb, int w, int h)
{
    int x, i;
    for (i = 0; i < P172_W * P172_H * 3; i++) {
        int ti = (int)(p172_acc[i] * 256.0f);
        p172_img[i] = p172_tone[ti < 0 ? 0 : ti > 1023 ? 1023 : ti];
    }
    if (p172_xmw != w) {
        free(p172_xm);
        p172_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p172_xm[x] = (int)(((long long)x * (P172_W - 1) << 8) / (w > 1 ? w - 1 : 1));
        p172_xmw = w;
    }
    jd_up_blit(&p172_up, fb, w, h, p172_img, P172_W, P172_H);
}

void pattern_172(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float R = 1.0f, r, Rc, ax, ay, ca, sa2, cb, sb, sc, D, crl, srl;
    int k, chir, i;
    (void)sl;
    if (!p172_ready || p172_seedc != seed) p172_setup(seed);
    p172_hues(pal);
    memset(p172_acc, 0, sizeof p172_acc);

    r  = p172_rr * (1.0f + 0.16f * sinf(t * 0.00073f));
    Rc = sqrtf(R * R - r * r);
    ax = p172_tilt + 0.26f * sinf(t * 0.00051f);   /* tip toward the viewer  */
    ay = 0.34f * sinf(t * 0.00031f + 1.1f);        /* gentle wobble, no tumble */
    ca = cosf(ax); sa2 = sinf(ax);
    cb = cosf(ay); sb = sinf(ay);
    sc = (float)P172_H * 0.43f;
    D  = 3.6f;
    crl = cosf(t * 0.00068f * p172_dir); srl = sinf(t * 0.00068f * p172_dir);

    for (chir = 0; chir < 2; chir++) {
        float zs = chir ? -1.0f : 1.0f;
        for (k = 0; k < P172_NR; k++) {
            float ph = 6.2831853f * ((float)k + (chir ? 0.5f : 0.0f)) / P172_NR
                     + t * 0.0011f * p172_dir * (chir ? -1.0f : 1.0f);
            float cp = cosf(ph), sp = sinf(ph);
            float px0 = 0.0f, py0 = 0.0f, pw0 = 0.0f;
            int have = 0;
            const float *col = p172_col[(int)((float)k / (P172_NR - 1) * 63.0f
                                              * 0.5f + (chir ? 31.5f : 0.0f)) & 63];
            for (i = 0; i <= P172_NS; i++) {
                float u = 6.2831853f * (float)i / P172_NS;
                float cu = cosf(u), su = sinf(u);
                float X = r + R * cu, Y = Rc * su, Z = zs * r * su;
                float x1, y1, z1, x2, y2, z2, den, sxp, syp, wgt, pulse;
                x1 = X * cp - Y * sp; y1 = X * sp + Y * cp; z1 = Z;   /* spin  */
                x2 = x1 * cb + z1 * sb; z2 = -x1 * sb + z1 * cb;      /* yaw   */
                y2 = y1 * ca - z2 * sa2; z2 = y1 * sa2 + z2 * ca;     /* pitch */
                den = D - z2;
                if (den < 0.6f) { have = 0; continue; }
                {
                    float q = sc * (D / den) * 0.72f;
                    float ux = x2 * q, uy = y2 * q;
                    sxp = P172_W * 0.5f + ux * crl - uy * srl;
                    syp = P172_H * 0.5f + ux * srl + uy * crl;
                }
                pulse = 0.55f + 0.45f * sinf(u * 3.0f - t * 0.014f + ph * 2.0f);
                wgt = (0.34f + 1.70f / (den * den)) * (0.35f + 0.65f * pulse);
                if (have) p172_seg(px0, py0, sxp, syp, col, (wgt + pw0) * 0.5f);
                px0 = sxp; py0 = syp; pw0 = wgt; have = 1;
            }
        }
    }
    p172_blit(fb, w, h);
}
