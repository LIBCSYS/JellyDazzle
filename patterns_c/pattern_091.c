/* 091 Diamond Confetti — port of lab/patterns/091_diamond_confetti/proto.py
 * One huge rhombus: magenta inner rim + cyan outer rim, an interior packed with
 * a 4-fold mirrored quilt of 6px rainbow confetti cells, expanding concentric
 * diamond outlines born at the centre, and thin cyan streaks off the side
 * vertices. Full-repaint pattern, logical 320x240 coordinate space. */
#include "../jellydazzle.h"
#include <math.h>

#define P91_CW 28          /* confetti cells across a half-diamond */
#define P91_CH 22

static uint32_t p91_cell[P91_CH][P91_CW];   /* per-frame confetti colours */
static float    p91_hue[256][3];            /* brightness-normalised palette */

static void p91_buildhue(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i << 7) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255);
        float g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g, mn = r < g ? r : g, k;
        if (b > mx) mx = b; if (b < mn) mn = b;
        if (mx < 20.0f) mx = 20.0f;
        k = (mn / mx) * 0.72f;                      /* saturation boost */
        p91_hue[i][0] = (r / mx - k) / (1.0f - k);
        p91_hue[i][1] = (g / mx - k) / (1.0f - k);
        p91_hue[i][2] = (b / mx - k) / (1.0f - k);
    }
}

static void p91_cells(float t)
{
    int cy, cx;
    for (cy = 0; cy < P91_CH; cy++) {
        for (cx = 0; cx < P91_CW; cx++) {
            int h1 = (int)(((uint32_t)cx * 73856093u) ^ ((uint32_t)cy * 19349663u)) & 255;
            int h2 = (int)(((uint32_t)cx * 83492791u) ^ ((uint32_t)cy * 2971215073u)) & 255;
            float hu = (float)h1 * (1.0f / 255.0f)
                     + t * 0.0022f * (0.5f + (float)h2 * (1.0f / 255.0f));
            float val = 0.66f + 0.34f * sinf((float)h1 * 0.21f + t * 0.01f + (float)h2 * 0.05f);
            int hi, r, g, b;
            hu = hu - floorf(hu);
            hi = (int)(hu * 256.0f) & 255;
            if (val < 0.0f) val = -val;
            if (val > 1.0f) val = 1.0f;
            val *= 255.0f;
            r = (int)(p91_hue[hi][0] * val);
            g = (int)(p91_hue[hi][1] * val);
            b = (int)(p91_hue[hi][2] * val);
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            p91_cell[cy][cx] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}

void pattern_091(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sxf = 320.0f / (float)w, syf = 240.0f / (float)h;
    float tsh = t * 0.016f;
    float puls, sstep, sph0;
    int x, y;
    (void)sl; (void)seed;

    tsh = tsh - 3.0f * floorf(tsh * (1.0f / 3.0f));   /* keep floor(ph)%3 stable */
    puls = 0.8f + 0.2f * sinf(t * 0.02f);
    sstep = 0.20f * sxf;
    sph0 = -t * 0.05f;

    p91_buildhue(pal);
    p91_cells(t);

    for (y = 0; y < h; y++) {
        float ly = ((float)y + 0.5f) * syf;
        float ay = ly - 120.0f; if (ay < 0.0f) ay = -ay;
        float ayk = ay * (1.0f / 92.0f);
        int   cyi = (int)(ay * (1.0f / 6.0f));
        int   streakrow = (ay < 3.0f);
        uint32_t *dst = fb + (long)y * (long)w;
        if (cyi >= P91_CH) cyi = P91_CH - 1;
        for (x = 0; x < w; x++) {
            float lx = ((float)x + 0.5f) * sxf;
            float ax = lx - 160.0f; if (ax < 0.0f) ax = -ax;
            float dd = ax * (1.0f / 148.0f) + ayk;
            uint32_t c = 0;
            if (dd < 0.88f) {
                int cxi = (int)(ax * (1.0f / 6.0f));
                float ph, fr;
                if (cxi >= P91_CW) cxi = P91_CW - 1;
                c = p91_cell[cyi][cxi];
                ph = dd * 5.0f - tsh + 6.0f;
                fr = ph - (float)(int)ph;
                if (fr < 0.16f && dd > 0.06f) {
                    int k = (int)ph % 3;
                    float fade = dd * (1.0f / 0.9f);
                    int r, g, b;
                    if (fade > 1.0f) fade = 1.0f;
                    fade = 0.35f + 0.65f * fade;
                    if (k == 0)      { r = (int)(0.0f   * fade); g = (int)(255.0f * fade); b = (int)(220.0f * fade); }
                    else if (k == 1) { r = (int)(255.0f * fade); g = (int)(40.0f  * fade); b = (int)(255.0f * fade); }
                    else             { r = (int)(60.0f  * fade); g = (int)(255.0f * fade); b = (int)(80.0f  * fade); }
                    c = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
                }
            } else if (dd < 0.94f) {
                c = 0x00FF28FFu;                       /* magenta inner rim */
            } else if (dd < 1.02f) {
                int g = (int)(255.0f * puls), b = (int)(220.0f * puls);
                c = ((uint32_t)g << 8) | (uint32_t)b;  /* cyan outer rim */
            } else if (streakrow) {
                float sp = 0.5f + 0.5f * sinf(ax * 0.20f + sph0);
                int r = (int)(120.0f * sp), g = (int)(255.0f * sp), b = (int)(235.0f * sp);
                c = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
            }
            dst[x] = 0xFF000000u | c;
        }
        (void)sstep;
    }
}
