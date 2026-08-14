/* 070 Vortex Petals — six log-spiral petal arms curling into a dark core, the
 * spiral tightness slowly breathing, ring pulses streaming down the arms.
 * Port of lab/patterns/070_vortex_petals/proto.py (repaint, ignores sl). */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

static int16_t p70_sin[1024];
static int p70_tab_ok = 0;
static int p70_w = 0, p70_h = 0;
static uint16_t *p70_a6;    /* 6*angle, x16 1024-unit turn */
static uint16_t *p70_lr;    /* ln(r+3) * 4096 */
static uint16_t *p70_dph;   /* phase of depth*0.5 */
static uint16_t *p70_rph;   /* phase of 8*ln(r+3) */
static uint8_t  *p70_shd;   /* r/(r+30) * 63 */
static uint32_t p70_lut[64][256];

static void p70_init(int w, int h) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (!p70_tab_ok) {
        for (i = 0; i < 1024; i++)
            p70_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
        p70_tab_ok = 1;
    }
    if (p70_w == w && p70_h == h) return;
    free(p70_a6); free(p70_lr); free(p70_dph); free(p70_rph); free(p70_shd);
    p70_a6  = (uint16_t *)malloc((size_t)w * h * 2);
    p70_lr  = (uint16_t *)malloc((size_t)w * h * 2);
    p70_dph = (uint16_t *)malloc((size_t)w * h * 2);
    p70_rph = (uint16_t *)malloc((size_t)w * h * 2);
    p70_shd = (uint8_t *)malloc((size_t)w * h);
    for (y = 0, i = 0; y < h; y++) {
        double py = (y - h * 0.5) * sy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx;
            double r = hypot(px, py) + 1e-3;
            double a = atan2(py, px);
            double lr = log(r + 3.0);
            double dep = 2600.0 / (r + 16.0);
            double q6 = fmod(a * 6.0 * 2607.6, 16384.0);
            double qd = fmod(dep * 0.5 * 2607.6, 16384.0);
            double qr = fmod(lr * 8.0 * 2607.6, 16384.0);
            if (q6 < 0) q6 += 16384.0;
            p70_a6[i]  = (uint16_t)q6;
            p70_lr[i]  = (uint16_t)(lr * 4096.0);
            p70_dph[i] = (uint16_t)qd;
            p70_rph[i] = (uint16_t)qr;
            p70_shd[i] = (uint8_t)(63.0 * r / (r + 30.0));
        }
    }
    p70_w = w; p70_h = h;
}

/* smooth 6-stop looping ramp from the engine palette under the lab's
 * meadow-fire luminance envelope (moss -> chartreuse -> gold -> maroon) */
static void p70_build_lut(const uint32_t *pal) {
    static const double xs[7] = { 0.00, 0.18, 0.36, 0.54, 0.72, 0.88, 1.00 };
    static const double ys[7] = { 0.22, 0.48, 0.86, 1.00, 0.72, 0.30, 0.22 };
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
            p70_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_070(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int i, n = w * h, K, rot, pph, rrp;
    double t = (double)frame;
    double tw = 2.6 + 0.8 * sin(t * 0.0012);      /* spiral tightness breathes */
    (void)sl; (void)seed;
    p70_init(w, h);
    p70_build_lut(pal);
    K   = (int)(tw * 1.9099 * 256.0);             /* lr16 -> arm phase scale */
    rot = 16384 - (int)fmod(t * 0.003 * 6.0 * 2607.6, 16384.0);
    pph = (int)fmod(t * 0.030 * 2607.6, 16384.0);
    rrp = 16384 - (int)fmod(t * 0.020 * 2607.6, 16384.0);
    for (i = 0; i < n; i++) {
        int arms = p70_sin[((p70_a6[i] + ((p70_lr[i] * K) >> 8) + rot) >> 4) & 1023];
        int pull = p70_sin[((p70_dph[i] + pph) >> 4) & 1023];
        int rib  = p70_sin[((p70_rph[i] + rrp) >> 4) & 1023];
        int field = ((arms * 79) >> 7) + ((pull * 33) >> 7) + ((rib * 38) >> 7);
        int idx = (((field * 100) >> 15) + 128) & 255;
        fb[i] = p70_lut[p70_shd[i]][idx];
    }
}
