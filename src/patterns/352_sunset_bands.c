/* pattern_352 — SUNSET BANDS (ground): stacked horizontal colour bands with
 * soft edges that undulate — the classic banded sunset, each stripe a
 * different stop on the ramp, the boundaries breathing on separate clocks. */
#include "_gk336.h"

void pattern_352(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0025f;
    float hue0 = gk_sf(seed, 17) + t * 0.01f;
    int nb = 6 + (int)(gk_sf(seed, 18) * 4.0f);
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_W;
            float wob = 0.03f * gk_sin(fx * 6.0f + t) + 0.02f * gk_sin(fx * 11.0f - t * 0.7f + fy * 3.0f);
            float p = (fy + wob) * (float)nb;
            int band = (int)floorf(p); float fr = p - (float)band;
            float e = gk_sstep(0.35f, 0.65f, fr);            /* soft edge */
            float hue_a = hue0 + (float)band * 0.07f, hue_b = hue_a + 0.07f;
            uint32_t c = gk_mix(gk_pal(pal, hue_a), gk_pal(pal, hue_b), e);
            float lit = 0.68f + 0.22f * gk_sin(fy * 3.14f) + 0.08f * gk_sin(fx * 4.0f + t * 0.5f);
            gk_put(y * GK_W + x, gk_shade(c, lit));
        }
    }
    gk_blit(fb, w, h);
}
