/* 067 Twin Tunnels — two orbiting tunnel mouths inside a 4-fold kaleidoscope
 * fold, their ring systems beating into hyperbolic moire lenses.
 * Port of lab/patterns/067_twin_tunnels/proto.py (repaint, ignores sl). */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P67_RQ 1600            /* radius quantised to 1/4 of a 320-space px */

static int16_t p67_sin[1024];
static uint16_t p67_dph[P67_RQ];   /* phase of 1900/(r+9)*0.5, x16 1024-turn */
static uint8_t  p67_shd[P67_RQ];   /* r/(r+40) * 255 */
static int p67_tab_ok = 0;
static int p67_w = 0, p67_h = 0;
static float *p67_fx;              /* folded |x| in 320-space */
static float *p67_fy;              /* folded |y| in 240-space */
static uint32_t p67_lut[64][256];
/* 3.6.5: seed now picks both mouths' orbits, ring density, spin directions,
 * moire strength and the ramp start — every appearance used to be the same
 * pair on the same tracks (variety audit dSeed 0.00). */
static uint32_t p67_seed = 0xFFFFFFFFu;
static float  p67_c[4], p67_amp[4], p67_frq[4], p67_phs[4];  /* ox1 oy1 ox2 oy2 */
static double p67_rk = 0.5;          /* depth -> ring phase */
static double p67_s1 = 1.0, p67_s2 = 1.0;
static int    p67_moire = 59;
static int    p67_poff = 0;

static uint32_t p67_hash(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}

static void p67_reseed(uint32_t seed) {
    /* lab track: centre, swing, rate, phase for ox1 oy1 ox2 oy2 */
    static const float c0[4] = { 88.0f, 62.0f, 40.0f, 34.0f };
    static const float a0[4] = { 30.0f, 24.0f, 26.0f, 20.0f };
    static const float f0[4] = { 0.0040f, 0.0031f, 0.0026f, 0.0047f };
    uint32_t hsh = p67_hash(seed ^ 0x67B0u);
    int k;
    for (k = 0; k < 4; k++) {
        hsh = p67_hash(hsh + (uint32_t)k);
        p67_c[k]   = c0[k] * (0.65f + (hsh & 255) * (0.70f / 255.0f));
        p67_amp[k] = a0[k] * (0.7f + ((hsh >> 8) & 127) * (0.6f / 127.0f));
        p67_frq[k] = f0[k] * (0.8f + ((hsh >> 15) & 127) * (0.5f / 127.0f));
        p67_phs[k] = ((hsh >> 22) & 1023) * (6.2831853f / 1024.0f);
    }
    hsh = p67_hash(hsh ^ 0xA5u);
    p67_rk = 0.38 + (hsh & 255) * (0.26 / 255.0);
    p67_s1 = ((hsh >> 8) & 1u) ? -1.0 : 1.0;
    p67_s2 = ((hsh >> 9) & 1u) ? -1.0 : 1.0;
    p67_moire = 40 + (int)((hsh >> 10) & 31u);
    p67_poff = (int)(hsh >> 17) & JD_PAL_MASK;
    p67_tab_ok = 0;                  /* ring-phase table depends on p67_rk */
    p67_seed = seed;
}

static void p67_init(int w, int h) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (!p67_tab_ok) {
        for (i = 0; i < 1024; i++)
            p67_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
        for (i = 0; i < P67_RQ; i++) {
            double r = i * 0.25;
            double d = 1900.0 / (r + 9.0);
            double ph = fmod(d * p67_rk * (1024.0 / (2.0 * M_PI)) * 16.0, 16384.0);
            p67_dph[i] = (uint16_t)ph;
            p67_shd[i] = (uint8_t)(255.0 * r / (r + 40.0));
        }
        p67_tab_ok = 1;
    }
    if (p67_w == w && p67_h == h) return;
    free(p67_fx); free(p67_fy);
    p67_fx = (float *)malloc(sizeof(float) * w);
    p67_fy = (float *)malloc(sizeof(float) * h);
    for (x = 0; x < w; x++) p67_fx[x] = (float)fabs((x - w * 0.5) * sx);
    for (y = 0; y < h; y++) p67_fy[y] = (float)fabs((y - h * 0.5) * sy);
    p67_w = w; p67_h = h;
}

/* smooth 6-stop looping ramp lifted from the engine palette */
static void p67_build_lut(const uint32_t *pal) {
    int idx, s, j;
    int ar[7], ag[7], ab[7];
    for (j = 0; j < 6; j++) {
        uint32_t c = pal[(j * 5461 + p67_poff) & JD_PAL_MASK];
        int m;
        ar[j] = (c >> 16) & 255; ag[j] = (c >> 8) & 255; ab[j] = c & 255;
        /* 3.6.5: seeded ramp start may sit on a dark stretch; lift the
         * brightest channel to 190, the mouth shading supplies the dark */
        m = ar[j] > ag[j] ? ar[j] : ag[j]; if (ab[j] > m) m = ab[j];
        if (m < 190) {
            if (m < 8) { ar[j] = ag[j] = ab[j] = 8; m = 8; }
            ar[j] = ar[j] * 190 / m; ag[j] = ag[j] * 190 / m; ab[j] = ab[j] * 190 / m;
        }
    }
    ar[6] = ar[0]; ag[6] = ag[0]; ab[6] = ab[0];
    for (idx = 0; idx < 256; idx++) {
        int q = idx * 6, j0 = q >> 8, f = q & 255;
        uint32_t r = (uint32_t)(ar[j0] + (((ar[j0+1] - ar[j0]) * f) >> 8));
        uint32_t g = (uint32_t)(ag[j0] + (((ag[j0+1] - ag[j0]) * f) >> 8));
        uint32_t b = (uint32_t)(ab[j0] + (((ab[j0+1] - ab[j0]) * f) >> 8));
        for (s = 0; s < 64; s++) {
            uint32_t k = (uint32_t)(s * 65536 / 63);
            p67_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_067(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int x, y, ph1, ph2;
    double t = (double)frame;
    float ox1, oy1, ox2, oy2;
    (void)sl;
    if (seed != p67_seed) p67_reseed(seed);
    p67_init(w, h);
    p67_build_lut(pal);
    ox1 = (float)(p67_c[0] + p67_amp[0] * sin(t * p67_frq[0] + p67_phs[0]));
    oy1 = (float)(p67_c[1] + p67_amp[1] * sin(t * p67_frq[1] + p67_phs[1]));
    ox2 = (float)(p67_c[2] + p67_amp[2] * sin(t * p67_frq[2] + p67_phs[2]));
    oy2 = (float)(p67_c[3] + p67_amp[3] * sin(t * p67_frq[3] + p67_phs[3]));
    /* 1 rad = 2607.6 units of the x16 1024-step turn */
    ph1 = (int)fmod(t * 0.020 * p67_s1 * 2607.6, 16384.0);
    if (ph1 < 0) ph1 += 16384;
    ph2 = 16384 - (int)fmod(t * 0.017 * p67_s2 * 2607.6, 16384.0);
    if (ph2 > 16384) ph2 -= 16384;

    for (y = 0; y < h; y++) {
        float fy = p67_fy[y];
        float ay = fy - oy1, by = fy - oy2;
        float ay2 = ay * ay, by2 = by * by;
        uint32_t *out = fb + (size_t)y * w;
        for (x = 0; x < w; x++) {
            float fx = p67_fx[x];
            float ax = fx - ox1, bx = fx - ox2;
            float r1 = sqrtf(ax * ax + ay2), r2 = sqrtf(bx * bx + by2);
            int q1 = (int)(r1 * 4.0f), q2 = (int)(r2 * 4.0f);
            int s1, s2, s3, sum, idx, lum;
            if (q1 > P67_RQ - 1) q1 = P67_RQ - 1;
            if (q2 > P67_RQ - 1) q2 = P67_RQ - 1;
            s1 = p67_sin[((p67_dph[q1] + ph1) >> 4) & 1023];
            s2 = p67_sin[((p67_dph[q2] + ph2) >> 4) & 1023];
            s3 = p67_sin[((((q1 - q2) * p67_moire) >> 4) + 16384) & 1023];
            sum = s1 + s2 + ((s3 * 18) >> 5);
            idx = (((sum * 88) >> 15) + 128) & 255;
            lum = (p67_shd[q1] * p67_shd[q2] * 11) >> 13;  /* (a*b)*1.35, 0..63 */
            if (lum > 63) lum = 63;
            out[x] = p67_lut[lum][idx];
        }
    }
}
