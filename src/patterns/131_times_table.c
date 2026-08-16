/* 131 Times Table — the caustic of modular multiplication.
 *
 * The 19th-century string-art construction: mark N points evenly on a
 * circle and join every point k to point m.k.  The chords do not fill the
 * disc evenly -- they bunch onto an envelope, and that envelope is an
 * epicycloid with m-1 cusps.  m = 2 draws a cardioid, m = 3 a nephroid,
 * m = 4 a three-cusped curve, and so on up the integers.
 *
 * The point of this one is that m is NOT an integer.  It is a real number
 * crawling continuously between 2 and 13, so the picture is never a
 * finished epicycloid: cusps are perpetually being born at the rim,
 * migrating inward and dissolving, and between integers the chord family
 * degenerates into slowly rotating multi-armed lattices.  Nothing snaps,
 * because the endpoint angle is m.theta with no modulus anywhere -- the
 * wrap is carried entirely by the cosine.  Two counter-drifting families
 * are drawn at once, on two radii and in two parts of the palette, so a
 * slow envelope and a fast one orbit through each other.  Hue is keyed to
 * the chord's starting angle, brightness to a wave travelling around the
 * rim.  Chord art on black, ~90% near-black: an overlay layer. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 384
#define AH 288
#define AN (AW * AH)
#define NCH 640

static float p131_acc[AN * 3];
static float p131_blur[AN * 3];
static uint8_t p131_img[AN * 3];
static uint8_t p131_tone[2048];
static float p131_hue[512][3];
static int p131_ready;
static int p131_uw = -1;
static int *p131_uxi;
static uint8_t *p131_ufx;

static void p131_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (5.2f / 2048.0f)));
        p131_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    p131_ready = 1;
}

static void p131_build_hue(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 512; i++) {
        uint32_t u = pal[(i << 6) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255);
        float g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float m = r > g ? r : g;
        if (b > m) m = b;
        if (m < 30.0f) m = 30.0f;
        m = 1.0f / m;
        p131_hue[i][0] = 0.09f + 0.91f * r * m;
        p131_hue[i][1] = 0.09f + 0.91f * g * m;
        p131_hue[i][2] = 0.09f + 0.91f * b * m;
    }
}

static void p131_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx, fy, w00, w10, w01, w11;
    if ((unsigned)xi >= AW - 1 || (unsigned)yi >= AH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy) * wgt; w10 = fx * (1.0f - fy) * wgt;
    w01 = (1.0f - fx) * fy * wgt;          w11 = fx * fy * wgt;
    {
        float *a = p131_acc + ((size_t)yi * AW + xi) * 3;
        float *b = a + 3, *d = a + AW * 3, *e = d + 3;
        a[0] += c[0] * w00; a[1] += c[1] * w00; a[2] += c[2] * w00;
        b[0] += c[0] * w10; b[1] += c[1] * w10; b[2] += c[2] * w10;
        d[0] += c[0] * w01; d[1] += c[1] * w01; d[2] += c[2] * w01;
        e[0] += c[0] * w11; e[1] += c[1] * w11; e[2] += c[2] * w11;
    }
}

/* one chord, additively rasterised at a fixed sample density so that long
 * and short chords deposit the same energy per unit length */
static void p131_chord(float x0, float y0, float x1, float y1,
                       const float *c, float b)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    float col[3], wgt;
    int n, i;
    if (len < 0.6f) return;
    n = (int)(len * 1.5f) + 2;
    if (n > 900) n = 900;
    wgt = b * (0.150f / 1.5f);
    col[0] = c[0]; col[1] = c[1]; col[2] = c[2];
    for (i = 0; i <= n; i++) {
        float u = (float)i / (float)n;
        p131_splat(x0 + dx * u, y0 + dy * u, col, wgt);
    }
}

static void p131_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p131_acc + (size_t)y * AW * 3;
        float *out = p131_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p131_blur + (size_t)ym * AW * 3;
        const float *b = p131_blur + (size_t)y * AW * 3;
        const float *c = p131_blur + (size_t)yp * AW * 3;
        float *out = p131_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] += 1.05f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p131_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p131_acc[i] * 700.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p131_img[i] = p131_tone[t];
    }
}

static void p131_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p131_uw) {
        free(p131_uxi); free(p131_ufx);
        p131_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p131_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p131_uxi[x] = xi * 3;
            p131_ufx[x] = (uint8_t)(q & 255);
        }
        p131_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p131_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p131_uxi[x], fx = p131_ufx[x], c[3];
            for (k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16)
                   | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

void pattern_131(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float mA, mB, spin, R, ox, oy, hd, wv;
    int fam, k;
    (void)sl;

    if (!p131_ready) p131_tabs();
    p131_build_hue(pal);
    memset(p131_acc, 0, sizeof p131_acc);

    /* the multipliers are real and crawl: the epicycloid never finishes */
    mA = 7.4f + 5.3f * sinf(t * 0.0000880f + sd * 6.2831f);
    mB = 3.1f + 1.9f * sinf(t * 0.0001100f + 2.2f + sd * 3.1f);
    spin = t * 0.00015f + sd * 6.2831f;
    R = (float)AH * 0.470f;
    ox = (float)AW * 0.5f;
    oy = (float)AH * 0.5f;
    hd = t * 0.00020f + sd;
    wv = t * 0.0031f;

    for (fam = 0; fam < 2; fam++) {
        float m = fam ? mB : mA;
        float hb = fam ? 0.44f : 0.0f;
        float gain = fam ? 0.78f : 1.0f;
        float rr = fam ? R * 0.68f : R;
        float sp = fam ? -spin * 1.6f : spin;
        int n = fam ? NCH * 3 / 5 : NCH;
        for (k = 0; k < n; k++) {
            float th = (float)k * (6.2831853f / (float)n) + sp;
            float ph = th * m + sp * 0.35f;
            float x0 = ox + rr * cosf(th), y0 = oy + rr * sinf(th);
            float x1 = ox + rr * cosf(ph), y1 = oy + rr * sinf(ph);
            float b = gain * (0.32f + 0.68f * (0.5f + 0.5f * sinf(th * 3.0f - wv)));
            int hi = (int)((hb + th * 0.0836f + hd) * 512.0f) & 511;
            p131_chord(x0, y0, x1, y1, p131_hue[hi], b);
        }
    }

    p131_bloom();
    p131_resolve();
    p131_upscale(fb, w, h);
}
