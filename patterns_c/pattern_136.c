/* 136 Dipole Lines — traced field lines of drifting charges.
 *
 * Six point charges of alternating sign drift on slow, mutually detuned
 * Lissajous orbits.  From every positive charge 44 seeds are released on a
 * small circle and integrated along the true 2D field E = sum q(p-c)/|p-c|^2
 * with a fixed arc-length step, so the streamlines are equal-flux field
 * lines: they crowd where the field is strong, splay where it is weak, and
 * terminate on negative charges or run off the frame.  Nothing is drawn
 * except the lines themselves and a small glow at each charge, so the
 * picture is the geometry of the field and nothing else -- the classic
 * physics-plate look, in motion.  Hue tracks log|E| along the line, which
 * makes each filament shade continuously from its hot origin to its cool
 * far field.  Brightness also decays with arc length, which has a second
 * purpose: when the drifting charges cross a separatrix and a line
 * re-attaches to a different sink, only its dim far end moves, so the
 * topology change is invisible.  Sparse line art on black: an overlay. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 384
#define AH 288
#define AN (AW * AH)
#define NQ 6
#define NSEED 44
#define NSTEP 300

static float p136_acc[AN * 3];
static float p136_blur[AN * 3];
static uint8_t p136_img[AN * 3];
static uint8_t p136_tone[2048];
static float p136_hue[512][3];
static float p136_qx[NQ], p136_qy[NQ], p136_qq[NQ];
static int p136_ready;
static int p136_uw = -1;
static int *p136_uxi;
static uint8_t *p136_ufx;

static void p136_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (5.0f / 2048.0f)));
        p136_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    p136_ready = 1;
}

static void p136_build_hue(const uint32_t *pal)
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
        p136_hue[i][0] = 0.10f + 0.90f * r * m;
        p136_hue[i][1] = 0.10f + 0.90f * g * m;
        p136_hue[i][2] = 0.10f + 0.90f * b * m;
    }
}

static void p136_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx, fy, w00, w10, w01, w11;
    if ((unsigned)xi >= AW - 1 || (unsigned)yi >= AH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy) * wgt; w10 = fx * (1.0f - fy) * wgt;
    w01 = (1.0f - fx) * fy * wgt;          w11 = fx * fy * wgt;
    {
        float *a = p136_acc + ((size_t)yi * AW + xi) * 3;
        float *b = a + 3, *d = a + AW * 3, *e = d + 3;
        a[0] += c[0] * w00; a[1] += c[1] * w00; a[2] += c[2] * w00;
        b[0] += c[0] * w10; b[1] += c[1] * w10; b[2] += c[2] * w10;
        d[0] += c[0] * w01; d[1] += c[1] * w01; d[2] += c[2] * w01;
        e[0] += c[0] * w11; e[1] += c[1] * w11; e[2] += c[2] * w11;
    }
}

static void p136_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p136_acc + (size_t)y * AW * 3;
        float *out = p136_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p136_blur + (size_t)ym * AW * 3;
        const float *b = p136_blur + (size_t)y * AW * 3;
        const float *c = p136_blur + (size_t)yp * AW * 3;
        float *out = p136_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] += 0.80f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p136_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p136_acc[i] * 640.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p136_img[i] = p136_tone[t];
    }
}

static void p136_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p136_uw) {
        free(p136_uxi); free(p136_ufx);
        p136_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p136_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p136_uxi[x] = xi * 3;
            p136_ufx[x] = (uint8_t)(q & 255);
        }
        p136_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p136_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p136_uxi[x], fx = p136_ufx[x], c[3];
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

void pattern_136(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float hd = t * 0.00019f + sd;
    int i, j, s;
    (void)sl;

    if (!p136_ready) p136_tabs();
    p136_build_hue(pal);
    memset(p136_acc, 0, sizeof p136_acc);

    for (i = 0; i < NQ; i++) {
        float ph = (float)i * 1.0471976f + sd * 6.2831f;
        p136_qx[i] = (float)AW * (0.5f + 0.335f * sinf(t * 0.00058f + ph)
                                       + 0.085f * sinf(t * 0.00131f + ph * 2.3f));
        p136_qy[i] = (float)AH * (0.5f + 0.330f * sinf(t * 0.00047f + ph * 1.7f)
                                       + 0.075f * sinf(t * 0.00107f + ph * 0.9f));
        p136_qq[i] = (i & 1) ? -1.0f : 1.0f;
        if (i == 5) p136_qq[i] = -1.35f;
    }

    for (i = 0; i < NQ; i++) {
        if (p136_qq[i] <= 0.0f) continue;
        for (j = 0; j < NSEED; j++) {
            float a = (float)j * (6.2831853f / (float)NSEED)
                    + t * 0.00040f + (float)i * 0.37f;
            float px = p136_qx[i] + 3.2f * cosf(a);
            float py = p136_qy[i] + 3.2f * sinf(a);
            for (s = 0; s < NSTEP; s++) {
                float ex = 0.0f, ey = 0.0f, e2, ie, b, sink = 1e30f;
                float c[3];
                int hi, q;
                for (q = 0; q < NQ; q++) {
                    float dx = px - p136_qx[q], dy = py - p136_qy[q];
                    float d2 = dx * dx + dy * dy;
                    float r2 = d2 + 1.6f;
                    float k = p136_qq[q] / r2;
                    ex += dx * k; ey += dy * k;
                    if (p136_qq[q] < 0.0f && d2 < sink) sink = d2;
                }
                /* stop on arrival at a sink: without this the line stalls at
                 * the charge and stacks hundreds of splats on one pixel. */
                if (sink < 26.0f) break;
                e2 = ex * ex + ey * ey;
                if (e2 < 1e-9f) break;
                ie = 1.0f / sqrtf(e2);
                /* hue by field strength: hot at the source, cool far out */
                hi = (int)((0.5f + 0.075f * logf(e2 + 1e-8f) + hd) * 512.0f) & 511;
                b = (1.0f - (float)s / (float)NSTEP);
                b = b * b * 0.20f + 0.085f;
                c[0] = p136_hue[hi][0] * b;
                c[1] = p136_hue[hi][1] * b;
                c[2] = p136_hue[hi][2] * b;
                p136_splat(px, py, c, 0.55f);
                px += ex * ie * 1.15f;
                py += ey * ie * 1.15f;
                if (px < -12.0f || px > AW + 12.0f ||
                    py < -12.0f || py > AH + 12.0f) break;
            }
        }
    }

    /* charge cores */
    for (i = 0; i < NQ; i++) {
        int hi = (int)((0.5f + (p136_qq[i] > 0.0f ? 0.16f : -0.16f) + hd)
                       * 512.0f) & 511;
        float c[3];
        int k;
        c[0] = p136_hue[hi][0] * 0.75f;
        c[1] = p136_hue[hi][1] * 0.75f;
        c[2] = p136_hue[hi][2] * 0.75f;
        for (k = 0; k < 14; k++) {
            float a = (float)k * (6.2831853f / 14.0f);
            p136_splat(p136_qx[i] + 1.7f * cosf(a),
                       p136_qy[i] + 1.7f * sinf(a), c, 0.5f);
        }
        p136_splat(p136_qx[i], p136_qy[i], c, 0.7f);
    }

    p136_bloom();
    p136_resolve();
    p136_upscale(fb, w, h);
}
