/* 183 String Hyperboloid — the string-sculpture proof that a curved surface can
 * be made of nothing but straight lines. Two rings of 44 pins sit at z = +-H;
 * pin i on the top is joined by a taut string to pin i+tau on the bottom, and
 * the second family runs to pin i-tau. Every string is dead straight, yet the
 * envelope they sweep is the hyperboloid of one sheet  x^2 + y^2 - (z/k)^2 = R^2,
 * whose waist radius is R*cos(tau/2): as tau breathes the solid inflates from a
 * cylinder, pinches through the double cone at tau = pi, and opens again, while
 * the two families cross into a woven diamond lattice. Drawn in real 3-D with a
 * perspective divide and a nodding tilt; depth sets brightness so the far wall
 * sinks into the black, and a travelling sine runs light up each string like
 * current in a filament. Line art on black, ~92% near-zero. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p183_up;

#define CW 480
#define CH 360
#define NSTR 44
#define NSAMP 330

static float p183_acc[CW * CH * 3];
static unsigned char p183_img[CW * CH * 3];
static unsigned char p183_tone[2048];
static int *p183_xm;
static int p183_xmw;
static float p183_hue0, p183_huew, p183_spin, p183_tilt, p183_wav, p183_kw;
static uint32_t p183_seedc;
static int p183_ready, p183_tabs;

static uint32_t p183_rs;
static float p183_rf(void)
{
    p183_rs ^= p183_rs << 13; p183_rs ^= p183_rs >> 17; p183_rs ^= p183_rs << 5;
    return (float)(p183_rs >> 8) * (1.0f / 16777216.0f);
}

static void p183_pal3(const uint32_t *pal, float hue, float sat, float *o)
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

static void p183_setup(uint32_t seed)
{
    int i;
    p183_rs = seed ? seed ^ 0x5731A6u : 0x5731A6u;
    p183_rf(); p183_rf();
    p183_hue0 = p183_rf();
    p183_huew = 0.12f + p183_rf() * 0.46f;
    p183_spin = (p183_rf() < 0.5f ? -1.0f : 1.0f) * (0.0016f + p183_rf() * 0.0016f);
    p183_tilt = 0.00047f + p183_rf() * 0.00040f;
    p183_wav  = 0.0075f + p183_rf() * 0.0065f;
    p183_kw   = 2.0f + floorf(p183_rf() * 4.0f);
    if (!p183_tabs) {
        for (i = 0; i < 2048; i++) {
            float v = 255.0f * (1.0f - expf(-(float)i * (5.4f / 2048.0f)));
            p183_tone[i] = (unsigned char)(v > 255.0f ? 255.0f : v);
        }
        p183_tabs = 1;
    }
    p183_ready = 1;
    p183_seedc = seed;
}

static void p183_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w0, w1;
    float *p;
    if (x < 1.0f || y < 1.0f || xi >= CW - 2 || yi >= CH - 2) return;
    fx = x - (float)xi; fy = y - (float)yi;
    p = p183_acc + (yi * CW + xi) * 3;
    w0 = (1.0f - fx) * (1.0f - fy) * wgt; w1 = fx * (1.0f - fy) * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
    p += CW * 3;
    w0 = (1.0f - fx) * fy * wgt; w1 = fx * fy * wgt;
    p[0] += c[0] * w0; p[1] += c[1] * w0; p[2] += c[2] * w0;
    p[3] += c[0] * w1; p[4] += c[1] * w1; p[5] += c[2] * w1;
}

static void p183_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p183_xmw != w) {
        free(p183_xm);
        p183_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p183_xm[x] = (int)(((long long)x * (CW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p183_xmw = w;
    }
    jd_up_blit(&p183_up, fb, w, h, p183_img, CW, CH);
}

void pattern_183(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame, tau, ct, st, spin, cx, cy, sc, H;
    float col[48][3];
    int i, j, k, fam, o;
    (void)sl;
    if (!p183_ready || p183_seedc != seed) p183_setup(seed);

    for (k = 0; k < 48; k++)
        p183_pal3(pal, p183_hue0 + p183_huew * ((float)k * (1.0f / 48.0f)), 0.90f, col[k]);

    tau = 1.62f + 1.38f * sinf(t * 0.00058f);          /* twist, rad          */
    {
        float ph = 0.62f + 0.34f * sinf(t * p183_tilt);
        ct = cosf(ph); st = sinf(ph);
    }
    spin = t * p183_spin;
    H = 1.18f;
    cx = CW * 0.5f; cy = CH * 0.5f; sc = 196.0f;

    memset(p183_acc, 0, sizeof p183_acc);
    for (fam = 0; fam < 2; fam++) {
        float sgn = fam ? -1.0f : 1.0f;
        for (i = 0; i < NSTR; i++) {
            float th = (float)i * (6.2831853f / NSTR) + spin;
            float ph = th + sgn * tau;
            float ax = cosf(th), ay = sinf(th);
            float bx = cosf(ph), by = sinf(ph);
            const float *c = col[(i * 48 / NSTR + (fam ? 24 : 0)) % 48];
            float wph = t * p183_wav + (float)i * 0.37f + (fam ? 3.1f : 0.0f);
            for (j = 0; j < NSAMP; j++) {
                float s = (float)j * (1.0f / (NSAMP - 1));
                float x = ax + (bx - ax) * s;
                float y = ay + (by - ay) * s;
                float z = H - 2.0f * H * s;
                float yr = y * ct - z * st;
                float zr = y * st + z * ct;
                float pd = 4.4f - zr, ip, sx, sy, dep, gl, v;
                if (pd < 0.35f) continue;
                ip = 3.9f / pd;
                sx = cx + x * ip * sc * 0.62f;
                sy = cy + yr * ip * sc * 0.62f;
                dep = (zr + 1.6f) * 0.31f;
                if (dep < 0.0f) dep = 0.0f; else if (dep > 1.0f) dep = 1.0f;
                gl = 0.30f + 0.70f * (0.5f + 0.5f * sinf(p183_kw * s * 6.2831853f - wph));
                v = (0.16f + 0.84f * dep * dep) * gl;
                p183_splat(sx, sy, c, v * 0.44f);
            }
        }
    }
    /* the two pin rings, faint, to close the sculpture */
    for (fam = 0; fam < 2; fam++) {
        float z = fam ? -H : H;
        for (j = 0; j < 420; j++) {
            float a = (float)j * (6.2831853f / 420.0f) + spin;
            float x = cosf(a), y = sinf(a);
            float yr = y * ct - z * st, zr = y * st + z * ct;
            float pd = 4.4f - zr, ip, dep;
            if (pd < 0.35f) continue;
            ip = 3.9f / pd;
            dep = (zr + 1.6f) * 0.31f;
            if (dep < 0.0f) dep = 0.0f; else if (dep > 1.0f) dep = 1.0f;
            p183_splat(cx + x * ip * sc * 0.62f, cy + yr * ip * sc * 0.62f,
                       col[fam ? 12 : 36], 0.10f + 0.22f * dep);
        }
    }
    for (o = 0; o < CW * CH * 3; o++) {
        int v = (int)(p183_acc[o] * 620.0f);
        p183_img[o] = p183_tone[v < 0 ? 0 : v > 2047 ? 2047 : v];
    }
    p183_blit(fb, w, h);
}
