/* 140 Newton Filigree — the boundary of Newton's basins, drawn as light.
 *
 * Every pixel is a starting point for twelve steps of a RELAXED Newton
 * iteration on z^3 - 1,  z <- z(1 - a/3) + (a/3)/z^2, where the relaxation
 * constant a is complex and crawls around a small circle centred on 1.
 * Interior points converge quadratically, so after twelve steps their
 * residual |z^3 - 1| is at the float floor; points on the basin boundary -- the Julia
 * set of the Newton map, a fractal of infinite detail -- never converge and
 * their residual stays order one.  Brightness is r^(1/k), which turns that
 * enormous dynamic range into a thin glowing filament exactly on the
 * boundary and pure black everywhere else.  Hue comes from the argument of
 * the final iterate, so each of the three basins stains its own side of the
 * filigree.  Moving a deforms the whole fractal continuously (at a = 1 it
 * is the classic three-fold Newton set; off-axis it spirals), and the frame
 * also turns and breathes.  log2 is read straight off the float exponent
 * and the tone curve is a table, so the whole per-pixel tail is two table
 * lookups.  ~92% near-black: an overlay. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 320
#define AH 240
#define AN (AW * AH)
#define NIT 12

static float p140_acc[AN * 3];
static float p140_blur[AN * 3];
static uint8_t p140_img[AN * 3];
static uint8_t p140_tone[2048];
static float p140_hue[512][3];
static float p140_glut[512];
static int p140_ready;
static int p140_uw = -1;
static int *p140_uxi;
static uint8_t *p140_ufx;

static void p140_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (5.2f / 2048.0f)));
        p140_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    /* glut[j] = 2^(l2 * g) for l2 = j/4 - 112 : the filament profile */
    for (i = 0; i < 512; i++) {
        /* single precision bottoms the residual out near 2^-24, so the
         * usable range of log2|z^3-1| is only 0..-24: the exponent has to
         * be steep or every basin interior stays lit. */
        float l2 = (float)i * 0.25f - 112.0f;
        float v = exp2f((l2 + 1.0f) * 0.50f);
        p140_glut[i] = v > 1.0f ? 1.0f : v;
    }
    p140_ready = 1;
}

static void p140_build_hue(const uint32_t *pal)
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
        p140_hue[i][0] = 0.10f + 0.90f * r * m;
        p140_hue[i][1] = 0.10f + 0.90f * g * m;
        p140_hue[i][2] = 0.10f + 0.90f * b * m;
    }
}

/* piecewise-linear log2 straight off the IEEE exponent (max error 0.09) */
static float p140_log2(float x)
{
    union { float f; uint32_t u; } v;
    v.f = x;
    return (float)((int)(v.u >> 23) - 127)
         + (float)(v.u & 0x7FFFFFu) * (1.0f / 8388608.0f);
}

static float p140_atan2(float y, float x)
{
    float ax = fabsf(x), ay = fabsf(y), a, s;
    if (ax + ay < 1e-24f) return 0.0f;
    a = (ax > ay) ? ay / ax : ax / ay;
    s = a * a;
    s = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) s = 1.57079637f - s;
    if (x < 0.0f) s = 3.14159274f - s;
    return (y < 0.0f) ? -s : s;
}

static void p140_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p140_acc + (size_t)y * AW * 3;
        float *out = p140_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p140_blur + (size_t)ym * AW * 3;
        const float *b = p140_blur + (size_t)y * AW * 3;
        const float *c = p140_blur + (size_t)yp * AW * 3;
        float *out = p140_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] += 0.62f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p140_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p140_acc[i] * 620.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p140_img[i] = p140_tone[t];
    }
}

static void p140_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p140_uw) {
        free(p140_uxi); free(p140_ufx);
        p140_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p140_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p140_uxi[x] = xi * 3;
            p140_ufx[x] = (uint8_t)(q & 255);
        }
        p140_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p140_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p140_uxi[x], fx = p140_ufx[x], c[3];
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

void pattern_140(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float phi, ar, ai, c1r, c1i, c2r, c2i;
    float zoom, th, ct, st, hd;
    int x, y;
    (void)sl;

    if (!p140_ready) p140_tabs();
    p140_build_hue(pal);

    /* relaxation constant on a small circle about 1 */
    phi = t * 0.00113f + sd * 6.2831f;
    ar = 1.0f + 0.195f * cosf(phi);
    ai = 0.195f * sinf(phi);
    c1r = 1.0f - ar * (1.0f / 3.0f);
    c1i = -ai * (1.0f / 3.0f);
    c2r = ar * (1.0f / 3.0f);
    c2i = ai * (1.0f / 3.0f);

    zoom = 1.55f + 0.42f * sinf(t * 0.00047f + sd * 3.7f);
    th = t * 0.00039f;
    ct = cosf(th) * zoom / (float)AH * 2.0f;
    st = sinf(th) * zoom / (float)AH * 2.0f;
    hd = t * 0.00021f + sd;

    for (y = 0; y < AH; y++) {
        float vy = (float)y - (float)AH * 0.5f;
        float *row = p140_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            float vx = (float)x - (float)AW * 0.5f;
            float zx = vx * ct - vy * st;
            float zy = vx * st + vy * ct;
            float r2, l2, b, ang;
            const float *c;
            int i, gi, hi;
            for (i = 0; i < NIT; i++) {
                float x2 = zx * zx, y2 = zy * zy;
                float d = x2 + y2, dd;
                float sr, si, ur, ui;
                if (d < 1e-12f) { zx = 1.0f; zy = 0.0f; break; }
                sr = x2 - y2; si = 2.0f * zx * zy;   /* z^2 */
                dd = 1.0f / (d * d);                 /* 1/|z^2|^2 */
                ur = sr * dd; ui = -si * dd;         /* 1/z^2 */
                {
                    float nx = c1r * zx - c1i * zy + c2r * ur - c2i * ui;
                    float ny = c1r * zy + c1i * zx + c2r * ui + c2i * ur;
                    zx = nx; zy = ny;
                }
            }
            /* residual |z^3 - 1| : ~1e-30 inside a basin, order 1 on the
             * boundary.  r^(1/k) collapses that range into a filament. */
            {
                float x2 = zx * zx, y2 = zy * zy;
                float sr = x2 - y2, si = 2.0f * zx * zy;
                float cr = sr * zx - si * zy - 1.0f;
                float ci = sr * zy + si * zx;
                r2 = cr * cr + ci * ci + 1e-34f;
            }
            l2 = p140_log2(r2) * 0.5f;
            gi = (int)((l2 + 112.0f) * 4.0f);
            if (gi < 0) gi = 0;
            if (gi > 511) gi = 511;
            b = p140_glut[gi];
            ang = p140_atan2(zy, zx) * (1.0f / 6.2831853f);
            hi = (int)((ang * 0.90f + hd) * 512.0f) & 511;
            c = p140_hue[hi];
            row[x * 3 + 0] = c[0] * b;
            row[x * 3 + 1] = c[1] * b;
            row[x * 3 + 2] = c[2] * b;
        }
    }

    p140_bloom();
    p140_resolve();
    p140_upscale(fb, w, h);
}
