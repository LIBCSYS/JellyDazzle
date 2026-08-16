/* pattern_443 — INK TENDRILS (ground): ink sinking through water — long
 * feathered tendrils hanging from soft clouds above, made from a strongly
 * anisotropic domain-warped noise, drifting downward at a crawl. */
#include "_gk336.h"

void pattern_443(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 45) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H;
            float qx = gk_fbm3(fx * 3.0f, fy * 1.2f - t * 0.3f, t * 0.2f, 3);
            float qy = gk_fbm3(fx * 3.0f + 5.0f, fy * 1.2f - t * 0.3f + 2.0f, t * 0.2f, 3);
            float ink = gk_fbm3(fx * 6.0f + qx * 1.5f, fy * 1.5f + qy * 1.5f - t * 0.4f, t * 0.15f, 4);
            float dens = gk_sstep(-0.1f, 0.35f, ink + (0.5f - fy) * 0.5f);
            uint32_t water = gk_lift(gk_pal(pal, hue0 + fy * 0.1f), 0.3f);
            uint32_t inkc = gk_pal(pal, hue0 + 0.4f + ink * 0.1f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(water, inkc, dens * 0.9f), 0.7f + 0.2f * (1.0f - dens) + 0.1f * ink));
        }
    }
    gk_blit(fb, w, h);
}
