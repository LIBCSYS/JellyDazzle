/* 138 Soap Bubbles — thin-film interference on drifting spheres.
 *
 * Sixteen bubbles wander on closed Lissajous paths.  Each is shaded by the
 * real optics of a soap film: the film drains under gravity, so its
 * thickness T runs from thin at the crown to thick at the foot; the
 * viewing path through the film grows as 1/sqrt(1-u^2) toward the rim,
 * where u is the normalised radius; and the reflected intensity in each
 * channel is sin^2(2.pi.n.T.path / lambda) evaluated at 650, 550 and 450 nm.
 * That single formula produces the entire soap-bubble palette -- the
 * magenta/gold/cyan order bands, the way they crowd toward the rim, the
 * black spot where the film thins below a quarter wave -- with no colour
 * ramp involved.  Fresnel weighting (u^4) keeps the centres transparent, so
 * each bubble is a bright rim around a dark disc and the frame is ~85%
 * near-black: a natural SCREEN overlay.  T0 crawls, which walks every
 * bubble slowly up through the interference orders. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 384
#define AH 288
#define AN (AW * AH)
#define NB 16
#define SQN 1024

static float p138_acc[AN * 3];
static float p138_blur[AN * 3];
static uint8_t p138_img[AN * 3];
static uint8_t p138_tone[2048];
static float p138_sq[SQN];
static float p138_bx[NB], p138_by[NB], p138_br[NB], p138_bp[NB], p138_bt[NB];
static int p138_ready;
static int p138_uw = -1;
static int *p138_uxi;
static uint8_t *p138_ufx;

static void p138_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (4.6f / 2048.0f)));
        p138_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    for (i = 0; i < SQN; i++) {
        float s = sinf((float)i * (3.14159265f / (float)SQN));
        p138_sq[i] = s * s;
    }
    p138_ready = 1;
}

static void p138_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p138_acc + (size_t)y * AW * 3;
        float *out = p138_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p138_blur + (size_t)ym * AW * 3;
        const float *b = p138_blur + (size_t)y * AW * 3;
        const float *c = p138_blur + (size_t)yp * AW * 3;
        float *out = p138_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] = 0.55f * out[x] + 0.45f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p138_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p138_acc[i] * 900.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p138_img[i] = p138_tone[t];
    }
}

static void p138_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p138_uw) {
        free(p138_uxi); free(p138_ufx);
        p138_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p138_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p138_uxi[x] = xi * 3;
            p138_ufx[x] = (uint8_t)(q & 255);
        }
        p138_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p138_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p138_uxi[x], fx = p138_ufx[x], c[3];
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

void pattern_138(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float pr, pg, pb;
    int i, x, y;
    (void)sl;

    if (!p138_ready) p138_tabs();
    memset(p138_acc, 0, sizeof p138_acc);

    /* palette only modulates the iridescence, never replaces it */
    {
        uint32_t u = pal[(int)((t * 0.000029f + sd) * 32768.0f) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), m = r > g ? r : g;
        if (b > m) m = b;
        if (m < 28.0f) m = 28.0f;
        m = 1.0f / m;
        pr = 0.42f + 0.58f * r * m;
        pg = 0.42f + 0.58f * g * m;
        pb = 0.42f + 0.58f * b * m;
    }

    for (i = 0; i < NB; i++) {
        float ph = (float)i * 0.9817477f + sd * 6.2831f;
        /* the vertical phase uses the golden angle so the sixteen paths do
         * not lock into bands */
        float qh = (float)i * 2.3999632f + sd * 4.13f;
        p138_bx[i] = (float)AW * (0.5f + 0.415f * sinf(t * 0.00043f + ph)
                                       + 0.075f * sinf(t * 0.00119f + ph * 2.7f));
        p138_by[i] = (float)AH * (0.5f + 0.400f * sinf(t * 0.00035f + qh)
                                       + 0.070f * sinf(t * 0.00097f + qh * 1.7f));
        p138_br[i] = (float)AH * (0.075f + 0.080f * (0.5f + 0.5f * sinf(qh * 1.7f))
                                  + 0.014f * sinf(t * 0.00061f + ph));
        p138_bp[i] = ph;
        /* film thickness at the crown, in micrometres, crawling upward */
        p138_bt[i] = 0.26f + 0.13f * sinf(ph * 2.3f)
                   + 0.055f * sinf(t * 0.00040f + ph * 1.4f);
    }

    for (i = 0; i < NB; i++) {
        float cx = p138_bx[i], cy = p138_by[i], r = p138_br[i];
        float ir = 1.0f / r, t0 = p138_bt[i];
        float spx = cx - r * 0.42f, spy = cy - r * 0.46f;
        int x0 = (int)(cx - r) - 1, x1 = (int)(cx + r) + 1;
        int y0 = (int)(cy - r) - 1, y1 = (int)(cy + r) + 1;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > AW - 1) x1 = AW - 1;
        if (y1 > AH - 1) y1 = AH - 1;
        for (y = y0; y <= y1; y++) {
            float dy = ((float)y - cy) * ir;
            float vy = 0.5f + dy * 0.5f;             /* 0 crown .. 1 foot */
            float dy2 = dy * dy;
            float *row = p138_acc + (size_t)y * AW * 3;
            for (x = x0; x <= x1; x++) {
                float dx = ((float)x - cx) * ir;
                float u2 = dx * dx + dy2;
                float u, path, T, opd, vis, edge, sp, k;
                int ir_, ig_, ib_;
                if (u2 >= 1.0f) continue;
                u = sqrtf(u2);
                /* slant path through the film; clamped so the rim fringes
                 * stay resolvable instead of aliasing into noise */
                path = 1.0f / sqrtf(1.0f - u2 + 0.055f);
                if (path > 3.1f) path = 3.1f;
                T = t0 * (0.34f + 0.66f * vy) * path;
                opd = 2.0f * 1.35f * T;              /* micrometres */
                ir_ = (int)(opd * (float)SQN / 0.650f) & (SQN - 1);
                ig_ = (int)(opd * (float)SQN / 0.550f) & (SQN - 1);
                ib_ = (int)(opd * (float)SQN / 0.450f) & (SQN - 1);
                vis = u2 * u2;                        /* Fresnel toward the rim */
                edge = (1.0f - u) * r;
                if (edge < 1.6f) vis *= edge * 0.625f;
                if (vis < 0.0f) vis = 0.0f;
                k = vis * 0.90f;
                /* specular pip */
                {
                    float sx = ((float)x - spx), sy = ((float)y - spy);
                    float s2 = (sx * sx + sy * sy) * ir * ir * 26.0f;
                    sp = s2 < 8.0f ? 0.55f * expf(-s2) : 0.0f;
                }
                row[x * 3 + 0] += (p138_sq[ir_] * k + sp) * pr;
                row[x * 3 + 1] += (p138_sq[ig_] * k + sp) * pg;
                row[x * 3 + 2] += (p138_sq[ib_] * k + sp) * pb;
            }
        }
    }

    p138_bloom();
    p138_resolve();
    p138_upscale(fb, w, h);
}
