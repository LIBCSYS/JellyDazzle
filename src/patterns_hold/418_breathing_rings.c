/* pattern_418 — BREATHING RINGS (ground): concentric rings about a slow
 * wandering centre, expanding outward at a crawl while their spacing
 * breathes; the ring profile is a soft cosine, colour by ring index. */
#include "_gk336.h"

void pattern_418(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.002f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    float cx = GK_W * (0.5f + 0.2f * gk_sin(t * 0.3f)), cy = GK_H * (0.5f + 0.2f * gk_cos(t * 0.23f));
    float sp = 0.045f + 0.012f * gk_sin(t * 0.5f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy);
            float ph = r * sp - t * 1.5f;
            float ring = gk_cos(ph) * 0.5f + 0.5f;
            float idx = ph * (1.0f / 6.2832f);
            float band = gk_sstep(0.3f, 0.7f, ring);
            uint32_t ca = gk_pal(pal, hue0 + idx * 0.07f), cb = gk_pal(pal, hue0 + idx * 0.07f + 0.25f);
            uint32_t c = gk_mix(ca, cb, band);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.3f * ring + 0.1f * expf(-r * 0.01f)));
        }
    }
    gk_blit(fb, w, h);
}
