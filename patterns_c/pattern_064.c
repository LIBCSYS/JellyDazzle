/* 064 Starburst Forge — 24 molten rays bending out of a dark core, crossed by
 * copper rings racing inward. Port of
 * lab/patterns/064_starburst_forge/proto.py (repaint, ignores sl). */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

static int16_t p64_sin[1024];
static uint32_t p64_env[256];
static int     p64_tab_ok = 0;
static int     p64_w = 0, p64_h = 0;
static uint16_t *p64_a12;   /* 12*angle, 1024-unit turn scaled x16 */
static uint16_t *p64_dep;   /* depth * 64 */
static uint8_t  *p64_shd;   /* r/(r+42) * 63 */
static uint32_t p64_lut[64][256];

/* luminance envelope of the lab forge ramp: iron black -> ember -> gold ->
 * white-hot -> gold -> ember -> iron black. Colour still comes from pal. */
static void p64_build_env(void) {
    static const double xs[8] = { 0.00, 0.18, 0.36, 0.52, 0.62, 0.72, 0.86, 1.00 };
    static const double ys[8] = { 0.06, 0.30, 0.58, 0.86, 1.00, 0.86, 0.36, 0.06 };
    int i, j = 0;
    for (i = 0; i < 256; i++) {
        double u = i / 255.0, v;
        while (j < 6 && u > xs[j + 1]) j++;
        v = ys[j] + (ys[j + 1] - ys[j]) * (u - xs[j]) / (xs[j + 1] - xs[j]);
        p64_env[i] = (uint32_t)(v * 65536.0);
    }
}

static void p64_init(int w, int h) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (!p64_tab_ok) {
        for (i = 0; i < 1024; i++)
            p64_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
        p64_build_env();
        p64_tab_ok = 1;
    }
    if (p64_w == w && p64_h == h) return;
    free(p64_a12); free(p64_dep); free(p64_shd);
    p64_a12 = (uint16_t *)malloc((size_t)w * h * 2);
    p64_dep = (uint16_t *)malloc((size_t)w * h * 2);
    p64_shd = (uint8_t *)malloc((size_t)w * h);
    for (y = 0, i = 0; y < h; y++) {
        double py = (y - h * 0.5) * sy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx;
            double r = hypot(px, py) + 1e-3;
            double a = atan2(py, px);
            double depth = 2200.0 / (r + 12.0);
            double ph = fmod(a * 12.0 * (1024.0 / (2.0 * M_PI)) * 16.0, 16384.0);
            if (ph < 0) ph += 16384.0;
            p64_a12[i] = (uint16_t)ph;
            p64_dep[i] = (uint16_t)(depth * 64.0);
            p64_shd[i] = (uint8_t)(63.0 * r / (r + 42.0));
        }
    }
    p64_w = w; p64_h = h;
}

/* smooth 6-stop looping ramp lifted from the engine palette */
static void p64_build_lut(const uint32_t *pal) {
    int idx, s, j;
    int ar[7], ag[7], ab[7];
    for (j = 0; j < 6; j++) {
        uint32_t c = pal[(j * 5461) & JD_PAL_MASK];
        ar[j] = (c >> 16) & 255; ag[j] = (c >> 8) & 255; ab[j] = c & 255;
    }
    ar[6] = ar[0]; ag[6] = ag[0]; ab[6] = ab[0];
    for (idx = 0; idx < 256; idx++) {
        int q = idx * 6, j0 = q >> 8, f = q & 255;
        uint32_t e = p64_env[idx];
        uint32_t r = (uint32_t)(ar[j0] + (((ar[j0+1] - ar[j0]) * f) >> 8));
        uint32_t g = (uint32_t)(ag[j0] + (((ag[j0+1] - ag[j0]) * f) >> 8));
        uint32_t b = (uint32_t)(ab[j0] + (((ab[j0+1] - ab[j0]) * f) >> 8));
        r = (r * e) >> 16; g = (g * e) >> 16; b = (b * e) >> 16;
        for (s = 0; s < 64; s++) {
            uint32_t k = (uint32_t)(s * 65536 / 63);
            p64_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_064(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int i, n = w * h;
    double t = (double)frame;
    int sweep, bendt, ringt;
    (void)sl; (void)seed;
    p64_init(w, h);
    p64_build_lut(pal);
    /* x16 units of a 1024-step turn; 1 rad = 2607.6 units */
    sweep = (int)fmod(t * 0.004 * 2607.6, 16384.0);
    bendt = (int)fmod(t * 0.008 * 2607.6, 16384.0);
    ringt = 16384 - (int)fmod(t * 0.040 * 2607.6, 16384.0);
    for (i = 0; i < n; i++) {
        int dep = p64_dep[i];
        int bph = ((dep * 117) >> 6) + bendt;            /* depth*0.045 rad */
        int bend = (p64_sin[(bph >> 4) & 1023] * 1434) >> 15;   /* 0.55 rad */
        int rays = p64_sin[((p64_a12[i] + sweep + bend) >> 4) & 1023];
        int rph = ((dep * 1173) >> 6) + ringt;           /* depth*0.45 rad */
        int rngs = p64_sin[(rph >> 4) & 1023];
        int field = (rays * 79 + rngs * 49) >> 7;
        int idx = (((field * 118) >> 15) + 128) & 255;
        fb[i] = p64_lut[p64_shd[i]][idx];
    }
}
