/* pattern_445 — ALCOHOL INK (ground): the alcohol-ink look — cells of
 * colour with pale bleached centres and saturated ragged rims that push
 * against each other, all slowly swelling and shifting hue. */
#include "_gk336.h"

void pattern_445(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 53) + t * 0.008f;
    float cell = 70.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float rag = gk_fbm3(fx * 0.02f, fy * 0.02f, t * 0.4f, 3) * 10.0f;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float d1 = 1e9f, d2 = 1e9f; uint32_t id = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + 0.5f + 0.4f * gk_sin(t * 0.7f + gk_hf(hh) * 6.28f)) * cell;
                float sy = ((float)(cy + j) + 0.5f + 0.4f * gk_cos(t * 0.5f + gk_hf(hh >> 5) * 6.28f)) * cell;
                float dx = fx - sx, dy = fy - sy, d = sqrtf(dx * dx + dy * dy) + rag;
                if (d < d1) { d2 = d1; d1 = d; id = hh; } else if (d < d2) d2 = d;
            }
            float rim = gk_sstep(14.0f, 2.0f, d2 - d1);         /* 1 at cell boundary */
            float centre = gk_sstep(cell * 0.45f, 0.0f, d1);
            uint32_t pale = gk_lift(gk_pal(pal, hue0 + gk_hf(id) * 0.5f), 0.45f);
            uint32_t sat = gk_pal(pal, hue0 + gk_hf(id) * 0.5f + 0.05f);
            uint32_t c = gk_mix(sat, pale, centre * 0.8f);
            c = gk_mix(c, gk_pal(pal, hue0 + gk_hf(id) * 0.5f + 0.15f), rim * 0.8f);
            gk_put(y * GK_W + x, gk_shade(c, 0.85f - 0.2f * rim + 0.1f * centre));
        }
    }
    gk_blit(fb, w, h);
}
