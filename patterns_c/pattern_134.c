/* 134 Aurora Curtain — folded auroral sheets over a star field.
 *
 * Five curtains, each a ribbon standing vertically on a ground track that
 * wanders in the horizontal plane: X(s), Z(s) are two slow low-order Fourier
 * curves, and the ribbon is drawn in perspective (screen x = X/Z, height
 * proportional to 1/Z).  Because the track curves toward and away from the
 * viewer, the projection of s -> x is NOT monotonic: the sheet folds back on
 * itself and the folds pile light on light, which is exactly how a real
 * aurora gets its bright creases.  Along the sheet a fine sinusoid in s
 * gives the vertical ray striation; up the sheet the intensity falls as a
 * gamma ramp and the palette index climbs, so each curtain runs from a dense
 * base to a thin, differently-coloured crown.  Everything is additive, so
 * overlapping curtains sum instead of occluding.  ~200 slow-twinkling stars
 * sit behind.  Sky is near-black top and bottom: composites as an overlay,
 * but it is dense enough through the middle band to carry a scene alone. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 384
#define AH 288
#define AN (AW * AH)
#define NCUR 7
#define NSAMP 1600
#define NSTAR 210

static float p134_acc[AN * 3];
static float p134_blur[AN * 3];
static uint8_t p134_img[AN * 3];
static uint8_t p134_tone[2048];
static float p134_pcol[1024][3];
static float p134_gr[128][3], p134_pf[128];
static float p134_stx[NSTAR], p134_sty[NSTAR], p134_stp[NSTAR], p134_stb[NSTAR];
static int p134_ready;
static int p134_uw = -1;
static int *p134_uxi;
static uint8_t *p134_ufx;

static uint32_t p134_rs = 0x1F35D0A7u;
static float p134_rf(void)
{
    p134_rs ^= p134_rs << 13; p134_rs ^= p134_rs >> 17; p134_rs ^= p134_rs << 5;
    return (float)(p134_rs >> 8) * (1.0f / 16777216.0f);
}

static void p134_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (4.4f / 2048.0f)));
        p134_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    for (i = 0; i < NSTAR; i++) {
        p134_stx[i] = p134_rf() * (AW - 2);
        p134_sty[i] = p134_rf() * (AH * 0.80f);
        p134_stp[i] = p134_rf() * 6.2831853f;
        p134_stb[i] = 0.06f + 0.30f * p134_rf() * p134_rf();
    }
    p134_ready = 1;
}

/* palette straight through, lightly lifted so dark ramp zones still emit */
static void p134_build_col(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 1024; i++) {
        uint32_t u = pal[(i << 5) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255) * (1.0f / 255.0f);
        float g = (float)((u >> 8) & 255) * (1.0f / 255.0f);
        float b = (float)(u & 255) * (1.0f / 255.0f);
        float m = r > g ? r : g;
        if (b > m) m = b;
        if (m < 0.10f) m = 0.10f;
        m = 0.35f + 0.65f / m;         /* partial brightness flattening */
        if (m > 3.2f) m = 3.2f;
        p134_pcol[i][0] = r * m;
        p134_pcol[i][1] = g * m;
        p134_pcol[i][2] = b * m;
    }
}

static void p134_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p134_acc + (size_t)y * AW * 3;
        float *out = p134_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p134_blur + (size_t)ym * AW * 3;
        const float *b = p134_blur + (size_t)y * AW * 3;
        const float *c = p134_blur + (size_t)yp * AW * 3;
        float *out = p134_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] += 0.80f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p134_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p134_acc[i] * 780.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p134_img[i] = p134_tone[t];
    }
}

static void p134_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p134_uw) {
        free(p134_uxi); free(p134_ufx);
        p134_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p134_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p134_uxi[x] = xi * 3;
            p134_ufx[x] = (uint8_t)(q & 255);
        }
        p134_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p134_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p134_uxi[x], fx = p134_ufx[x], c[3];
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

void pattern_134(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 1023) * (1.0f / 1024.0f);
    float horizon = (float)AH * 1.00f;
    int cu, i, x, y;
    (void)sl;

    if (!p134_ready) p134_tabs();
    p134_build_col(pal);
    memset(p134_acc, 0, sizeof p134_acc);

    /* sky: a very dark vertical wash so the frame is never dead black */
    for (y = 0; y < AH; y++) {
        float v = (float)y / (float)AH;
        float g = 0.008f + 0.020f * v * v;
        float *row = p134_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            row[x * 3 + 0] += g * 0.35f;
            row[x * 3 + 1] += g * 0.55f;
            row[x * 3 + 2] += g * 1.00f;
        }
    }

    /* stars */
    for (i = 0; i < NSTAR; i++) {
        int xi = (int)p134_stx[i], yi = (int)p134_sty[i];
        float b = p134_stb[i] * (0.65f + 0.35f * sinf(t * 0.0085f + p134_stp[i]));
        float *px = p134_acc + ((size_t)yi * AW + xi) * 3;
        px[0] += b * 0.85f; px[1] += b * 0.90f; px[2] += b;
    }

    for (cu = 0; cu < NCUR; cu++) {
        float ph = (float)cu * 1.7f + sd * 6.2831f;
        float w1 = 0.00061f + 0.00013f * (float)cu;
        float w2 = 0.00044f + 0.00009f * (float)cu;
        float hgt = 1.05f + 0.28f * sinf(ph * 2.1f);
        float amp = 0.42f + 0.38f * (0.5f + 0.5f * sinf(t * 0.00037f + ph));
        float hbase = 0.06f + 0.16f * (float)cu / (float)NCUR;
        float ray = 34.0f + 13.0f * (float)cu;
        float rdr = t * 0.0031f + ph;
        int s, j;
        /* per-curtain vertical gradient: colour climbs, brightness falls */
        for (j = 0; j < 128; j++) {
            float v = (float)j * (1.0f / 127.0f);   /* 0 crown .. 1 foot */
            int ci = (int)((hbase + (1.0f - v) * 0.30f + t * 0.000041f) * 1024.0f) & 1023;
            float pf = powf(v, 1.35f);
            p134_gr[j][0] = p134_pcol[ci][0];
            p134_gr[j][1] = p134_pcol[ci][1];
            p134_gr[j][2] = p134_pcol[ci][2];
            p134_pf[j] = pf;
        }
        for (s = 0; s < NSAMP; s++) {
            float u = (float)s / (float)(NSAMP - 1);
            float q = u * 6.2831853f;
            float X = (u - 0.5f) * 7.4f
                    + 1.15f * sinf(q * 0.9f + t * w1 + ph)
                    + 0.55f * sinf(q * 2.3f - t * w2 * 1.6f + ph * 1.9f);
            float Z = 2.55f + 1.45f * sinf(q * 0.7f + t * w2 + ph * 0.7f)
                            + 0.55f * sinf(q * 1.9f - t * w1 * 1.3f);
            float iz, sx, hh, top, bot, env, strk, base, dy, vs;
            int y0, y1, xi;
            float fx;
            if (Z < 0.85f) Z = 0.85f;
            iz = 1.0f / Z;
            sx = (float)AW * 0.5f + X * iz * (float)AW * 0.30f;
            /* the crown is scalloped by the same striation that lights the
             * rays, so bright rays stand taller: no flat-topped slab */
            hh = hgt * (0.42f + 0.58f * (0.5f + 0.5f * sinf(u * ray * 0.5f + rdr * 0.7f))
                              * (0.55f + 0.45f * sinf(u * 4.1f + t * w1 * 1.7f + ph)))
                     * iz * (float)AH * 1.02f;
            bot = horizon - hh * 0.06f;
            top = horizon - hh;
            if (top < -60.0f) top = -60.0f;
            /* end-fade window + slow luminous surge travelling along the sheet */
            env = sinf(u * 3.14159265f);
            env *= env;
            env *= 0.42f + 0.58f * (0.5f + 0.5f * sinf(u * 7.3f - t * 0.0042f + ph));
            /* vertical ray striation */
            strk = 0.30f + 0.70f * fabsf(sinf(u * ray + rdr));
            base = env * strk * amp * iz * (0.55f * 520.0f / (float)NSAMP);
            if (base < 0.0004f) continue;
            xi = (int)sx;
            if (xi < 0 || xi >= AW - 1) continue;
            fx = sx - (float)xi;
            dy = bot - top;
            if (dy < 1.0f) continue;
            y0 = (int)(top < 0.0f ? 0.0f : top);
            y1 = (int)(bot > (float)(AH - 1) ? (float)(AH - 1) : bot);
            vs = 127.0f / dy;
            {
                float k0 = base * (1.0f - fx), k1 = base * fx;
                float *px = p134_acc + ((size_t)y0 * AW + xi) * 3;
                float vv = ((float)y0 - top) * vs;
                for (y = y0; y <= y1; y++, px += AW * 3, vv += vs) {
                    int jj = (int)vv;
                    float pf;
                    const float *c;
                    if (jj < 0) jj = 0; else if (jj > 127) jj = 127;
                    pf = p134_pf[jj];
                    c = p134_gr[jj];
                    px[0] += c[0] * pf * k0; px[1] += c[1] * pf * k0;
                    px[2] += c[2] * pf * k0;
                    px[3] += c[0] * pf * k1; px[4] += c[1] * pf * k1;
                    px[5] += c[2] * pf * k1;
                }
            }
        }
    }

    p134_bloom();
    p134_resolve();
    p134_upscale(fb, w, h);
}
