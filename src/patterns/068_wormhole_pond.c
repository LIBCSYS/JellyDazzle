/* 068 Wormhole Pond — a breathing organic wormhole (2- and 3-lobe rim modes)
 * crossed by swells and a drifting rain-drop ripple source.
 * Port of lab/patterns/068_wormhole_pond/proto.py (repaint, ignores sl). */
#include "../engine/jellydazzle.h"
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
/* 3.6.5: seed now picks the two rim modes, the wormhole's position, the
 * rain-drop's path, swell speeds/directions and the ramp start — every
 * appearance used to be the same 2+3-lobe pond (variety audit dSeed 0.00). */
static uint32_t p68_seed = 0xFFFFFFFFu;
static int    p68_m1 = 2, p68_m2 = 3;          /* rim lobe counts */
static double p68_ox = 0.0, p68_oy = 0.0;      /* wormhole centre, 320-space */
static double p68_dax = 70.0, p68_day = 50.0, p68_dfx = 0.0035, p68_dfy = 0.0052;
static double p68_dpx = 0.9, p68_dpy = 2.2;
static double p68_sp[5] = { 1.0, 1.0, 1.0, 1.0, 1.0 };
static int    p68_poff = 0;

static uint32_t p68_hash(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}

static void p68_reseed(uint32_t seed) {
    static const int modes[6][2] = { {2,3}, {3,4}, {2,5}, {3,5}, {4,6}, {2,4} };
    uint32_t a = p68_hash(seed ^ 0x68C0u), b = p68_hash(a), c = p68_hash(b);
    int k;
    p68_m1 = modes[a % 6u][0]; p68_m2 = modes[a % 6u][1];
    p68_ox = (double)((int)((a >> 8) & 63) - 32) * 1.2;
    p68_oy = (double)((int)((a >> 14) & 63) - 32) * 0.8;
    p68_dax = 50.0 + ((a >> 20) & 63) * (40.0 / 63.0);
    p68_day = 35.0 + ((a >> 26) & 63) * (30.0 / 63.0);
    p68_dfx = 0.0028 + (b & 255) * (0.0020 / 255.0);
    p68_dfy = 0.0040 + ((b >> 8) & 255) * (0.0025 / 255.0);
    p68_dpx = ((b >> 16) & 255) * (6.2831853 / 256.0);
    p68_dpy = ((b >> 24) & 255) * (6.2831853 / 256.0);
    /* rim and swell speeds: magnitude never below the lab's, sign free */
    for (k = 0; k < 5; k++)
        p68_sp[k] = (((c >> k) & 1u) ? -1.0 : 1.0) * (1.0 + ((c >> (5 + 3 * k)) & 7) * 0.05);
    p68_poff = (int)(p68_hash(c) >> 17) & JD_PAL_MASK;
    p68_w = 0;                       /* lobe/centre tables depend on it */
    p68_seed = seed;
}

static int p68_ph(double v) {        /* wrap into [0,16384) */
    int q = (int)fmod(v, 16384.0);
    return q < 0 ? q + 16384 : q;
}

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
        double py = (y - h * 0.5) * sy - p68_oy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx - p68_ox;
            double r = hypot(px, py) + 1e-3;
            double a = atan2(py, px);
            double q2 = fmod(a * p68_m1 * 2607.6, 16384.0);
            double q3 = fmod(a * p68_m2 * 2607.6, 16384.0);
            if (q2 < 0) q2 += 16384.0;
            if (q3 < 0) q3 += 16384.0;
            p68_a2[i] = (uint16_t)q2;
            p68_a3[i] = (uint16_t)q3;
            if (r > 1023.0) r = 1023.0;
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
        uint32_t c = pal[(j * 5461 + p68_poff) & JD_PAL_MASK];
        int m;
        ar[j] = (c >> 16) & 255; ag[j] = (c >> 8) & 255; ab[j] = c & 255;
        /* 3.6.5: seeded ramp start may sit on a dark stretch; lift the
         * brightest channel to 190, the basin envelope supplies the dark */
        m = ar[j] > ag[j] ? ar[j] : ag[j]; if (ab[j] > m) m = ab[j];
        if (m < 190) {
            if (m < 8) { ar[j] = ag[j] = ab[j] = 8; m = 8; }
            ar[j] = ar[j] * 190 / m; ag[j] = ag[j] * 190 / m; ab[j] = ab[j] * 190 / m;
        }
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
    (void)sl;
    if (seed != p68_seed) p68_reseed(seed);
    p68_init(w, h);
    p68_build_lut(pal);
    w2 = p68_ph(t * 0.012 * p68_sp[0] * 2607.6);
    w3 = p68_ph(-t * 0.008 * p68_sp[1] * 2607.6);
    pd = p68_ph(-t * 0.018 * p68_sp[2] * 2607.6);
    ps = p68_ph(-t * 0.011 * p68_sp[3] * 2607.6);
    pr = p68_ph(-t * 0.015 * p68_sp[4] * 2607.6);
    dsx = (float)(p68_dax * sin(t * p68_dfx + p68_dpx));
    dsy = (float)(p68_day * sin(t * p68_dfy + p68_dpy));

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
