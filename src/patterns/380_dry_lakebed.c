/* pattern_380 — DRY LAKEBED (ground): cracked mud plates — Voronoi cells
 * with soft dark seams, each plate a slightly different tint, the whole
 * bed lit by a slow-drifting light so the plates gently gain and lose relief. */
#include "_gk336.h"

void pattern_380(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 85) + t * 0.01f;
    float cell = 36.0f;
    float lx = gk_cos(t * 0.5f), ly = gk_sin(t * 0.5f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x + gk_n2((float)x * 0.03f, (float)y * 0.03f) * 6.0f, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float d1 = 1e9f, d2 = 1e9f; uint32_t id = 0; float nx = 0, ny = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + gk_hf(hh)) * cell, sy = ((float)(cy + j) + gk_hf(hh >> 7)) * cell;
                float dx = fx - sx, dy = fy - sy, d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; id = hh; nx = dx / (d + 0.01f); ny = dy / (d + 0.01f); } else if (d < d2) d2 = d;
            }
            float seam = gk_sstep(0.0f, 5.0f, d2 - d1);
            float dome = 1.0f - gk_clamp01(d1 / (cell * 0.7f));
            float lit = 0.5f + 0.25f * (nx * lx + ny * ly) * (1.0f - dome) + 0.2f * dome;
            float grain = gk_fbm2(fx * 0.03f, fy * 0.03f, 2) * 0.05f;
            uint32_t plate = gk_pal(pal, hue0 + gk_hf(id >> 11) * 0.12f + grain);
            uint32_t crack = gk_pal(pal, hue0 + 0.4f);
            uint32_t c = gk_mix(crack, plate, seam);
            gk_put(y * GK_W + x, gk_shade(c, (0.5f + 0.4f * lit) * (0.55f + 0.45f * seam) + grain));
        }
    }
    gk_blit(fb, w, h);
}
