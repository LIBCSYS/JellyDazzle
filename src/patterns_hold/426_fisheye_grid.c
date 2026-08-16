/* pattern_426 — FISHEYE GRID (ground): a grid bulged through a fisheye
 * lens whose centre wanders and whose strength breathes — the classic
 * "grid on a sphere", lines soft, cells shaded like a lit dome. */
#include "_gk336.h"

void pattern_426(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 9) + t * 0.008f;
    float cx = GK_W * (0.5f + 0.2f * gk_sin(t * 0.4f)), cy = GK_H * (0.5f + 0.2f * gk_cos(t * 0.3f));
    float str = 0.5f + 0.3f * gk_sin(t * 0.5f);
    float cell = 26.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy) / (GK_H * 0.75f);
            float k = 1.0f / (1.0f + str * r * r);          /* fisheye compress */
            float ux = cx + dx * k * (1.0f + str), uy = cy + dy * k * (1.0f + str);
            float u = ux / cell, v = uy / cell;
            float gu = gk_absf(gk_fract(u) - 0.5f), gv = gk_absf(gk_fract(v) - 0.5f);
            float line = 1.0f - gk_sstep(0.38f, 0.5f, fmaxf(gu, gv));
            float dome = expf(-r * r * 1.5f);
            float wave = gk_n3(floorf(u) * 0.25f, floorf(v) * 0.25f, t * 0.4f);
            uint32_t fill = gk_pal(pal, hue0 + wave * 0.25f + r * 0.1f);
            uint32_t ink = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.7f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(fill, ink, 1.0f - line), 0.55f + 0.2f * line + 0.25f * dome));
        }
    }
    gk_blit(fb, w, h);
}
