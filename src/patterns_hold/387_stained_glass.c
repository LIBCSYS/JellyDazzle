/* pattern_387 — STAINED GLASS (ground): Voronoi panes with soft dark
 * leading, each pane a jewel colour with a slow inner light gradient, the
 * seed points on tiny slow orbits so the leading flexes. */
#include "_gk336.h"

void pattern_387(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.002f;
    float hue0 = gk_sf(seed, 25) + t * 0.008f;
    float cell = 40.0f + 15.0f * gk_sf(seed, 26);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float d1 = 1e9f, d2 = 1e9f; uint32_t id = 0; float ox = 0, oy = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + 0.5f + 0.4f * gk_sin(t + gk_hf(hh) * 6.28f)) * cell;
                float sy = ((float)(cy + j) + 0.5f + 0.4f * gk_cos(t * 0.8f + gk_hf(hh >> 3) * 6.28f)) * cell;
                float dx = fx - sx, dy = fy - sy, d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; id = hh; ox = dx; oy = dy; } else if (d < d2) d2 = d;
            }
            float lead = gk_sstep(1.5f, 5.0f, d2 - d1);
            float inner = gk_clamp01(0.5f + (ox * gk_cos(t * 0.4f) + oy * gk_sin(t * 0.4f)) / cell);
            float glow = gk_n3(fx * 0.01f, fy * 0.01f, t * 0.3f) * 0.5f + 0.5f;
            uint32_t pane = gk_pal(pal, hue0 + gk_hf(id) * 0.5f + inner * 0.04f);
            uint32_t leadc = gk_pal(pal, hue0 + 0.5f);
            uint32_t c = gk_mix(gk_shade(leadc, 0.35f), pane, lead);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.25f * inner + 0.15f * glow));
        }
    }
    gk_blit(fb, w, h);
}
