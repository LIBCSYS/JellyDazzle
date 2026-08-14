/* 100 Pinwheel Swirl — port of lab/patterns/100_pinwheel_swirl/proto.py
 * Six smooth-shaded comma arms spiral out of the centre over black, turning
 * slowly clockwise while the spiral tightness breathes and the shading ramp
 * rolls through the palette. The one true computed field: per-pixel angle and
 * radius are baked once, the frame is a phase add plus two table reads. */
#include "../jellydazzle.h"
#include <math.h>

#define P100_LW 640
#define P100_LH 480
#define P100_N  (P100_LW * P100_LH)
#define P100_PI 3.14159265f

static uint32_t p100_low[P100_N];
static uint16_t p100_a6[P100_N];      /* 6*theta in 2048-step units      */
static uint16_t p100_rp[P100_N];      /* r^0.95 * 64                     */
static uint8_t  p100_edge[P100_N];    /* radial fade, 0..255             */
static int8_t   p100_dith[P100_N];    /* static ordered dither, +-9      */
static uint8_t  p100_shp[2048];       /* ((cos+1)/2)^1.3 * 255           */
static uint32_t p100_ramp[256];       /* per-frame shade -> ARGB         */
static float    p100_hue[256][3];
static int      p100_ready;

static void p100_buildhue(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i << 7) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g, mn = r < g ? r : g, k;
        if (b > mx) mx = b; if (b < mn) mn = b;
        if (mx < 20.0f) mx = 20.0f;
        k = (mn / mx) * 0.75f;
        p100_hue[i][0] = (r / mx - k) / (1.0f - k);
        p100_hue[i][1] = (g / mx - k) / (1.0f - k);
        p100_hue[i][2] = (b / mx - k) / (1.0f - k);
    }
}

static uint32_t p100_rs = 77u;
static uint32_t p100_rnd(void)
{
    p100_rs ^= p100_rs << 13; p100_rs ^= p100_rs >> 17; p100_rs ^= p100_rs << 5;
    return p100_rs;
}

static void p100_init(void)
{
    int i, x, y;
    for (i = 0; i < 2048; i++) {
        float c = cosf((float)i * (2.0f * P100_PI / 2048.0f));
        float s = powf((c + 1.0f) * 0.5f, 1.3f) * 255.0f;
        p100_shp[i] = (uint8_t)(s + 0.5f);
    }
    for (y = 0; y < P100_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P100_LH);
        float dy = ly - 120.0f;
        for (x = 0; x < P100_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P100_LW);
            float dx = lx - 160.0f;
            float r = sqrtf(dx * dx + dy * dy);
            float th = atan2f(dy, dx);
            float e = 1.15f - r / 195.0f;
            int idx = y * P100_LW + x;
            int a6 = (int)(6.0f * th * (2048.0f / (2.0f * P100_PI)));
            if (e < 0.0f) e = 0.0f; if (e > 1.0f) e = 1.0f;
            p100_a6[idx] = (uint16_t)(((a6 % 2048) + 4096) & 2047);
            p100_rp[idx] = (uint16_t)(powf(r, 0.95f) * 64.0f);
            p100_edge[idx] = (uint8_t)(e * 255.0f + 0.5f);
            p100_dith[idx] = (int8_t)((int)(p100_rnd() % 19u) - 9);
        }
    }
    p100_ready = 1;
}

static void p100_buildramp(float hoff)
{
    int i;
    for (i = 0; i < 256; i++) {
        float s = (float)i * (1.0f / 255.0f);
        float hue = 0.34f - s * 0.30f + hoff;
        float sat = 1.15f - s * 0.35f;
        float val = powf(s, 0.85f) * 255.0f;
        const float *c;
        float wgt;
        int r, g, b, hi;
        if (sat > 1.0f) sat = 1.0f; if (sat < 0.0f) sat = 0.0f;
        hue = hue - floorf(hue);
        hi = (int)(hue * 256.0f) & 255;
        c = p100_hue[hi];
        wgt = 1.0f - sat;
        r = (int)((wgt + sat * c[0]) * val);
        g = (int)((wgt + sat * c[1]) * val);
        b = (int)((wgt + sat * c[2]) * val);
        if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
        if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
        p100_ramp[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
}

static void p100_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P100_LW << 16) / w);
    int fx0 = (int)(((long)P100_LW << 15) / w) - (1 << 15);
    int maxx = (P100_LW - 1) << 16, maxy = (P100_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P100_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P100_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p100_low + (long)y0 * P100_LW;
        r1 = p100_low + (long)y1 * P100_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P100_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx, sy2 = 256u - (unsigned)wy;
            uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            uint32_t trb = (((a & 0xFF00FFu) * sx + (b & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t tg  = (((a & 0x00FF00u) * sx + (b & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t brb = (((c & 0xFF00FFu) * sx + (d & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t bg  = (((c & 0x00FF00u) * sx + (d & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t orb = ((trb * sy2 + brb * (unsigned)wy) >> 8) & 0xFF00FFu;
            uint32_t og  = ((tg  * sy2 + bg  * (unsigned)wy) >> 8) & 0x00FF00u;
            dst[x] = 0xFF000000u | orb | og;
            fx += stepx;
        }
    }
}

void pattern_100(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float twist, kq, hoff, spin;
    int tph, i, n;
    (void)sl; (void)seed;

    if (!p100_ready) p100_init();
    p100_buildhue(pal);

    spin = t * 0.003f; spin -= 2.0f * P100_PI * floorf(spin / (2.0f * P100_PI));
    twist = 0.085f * (1.0f + 0.22f * sinf(spin));
    kq = twist * (2048.0f / (2.0f * P100_PI)) / 64.0f;
    tph = (int)(t * 0.012f * (2048.0f / (2.0f * P100_PI)));
    hoff = -t * 0.00055f; hoff -= floorf(hoff);
    p100_buildramp(hoff);

    n = P100_N;
    for (i = 0; i < n; i++) {
        int idx = ((int)p100_a6[i] + (int)(kq * (float)p100_rp[i]) - tph) & 2047;
        int s = (((int)p100_shp[idx] * (int)p100_edge[i]) >> 8) + (int)p100_dith[i];
        if (s < 0) s = 0; else if (s > 255) s = 255;
        p100_low[i] = p100_ramp[s];
    }
    p100_blit(fb, w, h);
}
