/* 149 Bubble Iris — soap films seen by thin-film interference.
 * Each bubble is a spherical shell of film; a ray entering at normalised
 * radius u travels 1/sqrt(1-u^2) times the film thickness, so the
 * interference order — and therefore the colour — runs away toward the rim
 * and the familiar tight rainbow banding appears there and nowhere else.
 * The films also drain: thickness carries a vertical gradient, so the bands
 * stack and creep downward, and each bubble's base thickness wanders on its
 * own slow sinusoid, walking it through the whole interference sequence.
 * Fresnel keeps the centres nearly transparent, which is why nine overlapping
 * bubbles never turn to mush and why the pattern layers over anything. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stddef.h>

#define P149_NB 9

static float p149_path[1024];      /* 1/sqrt(1-u^2), u^2 index */
static float p149_sin[2048];
static int p149_ready;

static inline float p149_s(float a)
{
    return p149_sin[((int)(a * 325.9493f + 32768.5f)) & 2047];
}

static void p149_init(void)
{
    for (int i = 0; i < 1024; i++) {
        float u2 = (float)i * (1.0f / 1024.0f);
        float v = 1.0f / sqrtf(1.0f - u2 * 0.9975f);
        p149_path[i] = v > 9.0f ? 9.0f : v;
    }
    for (int i = 0; i < 2048; i++)
        p149_sin[i] = sinf((float)i * (6.28318531f / 2048.0f));
    p149_ready = 1;
}

void pattern_149(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    if (!p149_ready) p149_init();

    const float t = (float)frame;
    const float sd = (float)(seed & 1023u) * 0.006136f;
    const float M = (float)(w < h ? w : h);

    /* ground: the palette's shadow end, very dark, with a soft vignette */
    uint32_t gc = pal[(uint32_t)((int)(t * 0.5f) + 700 + (int)(seed & 4095u)) & JD_PAL_MASK];
    int gr = (int)((gc >> 16) & 255u), gg = (int)((gc >> 8) & 255u), gb = (int)(gc & 255u);
    for (int y = 0; y < h; y++) {
        int dy = y - h / 2;
        int v = 44 - (dy * dy * 30) / (h * h / 4 + 1);
        if (v < 8) v = 8;
        uint32_t c = 0xFF000000u | ((uint32_t)((gr * v) >> 8) << 16)
                   | ((uint32_t)((gg * v) >> 8) << 8) | (uint32_t)((gb * v) >> 8);
        uint32_t *row = fb + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) row[x] = c;
    }

    const int cidx = (int)(t * 0.9f) + (int)(seed & 8191u);

    for (int i = 0; i < P149_NB; i++) {
        float fi = (float)i;
        float R = M * (0.085f + 0.032f * (float)(i % 5))
                * (1.0f + 0.05f * p149_s(t * 0.0022f + fi));
        float cx = (float)w * (0.5f + 0.40f * p149_s(t * 0.00061f * (1.0f + 0.19f * fi)
                                                     + fi * 2.1f + sd));
        float cy = (float)h * (0.5f + 0.40f * p149_s(t * 0.00048f * (1.0f + 0.23f * fi)
                                                     + fi * 3.7f + sd * 1.7f));
        /* film thickness: base walk + vertical drain gradient */
        float d0 = 1.0f + 0.62f * p149_s(t * 0.00083f + fi * 1.3f + sd);
        float drain = 0.55f + 0.22f * p149_s(t * 0.00037f + fi);
        int bi = cidx + i * 900;
        float kscale = 2400.0f * (0.7f + 0.35f * (float)(i % 3));
        float spx = cx - R * 0.42f, spy = cy - R * 0.42f;
        float sprad = R * 0.16f, isp = 1.0f / (sprad * sprad);

        int y0 = (int)(cy - R), y1 = (int)(cy + R) + 1;
        if (y0 < 0) y0 = 0;
        if (y1 > h) y1 = h;
        float iR2 = 1.0f / (R * R), iR = 1.0f / R;
        for (int y = y0; y < y1; y++) {
            float dy = (float)y + 0.5f - cy;
            float dy2 = dy * dy;
            if (dy2 >= R * R) continue;
            float xh = sqrtf(R * R - dy2);
            int xa = (int)(cx - xh), xb = (int)(cx + xh) + 1;
            if (xa < 0) xa = 0;
            if (xb > w) xb = w;
            float dyn = dy * iR;
            float thick = d0 * (1.0f + drain * dyn);
            uint32_t *row = fb + (size_t)y * (size_t)w;
            for (int x = xa; x < xb; x++) {
                float dx = (float)x + 0.5f - cx;
                float u2 = (dx * dx + dy2) * iR2;
                if (u2 >= 1.0f) continue;
                int pi = (int)(u2 * 1023.0f);
                float path = p149_path[pi];
                int idx = bi + (int)(thick * path * kscale);
                uint32_t col = pal[(uint32_t)idx & JD_PAL_MASK];
                /* Fresnel: transparent centre, luminous rim, soft outer edge */
                float u4 = u2 * u2;
                float fres = 0.09f + 0.91f * u4 * u2;
                float edge = (1.0f - u2) * 12.0f;
                if (edge < 1.0f) fres *= edge;
                int v8 = (int)(fres * 232.0f);
                /* specular window highlight */
                float sx = (float)x + 0.5f - spx, sy = (float)y + 0.5f - spy;
                float sd2 = (sx * sx + sy * sy) * isp;
                int hl = sd2 < 6.0f ? (int)(150.0f / (1.0f + sd2 * sd2 * 0.8f)) : 0;
                uint32_t o = row[x];
                int r = (int)((o >> 16) & 255u) + ((int)((col >> 16) & 255u) * v8 >> 8) + hl;
                int g = (int)((o >> 8) & 255u) + ((int)((col >> 8) & 255u) * v8 >> 8) + hl;
                int b = (int)(o & 255u) + ((int)(col & 255u) * v8 >> 8) + hl;
                if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
                row[x] = 0xFF000000u | ((uint32_t)r << 16)
                       | ((uint32_t)g << 8) | (uint32_t)b;
            }
        }
    }
}
