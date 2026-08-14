/* 065 Echo Mandala — a 5-petal flower stamped at 8 rotated/scaled echoes that
 * dive continuously, so fresh echoes bloom out of the center forever.
 * Port of lab/patterns/065_echo_mandala/proto.py (repaint, ignores sl).
 * Internal 320x240 canvas (the lab resolution), bilinear upscale to (w,h). */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P65_GW 320
#define P65_GH 240
#define P65_NOCT 8

static int16_t p65_sin[1024];
static int p65_tab_ok = 0;
static float p65_r[P65_GH * P65_GW];    /* radius in 320-space px */
static float p65_a5[P65_GH * P65_GW];   /* 5*angle in 1024-unit turns */
static uint32_t p65_img[P65_GH * P65_GW];
static uint32_t p65_ramp[256];
static int p65_uw = -1;
static int *p65_uxi;
static uint8_t *p65_uxf;

static void p65_init(void) {
    int x, y, i;
    if (p65_tab_ok) return;
    for (i = 0; i < 1024; i++)
        p65_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
    for (y = 0, i = 0; y < P65_GH; y++) {
        double py = y - P65_GH * 0.5;
        for (x = 0; x < P65_GW; x++, i++) {
            double px = x - P65_GW * 0.5;
            p65_r[i] = (float)hypot(px, py);
            p65_a5[i] = (float)(atan2(py, px) * 5.0 * (1024.0 / (2.0 * M_PI)));
        }
    }
    p65_tab_ok = 1;
}

/* smooth 6-stop looping ramp from the engine palette, with the lab jewel
 * ramp's luminance envelope (midnight -> emerald -> gold -> ruby -> violet) */
static void p65_build_ramp(const uint32_t *pal) {
    static const double xs[6] = { 0.00, 0.20, 0.42, 0.62, 0.82, 1.00 };
    static const double ys[6] = { 0.18, 0.62, 1.00, 0.66, 0.58, 0.18 };
    int idx, j = 0, k;
    int ar[7], ag[7], ab[7];
    for (k = 0; k < 6; k++) {
        uint32_t c = pal[(k * 5461) & JD_PAL_MASK];
        ar[k] = (c >> 16) & 255; ag[k] = (c >> 8) & 255; ab[k] = c & 255;
    }
    ar[6] = ar[0]; ag[6] = ag[0]; ab[6] = ab[0];
    for (idx = 0; idx < 256; idx++) {
        int q = idx * 6, j0 = q >> 8, f = q & 255;
        double u = idx / 255.0, v;
        uint32_t e, r, g, b;
        while (j < 4 && u > xs[j + 1]) j++;
        v = ys[j] + (ys[j + 1] - ys[j]) * (u - xs[j]) / (xs[j + 1] - xs[j]);
        e = (uint32_t)(v * 65536.0);
        r = (uint32_t)(ar[j0] + (((ar[j0+1] - ar[j0]) * f) >> 8));
        g = (uint32_t)(ag[j0] + (((ag[j0+1] - ag[j0]) * f) >> 8));
        b = (uint32_t)(ab[j0] + (((ab[j0+1] - ab[j0]) * f) >> 8));
        p65_ramp[idx] = 0xFF000000u | (((r * e) >> 16) << 16)
                      | (((g * e) >> 16) << 8) | ((b * e) >> 16);
    }
}

static void p65_upscale(uint32_t *fb, int w, int h) {
    int x, y;
    if (w != p65_uw) {
        free(p65_uxi); free(p65_uxf);
        p65_uxi = (int *)malloc(sizeof(int) * w);
        p65_uxf = (uint8_t *)malloc(w);
        for (x = 0; x < w; x++) {
            int q = (int)(((int64_t)x * (P65_GW - 1) * 256) / (w > 1 ? w - 1 : 1));
            int xi = q >> 8;
            if (xi > P65_GW - 2) { xi = P65_GW - 2; q = (P65_GW - 1) * 256; }
            p65_uxi[x] = xi; p65_uxf[x] = (uint8_t)(q & 255);
        }
        p65_uw = w;
    }
    for (y = 0; y < h; y++) {
        int qy = (int)(((int64_t)y * (P65_GH - 1) * 256) / (h > 1 ? h - 1 : 1));
        int yi = qy >> 8, fy;
        const uint32_t *r0, *r1;
        uint32_t *out = fb + (size_t)y * w;
        if (yi > P65_GH - 2) { yi = P65_GH - 2; qy = (P65_GH - 1) * 256; }
        fy = qy & 255;
        r0 = p65_img + (size_t)yi * P65_GW;
        r1 = r0 + P65_GW;
        for (x = 0; x < w; x++) {
            int xi = p65_uxi[x], fx = p65_uxf[x], k, c[3];
            uint32_t a = r0[xi], b = r0[xi + 1], cc = r1[xi], d = r1[xi + 1];
            for (k = 0; k < 3; k++) {
                int sh = k * 8;
                int a0 = (a >> (16 - sh)) & 255, b0 = (b >> (16 - sh)) & 255;
                int c0 = (cc >> (16 - sh)) & 255, d0 = (d >> (16 - sh)) & 255;
                int t0 = a0 + (((b0 - a0) * fx) >> 8);
                int t1 = c0 + (((d0 - c0) * fx) >> 8);
                c[k] = t0 + (((t1 - t0) * fy) >> 8);
            }
            out[x] = 0xFF000000u | ((uint32_t)c[0] << 16)
                   | ((uint32_t)c[1] << 8) | (uint32_t)c[2];
        }
    }
}

void pattern_065(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    float A[P65_NOCT], B[P65_NOCT], C[P65_NOCT];
    int wi[P65_NOCT];
    int k, i, wsum = 0;
    double t = (double)frame;
    double z = t * 0.0035, frac = z - floor(z), spin = t * 0.0009;
    float fac;
    (void)sl; (void)seed;
    p65_init();
    p65_build_ramp(pal);

    for (k = 0; k < P65_NOCT; k++) {
        double m = k - frac;
        double sc = pow(1.9, m) * 0.021;
        double th = m * 0.5 + spin;
        double ww = pow(0.62, m) * (m + 1.0 < 1.0 ? (m + 1.0 < 0.0 ? 0.0 : m + 1.0) : 1.0);
        A[k] = (float)(sc * 6.0 * (1024.0 / (2.0 * M_PI)));
        B[k] = (float)(sc * 1.5 * (1024.0 / (2.0 * M_PI)));
        C[k] = (float)(th * 5.0 * (1024.0 / (2.0 * M_PI)));
        wi[k] = (int)(ww * 256.0);
        wsum += wi[k];
    }
    fac = (float)(120.0 / ((double)wsum * 32767.0));

    for (i = 0; i < P65_GH * P65_GW; i++) {
        float r = p65_r[i], a5 = p65_a5[i];
        int acc = 0, idx;
        for (k = 0; k < P65_NOCT; k++) {
            int p1 = (int)(r * A[k]) + 256;
            int p2 = (int)(a5 + C[k] + r * B[k]) + 256;
            int s1 = p65_sin[p1 & 1023], s2 = p65_sin[p2 & 1023];
            acc += ((s1 * s2) >> 15) * wi[k];
        }
        idx = (128 + (int)((float)acc * fac)) & 255;
        p65_img[i] = p65_ramp[idx];
    }
    p65_upscale(fb, w, h);
}
