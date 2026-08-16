/* pattern_369 — SUNLIT SHALLOWS (ground): caustics over rippled sand — the
 * sand is a fine noise texture with slow ripple lines, the caustic net is
 * broader and softer than the pool version, warmer where the light lands. */
#include "_gk336.h"

void pattern_369(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.003f;
    float hue0 = gk_sf(seed, 5) + t * 0.01f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float sand = gk_fbm2(fx * 0.02f, fy * 0.02f, 3) * 0.5f;
            float ripple = gk_sin(fy * 0.12f + gk_n2(fx * 0.02f, fy * 0.02f) * 3.0f) * 0.5f + 0.5f;
            float wx = gk_n3(fx * 0.012f, fy * 0.012f, t * 0.5f) * 40.0f;
            float px = fx + wx, py = fy - wx * 0.6f;
            float a = gk_sin(px * 0.045f + t), b = gk_sin((px * 0.5f + py * 0.866f) * 0.045f - t * 0.7f);
            float c = gk_sin((px * 0.5f - py * 0.866f) * 0.045f + t * 0.5f);
            float web = 1.0f - gk_absf(a * b * c); web = web * web * web;
            uint32_t base = gk_pal(pal, hue0 + sand * 0.2f + ripple * 0.08f);
            uint32_t light = gk_pal(pal, hue0 + 0.35f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(base, light, web * 0.8f), 0.5f + 0.15f * ripple + 0.1f * sand + 0.3f * web));
        }
    }
    gk_blit(fb, w, h);
}
