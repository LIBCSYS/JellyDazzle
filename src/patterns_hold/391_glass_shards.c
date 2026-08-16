/* pattern_391 — GLASS SHARDS (ground): a mosaic of long sharp shards —
 * an anisotropic Voronoi (cells stretched along a slowly rotating axis)
 * with soft seams; every shard tinted, a light sweeping across them. */
#include "_gk336.h"

void pattern_391(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 49) + t * 0.008f;
    float ang = gk_sf(seed, 50) * 3.14f + t * 0.1f, ca = gk_cos(ang), sa = gk_sin(ang);
    float cell = 30.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = (fx * ca + fy * sa) * 0.35f, v = -fx * sa + fy * ca;     /* stretch 3:1 */
            int cx = (int)floorf(u / cell), cy = (int)floorf(v / cell);
            float d1 = 1e9f, d2 = 1e9f; uint32_t id = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + gk_hf(hh)) * cell, sy = ((float)(cy + j) + gk_hf(hh >> 5)) * cell;
                float dx = u - sx, dy = v - sy, d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; id = hh; } else if (d < d2) d2 = d;
            }
            float seam = gk_sstep(0.5f, 3.5f, d2 - d1);
            float sweep = 0.5f + 0.5f * gk_sin(u * 0.02f + t * 1.2f + gk_hf(id) * 1.0f);
            uint32_t sh = gk_pal(pal, hue0 + gk_hf(id) * 0.4f);
            uint32_t seamc = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.45f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(seamc, sh, seam), 0.58f + 0.35f * sweep));
        }
    }
    gk_blit(fb, w, h);
}
