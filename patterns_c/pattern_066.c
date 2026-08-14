/* 066 Hex Tunnel — neon hexagon hoops receding into a black vanishing point,
 * the shaft rotating almost imperceptibly, corners catching extra glow.
 * Port of lab/patterns/066_hex_tunnel/proto.py (repaint, ignores sl). */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define P66_DQ 1024

static int16_t p66_sin[1024];
static uint8_t p66_cub[256];
static int p66_tab_ok = 0;
static int p66_w = 0, p66_h = 0;
static int16_t *p66_dx;     /* per-column dx, 1/16 of a 320-space pixel */
static int16_t *p66_dy;     /* per-row dy */
static uint16_t *p66_a6;    /* 6*angle, 1024-unit turn scaled x16 */
static uint16_t p66_dep[P66_DQ];   /* depth * 64, clamped to 300 */
static uint8_t  p66_shd[P66_DQ];   /* d/(d+46) * 63 */
static uint32_t p66_lut[64][256];

static void p66_init(int w, int h) {
    int x, y, i;
    double sx = 320.0 / w, sy = 240.0 / h;
    if (!p66_tab_ok) {
        for (i = 0; i < 1024; i++)
            p66_sin[i] = (int16_t)(32767.0 * sin(i * (2.0 * M_PI / 1024.0)));
        for (i = 0; i < 256; i++)
            p66_cub[i] = (uint8_t)(((i * i * i) >> 16) & 255);
        for (i = 0; i < P66_DQ; i++) {
            double d = i * 0.25;
            double dep = 2400.0 / (d + 8.0);
            if (dep > 300.0) dep = 300.0;
            p66_dep[i] = (uint16_t)(dep * 64.0);
            p66_shd[i] = (uint8_t)(63.0 * d / (d + 46.0));
        }
        p66_tab_ok = 1;
    }
    if (p66_w == w && p66_h == h) return;
    free(p66_dx); free(p66_dy); free(p66_a6);
    p66_dx = (int16_t *)malloc(sizeof(int16_t) * w);
    p66_dy = (int16_t *)malloc(sizeof(int16_t) * h);
    p66_a6 = (uint16_t *)malloc((size_t)w * h * 2);
    for (x = 0; x < w; x++) p66_dx[x] = (int16_t)((x - w * 0.5) * sx * 16.0);
    for (y = 0; y < h; y++) p66_dy[y] = (int16_t)((y - h * 0.5) * sy * 16.0);
    for (y = 0, i = 0; y < h; y++) {
        double py = (y - h * 0.5) * sy;
        for (x = 0; x < w; x++, i++) {
            double px = (x - w * 0.5) * sx;
            double ph = fmod(atan2(py, px) * 6.0 * (1024.0 / (2.0 * M_PI)) * 16.0,
                             16384.0);
            if (ph < 0) ph += 16384.0;
            p66_a6[i] = (uint16_t)ph;
        }
    }
    p66_w = w; p66_h = h;
}

/* smooth 6-stop looping ramp lifted from the engine palette */
static void p66_build_lut(const uint32_t *pal) {
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
            p66_lut[s][idx] = 0xFF000000u | (((r * k) >> 16) << 16)
                            | (((g * k) >> 16) << 8) | ((b * k) >> 16);
        }
    }
}

void pattern_066(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    int x, y, j, cj[3], sj[3];
    double t = (double)frame;
    double th = t * 0.0015;
    int rph, hue, vrot;
    (void)sl; (void)seed;
    p66_init(w, h);
    p66_build_lut(pal);
    for (j = 0; j < 3; j++) {
        double ax = th + j * (M_PI / 3.0);
        cj[j] = (int)(cos(ax) * 256.0);
        sj[j] = (int)(sin(ax) * 256.0);
    }
    /* x16 units of a 1024-step turn; 1 rad = 2607.6 units */
    rph  = (int)fmod(t * 0.10 * 2607.6, 16384.0);
    hue  = (int)fmod(t * 0.12 * 16.0, 4096.0);          /* idx * 16 */
    vrot = (int)fmod(th * 6.0 * 2607.6, 16384.0);

    for (y = 0; y < h; y++) {
        const uint16_t *ap = p66_a6 + (size_t)y * w;
        uint32_t *out = fb + (size_t)y * w;
        int dy = p66_dy[y];
        for (x = 0; x < w; x++) {
            int dx = p66_dx[x];
            int p0 = (dx * cj[0] + dy * sj[0]) >> 8;
            int p1 = (dx * cj[1] + dy * sj[1]) >> 8;
            int p2 = (dx * cj[2] + dy * sj[2]) >> 8;
            int d, dq, dep, idx, s, ring, vert, A, B, lum;
            if (p0 < 0) p0 = -p0;
            if (p1 < 0) p1 = -p1;
            if (p2 < 0) p2 = -p2;
            d = p0 > p1 ? p0 : p1;
            if (p2 > d) d = p2;
            dq = d >> 2; if (dq > P66_DQ - 1) dq = P66_DQ - 1;
            dep = p66_dep[dq];
            s = p66_sin[((((dep * 1304) >> 6) + rph) >> 4) & 1023];
            ring = p66_cub[(s + 32768) >> 8];                   /* hoop^3 */
            vert = (p66_sin[(((ap[x] - vrot + 65536 + 4096) >> 4)) & 1023]
                    + 32768) >> 8;                              /* corner glow */
            idx = ((((dep * 3686) >> 14) + hue) >> 4) & 255;
            A = 56 + ((200 * ring) >> 8);
            B = 184 + ((72 * vert) >> 8);
            lum = (((A * B) >> 8) * p66_shd[dq]) >> 8;
            if (lum > 63) lum = 63;
            out[x] = p66_lut[lum][idx];
        }
    }
}
