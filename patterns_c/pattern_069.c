/* 069 Pillar Hall — first-person glide down a checkered amber corridor with
 * four-plane perspective (floor, ceiling, two walls), dark pillars marching
 * past and a slow camera sway/bob.
 * Port of lab/patterns/069_pillar_hall/proto.py (repaint, ignores sl). */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P69_AQ 4096            /* |coord| quantised to 1/16 of a 320-space px */
#define P69_ASPECT 1.3333333f

static uint16_t p69_dep[P69_AQ];   /* depth * 64, clamped to 420 */
static uint8_t  p69_fog[P69_AQ];   /* clip(1 - depth/460) * 63 */
static uint8_t  p69_idb[P69_AQ];   /* (depth*0.55) & 255 */
static float    p69_inv[P69_AQ];   /* 34 / coord */
static int p69_tab_ok = 0;
static int p69_w = 0, p69_h = 0;
static float *p69_cx;              /* per-column x in 320-space, centred */
static float *p69_cy;              /* per-row y in 240-space, centred */
static int   *p69_qx;              /* per-frame |dx| in 1/16 px */
static int   *p69_qy;              /* per-frame |dy|*aspect in 1/16 px */
static float *p69_ox;              /* per-frame dx */
static float *p69_oy;              /* per-frame dy*aspect */
static uint32_t p69_lut[64][256];

static void p69_init(int w, int h) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (!p69_tab_ok) {
        for (i = 0; i < P69_AQ; i++) {
            double a = i * 0.0625; if (a < 0.5) a = 0.5;
            double dep = 2400.0 / a; if (dep > 420.0) dep = 420.0;
            double fg = 1.0 - dep / 460.0; if (fg < 0.0) fg = 0.0;
            p69_dep[i] = (uint16_t)(dep * 64.0);
            p69_fog[i] = (uint8_t)(fg * 63.0);
            p69_idb[i] = (uint8_t)(((int)(dep * 0.55)) & 255);
            p69_inv[i] = (float)(34.0 / a);
        }
        p69_tab_ok = 1;
    }
    if (p69_w == w && p69_h == h) return;
    free(p69_cx); free(p69_cy); free(p69_qx); free(p69_qy);
    free(p69_ox); free(p69_oy);
    p69_cx = (float *)malloc(sizeof(float) * w);
    p69_cy = (float *)malloc(sizeof(float) * h);
    p69_qx = (int *)malloc(sizeof(int) * w);
    p69_qy = (int *)malloc(sizeof(int) * h);
    p69_ox = (float *)malloc(sizeof(float) * w);
    p69_oy = (float *)malloc(sizeof(float) * h);
    for (x = 0; x < w; x++) p69_cx[x] = (float)((x - w * 0.5) * sx);
    for (y = 0; y < h; y++) p69_cy[y] = (float)((y - h * 0.5) * sy);
    p69_w = w; p69_h = h;
}

/* smooth 6-stop looping ramp from the engine palette under the lab's amber
 * hall luminance envelope (near-black -> mahogany -> amber -> cream) */
static void p69_build_lut(const uint32_t *pal) {
    static const double xs[7] = { 0.00, 0.20, 0.42, 0.58, 0.70, 0.86, 1.00 };
    static const double ys[7] = { 0.10, 0.36, 0.72, 0.92, 1.00, 0.38, 0.10 };
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
            p69_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_069(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int x, y, fly;
    double t = (double)frame;
    float sway = (float)(16.0 * sin(t * 0.004));
    float bob  = (float)(7.0 * sin(t * 0.0027));
    (void)sl; (void)seed;
    p69_init(w, h);
    p69_build_lut(pal);
    /* v is kept mod 360 (= LCM of the 40-unit tile parity and 90-unit pillar
     * spacing) so both stay phase-consistent forever */
    fly = (int)fmod(t * 0.35 * 64.0, 23040.0);
    for (x = 0; x < w; x++) {
        float d = p69_cx[x] - sway;
        int q;
        p69_ox[x] = d;
        d = d < 0 ? -d : d;
        q = (int)(d * 16.0f); if (q > P69_AQ - 1) q = P69_AQ - 1;
        p69_qx[x] = q;
    }
    for (y = 0; y < h; y++) {
        float d = (p69_cy[y] - bob) * P69_ASPECT;
        int q;
        p69_oy[y] = d;
        d = d < 0 ? -d : d;
        q = (int)(d * 16.0f); if (q > P69_AQ - 1) q = P69_AQ - 1;
        p69_qy[y] = q;
    }

    for (y = 0; y < h; y++) {
        uint32_t *out = fb + (size_t)y * w;
        int qy = p69_qy[y];
        float oy = p69_oy[y];
        for (x = 0; x < w; x++) {
            int qx = p69_qx[x], wall = qx > qy, a = wall ? qx : qy;
            float other = wall ? oy : p69_ox[x];
            float u = other * p69_inv[a];
            int iu = (int)floorf(u * 0.0625f);
            int v64 = ((p69_dep[a] * 141) >> 8) + fly;
            int tile = (iu + (v64 / 1280)) & 1;
            int pil = wall && ((v64 % 5760) < 1024);
            int fog = p69_fog[a];
            int idx = (p69_idb[a] + tile * 46 + (wall ? 36 : 0)) & 255;
            int lum;
            if (pil)       lum = (fog * 56) >> 8;
            else if (tile) lum = fog;
            else           lum = (fog * 141) >> 8;
            out[x] = p69_lut[lum][idx];
        }
    }
}
