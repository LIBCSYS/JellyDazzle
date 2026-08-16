/* 061 Checker Tunnel — flying polar tunnel, spiral bands + checker shading.
 * Port of lab/patterns/061_checker_tunnel/proto.py */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>

static int16_t p61_sin[1024];
static int p61_sin_ok = 0;
static int p61_w = 0, p61_h = 0;
static uint16_t *p61_ang3;   /* ang*3 (0..768) in x64 fixed, unwrapped */
static uint16_t *p61_dep;    /* depth (<=372) in x64 fixed */
static uint16_t *p61_wob;    /* sin-table phase index of depth*0.05, 0..1023 */
static uint8_t  *p61_shi;    /* shade * 63 (bright checker) */
static uint8_t  *p61_slo;    /* shade * 0.65 * 63 (dim checker) */
static uint32_t p61_lut[64][256];

static void p61_init(int w, int h) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (!p61_sin_ok) {
        for (i = 0; i < 1024; i++)
            p61_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
        p61_sin_ok = 1;
    }
    if (p61_w == w && p61_h == h) return;
    free(p61_ang3); free(p61_dep); free(p61_wob); free(p61_shi); free(p61_slo);
    p61_ang3 = (uint16_t *)malloc((size_t)w * h * 2);
    p61_dep  = (uint16_t *)malloc((size_t)w * h * 2);
    p61_wob  = (uint16_t *)malloc((size_t)w * h * 2);
    p61_shi  = (uint8_t *)malloc((size_t)w * h);
    p61_slo  = (uint8_t *)malloc((size_t)w * h);
    for (y = 0, i = 0; y < h; y++) {
        double py = (y - h * 0.5) * sy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx;
            double r = hypot(px, py) + 1e-3;
            double a = atan2(py, px);
            double ang = (a / M_PI + 1.0) * 128.0;          /* 0..256 */
            double depth = 5200.0 / (r + 14.0);
            double shade = r / (r + 60.0);
            p61_ang3[i] = (uint16_t)(ang * 3.0 * 64.0);     /* <= 49152 */
            p61_dep[i]  = (uint16_t)(depth * 64.0);
            p61_wob[i]  = (uint16_t)fmod(depth * 0.05 * (1024.0 / (2.0 * M_PI)), 1024.0);
            p61_shi[i]  = (uint8_t)(shade * 63.0);
            p61_slo[i]  = (uint8_t)(shade * 0.65 * 63.0);
        }
    }
    p61_w = w; p61_h = h;
}

/* smooth looping 6-stop rainbow sampled from the engine palette (the lab
 * proto uses a 6-stop looping rainbow; raw pal indexing is too high-frequency
 * and would strobe). */
static void p61_build_lut(const uint32_t *pal) {
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
        for (s = 0; s < 64; s++) {
            uint32_t k = (uint32_t)(s * 65536 / 63);
            p61_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_061(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int i, n = w * h;
    (void)sl; (void)seed;
    p61_init(w, h);
    p61_build_lut(pal);
    {
        double t = (double)frame;
        /* idx-domain accumulators (mod 256 units, x64) */
        int rot_c = (int)(fmod(t * 0.09, 256.0) * 64.0);
        int fly_c = (int)(fmod(t * 0.42, 256.0) * 64.0);
        /* checker-domain accumulators (mod 42 u-units = 2688 in x64) */
        int rot_k = (int)fmod(t * 0.09 * 64.0, 2688.0);
        int fly_k = (int)fmod(t * 0.42 * 64.0, 2688.0);
        int dt = (int)fmod(t * 0.01 * (1024.0 / (2.0 * M_PI)), 1024.0);
        for (i = 0; i < n; i++) {
            int s = p61_sin[(p61_wob[i] + dt) & 1023];
            int wob = (s * 384) >> 15;                   /* +-6 in x64 */
            int a3 = p61_ang3[i], dp = p61_dep[i];
            int idx = ((a3 + rot_c + wob + dp + fly_c) >> 6) & 255;
            /* checker: floor(u/21)+floor(v/21) parity; /1344 via mul-shift */
            unsigned uk = (unsigned)(a3 + rot_k + wob + 10752);
            unsigned vk = (unsigned)(dp + fly_k);
            int par = (int)(((uk * 3121u) >> 22) + ((vk * 3121u) >> 22)) & 1;
            int lum = par ? p61_shi[i] : p61_slo[i];
            fb[i] = p61_lut[lum][idx];
        }
    }
}
