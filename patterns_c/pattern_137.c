/* 137 Rutt Etra Relief — a height field drawn only as displaced scanlines.
 *
 * The Rutt/Etra Scan Processor (1972) deflected a CRT's raster by a video
 * signal, turning a picture into a ribbed 3D relief made of nothing but its
 * own scanlines.  This is that instrument, driven by a synthetic terrain:
 * 96 rows are placed in perspective (world depth z = 1..7, screen span and
 * row spacing both proportional to 1/z), each row is displaced upward by a
 * four-term travelling height field, and the rows are rasterised front to
 * back through a per-column horizon buffer -- the voxel-terrain hidden-line
 * rule -- so a near ridge genuinely occludes what is behind it.  The result
 * is a solid-reading landscape whose entire substance is line, with large
 * true-black regions in every shadowed trough, which is what makes it
 * composite well over anything.  Hue follows altitude and brightness
 * follows the local slope, so ridge crests catch the light.  The terrain
 * scrolls toward the viewer at a fraction of a wavelength per second. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 384
#define AH 288
#define AN (AW * AH)
#define NROW 96

static float p137_acc[AN * 3];
static float p137_blur[AN * 3];
static uint8_t p137_img[AN * 3];
static uint8_t p137_tone[2048];
static float p137_hue[512][3];
static float p137_hor[AW];
static int p137_ready;
static int p137_uw = -1;
static int *p137_uxi;
static uint8_t *p137_ufx;

static void p137_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (5.4f / 2048.0f)));
        p137_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    p137_ready = 1;
}

static void p137_build_hue(const uint32_t *pal)
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
        p137_hue[i][0] = 0.09f + 0.91f * r * m;
        p137_hue[i][1] = 0.09f + 0.91f * g * m;
        p137_hue[i][2] = 0.09f + 0.91f * b * m;
    }
}

static void p137_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx, fy, w00, w10, w01, w11;
    if ((unsigned)xi >= AW - 1 || (unsigned)yi >= AH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy) * wgt; w10 = fx * (1.0f - fy) * wgt;
    w01 = (1.0f - fx) * fy * wgt;          w11 = fx * fy * wgt;
    {
        float *a = p137_acc + ((size_t)yi * AW + xi) * 3;
        float *b = a + 3, *d = a + AW * 3, *e = d + 3;
        a[0] += c[0] * w00; a[1] += c[1] * w00; a[2] += c[2] * w00;
        b[0] += c[0] * w10; b[1] += c[1] * w10; b[2] += c[2] * w10;
        d[0] += c[0] * w01; d[1] += c[1] * w01; d[2] += c[2] * w01;
        e[0] += c[0] * w11; e[1] += c[1] * w11; e[2] += c[2] * w11;
    }
}

static void p137_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p137_acc + (size_t)y * AW * 3;
        float *out = p137_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p137_blur + (size_t)ym * AW * 3;
        const float *b = p137_blur + (size_t)y * AW * 3;
        const float *c = p137_blur + (size_t)yp * AW * 3;
        float *out = p137_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] += 0.62f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p137_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p137_acc[i] * 720.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p137_img[i] = p137_tone[t];
    }
}

static void p137_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p137_uw) {
        free(p137_uxi); free(p137_ufx);
        p137_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p137_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p137_uxi[x] = xi * 3;
            p137_ufx[x] = (uint8_t)(q & 255);
        }
        p137_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p137_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p137_uxi[x], fx = p137_ufx[x], c[3];
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

void pattern_137(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float scroll = t * 0.0024f;
    float hd = t * 0.00021f + sd;
    float cy = (float)AH * 0.26f;
    float bumpx, bumpv;
    int l, i;
    (void)sl;

    if (!p137_ready) p137_tabs();
    p137_build_hue(pal);
    memset(p137_acc, 0, sizeof p137_acc);
    for (i = 0; i < AW; i++) p137_hor[i] = (float)AH + 8.0f;

    bumpx = 1.35f * sinf(t * 0.00071f + sd * 6.2831f);
    bumpv = 3.9f + 2.6f * sinf(t * 0.00053f + sd * 3.1f);

    for (l = 0; l < NROW; l++) {
        float d = (float)l / (float)(NROW - 1);
        float zw = 1.05f + d * d * 6.4f;          /* rows bunch toward the horizon */
        float iz = 1.0f / zw;
        float ybase = cy + (float)AH * 0.70f * iz;
        float xk = (float)AW * 0.62f * iz;
        float vw = (float)l * 0.115f - scroll;
        float amp = (float)AH * 0.46f * iz;
        int ns = (int)(2.9f * xk);
        float du, py = 0.0f;
        int first = 1, s;
        if (ns < 24) ns = 24;
        if (ns > 1500) ns = 1500;
        du = 2.9f / (float)ns;
        for (s = 0; s <= ns; s++) {
            float uw = -1.45f + (float)s * du;
            float hgt, dh, px, yy, br, bs;
            float c[3];
            int hi, xi;
            hgt = 0.44f * sinf(uw * 2.05f + vw * 0.85f + t * 0.0012f)
                + 0.30f * sinf(uw * 3.70f - vw * 1.35f - t * 0.00082f + 1.7f)
                + 0.20f * sinf(uw * 6.10f + vw * 2.30f + t * 0.0017f + 3.1f)
                + 0.14f * sinf(uw * 9.40f - vw * 0.55f - t * 0.0015f);
            /* one broad wandering swell so the terrain has a subject */
            {
                float ex = (uw - bumpx) * (uw - bumpx) * 1.5f
                         + (vw - bumpv) * (vw - bumpv) * 0.10f;
                hgt += 1.15f * expf(-ex);
            }
            px = (float)AW * 0.5f + uw * xk;
            yy = ybase - hgt * amp;
            xi = (int)px;
            if (xi < 0 || xi >= AW - 1) { first = 1; continue; }
            /* hidden-line: a farther row shows only above the near horizon */
            if (yy >= p137_hor[xi]) { first = 1; py = yy; continue; }
            /* local slope drives the specular read of the crest */
            dh = first ? 0.0f : (yy - py);
            bs = 1.0f / (1.0f + fabsf(dh) * 0.55f);
            br = (0.30f + 0.62f * bs) * (0.34f + 0.66f * iz * 1.9f);
            hi = (int)((0.30f + hgt * 0.17f + hd) * 512.0f) & 511;
            c[0] = p137_hue[hi][0] * br;
            c[1] = p137_hue[hi][1] * br;
            c[2] = p137_hue[hi][2] * br;
            if (!first && fabsf(dh) > 1.0f) {
                /* connect steep steps so the ribbon never breaks */
                int n = (int)fabsf(dh), q;
                float step = (yy - py) / (float)(n + 1);
                float wgt = 0.55f / (float)(n + 1);
                for (q = 1; q <= n; q++)
                    p137_splat(px, py + step * (float)q, c, wgt);
            }
            p137_splat(px, yy, c, 0.55f);
            p137_hor[xi] = yy;
            py = yy;
            first = 0;
        }
    }

    p137_bloom();
    p137_resolve();
    p137_upscale(fb, w, h);
}
