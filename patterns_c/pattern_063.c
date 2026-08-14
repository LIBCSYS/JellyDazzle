/* 063 Spiral Zoom — two interlocked log-spirals (3-arm inward, 2-arm outward)
 * interfering into paisley whorls that pour into the center forever.
 * Port of lab/patterns/063_spiral_zoom/proto.py (repaint, ignores sl). */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

static int16_t p63_sin[1024];
static int     p63_tab_ok = 0;
static int     p63_w = 0, p63_h = 0;
static uint16_t *p63_b1;    /* static phase of spiral 1, 64ths of an idx unit */
static uint16_t *p63_b2;    /* static phase of spiral 2 */
static uint8_t  *p63_shd;   /* r/(r+26) * 63 */
static uint32_t p63_lut[64][256];

static uint32_t p63_env[256];
static void p63_build_env(void) {
    static const double xs[6] = { 0.00, 0.20, 0.38, 0.55, 0.75, 1.00 };
    static const double ys[6] = { 0.26, 0.58, 1.00, 0.74, 0.20, 0.26 };
    int i, j = 0;
    for (i = 0; i < 256; i++) {
        double u = i / 255.0, v;
        while (j < 4 && u > xs[j + 1]) j++;
        v = ys[j] + (ys[j + 1] - ys[j]) * (u - xs[j]) / (xs[j + 1] - xs[j]);
        p63_env[i] = (uint32_t)(v * 65536.0);
    }
}

static void p63_init(int w, int h) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (!p63_tab_ok) {
        for (i = 0; i < 1024; i++)
            p63_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
        p63_build_env();
        p63_tab_ok = 1;
    }
    if (p63_w == w && p63_h == h) return;
    free(p63_b1); free(p63_b2); free(p63_shd);
    p63_b1  = (uint16_t *)malloc((size_t)w * h * 2);
    p63_b2  = (uint16_t *)malloc((size_t)w * h * 2);
    p63_shd = (uint8_t *)malloc((size_t)w * h);
    for (y = 0, i = 0; y < h; y++) {
        double py = (y - h * 0.5) * sy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx;
            double r = hypot(px, py);
            double lr = log(r + 2.0) * 82.0;
            /* angle expressed in idx units (256 = full turn) */
            double an = (atan2(py, px) * (1.0 / (2.0 * M_PI)) + 0.5) * 256.0;
            double f1 = lr * 2.4 + an * 3.0;
            double f2 = lr * 1.1 - an * 2.0;
            double m1 = fmod(f1, 256.0); if (m1 < 0) m1 += 256.0;
            double m2 = fmod(f2, 256.0); if (m2 < 0) m2 += 256.0;
            p63_b1[i] = (uint16_t)(m1 * 64.0) & 16383;
            p63_b2[i] = (uint16_t)(m2 * 64.0) & 16383;
            p63_shd[i] = (uint8_t)(63.0 * r / (r + 26.0));
        }
    }
    p63_w = w; p63_h = h;
}

/* smooth 6-stop looping ramp lifted from the engine palette */
static void p63_build_lut(const uint32_t *pal) {
    int idx, s, j;
    int ar[7], ag[7], ab[7];
    for (j = 0; j < 6; j++) {
        uint32_t c = pal[(j * 5461) & JD_PAL_MASK];
        ar[j] = (c >> 16) & 255; ag[j] = (c >> 8) & 255; ab[j] = c & 255;
    }
    ar[6] = ar[0]; ag[6] = ag[0]; ab[6] = ab[0];
    for (idx = 0; idx < 256; idx++) {
        int q = idx * 6, j0 = q >> 8, f = q & 255;
        uint32_t r = (uint32_t)(ar[j0] + (((ar[j0+1] - ar[j0]) * f) >> 8));
        uint32_t g = (uint32_t)(ag[j0] + (((ag[j0+1] - ag[j0]) * f) >> 8));
        uint32_t b = (uint32_t)(ab[j0] + (((ab[j0+1] - ab[j0]) * f) >> 8));
        /* luminance envelope copied from the lab's royal ramp (purple ->
         * magenta -> gold -> teal -> midnight): colour comes from pal, the
         * light/dark shape comes from the proto so whorls read as inked. */
        uint32_t sh = p63_env[idx];
        r = (r * sh) >> 16; g = (g * sh) >> 16; b = (b * sh) >> 16;
        for (s = 0; s < 64; s++) {
            uint32_t k = (uint32_t)(s * 65536 / 63);
            p63_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_063(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int i, n = w * h, p1, p2;
    double t = (double)frame;
    (void)sl; (void)seed;
    p63_init(w, h);
    p63_build_lut(pal);
    p1 = 16384 - (int)fmod(t * 0.55 * 64.0, 16384.0);     /* zoom in  */
    p2 = (int)fmod(t * 0.25 * 64.0, 16384.0);             /* counter-spiral */
    for (i = 0; i < n; i++) {
        int i1 = ((p63_b1[i] + p1) & 16383) >> 4;
        int i2 = ((p63_b2[i] + p2) & 16383) >> 4;
        int s = p63_sin[i1] + p63_sin[i2];
        int idx = (((s * 63) >> 16) + 128) & 255;
        fb[i] = p63_lut[p63_shd[i]][idx];
    }
}
