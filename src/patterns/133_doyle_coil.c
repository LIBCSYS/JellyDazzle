/* 133 Doyle Coil — a logarithmic-spiral circle packing, breathing.
 *
 * A hexagonal circle packing can be bent into a logarithmic spiral, and
 * then the tangencies survive exactly if two conditions hold at once.  Put
 * q circles in a ring at radius R, each of radius sR: neighbours in the
 * ring touch iff s = sin(pi/q).  Advance to the next ring by rotating phi
 * and scaling by lambda; those two rings touch iff
 *   1 + lambda^2 - 2.lambda.cos(phi) = s^2 (1 + lambda)^2,
 * which is a quadratic in lambda with the closed-form root
 *   lambda = (B + sqrt(B^2 - A^2)) / A,  A = 1 - s^2,  B = cos(phi) + s^2.
 * So the whole packing -- every centre and every radius, out to any depth --
 * follows from the single free parameter phi, and phi is animated.  As it
 * runs toward its limit acos(1 - 2s^2) the growth ratio lambda falls to 1
 * and the spiral relaxes into flat concentric rings; as it runs down, the
 * coil tightens and the circles balloon outward.  The packing stays exactly
 * tangent through the whole sweep, which is what makes the motion read as a
 * physical mechanism rather than an animation.
 *
 * Drawn as rims only, hue keyed to log radius so each generation of the
 * spiral takes its own colour band, with a travelling wave in the same
 * variable lighting the generations in turn and smooth fades at both size
 * limits so nothing pops in or out.  ~90% near-black: an overlay layer. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AW 384
#define AH 288
#define AN (AW * AH)

static float p133_acc[AN * 3];
static float p133_blur[AN * 3];
static uint8_t p133_img[AN * 3];
static uint8_t p133_tone[2048];
static float p133_hue[512][3];
static int p133_ready;
static int p133_uw = -1;
static int *p133_uxi;
static uint8_t *p133_ufx;

static void p133_tabs(void)
{
    int i;
    for (i = 0; i < 2048; i++) {
        float v = 255.0f * (1.0f - expf(-(float)i * (5.6f / 2048.0f)));
        p133_tone[i] = (uint8_t)(v > 255.0f ? 255.0f : v);
    }
    p133_ready = 1;
}

static void p133_build_hue(const uint32_t *pal)
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
        p133_hue[i][0] = 0.08f + 0.92f * r * m;
        p133_hue[i][1] = 0.08f + 0.92f * g * m;
        p133_hue[i][2] = 0.08f + 0.92f * b * m;
    }
}

static void p133_splat(float x, float y, const float *c, float wgt)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx, fy, w00, w10, w01, w11;
    if ((unsigned)xi >= AW - 1 || (unsigned)yi >= AH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy) * wgt; w10 = fx * (1.0f - fy) * wgt;
    w01 = (1.0f - fx) * fy * wgt;          w11 = fx * fy * wgt;
    {
        float *a = p133_acc + ((size_t)yi * AW + xi) * 3;
        float *b = a + 3, *d = a + AW * 3, *e = d + 3;
        a[0] += c[0] * w00; a[1] += c[1] * w00; a[2] += c[2] * w00;
        b[0] += c[0] * w10; b[1] += c[1] * w10; b[2] += c[2] * w10;
        d[0] += c[0] * w01; d[1] += c[1] * w01; d[2] += c[2] * w01;
        e[0] += c[0] * w11; e[1] += c[1] * w11; e[2] += c[2] * w11;
    }
}

static void p133_ring(float cx, float cy, float rr, const float *c, float b)
{
    int n = (int)(rr * 7.0f), j;
    float col[3], step, wgt;
    if (rr < 0.55f) return;
    if (n < 10) n = 10;
    if (n > 2600) n = 2600;
    wgt = 0.62f * (rr * 7.0f / (float)n);
    col[0] = c[0] * b; col[1] = c[1] * b; col[2] = c[2] * b;
    step = 6.2831853f / (float)n;
    for (j = 0; j < n; j++) {
        float a = (float)j * step;
        p133_splat(cx + rr * cosf(a), cy + rr * sinf(a), col, wgt);
    }
}

static void p133_bloom(void)
{
    int x, y, k;
    for (y = 0; y < AH; y++) {
        float *row = p133_acc + (size_t)y * AW * 3;
        float *out = p133_blur + (size_t)y * AW * 3;
        for (x = 0; x < AW; x++) {
            int xm = x > 0 ? x - 1 : 0, xp = x < AW - 1 ? x + 1 : AW - 1;
            for (k = 0; k < 3; k++)
                out[x * 3 + k] = 0.5f * row[x * 3 + k]
                               + 0.25f * (row[xm * 3 + k] + row[xp * 3 + k]);
        }
    }
    for (y = 0; y < AH; y++) {
        int ym = y > 0 ? y - 1 : 0, yp = y < AH - 1 ? y + 1 : AH - 1;
        const float *a = p133_blur + (size_t)ym * AW * 3;
        const float *b = p133_blur + (size_t)y * AW * 3;
        const float *c = p133_blur + (size_t)yp * AW * 3;
        float *out = p133_acc + (size_t)y * AW * 3;
        for (x = 0; x < AW * 3; x++)
            out[x] += 0.85f * (0.5f * b[x] + 0.25f * (a[x] + c[x]));
    }
}

static void p133_resolve(void)
{
    int i;
    for (i = 0; i < AN * 3; i++) {
        int t = (int)(p133_acc[i] * 560.0f);
        if (t < 0) t = 0;
        if (t > 2047) t = 2047;
        p133_img[i] = p133_tone[t];
    }
}

static void p133_upscale(uint32_t *fb, int w, int h)
{
    int x, y, k;
    if (w != p133_uw) {
        free(p133_uxi); free(p133_ufx);
        p133_uxi = (int *)malloc(sizeof(int) * (size_t)w);
        p133_ufx = (uint8_t *)malloc((size_t)w);
        for (x = 0; x < w; x++) {
            long long q = ((long long)x * (AW - 1) * 256) / (w > 1 ? w - 1 : 1);
            int xi = (int)(q >> 8);
            if (xi > AW - 2) { xi = AW - 2; q = (long long)(AW - 1) * 256; }
            p133_uxi[x] = xi * 3;
            p133_ufx[x] = (uint8_t)(q & 255);
        }
        p133_uw = w;
    }
    for (y = 0; y < h; y++) {
        long long qy = ((long long)y * (AH - 1) * 256) / (h > 1 ? h - 1 : 1);
        int yi = (int)(qy >> 8), fy;
        const uint8_t *r0, *r1;
        uint32_t *out;
        if (yi > AH - 2) { yi = AH - 2; qy = (long long)(AH - 1) * 256; }
        fy = (int)(qy & 255);
        r0 = p133_img + (size_t)yi * AW * 3; r1 = r0 + AW * 3;
        out = fb + (size_t)y * (size_t)w;
        for (x = 0; x < w; x++) {
            int X = p133_uxi[x], fx = p133_ufx[x], c[3];
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

void pattern_133(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame & 0xFFFFF);
    float sd = (float)(seed & 511) * (1.0f / 512.0f);
    float s, s2, A, B, phimax, phi, lam, lnlam, spin, ox, oy, hd, wv;
    float rmax, rmin, r0;
    int q, i, j, i0, i1;
    (void)sl;

    if (!p133_ready) p133_tabs();
    p133_build_hue(pal);
    memset(p133_acc, 0, sizeof p133_acc);

    q = 7 + (int)(seed % 5u);                       /* 7..11 arms */
    s = sinf(3.14159265f / (float)q);
    s2 = s * s;
    A = 1.0f - s2;
    /* beyond phimax the two-ring tangency has no real solution */
    phimax = acosf(1.0f - 2.0f * s2);
    /* phimax works out to exactly 2.pi/q; past HALF of it the nearest
     * neighbour in the next ring switches to the other side and the circles
     * start overlapping, so the sweep is capped just under 0.5. */
    phi = phimax * (0.15f + 0.335f * (0.5f + 0.5f * sinf(t * 0.00026f + sd * 6.2831f)));
    B = cosf(phi) + s2;
    if (B < A) B = A;
    lam = (B + sqrtf(B * B - A * A)) / A;
    if (lam < 1.004f) lam = 1.004f;
    lnlam = logf(lam);

    spin = t * 0.00031f + sd * 6.2831f;
    ox = (float)AW * 0.5f;
    oy = (float)AH * 0.5f;
    hd = t * 0.00019f + sd;
    wv = t * 0.0044f;

    /* choose the ring range from the visible size band, so the packing is
     * never truncated and never drawn where it cannot be seen */
    r0 = (float)AH * 0.14f;
    rmin = 0.75f / s;
    rmax = (float)AH * 1.45f;
    i0 = (int)ceilf(logf(rmin / r0) / lnlam) - 1;
    i1 = (int)floorf(logf(rmax / r0) / lnlam) + 1;
    if (i1 - i0 > 240) i1 = i0 + 240;

    for (i = i0; i <= i1; i++) {
        float R = r0 * expf((float)i * lnlam);
        float rr = s * R;
        float base = i * phi + spin;
        float lr = logf(R);
        float fade, pulse, b;
        int hi;
        if (rr < 0.55f) continue;
        /* smooth fades at both ends of the size band: nothing pops */
        fade = rr < 2.4f ? (rr - 0.55f) * (1.0f / 1.85f) : 1.0f;
        if (R > (float)AH * 0.80f) {
            float u = (R - (float)AH * 0.80f) / ((float)AH * 0.60f);
            fade *= u > 1.0f ? 0.0f : 1.0f - u;
        }
        if (fade <= 0.0f) continue;
        pulse = 0.55f + 0.45f * sinf(lr * 2.1f - wv);
        b = fade * pulse * (0.42f + 0.72f / (1.0f + rr * 0.16f));
        hi = (int)((lr * 0.115f + hd) * 512.0f) & 511;
        for (j = 0; j < q; j++) {
            float a = base + (float)j * (6.2831853f / (float)q);
            p133_ring(ox + R * cosf(a), oy + R * sinf(a), rr, p133_hue[hi], b);
        }
    }

    p133_bloom();
    p133_resolve();
    p133_upscale(fb, w, h);
}
