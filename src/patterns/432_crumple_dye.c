/* pattern_432 — CRUMPLE DYE (ground): the crumple tie-dye — dye pooled in
 * the creases of scrunched cloth: ridged noise makes the crease network,
 * three colours pooled by crease depth, all bleeding softly and drifting. */
#include "_gk336.h"

void pattern_432(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 55) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x * 0.012f, fy = (float)y * 0.012f;
            float crease = gk_ridge3(fx, fy, t * 0.3f, 4);
            float pool = gk_fbm3(fx * 0.5f + 9.0f, fy * 0.5f, t * 0.2f, 3);
            uint32_t a = gk_pal(pal, hue0 + pool * 0.2f);
            uint32_t b = gk_pal(pal, hue0 + 0.3f + pool * 0.1f);
            uint32_t c = gk_pal(pal, hue0 + 0.6f);
            uint32_t m = gk_mix(a, b, gk_sstep(0.3f, 0.6f, crease));
            m = gk_mix(m, c, gk_sstep(0.65f, 0.9f, crease));
            gk_put(y * GK_W + x, gk_shade(m, 0.62f + 0.3f * crease + 0.08f * pool));
        }
    }
    gk_blit(fb, w, h);
}
