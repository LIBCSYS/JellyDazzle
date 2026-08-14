/* 068 Wormhole Pond — a breathing organic wormhole (2- and 3-lobe rim modes)
 * crossed by swells and a drifting rain-drop ripple source.
 * Port of lab/patterns/068_wormhole_pond/proto.py (repaint, ignores sl). */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P68_RQ 1600            /* rr quantised to 1/4 of a 320-space px */

static int16_t p68_sin[1024];
static uint16_t p68_dph[P68_RQ];   /* phase of 2000/(rr+6)*0.40 */
static uint16_t p68_sph[P68_RQ];   /* phase of rr*0.16 */
static uint8_t  p68_shd[P68_RQ];   /* clip(rr/(rr+34)*1.15) * 63 */
static int p68_tab_ok = 0;
static int p68_w = 0, p68_h = 0;
static float *p68_px;              /* per-column x in 320-space, centred */
static float *p68_py;              /* per-row y in 240-space, centred */
static uint16_t *p68_a2;           /* 2*angle, x16 1024-turn */
static uint16_t *p68_a3;           /* 3*angle, x16 1024-turn */
static uint16_t *p68_r64;          /* radius * 64 */
static uint32_t p68_lut[64][256];

static void p68_init(int w, int h) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (!p68_tab_ok) {
        for (i = 0; i < 1024; i++)
            p68_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
        for (i = 0; i < P68_RQ; i++) {
            double rr = i * 0.25;
            double dep = 2000.0 / (rr + 6.0);
            double v = rr / (rr + 34.0) * 1.15;
            double a = fmod(dep * 0.40 * 2607.6, 16384.0);
            double b = fmod(rr * 0.16 * 2607.6, 16384.0);
            if (v > 1.0) v = 1.0;
            p68_dph[i] = (uint16_t)a;
            p68_sph[i] = (uint16_t)b;
            p68_shd[i] = (uint8_t)(v * 63.0);
        }
        p68_tab_ok = 1;
    }
    if (p68_w == w && p68_h == h) return;
    free(p68_px); free(p68_py); free(p68_a2); free(p68_a3); free(p68_r64);
    p68_px  = (float *)malloc(sizeof(float) * w);
    p68_py  = (float *)malloc(sizeof(float) * h);
    p68_a2  = (uint16_t *)malloc((size_t)w * h * 2);
    p68_a3  = (uint16_t *)malloc((size_t)w * h * 2);
    p68_r64 = (uint16_t *)malloc((size_t)w * h * 2);
    for (x = 0; x < w; x++) p68_px[x] = (float)((x - w * 0.5) * sx);
    for (y = 0; y < h; y++) p68_py[y] = (float)((y - h * 0.5) * sy);
    for (y = 0, i = 0; y < h; y++) {
        double py = (y - h * 0.5) * sy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx;
            double r = hypot(px, py) + 1e-3;
            double a = atan2(py, px);
            double q2 = fmod(a * 2.0 * 2607.6, 16384.0);
            double q3 = fmod(a * 3.0 * 2607.6, 16384.0);
            if (q2 < 0) q2 += 16384.0;
            if (q3 < 0) q3 += 16384.0;
            p68_a2[i] = (uint16_t)q2;
            p68_a3[i] = (uint16_t)q3;
            p68_r64[i] = (uint16_t)(r * 64.0);
        }
    }
    p68_w = w; p68_h = h;
}

/* smooth 6-stop looping ramp lifted from the engine palette, with the lab
 * pond ramp's luminance envelope (dark basins + a foam crest) */
static void p68_build_lut(const uint32_t *pal) {
    static const double xs[7] = { 0.00, 0.22, 0.44, 0.58, 0.74, 0.90, 1.00 };
    static const double ys[7] = { 0.14, 0.42, 0.78, 1.00, 0.46, 0.22, 0.14 };
    int idx, s, j, e = 0;
    int ar[7], ag[7], ab[7];
    for (j = 0; j < 6; j++) {
        uint32_t c = pal[(j * 5461) & JD_PAL_MASK];
        ar[j] = (c >> 16) & 255; ag[j] = (c >> 8) & 255; ab[j] = c & 255;
    }
    ar[6] = ar[0]; ag[6] = ag[0]; ab[6] = ab[0];
    for (idx = 0; idx < 256; idx++) {
        int q = idx * 6, j0 = q >> 8, f = q & 255;
        double u = idx / 255.0, v;
        uint32_t sh, r, g, b;
        while (e < 5 && u > xs[e + 1]) e++;
        v = ys[e] + (ys[e + 1] - ys[e]) * (u - xs[e]) / (xs[e + 1] - xs[e]);
        sh = (uint32_t)(v * 65536.0);
        r = (uint32_t)(ar[j0] + (((ar[j0+1] - ar[j0]) * f) >> 8));
        g = (uint32_t)(ag[j0] + (((ag[j0+1] - ag[j0]) * f) >> 8));
        b = (uint32_t)(ab[j0] + (((ab[j0+1] - ab[j0]) * f) >> 8));
        r = (r * sh) >> 16; g = (g * sh) >> 16; b = (b * sh) >> 16;
        for (s = 0; s < 64; s++) {
            uint32_t k = (uint32_t)(s * 65536 / 63);
            p68_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_068(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int x, y, w2, w3, pd, ps, pr;
    double t = (double)frame;
    float dsx, dsy;
    (void)sl; (void)seed;
    p68_init(w, h);
    p68_build_lut(pal);
    w2 = (int)fmod(t * 0.012 * 2607.6, 16384.0);
    w3 = 16384 - (int)fmod(t * 0.008 * 2607.6, 16384.0);
    pd = 16384 - (int)fmod(t * 0.018 * 2607.6, 16384.0);
    ps = 16384 - (int)fmod(t * 0.011 * 2607.6, 16384.0);
    pr = 16384 - (int)fmod(t * 0.015 * 2607.6, 16384.0);
    dsx = (float)(70.0 * sin(t * 0.0035 + 0.9));
    dsy = (float)(50.0 * sin(t * 0.0052 + 2.2));

    for (y = 0; y < h; y++) {
        const uint16_t *a2 = p68_a2 + (size_t)y * w;
        const uint16_t *a3 = p68_a3 + (size_t)y * w;
        const uint16_t *rr = p68_r64 + (size_t)y * w;
        uint32_t *out = fb + (size_t)y * w;
        float dy = p68_py[y] - dsy;
        float dy2 = dy * dy;
        for (x = 0; x < w; x++) {
            int s1 = p68_sin[((a2[x] + w2) >> 4) & 1023];
            int s2 = p68_sin[((a3[x] + w3) >> 4) & 1023];
            int wob = 65536 + ((s1 * 14418) >> 15) + ((s2 * 7864) >> 15);
            int rq = (int)(((unsigned)rr[x] * (unsigned)wob) >> 20);  /* rr*4 */
            float dx = p68_px[x] - dsx;
            int q2 = (int)(sqrtf(dx * dx + dy2) * 4.0f);
            int sa, sb, sc, sum, idx;
            if (rq > P68_RQ - 1) rq = P68_RQ - 1;
            if (q2 > P68_RQ - 1) q2 = P68_RQ - 1;
            sa = p68_sin[((p68_dph[rq] + pd) >> 4) & 1023];
            sb = p68_sin[((p68_sph[rq] + ps) >> 4) & 1023];
            sc = p68_sin[(((q2 * 130) + pr) >> 4) & 1023];
            sum = sa + ((sb * 26) >> 5) + ((sc * 19) >> 5);
            idx = (((sum * 74) >> 15) + 128) & 255;
            out[x] = p68_lut[p68_shd[rq]][idx];
        }
    }
}
