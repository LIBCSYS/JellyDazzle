/* 139 Harmonic Orb — spherical harmonics painted on turning spheres.
 *
 * Two shaded spheres, and on each one a real spherical harmonic evaluated
 * directly in Cartesian body coordinates -- no Legendre recursion, no
 * atan2.  Four low-order terms are written as polynomials in the surface
 * normal,
 *   Y30 ~ nz(5nz^2 - 3),        Y32 ~ (nx^2 - ny^2) nz,
 *   Y33 ~ nx(nx^2 - 3ny^2),     Y40 ~ 35nz^4 - 30nz^2 + 3,
 * and mixed with four slow, mutually detuned weights.  Because the mix is
 * continuous, the banding migrates smoothly between zonal (stacked
 * latitude bands), tesseral (a chequered lattice) and sectoral (orange-peel
 * segments) forms instead of cutting between named modes.
 *
 * The surface is read three ways at once: hue is signed, so positive and
 * negative lobes take opposite sides of the palette; a gamma of |Y| gives
 * the lobes their body; and a narrow Gaussian on Y alone lights the NODAL
 * SET -- the curves where the harmonic vanishes -- as bright wires laid
 * over the shading.  A Fresnel rim keeps the silhouette luminous.  The
 * spheres are orthographic, so the surface normal is just (x, y, sqrt(1 -
 * x^2 - y^2)) and the whole thing costs one rotation and a dozen multiplies
 * per pixel.  Two solid objects on black, ~75% near-black: an overlay, and
 * the only shaded volume in this batch. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 384
#define AH 288
#define AN (AW * AH)
#define NORB 2

static float p139_acc[AN * 3];
static float p139_blur[AN * 3];
static uint8_t p139_img[AN * 3];
static uint8_t p139_tone[2048];
static float p139_hue[512][3];
static int p139_ready;
static int p139_uw = -1;
static int *p139_uxi;
static uint8_t *p139_ufx;

static void p139_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (5.4f / 2048.0f)));
        p139_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    p139_ready = 1;
}

static void p139_build_hue(const uint32_t *pal)
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
        p139_hue[i][0] = 0.10f + 0.90f * r * m;
        p139_hue[i][1] = 0.10f + 0.90f * g * m;
        p139_hue[i][2] = 0.10f + 0.90f * b * m;
    }
}

static void p139_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p139_acc + (size_t)y * AW * 3;
        float *out = p139_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p139_blur + (size_t)ym * AW * 3;
        const float *b = p139_blur + (size_t)y * AW * 3;
        const float *c = p139_blur + (size_t)yp * AW * 3;
        float *out = p139_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] = 0.66f * out[x] + 0.34f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p139_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p139_acc[i] * 700.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p139_img[i] = p139_tone[t];
    }
}

static void p139_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p139_uw) {
        free(p139_uxi); free(p139_ufx);
        p139_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p139_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p139_uxi[x] = xi * 3;
            p139_ufx[x] = (uint8_t)(q & 255);
        }
        p139_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p139_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p139_uxi[x], fx = p139_ufx[x], c[3];
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

void pattern_139(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float hd = t * 0.00019f + sd;
    int o, x, y;
    (void)sl;

    if (!p139_ready) p139_tabs();
    p139_build_hue(pal);
    memset(p139_acc, 0, sizeof p139_acc);

    for (o = 0; o < NORB; o++) {
        float ph = (float)o * 2.7f + sd * 6.2831f;
        float cx = (float)AW * (0.5f + (o ? 0.315f : -0.115f)
                                + 0.055f * sinf(t * 0.00047f + ph));
        float cy = (float)AH * (0.5f + (o ? -0.245f : 0.065f)
                                + 0.060f * sinf(t * 0.00039f + ph * 1.7f));
        float R = (float)AH * (o ? 0.185f : 0.375f);
        float iR = 1.0f / R;
        /* two-angle body rotation, slow enough that the surface never races */
        float ya = t * 0.00158f + ph, pa = t * 0.00097f + ph * 0.6f;
        float cyw = cosf(ya), syw = sinf(ya), cpi = cosf(pa), spi = sinf(pa);
        /* harmonic mix weights on four detuned drifts */
        float w1 = 0.55f + 0.45f * sinf(t * 0.00061f + ph);
        float w2 = 0.50f + 0.50f * sinf(t * 0.00043f + ph * 2.1f);
        float w3 = 0.50f + 0.50f * sinf(t * 0.00035f + ph * 3.3f);
        float w4 = 0.40f + 0.40f * sinf(t * 0.00027f + ph * 1.3f);
        float wn = 1.0f / (w1 + w2 + w3 + w4 + 0.001f);
        float gain = 3.4f;
        int x0 = (int)(cx - R) - 1, x1 = (int)(cx + R) + 1;
        int y0 = (int)(cy - R) - 1, y1 = (int)(cy + R) + 1;
        float amp = o ? 0.80f : 1.0f;
        w1 *= wn; w2 *= wn; w3 *= wn; w4 *= wn;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > AW - 1) x1 = AW - 1;
        if (y1 > AH - 1) y1 = AH - 1;

        for (y = y0; y <= y1; y++) {
            float vy = ((float)y - cy) * iR;
            float vy2 = vy * vy;
            float *row = p139_acc + (size_t)y * AW * 3;
            for (x = x0; x <= x1; x++) {
                float vx = ((float)x - cx) * iR;
                float r2 = vx * vx + vy2;
                float vz, nx, ny, nz, tx, ty;
                float nx2, ny2, nz2, Y, lam, band, glow, rim, edge, b;
                const float *c;
                int hi;
                if (r2 >= 1.0f) continue;
                vz = sqrtf(1.0f - r2);
                /* view normal -> body normal: yaw about y, then pitch about x */
                tx = vx * cyw + vz * syw;
                ty = vy;
                {
                    float tz = -vx * syw + vz * cyw;
                    nx = tx;
                    ny = ty * cpi - tz * spi;
                    nz = ty * spi + tz * cpi;
                }
                nx2 = nx * nx; ny2 = ny * ny; nz2 = nz * nz;
                Y = w1 * (nz * (5.0f * nz2 - 3.0f)) * 0.5f
                  + w2 * ((nx2 - ny2) * nz) * 1.7f
                  + w3 * (nx * (nx2 - 3.0f * ny2)) * 1.1f
                  + w4 * (35.0f * nz2 * nz2 - 30.0f * nz2 + 3.0f) * 0.18f;
                /* Lambert from a fixed key light, plus a little fill */
                lam = 0.34f * vx + 0.30f * vy + 0.89f * vz;
                if (lam < 0.0f) lam = 0.0f;
                lam = 0.22f + 0.78f * lam;
                band = fabsf(Y);
                band = band > 1.0f ? 1.0f : powf(band, 0.42f);
                glow = expf(-Y * Y * 620.0f);        /* the nodal set */
                rim = 1.0f - vz;
                rim = rim * rim * rim * 1.05f;
                edge = (1.0f - r2) * R * 0.55f;      /* silhouette AA */
                if (edge > 1.0f) edge = 1.0f;
                hi = (int)((hd + 0.5f + Y * 0.46f) * 512.0f) & 511;
                c = p139_hue[hi];
                b = (0.06f + 0.94f * band) * lam * gain * (1.0f / 3.4f);
                b = (b + rim * 0.30f) * edge * amp;
                {
                    /* the nodal wire is lit in the palette's own colour so
                     * it does not bleach the lobes it separates */
                    float gw = glow * edge * amp * 0.85f;
                    row[x * 3 + 0] += c[0] * (b + gw) + gw * 0.30f;
                    row[x * 3 + 1] += c[1] * (b + gw) + gw * 0.30f;
                    row[x * 3 + 2] += c[2] * (b + gw) + gw * 0.34f;
                }
            }
        }
    }

    p139_bloom();
    p139_resolve();
    p139_upscale(fb, w, h);
}
