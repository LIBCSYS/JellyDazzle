/* 161 Curve Loom — a space-filling curve morphing between three orderings.
 * One 1024-point polyline visits every cell of a 32x32 grid. Three different
 * visit orders are held at once — the Hilbert curve, a boustrophedon snake and
 * an inward spiral — and each vertex is a weighted blend of its position in all
 * three, so the wire continuously unpicks a fractal into a serpentine into a
 * coil and back with no cut anywhere. A brightness pulse runs along the curve
 * by index, which reads as light travelling through a single unbroken filament.
 * Everything off the wire is black: a sparse line-art overlay.
 * The plate is deliberately never cleared on an sl discontinuity: it refreshes
 * itself within ~30 frames, so clearing it would only add a hard cut. */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static jd_up p161_up;

#define P161_LW 480
#define P161_LH 360
#define P161_G  32
#define P161_N  (P161_G * P161_G)

static uint8_t p161_img[P161_LW * P161_LH * 3];
static float   p161_acc[P161_LW * P161_LH * 3];
static int    *p161_xm;
static int     p161_xm_w;
static uint8_t p161_ramp[256][3];
static float   p161_cx[3][P161_N], p161_cy[3][P161_N];
static float   p161_bx[P161_N], p161_by[P161_N];
static int     p161_ready;

static void p161_build_ramp(const uint32_t *pal, int base, int span)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(base + i * span) & JD_PAL_MASK];
        int r = (u >> 16) & 255, g = (u >> 8) & 255, b = u & 255;
        int mx = r > g ? r : g; if (b > mx) mx = b;
        if (mx < 6) {
            if (i) { p161_ramp[i][0] = p161_ramp[i-1][0];
                     p161_ramp[i][1] = p161_ramp[i-1][1];
                     p161_ramp[i][2] = p161_ramp[i-1][2]; }
            else   { p161_ramp[i][0] = p161_ramp[i][1] = p161_ramp[i][2] = 255; }
            continue;
        }
        p161_ramp[i][0] = (uint8_t)((r * 255) / mx);
        p161_ramp[i][1] = (uint8_t)((g * 255) / mx);
        p161_ramp[i][2] = (uint8_t)((b * 255) / mx);
    }
}

static void p161_blit(uint32_t *fb, int w, int h)
{
    int x;
    if (p161_xm_w != w) {
        free(p161_xm);
        p161_xm = (int *)malloc(sizeof(int) * (size_t)w);
        for (x = 0; x < w; x++)
            p161_xm[x] = (int)(((long long)x * (P161_LW - 1) << 8) / (w > 1 ? w - 1 : 1));
        p161_xm_w = w;
    }
    jd_up_blit(&p161_up, fb, w, h, p161_img, P161_LW, P161_LH);
}

static void p161_splat(float x, float y, float r, float g, float b)
{
    int xi = (int)x, yi = (int)y;
    float fx, fy, w00, w01, w10, w11;
    float *p;
    if (xi < 0 || yi < 0 || xi >= P161_LW - 1 || yi >= P161_LH - 1) return;
    fx = x - (float)xi; fy = y - (float)yi;
    w00 = (1.0f - fx) * (1.0f - fy); w01 = fx * (1.0f - fy);
    w10 = (1.0f - fx) * fy;          w11 = fx * fy;
    p = p161_acc + (yi * P161_LW + xi) * 3;
    p[0] += r * w00; p[1] += g * w00; p[2] += b * w00;
    p[3] += r * w01; p[4] += g * w01; p[5] += b * w01;
    p += P161_LW * 3;
    p[0] += r * w10; p[1] += g * w10; p[2] += b * w10;
    p[3] += r * w11; p[4] += g * w11; p[5] += b * w11;
}

/* a soft 5-tap stroke: bilinear core plus a cross of dim satellites, so the
 * wire survives the 2.7x upscale as a glowing filament instead of a hairline */
static void p161_stroke(float x, float y, float r, float g, float b)
{
    p161_splat(x, y, r, g, b);
    r *= 0.30f; g *= 0.30f; b *= 0.30f;
    p161_splat(x - 1.6f, y, r, g, b);
    p161_splat(x + 1.6f, y, r, g, b);
    p161_splat(x, y - 1.6f, r, g, b);
    p161_splat(x, y + 1.6f, r, g, b);
}

static void p161_tone(float decay)
{
    int i, n = P161_LW * P161_LH * 3;
    for (i = 0; i < n; i++) {
        float v = p161_acc[i];
        int c = (int)(v * 255.0f);
        p161_img[i] = c <= 0 ? 0 : c >= 255 ? 255 : (uint8_t)c;
        p161_acc[i] = v * decay;
    }
}

/* ---- the three visit orders ---- */
static void p161_hilbert(int d, int *ox, int *oy)
{
    int rx, ry, s, t = d, x = 0, y = 0, tmp;
    for (s = 1; s < P161_G; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        if (ry == 0) {
            if (rx == 1) { x = s - 1 - x; y = s - 1 - y; }
            tmp = x; x = y; y = tmp;
        }
        x += s * rx; y += s * ry;
        t /= 4;
    }
    *ox = x; *oy = y;
}

static void p161_build_curves(void)
{
    int i, x, y;
    /* 0: Hilbert */
    for (i = 0; i < P161_N; i++) {
        p161_hilbert(i, &x, &y);
        p161_cx[0][i] = ((float)x + 0.5f) / (float)P161_G * 2.0f - 1.0f;
        p161_cy[0][i] = ((float)y + 0.5f) / (float)P161_G * 2.0f - 1.0f;
    }
    /* 1: boustrophedon snake */
    for (i = 0; i < P161_N; i++) {
        y = i / P161_G; x = i % P161_G;
        if (y & 1) x = P161_G - 1 - x;
        p161_cx[1][i] = ((float)x + 0.5f) / (float)P161_G * 2.0f - 1.0f;
        p161_cy[1][i] = ((float)y + 0.5f) / (float)P161_G * 2.0f - 1.0f;
    }
    /* 2: inward spiral */
    {
        int lo = 0, hi = P161_G - 1, k = 0;
        while (lo <= hi) {
            for (x = lo; x <= hi; x++) { p161_cx[2][k] = (float)x; p161_cy[2][k] = (float)lo; k++; }
            for (y = lo + 1; y <= hi; y++) { p161_cx[2][k] = (float)hi; p161_cy[2][k] = (float)y; k++; }
            if (lo < hi) {
                for (x = hi - 1; x >= lo; x--) { p161_cx[2][k] = (float)x; p161_cy[2][k] = (float)hi; k++; }
                for (y = hi - 1; y > lo; y--) { p161_cx[2][k] = (float)lo; p161_cy[2][k] = (float)y; k++; }
            }
            lo++; hi--;
        }
        for (i = 0; i < P161_N; i++) {
            p161_cx[2][i] = (p161_cx[2][i] + 0.5f) / (float)P161_G * 2.0f - 1.0f;
            p161_cy[2][i] = (p161_cy[2][i] + 0.5f) / (float)P161_G * 2.0f - 1.0f;
        }
    }
    p161_ready = 1;
}

void pattern_161(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)(frame % 4194304);
    float wt[3], ph, rot, cs, sn, scale, pulse;
    float px = 0.0f, py = 0.0f;
    int i, k;

    (void)sl;
    if (!p161_ready) p161_build_curves();

    p161_build_ramp(pal, (int)(t * 2.1f) + (int)(seed & 32767), 96);

    /* three-way crossfade of the visit orders, C1-smooth and cyclic */
    ph = t * 0.00055f + (float)(seed & 255) * 0.012f;
    {
        float s = 0.0f;
        for (i = 0; i < 3; i++) {
            float d = ph - (float)i;
            d -= 3.0f * floorf(d * (1.0f / 3.0f));      /* wrap to [0,3) */
            if (d > 1.5f) d -= 3.0f;
            d = fabsf(d);
            wt[i] = d >= 1.0f ? 0.0f : (1.0f - d) * (1.0f - d) * (3.0f - 2.0f * (1.0f - d));
            s += wt[i];
        }
        if (s < 1e-4f) s = 1e-4f;
        for (i = 0; i < 3; i++) wt[i] /= s;
    }

    rot   = t * 0.00042f;
    cs    = cosf(rot); sn = sinf(rot);
    pulse = t * 0.0011f;

    /* blending three orderings contracts the figure toward the middle when the
     * weights are mixed; renormalise by RMS radius so it always fills the frame */
    {
        float s2 = 0.0f;
        for (i = 0; i < P161_N; i++) {
            float ux = wt[0] * p161_cx[0][i] + wt[1] * p161_cx[1][i] + wt[2] * p161_cx[2][i];
            float uy = wt[0] * p161_cy[0][i] + wt[1] * p161_cy[1][i] + wt[2] * p161_cy[2][i];
            p161_bx[i] = ux; p161_by[i] = uy;
            s2 += ux * ux + uy * uy;
        }
        s2 = sqrtf(s2 / (float)P161_N);
        if (s2 < 0.05f) s2 = 0.05f;
        scale = (0.582f / s2) * 148.0f * (1.0f + 0.045f * sinf(t * 0.00061f));
    }

    for (i = 0; i < P161_N; i++) {
        float ux = p161_bx[i], uy = p161_by[i];
        float qx = 240.0f + (ux * cs - uy * sn) * scale * 1.24f;
        float qy = 180.0f + (ux * sn + uy * cs) * scale * 0.99f;
        if (i) {
            float dx = qx - px, dy = qy - py;
            float len = sqrtf(dx * dx + dy * dy);
            int steps = (int)(len * 0.9f) + 1;
            float inv = 1.0f / (float)steps;
            float fi = (float)i * (1.0f / (float)P161_N);
            float band = 0.5f + 0.5f * sinf((fi - pulse) * 12.566371f);
            float glow = 0.42f + 0.58f * band * band * band;
            const uint8_t *cp = p161_ramp[((int)(fi * 300.0f + t * 0.07f)) & 255];
            float r = (float)cp[0] * (1.0f / 255.0f) * glow;
            float g = (float)cp[1] * (1.0f / 255.0f) * glow;
            float b = (float)cp[2] * (1.0f / 255.0f) * glow;
            float st = 0.30f;      /* per-sample deposit: samples are ~1.1 px
                                     * apart, so line brightness is uniform */
            for (k = 0; k < steps; k++) {
                float u = (float)k * inv;
                p161_stroke(px + dx * u, py + dy * u, r * st, g * st, b * st);
            }
        }
        px = qx; py = qy;
    }
    p161_tone(0.74f);
    p161_blit(fb, w, h);
}
