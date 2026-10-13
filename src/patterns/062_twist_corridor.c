/* 062 Twist Corridor — nested squares receding to a dark vanishing point, the
 * shaft torquing harder at the deep end. Port of
 * lab/patterns/062_twist_corridor/proto.py (repaint pattern, ignores sl). */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P62_RINGS 256
#define P62_DQ    1024

static int16_t p62_sin[1024];
static int     p62_tab_ok = 0;
static int     p62_w = 0, p62_h = 0;
static int16_t *p62_dx;      /* per-column dx in 1/16 of a 320-space pixel */
static int16_t *p62_dy;      /* per-row dy, same units */
static uint8_t *p62_ring;    /* per-pixel radius bucket 0..255 */
static uint16_t *p62_ang;    /* per-pixel angle, 12-bit turn (0..4095) */
static uint16_t p62_dep[P62_DQ];   /* depth * 16 */
static uint8_t  p62_shd[P62_DQ];   /* d/(d+38) * 63 */
static int16_t  p62_ca[P62_RINGS], p62_sa[P62_RINGS];  /* 8.8 fixed */
static int16_t  p62_pb[P62_RINGS];                     /* sector phase bias */
static uint32_t p62_lut[64][256];

/* 3.6.5: per-appearance corridor — the seed used to be ignored, so every
 * appearance was the same four-sector shaft twisting the same way (variety
 * audit). The seed now picks sector count (2/4/8), twist direction and
 * strength, flight speed, a nudge of the vanishing point, and where on the
 * ramp the colour stops start. Geometry tables rebuild once per appearance. */
static uint32_t p62_seed = 0xFFFFFFFFu;
static int p62_sh = 10, p62_smask = 3, p62_off = 0;
static double p62_tw = 0.00055, p62_fsp = 0.45, p62_ox = 0.0, p62_oy = 0.0;
static uint32_t p62_hh(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }

static void p62_init(int w, int h, uint32_t seed) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (seed != p62_seed) {
        uint32_t a = p62_hh(seed ^ 0x6262u), b = p62_hh(a), c = p62_hh(b);
        int ns = 1 << (1 + (int)(a % 3u));             /* 2, 4 or 8 sectors */
        p62_smask = ns - 1;
        p62_sh = 12 - (ns == 2 ? 1 : ns == 4 ? 2 : 3);  /* 4096-turn >> sh */
        p62_tw  = (0.00038 + (double)((a >> 8) % 1000u) * 0.00000036)  /* 0.00038..0.00074 */
                  * ((b & 1u) ? 1.0 : -1.0);
        p62_fsp = 0.34 + (double)((b >> 4) % 1000u) * 0.00022;          /* 0.34..0.56 */
        p62_ox  = -22.0 + (double)((b >> 16) % 1000u) * 0.044;          /* +-22 px */
        p62_oy  = -16.0 + (double)((c >> 4) % 1000u) * 0.032;           /* +-16 px */
        p62_off = (int)((c >> 14) & JD_PAL_MASK);
        p62_seed = seed;
        p62_w = 0;                                      /* force a table rebuild */
    }
    if (!p62_tab_ok) {
        for (i = 0; i < 1024; i++)
            p62_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
        for (i = 0; i < P62_DQ; i++) {
            double d = i * 0.25;                /* dq = d16>>2 -> d = dq/4 px */
            double dep = 2400.0 / (d + 10.0);
            p62_dep[i] = (uint16_t)(dep * 16.0);
            p62_shd[i] = (uint8_t)(63.0 * d / (d + 38.0));
        }
        p62_tab_ok = 1;
    }
    if (p62_w == w && p62_h == h) return;
    free(p62_dx); free(p62_dy); free(p62_ring); free(p62_ang);
    p62_dx = (int16_t *)malloc(sizeof(int16_t) * w);
    p62_dy = (int16_t *)malloc(sizeof(int16_t) * h);
    p62_ring = (uint8_t *)malloc((size_t)w * h);
    p62_ang  = (uint16_t *)malloc((size_t)w * h * 2);
    for (x = 0; x < w; x++) p62_dx[x] = (int16_t)(((x - w * 0.5) * sx - p62_ox) * 16.0);
    for (y = 0; y < h; y++) p62_dy[y] = (int16_t)(((y - h * 0.5) * sy - p62_oy) * 16.0);
    for (y = 0, i = 0; y < h; y++) {
        double py = (y - h * 0.5) * sy - p62_oy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx - p62_ox;
            double r = hypot(px, py);
            int k = (int)(r * (P62_RINGS / 210.0));
            if (k > P62_RINGS - 1) k = P62_RINGS - 1;
            p62_ring[i] = (uint8_t)k;
            p62_ang[i] = (uint16_t)((int)((atan2(py, px) * (1.0 / (2.0 * M_PI))
                                          + 0.5) * 4096.0) & 4095);
        }
    }
    p62_w = w; p62_h = h;
}

/* smooth 6-stop looping ramp lifted from the engine palette */
static void p62_build_lut(const uint32_t *pal) {
    int idx, s, j;
    int ar[7], ag[7], ab[7];
    for (j = 0; j < 6; j++) {
        uint32_t c = pal[(j * 5461 + p62_off) & JD_PAL_MASK];
        ar[j] = (c >> 16) & 255; ag[j] = (c >> 8) & 255; ab[j] = c & 255;
    }
    ar[6] = ar[0]; ag[6] = ag[0]; ab[6] = ab[0];
    for (idx = 0; idx < 256; idx++) {
        int q = idx * 6, j0 = q >> 8, f = q & 255;
        uint32_t r = (uint32_t)(ar[j0] + (((ar[j0+1] - ar[j0]) * f) >> 8));
        uint32_t g = (uint32_t)(ag[j0] + (((ag[j0+1] - ag[j0]) * f) >> 8));
        uint32_t b = (uint32_t)(ab[j0] + (((ab[j0+1] - ab[j0]) * f) >> 8));
        for (s = 0; s < 64; s++) {
            uint32_t k = (uint32_t)(s * 65536 / 63);
            p62_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_062(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int x, y, k;
    double t = (double)frame;
    int fly, gph;
    (void)sl;
    p62_init(w, h, seed);
    p62_build_lut(pal);

    for (k = 0; k < P62_RINGS; k++) {
        double r = k * (210.0 / P62_RINGS);
        double phi = t * p62_tw * (1.0 + 70.0 / (r + 28.0));
        p62_ca[k] = (int16_t)(cos(phi) * 256.0);
        p62_sa[k] = (int16_t)(sin(phi) * 256.0);
        p62_pb[k] = (int16_t)(fmod(1024.0 * (phi * (2.0 / M_PI) - 1.5), 4096.0));
        /* reversed twist gives negative phi; keep the bias in 0..4095 so the
         * +65536 guard below still holds */
        if (p62_pb[k] < 0) p62_pb[k] = (int16_t)(p62_pb[k] + 4096);
    }
    fly = (int)fmod(t * p62_fsp * 16.0, 4096.0);          /* idx * 16 */
    gph = (int)fmod(t * 0.06 * (1024.0 / (2.0 * M_PI)), 1024.0);

    for (y = 0; y < h; y++) {
        const uint8_t *rp = p62_ring + (size_t)y * w;
        const uint16_t *ap = p62_ang + (size_t)y * w;
        uint32_t *out = fb + (size_t)y * w;
        int dy = p62_dy[y];
        for (x = 0; x < w; x++) {
            int kk = rp[x];
            int ca = p62_ca[kk], sa = p62_sa[kk];
            int dx = p62_dx[x];
            int rx = (dx * ca - dy * sa) >> 8;
            int ry = (dx * sa + dy * ca) >> 8;
            int d, dq, dep, v, sect, idx, s, glow, lum;
            if (rx < 0) rx = -rx;
            if (ry < 0) ry = -ry;
            d = rx > ry ? rx : ry;
            dq = d >> 2; if (dq > P62_DQ - 1) dq = P62_DQ - 1;
            dep = p62_dep[dq];                        /* depth * 16 */
            v = ((dep * 109) >> 6) + fly;             /* depth*1.7*16 + fly */
            sect = ((ap[x] + p62_pb[kk] + 65536) >> p62_sh) & p62_smask;
            idx = ((v >> 4) + sect * 32) & 255;
            s = p62_sin[(((dep * 326) >> 6) + gph) & 1023];
            glow = 46 + ((s * 18) >> 15);             /* (0.72+0.28 sin)*64 */
            lum = (glow * p62_shd[dq]) >> 6;
            if (lum > 63) lum = 63;
            out[x] = p62_lut[lum][idx];
        }
    }
}
