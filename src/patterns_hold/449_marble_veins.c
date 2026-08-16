/* pattern_449 — MARBLE VEINS (ground): polished marble — a pale stone
 * ground with dark branching veins (thin ridged noise) and soft cloudy
 * colour patches; the veins creep and the clouds drift over minutes. */
#include "_gk336.h"

void pattern_449(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0008f;
    float hue0 = gk_sf(seed, 13) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x * 0.006f, fy = (float)y * 0.006f;
            float wx = gk_fbm3(fx * 1.5f, fy * 1.5f, t * 0.3f, 3) * 1.5f;
            float vein = 1.0f - gk_absf(gk_n3(fx * 2.0f + wx, fy * 2.0f - wx * 0.5f, t * 0.2f));
            vein = powf(vein, 8.0f);
            float vein2 = 1.0f - gk_absf(gk_n3(fx * 5.0f + 9.0f + wx, fy * 5.0f, t * 0.25f));
            vein2 = powf(vein2, 12.0f) * 0.5f;
            float cloud = gk_fbm3(fx * 1.0f + 3.0f, fy * 1.0f, t * 0.2f, 3);
            uint32_t stone = gk_lift(gk_pal(pal, hue0 + cloud * 0.12f), 0.25f);
            uint32_t tint = gk_pal(pal, hue0 + 0.3f + cloud * 0.1f);
            uint32_t veinc = gk_pal(pal, hue0 + 0.55f);
            uint32_t c = gk_mix(stone, tint, gk_sstep(-0.1f, 0.5f, cloud) * 0.8f);
            c = gk_mix(c, veinc, gk_clamp01(vein + vein2) * 0.85f);
            gk_put(y * GK_W + x, gk_shade(c, 0.85f - 0.3f * gk_clamp01(vein + vein2) + 0.05f * cloud));
        }
    }
    gk_blit(fb, w, h);
}
