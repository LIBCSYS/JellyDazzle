/* 642 Clifford Dust — Clifford / de Jong strange attractors as coloured silk.
 *   Clifford  x' = sin(a y) + c cos(a x)    y' = sin(b x) + d cos(b y)
 *   de Jong   x' = sin(a y) - cos(b x)      y' = sin(c x) - cos(d y)
 * 125 and 142 colour their clouds by DENSITY, one ramp lookup per cell. This
 * one colours by the STRAND: every iterate is tinted by the direction of the
 * step that landed it, and the cell shows the average tint of everything that
 * fell there, so neighbouring filaments that cross at an angle come out in
 * different colours — the fold structure reads like woven silk rather than
 * smoke. Brightness is the log-density display of fractal flames,
 * (log(1+n)/log(1+max))^(1/gamma), so the faint outer veils survive next to
 * the caustic folds.
 * 2048 walkers keep their positions from frame to frame and take 56 steps
 * each; the field decays 8% a frame instead of clearing, so the picture is a
 * ~12-frame exposure and never flickers. The four coefficients breathe on
 * slow incommensurate sines around a CURATED set (drifting freely through
 * parameter space mostly lands on fixed points and dead dust). Each set was
 * swept across its whole drift range and kept only if it never collapses into
 * a cycle or a dense core in a sparse halo: two Clifford sets failed that.
 * The seed picks the set (Clifford or de Jong), the drift phases, the view
 * tilt and its sway direction, the ramp start and how much of the ramp the
 * strands span. Mostly dark: it screens cleanly over a ground. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define P642_GW   480
#define P642_GH   300
#define P642_N    (P642_GW * P642_GH)
#define P642_NW   2048
#define P642_STEP 48
#define P642_SKIP 48            /* a restarted walker is off-attractor: no ink
                                 * (some sets attract weakly; 20 sprayed dust) */

/* curated stable sets: { kind (0 Clifford, 1 de Jong), a, b, c, d } */
static const float P642_SET[][5] = {
    { 0, -1.4f,  1.6f,  1.0f,  0.7f }, { 0,  1.7f,  1.7f, 0.06f,  1.2f },
    { 0,  1.5f, -1.8f,  1.6f,  0.9f },
    { 0, -1.24458f, -1.25191f, -1.81590f, -1.90866f },
    { 1,  1.4f, -2.3f,  2.4f, -2.1f }, { 1,  2.01f, -2.53f, 1.61f, -0.33f },
    { 1, -2.7f, -0.09f, -0.86f, -2.2f }, { 1, -0.827f, -1.637f, 1.659f, -0.943f },
    { 1, -2.24f,  0.43f, -0.65f, -2.43f },
};
#define P642_NSET ((int)(sizeof P642_SET / sizeof P642_SET[0]))

static float    p642_d[P642_N], p642_r[P642_N], p642_g[P642_N], p642_b[P642_N];
static uint8_t  p642_img[P642_N * 3];
static float    p642_wx[P642_NW], p642_wy[P642_NW];
static uint8_t  p642_wskip[P642_NW];
static float    p642_sin[4096];
static uint32_t p642_lut[256];
static uint32_t p642_seed = 0xFFFFFFFFu;
static uint32_t p642_rs = 1u;
static int      p642_ready, p642_set, p642_base, p642_span;
static float    p642_ph[5], p642_tilt, p642_sway, p642_max;
static float    p642_zoom, p642_cu, p642_cv;   /* eased view fit */
static int      p642_uw = -1, *p642_xi;
static uint8_t *p642_fx;

static uint32_t p642_rnd(void)
{
    p642_rs ^= p642_rs << 13; p642_rs ^= p642_rs >> 17; p642_rs ^= p642_rs << 5;
    return p642_rs;
}
static float p642_r01(void) { return (float)(p642_rnd() >> 8) * (1.0f / 16777216.0f); }
static uint32_t p642_h(uint32_t x)
{
    x ^= x >> 16; x *= 0x85ebca6bu; x ^= x >> 13; x *= 0xc2b2ae35u; x ^= x >> 16;
    return x;
}
/* ln(x) for x >= 1 to ~1%: exponent bits + a quadratic on the mantissa.
 * Two logf per cell per frame were most of this pattern's budget. */
static inline float p642_ln(float x)
{
    union { float f; uint32_t u; } v = { x };
    float e = (float)((int)(v.u >> 23) - 127);
    v.u = (v.u & 0x007FFFFFu) | 0x3F800000u;               /* mantissa in [1,2) */
    float m = v.f;
    return (e + (-0.34484843f * m + 2.02466578f) * m - 1.67487759f) * 0.69314718f;
}
static inline float p642_s(float a) { return p642_sin[((int)(a * 651.8986f + 65536.5f)) & 4095]; }
static inline float p642_c(float a) { return p642_sin[((int)(a * 651.8986f + 65536.5f) + 1024) & 4095]; }

/* drifting coefficients: the curated set +/- a few percent, never far enough
 * to fall off the attractor */
static void p642_coef(float t, float *k)
{
    const float *S = P642_SET[p642_set];
    for (int i = 0; i < 4; i++)
        k[i] = S[i + 1] + 0.045f * sinf(t * (0.00041f + 0.00013f * (float)i) + p642_ph[i]);
}

static int p642_fcmp(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;
    return (x > y) - (x < y);
}

static void p642_walker(int i)
{
    p642_wx[i] = p642_r01() * 2.0f - 1.0f;
    p642_wy[i] = p642_r01() * 2.0f - 1.0f;
    p642_wskip[i] = P642_SKIP;
}

static void p642_run(float t, int steps, float ax)
{
    float k[4]; p642_coef(t, k);
    const float a = k[0], b = k[1], c = k[2], d = k[3];
    const int dj = P642_SET[p642_set][0] != 0.0f;
    float ang = p642_tilt + 0.33f * sinf(t * 0.00057f * p642_sway + p642_ph[4]);
    float ca = cosf(ang), sa = sinf(ang);
    /* Fit the view to the cloud the walkers actually trace, not the map's
     * theoretical bounds (those left most sets filling a third of the frame).
     * Eased 2% a frame, so as the folds open and close the camera follows
     * instead of jumping. ax keeps the cloud round whatever the screen shape. */
    {
        static float uu[P642_NW / 4], vv[P642_NW / 4];
        int n = 0;
        for (int i = 0; i < P642_NW; i += 4) {
            if (p642_wskip[i]) continue;
            uu[n] = p642_wx[i] * ca - p642_wy[i] * sa;
            vv[n] = p642_wx[i] * sa + p642_wy[i] * ca;
            n++;
        }
        if (n >= 64) {
            /* 3rd..97th percentile: a clump of walkers or a straggler on a
             * long transient must not decide where the camera points */
            qsort(uu, (size_t)n, sizeof *uu, p642_fcmp);
            qsort(vv, (size_t)n, sizeof *vv, p642_fcmp);
            int lo = n * 3 / 100, hi = n - 1 - lo;
            float du = uu[hi] - uu[lo], dv = vv[hi] - vv[lo];
            if (du > 1e-3f && dv > 1e-3f) {
                float tz = fminf((float)P642_GW * 0.74f / (du * ax), (float)P642_GH * 0.74f / dv);
                float cu = 0.5f * (uu[lo] + uu[hi]), cv = 0.5f * (vv[lo] + vv[hi]);
                if (p642_zoom == 0.0f) { p642_zoom = tz; p642_cu = cu; p642_cv = cv; }
                else {
                    /* back out fast when the cloud outgrows the frame, close in slowly */
                    float k = tz < p642_zoom ? 0.10f : 0.015f;
                    p642_zoom += (tz - p642_zoom) * k;
                    p642_cu += (cu - p642_cu) * 0.03f; p642_cv += (cv - p642_cv) * 0.03f;
                }
            }
        }
    }
    const float sx = p642_zoom * ax, sy = p642_zoom;
    const float ox = P642_GW * 0.5f - p642_cu * sx, oy = P642_GH * 0.5f - p642_cv * sy;

    for (int i = 0; i < P642_NW; i++) {
        float x = p642_wx[i], y = p642_wy[i];
        int skip = p642_wskip[i];
        for (int s = 0; s < steps; s++) {
            float nx, ny;
            if (dj) { nx = p642_s(a * y) - p642_c(b * x); ny = p642_s(c * x) - p642_c(d * y); }
            else    { nx = p642_s(a * y) + c * p642_c(a * x); ny = p642_s(b * x) + d * p642_c(b * y); }
            float dx = nx - x, dy = ny - y;
            x = nx; y = ny;
            /* a walker parked on a fixed point would pile ink into one cell */
            if (fabsf(dx) + fabsf(dy) < 1e-4f) {
                x = p642_r01() * 2.0f - 1.0f; y = p642_r01() * 2.0f - 1.0f;
                skip = P642_SKIP; continue;
            }
            if (skip) { skip--; continue; }
            int xi = (int)((x * ca - y * sa) * sx + ox), yi = (int)((x * sa + y * ca) * sy + oy);
            if ((unsigned)xi >= P642_GW || (unsigned)yi >= P642_GH) continue;
            /* pseudo-angle of the step, 0..4 around the circle, no atan2 */
            float m = fabsf(dx) + fabsf(dy) + 1e-6f;
            float pa = dy / m;
            pa = dx < 0.0f ? 2.0f - pa : (pa < 0.0f ? pa + 4.0f : pa);
            uint32_t col = p642_lut[(int)(pa * 63.99f) & 255];
            size_t o = (size_t)yi * P642_GW + (size_t)xi;
            p642_d[o] += 1.0f;
            p642_r[o] += (float)((col >> 16) & 255u);
            p642_g[o] += (float)((col >> 8) & 255u);
            p642_b[o] += (float)(col & 255u);
        }
        p642_wx[i] = x; p642_wy[i] = y; p642_wskip[i] = (uint8_t)skip;
    }
}

static void p642_upscale(uint32_t *fb, int w, int h)
{
    if (w != p642_uw) {
        free(p642_xi); free(p642_fx);
        p642_xi = (int *)malloc(sizeof(int) * (size_t)w);
        p642_fx = (uint8_t *)malloc((size_t)w);
        for (int x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (P642_GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8;
            if (xi > P642_GW - 2) { xi = P642_GW - 2; q = (P642_GW - 1) * 256; }
            p642_xi[x] = xi * 3; p642_fx[x] = (uint8_t)(q & 255);
        }
        p642_uw = w;
    }
    for (int y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (P642_GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8;
        if (yi > P642_GH - 2) { yi = P642_GH - 2; qy = (P642_GH - 1) * 256; }
        int fy = qy & 255;
        const uint8_t *r0 = p642_img + (size_t)yi * P642_GW * 3, *r1 = r0 + P642_GW * 3;
        uint32_t *out = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            int X = p642_xi[x], fx = p642_fx[x], cc[3];
            for (int k = 0; k < 3; k++) {
                int t0 = r0[X + k] + (((r0[X + 3 + k] - r0[X + k]) * fx) >> 8);
                int t1 = r1[X + k] + (((r1[X + 3 + k] - r1[X + k]) * fx) >> 8);
                cc[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)cc[0] << 16) | ((uint32_t)cc[1] << 8) | (uint32_t)cc[2];
        }
    }
}

void pattern_642(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (!p642_ready) {
        for (int i = 0; i < 4096; i++) p642_sin[i] = sinf((float)i * (6.28318531f / 4096.0f));
        p642_ready = 1;
    }
    const float t = (float)frame;
    if (seed != p642_seed) {
        uint32_t s = p642_h(seed ^ 0x642C1F0Du);
        p642_set  = (int)(s % (uint32_t)P642_NSET);
        p642_base = (int)((s >> 4) & JD_PAL_MASK);                 /* ramp start */
        p642_span = 6000 + (int)((s >> 19) % 9000u);               /* strand hue spread */
        uint32_t r = p642_h(s);
        for (int i = 0; i < 5; i++) { p642_ph[i] = (float)(r & 1023u) * 0.006136f; r = p642_h(r); }
        p642_tilt = ((float)(r & 255u) / 255.0f - 0.5f) * 0.04f;
        p642_sway = (r & 256u) ? 1.0f : -1.0f;
        p642_rs = (s | 1u);
        for (int i = 0; i < P642_NW; i++) p642_walker(i);
        memset(p642_d, 0, sizeof p642_d); memset(p642_r, 0, sizeof p642_r);
        memset(p642_g, 0, sizeof p642_g); memset(p642_b, 0, sizeof p642_b);
        p642_max = 0.0f; p642_zoom = 0.0f;
        p642_seed = seed;
    }

    /* strand tints from the live ramp, the brightest channel lifted to 190 so
     * a dark stretch of the ramp never inks the silk black */
    for (int k = 0; k < 256; k++) {
        uint32_t c = pal[(uint32_t)(p642_base + ((k * p642_span) >> 8)) & JD_PAL_MASK];
        int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
        int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
        if (mx < 190) {
            int f = mx > 8 ? (190 * 256) / mx : 0;
            if (f) { r = r * f >> 8; g = g * f >> 8; b = b * f >> 8; }
            else   { r = g = b = 190; }
        }
        p642_lut[k] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    for (int k = 0; k < 6; k++) p642_walker((int)(p642_rnd() % P642_NW));
    /* the first frame of a tenancy exposes longer, so it opens on the cloud */
    /* grid cells are not square on screen unless the screen is 16:10 */
    const float ax = ((float)P642_GW / (float)P642_GH) / ((float)w / (float)(h > 0 ? h : 1));
    if (p642_zoom == 0.0f) p642_run(t, P642_SKIP + 2, ax);   /* settle walkers so the first fit sees the attractor */
    p642_run(t, p642_max == 0.0f ? P642_STEP * 2 : P642_STEP, ax);

    /* Normalise against the 99.5th percentile of the lit cells, not the max:
     * some sets also hold a short cycle beside the attractor, and walkers
     * caught in it pile one cell so high that the max blacks out the silk. */
    float mx = 1.0f;
    for (int i = 0; i < P642_N; i++) if (p642_d[i] > mx) mx = p642_d[i];
    {
        static uint32_t hist[256];
        memset(hist, 0, sizeof hist);
        float sc = 254.0f / p642_ln(1.0f + mx);
        uint32_t lit = 0;
        for (int i = 0; i < P642_N; i += 3)                /* a third of the cells is plenty */
            if (p642_d[i] >= 0.5f) { hist[(int)(p642_ln(1.0f + p642_d[i]) * sc)]++; lit++; }
        uint32_t want = lit / 200, acc = 0;
        int k = 255;
        while (k > 0 && acc + hist[k] <= want) acc += hist[k--];
        float pc = expf((float)k / sc) - 1.0f;
        if (pc > 1.0f) mx = pc;
    }
    /* smoothed: the normalisation must not pump with the folds */
    p642_max = p642_max == 0.0f ? mx : p642_max * 0.96f + mx * 0.04f;
    const float il = 1.0f / p642_ln(1.0f + p642_max);
    uint8_t *o = p642_img;
    for (int i = 0; i < P642_N; i++) {
        float d = p642_d[i];
        if (d < 0.02f) { o[0] = o[1] = o[2] = 0; o += 3; p642_d[i] = 0.0f; continue; }
        float v = p642_ln(1.0f + d) * il;
        if (v > 1.0f) v = 1.0f;
        v = sqrtf(v) * (255.0f / 190.0f) / d;            /* gamma 2, mean tint */
        float r = p642_r[i] * v, g = p642_g[i] * v, b = p642_b[i] * v;
        o[0] = (uint8_t)(r > 255.0f ? 255 : (int)r);
        o[1] = (uint8_t)(g > 255.0f ? 255 : (int)g);
        o[2] = (uint8_t)(b > 255.0f ? 255 : (int)b);
        o += 3;
        /* the decay, folded into the display pass: one walk over the field */
        p642_d[i] = d * 0.92f; p642_r[i] *= 0.92f; p642_g[i] *= 0.92f; p642_b[i] *= 0.92f;
    }
    p642_upscale(fb, w, h);
}
