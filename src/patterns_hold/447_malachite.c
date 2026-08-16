/* pattern_447 — MALACHITE (ground): the botryoidal banding of malachite —
 * several bubble centres each with tight concentric bands, the nearest
 * centre winning, bands sheared by slow noise; deep and pale ramp stops. */
#include "_gk336.h"

void pattern_447(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 5) + t * 0.008f;
    float cell = 90.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float sh = gk_n3(fx * 0.008f, fy * 0.008f, t * 0.3f) * 10.0f;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float d1 = 1e9f, d2 = 1e9f; uint32_t id = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + gk_hf(hh)) * cell, sy = ((float)(cy + j) + gk_hf(hh >> 5)) * cell;
                float dx = fx - sx, dy = fy - sy, d = sqrtf(dx * dx + dy * dy) + sh;
                if (d < d1) { d2 = d1; d1 = d; id = hh; } else if (d < d2) d2 = d;
            }
            float ph = d1 * 0.14f + gk_hf(id) * 6.28f - t * 1.5f;
            float band = gk_sin(ph) * 0.5f + 0.5f;
            float band2 = gk_sin(ph * 0.23f) * 0.5f + 0.5f;
            float seam = gk_sstep(0.0f, 6.0f, d2 - d1);
            uint32_t deep = gk_pal(pal, hue0 + band2 * 0.15f);
            uint32_t pale = gk_lift(gk_pal(pal, hue0 + 0.3f + band2 * 0.15f), 0.15f);
            uint32_t c = gk_mix(deep, pale, band * (0.5f + 0.5f * band2));
            gk_put(y * GK_W + x, gk_shade(c, (0.55f + 0.35f * band) * (0.7f + 0.3f * seam)));
        }
    }
    gk_blit(fb, w, h);
}
