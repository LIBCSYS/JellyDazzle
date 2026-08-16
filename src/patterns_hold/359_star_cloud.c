/* pattern_359 — STAR CLOUD (ground): a bright glowing gas field with a
 * lattice of soft stars (gaussian bumps, jittered per cell) sitting in it,
 * every star pulsing on its own slow phase — a dense Milky Way field. */
#include "_gk336.h"

void pattern_359(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.002f;
    float hue0 = gk_sf(seed, 45) + t * 0.01f;
    float cell = 26.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float gas = gk_fbm3(fx * 0.006f, fy * 0.006f, t * 0.3f, 4);
            float glow = 0;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + gk_hf(hh)) * cell, sy = ((float)(cy + j) + gk_hf(hh * 7u + 1u)) * cell;
                float dx = fx - sx, dy = fy - sy;
                float ph = gk_hf(hh >> 3) * 6.28f;
                float amp = (0.55f + 0.45f * gk_sin(t * 2.0f + ph)) * (0.5f + 0.9f * gk_hf(hh >> 9));
                float sz = 0.05f + 0.25f * gk_hf(hh >> 5);
                glow += amp * expf(-(dx * dx + dy * dy) * sz);
            }
            uint32_t base = gk_pal(pal, hue0 + gas * 0.3f + fy * 0.0004f);
            uint32_t star = gk_lift(gk_pal(pal, hue0 + 0.4f), 0.5f);
            uint32_t c = gk_mix(base, star, gk_clamp01(glow));
            gk_put(y * GK_W + x, gk_shade(c, 0.62f + 0.25f * gas + 0.3f * gk_clamp01(glow)));
        }
    }
    gk_blit(fb, w, h);
}
