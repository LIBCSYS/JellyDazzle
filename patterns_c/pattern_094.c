/* 094 Thread Web — port of lab/patterns/094_thread_web/proto.py
 * Violet flood; fine curved rainbow threads accumulate corner-to-centre through
 * a 4-fold mirror until the screen knots into an X/butterfly web. Rainbow wedge
 * fans park at the side edges, green pinwheel triangles hug the corners.
 * Accumulator: the 640x480 canvas is flooded + furnished when sl < 2, then only
 * new threads are stamped each frame and the canvas is blitted out. */
#include "../jellydazzle.h"
#include <math.h>

#define P94_LW 640
#define P94_LH 480
#define P94_N  (P94_LW * P94_LH)
#define P94_STEPS 300

static uint32_t p94_low[P94_N];
static float    p94_hue[256][3];
static int      p94_drawn = -1;
static uint32_t p94_seed = 0;

static void p94_buildhue(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i << 7) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g, mn = r < g ? r : g, k;
        if (b > mx) mx = b; if (b < mn) mn = b;
        if (mx < 20.0f) mx = 20.0f;
        k = (mn / mx) * 0.7f;
        p94_hue[i][0] = (r / mx - k) / (1.0f - k);
        p94_hue[i][1] = (g / mx - k) / (1.0f - k);
        p94_hue[i][2] = (b / mx - k) / (1.0f - k);
    }
}

static uint32_t p94_rs;
static uint32_t p94_rnd(void)
{
    p94_rs ^= p94_rs << 13; p94_rs ^= p94_rs >> 17; p94_rs ^= p94_rs << 5;
    return p94_rs;
}
static float p94_r01(void) { return (float)(p94_rnd() >> 8) * (1.0f / 16777216.0f); }

static uint32_t p94_pack(const float *c, float v)
{
    int r = (int)(c[0] * v), g = (int)(c[1] * v), b = (int)(c[2] * v);
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static void p94_base(void)
{
    int y, x, i;
    for (y = 0; y < P94_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P94_LH);
        float dy = ly - 120.0f, ay = dy < 0.0f ? -dy : dy;
        uint32_t *out = p94_low + (long)y * P94_LW;
        for (x = 0; x < P94_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P94_LW);
            float dx = lx - 160.0f, ax = dx < 0.0f ? -dx : dx;
            uint32_t c = 0xFF5822C8u;                       /* violet flood */
            if (ax > 118.0f && ay < 0.45f * (ax - 112.0f)) {
                int s = (int)floorf((dy + ax * 0.55f) / 7.0f);
                s = ((s % 6) + 6) % 6;
                c = 0xFF000000u | p94_pack(p94_hue[(s * 256 / 6 + 20) & 255], 255.0f);
            }
            if ((ax + ay * 1.4f) > 300.0f && (ax - ay * 0.7f) > 60.0f) {
                int g = (int)(120.0f + 90.0f * sinf(ax * 0.15f));
                if (g < 0) g = 0; if (g > 255) g = 255;
                c = 0xFF000000u | (30u << 16) | ((uint32_t)g << 8) | 60u;
            }
            out[x] = c;
        }
    }
    (void)i;
}

static void p94_thread(int i)
{
    float x0, y0, x1, y1, swing, hue;
    float c[3];
    uint32_t col;
    int k;
    p94_rs = (uint32_t)(9000 + i) * 2654435761u ^ p94_seed;
    if (!p94_rs) p94_rs = 1u;
    p94_rnd(); p94_rnd();
    x0 = p94_r01() * 55.0f;
    y0 = p94_r01() * 45.0f;
    x1 = 160.0f - p94_r01() * 30.0f;
    y1 = 120.0f - p94_r01() * 26.0f;
    swing = sinf((float)i * 0.05f) * 30.0f + (p94_r01() * 28.0f - 14.0f);
    hue = p94_r01();
    {
        const float *hp = p94_hue[(int)(hue * 256.0f) & 255];
        c[0] = hp[0]; c[1] = hp[1]; c[2] = hp[2];
    }
    col = p94_pack(c, 255.0f);
    for (k = 0; k < P94_STEPS; k++) {
        float s = (float)k * (1.0f / (float)(P94_STEPS - 1));
        float bow = sinf(s * 3.14159265f);
        float px = x0 + (x1 - x0) * s + swing * bow;
        float py = y0 + (y1 - y0) * s - swing * 0.7f * bow;
        int ix, iy;
        if (px < 0.0f) px = 0.0f; if (px > 159.0f) px = 159.0f;
        if (py < 0.0f) py = 0.0f; if (py > 119.0f) py = 119.0f;
        ix = (int)(px * 2.0f); iy = (int)(py * 2.0f);
        if (ix > P94_LW / 2 - 1) ix = P94_LW / 2 - 1;
        if (iy > P94_LH / 2 - 1) iy = P94_LH / 2 - 1;
        p94_low[(long)iy * P94_LW + ix] = col;
        p94_low[(long)iy * P94_LW + (P94_LW - 1 - ix)] = col;
        p94_low[(long)(P94_LH - 1 - iy) * P94_LW + ix] = col;
        p94_low[(long)(P94_LH - 1 - iy) * P94_LW + (P94_LW - 1 - ix)] = col;
    }
}

void pattern_094(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int target, i, x, y;
    uint32_t stepx;
    (void)frame;

    p94_buildhue(pal);
    if (sl < 2 || p94_drawn < 0 || seed != p94_seed) {
        p94_seed = seed;
        p94_base();
        p94_drawn = 0;
    }
    target = 8 + (int)((float)sl * 1.2f);
    if (target > p94_drawn + 24) target = p94_drawn + 24;
    for (i = p94_drawn; i < target; i++) p94_thread(i);
    if (target > p94_drawn) p94_drawn = target;

    stepx = ((uint32_t)P94_LW << 16) / (uint32_t)w;
    for (y = 0; y < h; y++) {
        int sy = (int)(((long)y * P94_LH) / h);
        const uint32_t *src = p94_low + (long)sy * P94_LW;
        uint32_t *dst = fb + (long)y * w;
        uint32_t acc = 0;
        for (x = 0; x < w; x++) { dst[x] = src[acc >> 16]; acc += stepx; }
    }
}
