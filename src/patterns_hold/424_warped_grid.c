/* pattern_424 — WARPED GRID (ground): a soft grid of lines bent by a slow
 * noise warp, cells filled with a colour wave — the sheet flexes like
 * rubber; lines are wide and feathered, never crisp. */
#include "_gk336.h"

void pattern_424(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 1) + t * 0.008f;
    float cell = 34.0f + 12.0f * gk_sf(seed, 2);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float wx = gk_n3(fx * 0.006f, fy * 0.006f, t * 0.3f) * 30.0f, wy = gk_n3(fx * 0.006f + 7.0f, fy * 0.006f + 2.0f, t * 0.3f) * 30.0f;
            float u = (fx + wx) / cell, v = (fy + wy) / cell;
            float gu = gk_absf(gk_fract(u) - 0.5f), gv = gk_absf(gk_fract(v) - 0.5f);
            float line = 1.0f - gk_sstep(0.36f, 0.5f, fmaxf(gu, gv));
            float wave = gk_n3(floorf(u) * 0.2f, floorf(v) * 0.2f, t * 0.4f);
            float wave2 = gk_n3(fx * 0.004f, fy * 0.004f, t * 0.2f);
            uint32_t fill = gk_pal(pal, hue0 + wave * 0.2f + wave2 * 0.2f);
            uint32_t ink = gk_pal(pal, hue0 + 0.5f + wave2 * 0.1f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(fill, ink, 1.0f - line), 0.62f + 0.25f * line + 0.1f * wave2));
        }
    }
    gk_blit(fb, w, h);
}
