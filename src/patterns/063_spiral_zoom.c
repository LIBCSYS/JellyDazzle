/* 063 Spiral Zoom — two interlocked log-spirals (3-arm inward, 2-arm outward)
 * interfering into paisley whorls that pour into the center forever.
 * Port of lab/patterns/063_spiral_zoom/proto.py (repaint, ignores sl). */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>

static int16_t p63_sin[1024];
static int     p63_tab_ok = 0;
static int     p63_w = 0, p63_h = 0;
static uint16_t *p63_b1;    /* static phase of spiral 1, 64ths of an idx unit */
static uint16_t *p63_b2;    /* static phase of spiral 2 */
static uint8_t  *p63_shd;   /* r/(r+26) * 63 */
static uint32_t p63_lut[64][256];

/* 3.6.5: per-appearance whorls — the seed used to be ignored, so every
 * appearance was the same 3-in/2-out paisley (variety audit). The seed now
 * picks the two arm counts, how tightly each spiral winds, zoom speed and
 * direction, a nudge of the centre, and where on the ramp the colour stops
 * start. Arm counts stay integers so the spirals close without a seam. */
static uint32_t p63_seed = 0xFFFFFFFFu;
static int p63_k1 = 3, p63_k2 = 2, p63_off = 0, p63_zdir = 1;
static double p63_t1 = 2.4, p63_t2 = 1.1, p63_z1 = 0.55, p63_z2 = 0.25;
static double p63_ox = 0.0, p63_oy = 0.0;
static uint32_t p63_hh(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }

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

static void p63_init(int w, int h, uint32_t seed) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (seed != p63_seed) {
        uint32_t a = p63_hh(seed ^ 0x6363u), b = p63_hh(a), c = p63_hh(b);
        p63_k1 = 3 + (int)(a % 3u);                                   /* 3..5 in */
        p63_k2 = 1 + (int)((a >> 4) % 3u);                            /* 1..3 out */
        p63_t1 = 1.8 + (double)((a >> 8) % 1000u) * 0.0012;           /* 1.8..3.0 */
        p63_t2 = 0.8 + (double)((a >> 18) % 1000u) * 0.0008;          /* 0.8..1.6 */
        p63_z1 = 0.55 + (double)((b >> 4) % 1000u) * 0.00025;         /* 0.55..0.80 */
        p63_z2 = 0.25 + (double)((b >> 14) % 1000u) * 0.00015;        /* 0.25..0.40 */
        p63_zdir = (b & 1u) ? 1 : -1;
        p63_ox = -20.0 + (double)((c >> 4) % 1000u) * 0.040;          /* +-20 px */
        p63_oy = -14.0 + (double)((c >> 14) % 1000u) * 0.028;         /* +-14 px */
        p63_off = (int)(p63_hh(c) & JD_PAL_MASK);
        p63_seed = seed;
        p63_w = 0;                                       /* force a table rebuild */
    }
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
        double py = (y - h * 0.5) * sy - p63_oy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx - p63_ox;
            double r = hypot(px, py);
            double lr = log(r + 2.0) * 82.0;
            /* angle expressed in idx units (256 = full turn) */
            double an = (atan2(py, px) * (1.0 / (2.0 * M_PI)) + 0.5) * 256.0;
            double f1 = lr * p63_t1 + an * (double)p63_k1;
            double f2 = lr * p63_t2 - an * (double)p63_k2;
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
        uint32_t c = pal[(j * 5461 + p63_off) & JD_PAL_MASK];
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
    (void)sl;
    p63_init(w, h, seed);
    p63_build_lut(pal);
    p1 = (int)fmod(t * p63_z1 * 64.0, 16384.0);
    p2 = (int)fmod(t * p63_z2 * 64.0, 16384.0);
    /* zoom in (the original) or pour outward; both phases stay in 0..16384 */
    if (p63_zdir > 0) p1 = 16384 - p1; else p2 = 16384 - p2;
    for (i = 0; i < n; i++) {
        int i1 = ((p63_b1[i] + p1) & 16383) >> 4;
        int i2 = ((p63_b2[i] + p2) & 16383) >> 4;
        int s = p63_sin[i1] + p63_sin[i2];
        int idx = (((s * 63) >> 16) + 128) & 255;
        fb[i] = p63_lut[p63_shd[i]][idx];
    }
}
