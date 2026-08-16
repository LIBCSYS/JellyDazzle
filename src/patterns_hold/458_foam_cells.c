/* pattern_458 — FOAM CELLS (ground): soap foam — a packed cell network
 * whose walls glow with thin-film colour, interiors pale and translucent,
 * cells slowly jostling; walls soft, never hairline. */
#include "_gk336.h"

void pattern_458(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 9) + t * 0.008f;
    float cell = 34.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float d1 = 1e9f, d2 = 1e9f; uint32_t id = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + 0.5f + 0.35f * gk_sin(t + gk_hf(hh) * 6.28f)) * cell;
                float sy = ((float)(cy + j) + 0.5f + 0.35f * gk_cos(t * 0.8f + gk_hf(hh >> 5) * 6.28f)) * cell;
                float dx = fx - sx, dy = fy - sy, d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; id = hh; } else if (d < d2) d2 = d;
            }
            float wall = 1.0f - gk_sstep(0.0f, 11.0f, d2 - d1);
            float film = gk_sin((d2 - d1) * 0.15f + t * 2.0f + gk_hf(id) * 3.0f) * 0.5f + 0.5f;
            uint32_t inner = gk_lift(gk_pal(pal, hue0 + gk_hf(id) * 0.25f), 0.3f);
            uint32_t wallc = gk_pal(pal, hue0 + 0.3f + film * 0.3f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(inner, wallc, wall), 0.75f + 0.2f * wall * film + 0.05f * (1.0f - wall)));
        }
    }
    gk_blit(fb, w, h);
}
