/* pattern_035 — String Cardioid
 * Port of lab/patterns/035_string_cardioid/proto.py
 * Times-table string art: chord m joins pin p = (7m mod 180) to pin k*p on a
 * circle of 180 pins, with the multiplier k breathing 2 -> 3.6 so the caustic
 * envelope morphs cardioid -> nephroid -> three-cusp.  Chords are rasterised
 * by fixed-step DDA into a 320x240 RGB accumulator with exponential age fade
 * (per chord, tau 900), mirrored in y, blurred, tone-mapped through
 * 1-exp(-a*gain), warm-vignette blended and bilinearly upscaled.
 * Colour is rim-anchored: hue = pin index, so the wheel reads as a stable
 * rainbow ring.  Accumulator pattern. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define P35_AW    320
#define P35_AH    240
#define P35_AN    (P35_AW * P35_AH)
#define P35_MAXD  4096
#define P35_BASE  260
#define P35_CPF   3            /* chords per frame                 */
#define P35_STEPS 44           /* DDA samples per chord            */
#define P35_PINS  180
#define P35_RC    108.0
#define P35_GAIN  0.22f
#define P35_SAT   0.85f
#define P35_TSC   585.14f      /* 4096/7 : tone LUT index scale    */

static float    p35_ar[P35_AN], p35_ag[P35_AN], p35_ab[P35_AN];
static float    p35_br[P35_AN], p35_bg_[P35_AN], p35_bb[P35_AN];
static uint32_t p35_low[P35_AN];
static uint8_t  p35_bgv[P35_AN * 3];
static uint8_t  p35_lin[4096];
static uint8_t  p35_gam[256];
static float    p35_pinx[P35_PINS], p35_piny[P35_PINS];
static int      p35_ready = 0;
static int      p35_lastsl = -1000;
static double   p35_m = 0.0;      /* chord counter */
static double   p35_t = 0.0;      /* frame counter within segment */
static int      p35_uw = -1, p35_uh = -1;
static int32_t  p35_x0[P35_MAXD], p35_y0[P35_MAXD];
static uint8_t  p35_fx[P35_MAXD], p35_fy[P35_MAXD];

static void p35_tabs(void) {
    for (int i = 0; i < 4096; i++)
        p35_lin[i] = (uint8_t)((1.0 - exp(-(double)i * 7.0 / 4096.0)) * 255.0 + 0.5);
    for (int i = 0; i < 256; i++)
        p35_gam[i] = (uint8_t)(pow((double)i / 255.0, 0.85) * 255.0 + 0.5);
    for (int y = 0; y < P35_AH; y++)
        for (int x = 0; x < P35_AW; x++) {
            double dx = (x - P35_AW * 0.5) / P35_AW;
            double dy = (y - P35_AH * 0.5) / P35_AH;
            double e = exp(-(dx * dx + dy * dy) * 3.0) * 255.0;
            int i = (y * P35_AW + x) * 3;
            p35_bgv[i + 0] = (uint8_t)(e * 0.05 + 0.5);
            p35_bgv[i + 1] = (uint8_t)(e * 0.03 + 0.5);
            p35_bgv[i + 2] = 0;
        }
    for (int i = 0; i < P35_PINS; i++) {
        double a = i * (6.28318530717959 / P35_PINS);
        p35_pinx[i] = (float)(P35_RC * cos(a));
        p35_piny[i] = (float)(P35_RC * sin(a));
    }
    p35_ready = 1;
}

/* one chord, deposited twice (y-mirror) */
static void p35_chord(double m, float w, double hdrift, const uint32_t *pal) {
    double kk = 2.0 + 1.6 * (0.5 - 0.5 * cos(m * 0.0011));
    double pf = fmod(m * 7.0, (double)P35_PINS);
    if (pf < 0.0) pf += (double)P35_PINS;
    double a1 = pf * (6.28318530717959 / P35_PINS);
    double a2 = kk * pf * (6.28318530717959 / P35_PINS);
    float x1 = (float)(P35_RC * cos(a1)), y1 = (float)(P35_RC * sin(a1));
    float x2 = (float)(P35_RC * cos(a2)), y2 = (float)(P35_RC * sin(a2));

    double hue = pf / (double)P35_PINS + hdrift;
    int idx = (int)(hue * 32768.0) & JD_PAL_MASK;
    uint32_t c = pal[idx];
    int pr = (c >> 16) & 255, pg = (c >> 8) & 255, pb = c & 255;
    int hi = pr > pg ? pr : pg; if (pb > hi) hi = pb;
    int lo = pr < pg ? pr : pg; if (pb < lo) lo = pb;
    float cr, cg, cb;
    if (hi - lo < 1) { cr = cg = cb = 1.0f; }
    else {                       /* value 1, saturation P35_SAT, hue kept */
        float k1 = 1.0f - P35_SAT, k2 = P35_SAT / (float)(hi - lo);
        cr = k1 + k2 * (pr - lo);
        cg = k1 + k2 * (pg - lo);
        cb = k1 + k2 * (pb - lo);
    }
    cr *= w; cg *= w; cb *= w;

    const float ox = P35_AW * 0.5f, oy = P35_AH * 0.5f;
    const float du = 1.0f / (float)(P35_STEPS - 1);
    float dx = (x2 - x1) * du, dy = (y2 - y1) * du;
    float px = x1, py = y1;
    for (int i = 0; i < P35_STEPS; i++, px += dx, py += dy) {
        int xi = (int)lrintf(ox + px);
        int ya = (int)lrintf(oy + py), yb = (int)lrintf(oy - py);
        if ((unsigned)xi < (unsigned)P35_AW) {
            if ((unsigned)ya < (unsigned)P35_AH) {
                int j = ya * P35_AW + xi;
                p35_ar[j] += cr; p35_ag[j] += cg; p35_ab[j] += cb;
            }
            if ((unsigned)yb < (unsigned)P35_AH) {
                int j = yb * P35_AW + xi;
                p35_ar[j] += cr; p35_ag[j] += cg; p35_ab[j] += cb;
            }
        }
    }
}

static void p35_hblur1(const float *src, float *dst) {
    for (int y = 0; y < P35_AH; y++) {
        const float *r = src + y * P35_AW;
        float *o = dst + y * P35_AW;
        o[0] = 0.75f * r[0] + 0.25f * r[1];
        for (int x = 1; x < P35_AW - 1; x++)
            o[x] = 0.5f * r[x] + 0.25f * (r[x - 1] + r[x + 1]);
        o[P35_AW - 1] = 0.75f * r[P35_AW - 1] + 0.25f * r[P35_AW - 2];
    }
}

static void p35_compose(void) {
    p35_hblur1(p35_ar, p35_br);
    p35_hblur1(p35_ag, p35_bg_);
    p35_hblur1(p35_ab, p35_bb);
    for (int y = 0; y < P35_AH; y++) {
        int yu = y ? y - 1 : 0, yd = (y < P35_AH - 1) ? y + 1 : y;
        const float *r0 = p35_br + y * P35_AW, *r1 = p35_br + yu * P35_AW,
                    *r2 = p35_br + yd * P35_AW;
        const float *g0 = p35_bg_ + y * P35_AW, *g1 = p35_bg_ + yu * P35_AW,
                    *g2 = p35_bg_ + yd * P35_AW;
        const float *b0 = p35_bb + y * P35_AW, *b1 = p35_bb + yu * P35_AW,
                    *b2 = p35_bb + yd * P35_AW;
        uint32_t *out = p35_low + y * P35_AW;
        const uint8_t *bg = p35_bgv + y * P35_AW * 3;
        for (int x = 0; x < P35_AW; x++) {
            float ar = 0.5f * r0[x] + 0.25f * (r1[x] + r2[x]);
            float ag = 0.5f * g0[x] + 0.25f * (g1[x] + g2[x]);
            float ab = 0.5f * b0[x] + 0.25f * (b1[x] + b2[x]);
            int tr = (int)(ar * (P35_GAIN * P35_TSC));
            int tg = (int)(ag * (P35_GAIN * P35_TSC));
            int tb = (int)(ab * (P35_GAIN * P35_TSC));
            if (tr > 4095) tr = 4095; if (tg > 4095) tg = 4095; if (tb > 4095) tb = 4095;
            int vr = p35_lin[tr], vg = p35_lin[tg], vb = p35_lin[tb];
            vr += ((255 - vr) * bg[x * 3 + 0]) >> 8;
            vg += ((255 - vg) * bg[x * 3 + 1]) >> 8;
            vb += ((255 - vb) * bg[x * 3 + 2]) >> 8;
            out[x] = 0xFF000000u | ((uint32_t)p35_gam[vr] << 16)
                   | ((uint32_t)p35_gam[vg] << 8) | p35_gam[vb];
        }
    }
}

static uint32_t p35_lerp(uint32_t a, uint32_t b, int f) {
    uint32_t rb = ((((a & 0xFF00FFu) * (256 - f)) + ((b & 0xFF00FFu) * f)) >> 8) & 0xFF00FFu;
    uint32_t g  = ((((a & 0x00FF00u) * (256 - f)) + ((b & 0x00FF00u) * f)) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p35_utab(int w, int h) {
    for (int x = 0; x < w && x < P35_MAXD; x++) {
        long f = ((long)(2 * x + 1) * P35_AW << 15) / w - (1L << 15);
        if (f < 0) f = 0;
        long mx = ((long)(P35_AW - 1)) << 16; if (f > mx) f = mx;
        p35_x0[x] = (int32_t)(f >> 16);
        p35_fx[x] = (uint8_t)((f >> 8) & 255);
    }
    for (int y = 0; y < h && y < P35_MAXD; y++) {
        long f = ((long)(2 * y + 1) * P35_AH << 15) / h - (1L << 15);
        if (f < 0) f = 0;
        long my = ((long)(P35_AH - 1)) << 16; if (f > my) f = my;
        p35_y0[y] = (int32_t)(f >> 16);
        p35_fy[y] = (uint8_t)((f >> 8) & 255);
    }
    p35_uw = w; p35_uh = h;
}

static void p35_upscale(uint32_t *fb, int w, int h) {
    static uint32_t row[P35_AW];
    if (w > P35_MAXD || h > P35_MAXD) {
        for (int y = 0; y < h; y++) {
            const uint32_t *s = p35_low + (y * P35_AH / h) * P35_AW;
            uint32_t *d = fb + (long)y * w;
            for (int x = 0; x < w; x++) d[x] = s[x * P35_AW / w];
        }
        return;
    }
    if (w != p35_uw || h != p35_uh) p35_utab(w, h);
    for (int y = 0; y < h; y++) {
        int y0 = p35_y0[y], fy = p35_fy[y];
        int y1 = (y0 < P35_AH - 1) ? y0 + 1 : y0;
        const uint32_t *r0 = p35_low + y0 * P35_AW, *r1 = p35_low + y1 * P35_AW;
        if (fy) for (int i = 0; i < P35_AW; i++) row[i] = p35_lerp(r0[i], r1[i], fy);
        else    memcpy(row, r0, sizeof row);
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int x0 = p35_x0[x];
            int x1 = (x0 < P35_AW - 1) ? x0 + 1 : x0;
            d[x] = 0xFF000000u | p35_lerp(row[x0], row[x1], p35_fx[x]);
        }
    }
}

void pattern_035(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)frame;
    if (!p35_ready) p35_tabs();
    if (sl < 2 || sl != p35_lastsl + 1) {
        memset(p35_ar, 0, sizeof p35_ar);
        memset(p35_ag, 0, sizeof p35_ag);
        memset(p35_ab, 0, sizeof p35_ab);
        p35_m = (double)(seed & 8191u);
        p35_t = 0.0;
        for (int f = 0; f < P35_BASE; f++) {
            float wg = expf(-(float)((P35_BASE - f) * P35_CPF) / 900.0f);
            for (int c = 0; c < P35_CPF; c++, p35_m += 1.0)
                p35_chord(p35_m, wg, p35_t * 0.0003, pal);
            p35_t += 1.0;
        }
    } else {
        const float K = 0.99667221f;               /* exp(-3/900) per frame */
        for (int i = 0; i < P35_AN; i++) {
            p35_ar[i] *= K; p35_ag[i] *= K; p35_ab[i] *= K;
        }
        for (int c = 0; c < P35_CPF; c++, p35_m += 1.0)
            p35_chord(p35_m, 1.0f, p35_t * 0.0003, pal);
        p35_t += 1.0;
    }
    p35_lastsl = sl;
    p35_compose();
    p35_upscale(fb, w, h);
}
