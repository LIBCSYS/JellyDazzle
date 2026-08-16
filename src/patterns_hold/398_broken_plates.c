/* pattern_398 — BROKEN PLATES (ground): pique assiette — shards of
 * patterned crockery (each Voronoi shard carries its own stripe or dot
 * pattern) set in mortar, tinted per shard, lit by a wandering light. */
#include "_gk336.h"

void pattern_398(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 25) + t * 0.008f;
    float cell = 38.0f;
    float lx = GK_W * (0.5f + 0.4f * gk_sin(t * 0.4f)), ly = GK_H * (0.5f + 0.4f * gk_cos(t * 0.3f));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float d1 = 1e9f, d2 = 1e9f; uint32_t id = 0; float ox = 0, oy = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + gk_hf(hh)) * cell, sy = ((float)(cy + j) + gk_hf(hh >> 5)) * cell;
                float dx = fx - sx, dy = fy - sy, d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; id = hh; ox = dx; oy = dy; } else if (d < d2) d2 = d;
            }
            float seam = gk_sstep(1.0f, 4.0f, d2 - d1);
            int kind = (int)(id >> 28) & 3;
            float a = gk_hf(id >> 9) * 3.14f, ca = gk_cos(a), sa = gk_sin(a);
            float u = ox * ca + oy * sa, v = -ox * sa + oy * ca;
            float pat;
            if (kind == 0) pat = gk_sin(u * 0.5f) * 0.5f + 0.5f;                             /* stripes */
            else if (kind == 1) pat = gk_sstep(0.3f, 0.6f, gk_sin(u * 0.6f) * gk_sin(v * 0.6f)); /* dots */
            else if (kind == 2) pat = gk_sstep(0.0f, 0.4f, gk_sin(sqrtf(u * u + v * v) * 0.4f)); /* rings */
            else pat = 0.0f;                                                                   /* plain */
            pat = gk_sstep(0.3f, 0.7f, pat);
            uint32_t base = gk_pal(pal, hue0 + gk_hf(id) * 0.4f);
            uint32_t ink = gk_pal(pal, hue0 + 0.5f + gk_hf(id >> 3) * 0.2f);
            uint32_t shard = gk_mix(base, ink, pat * 0.6f);
            uint32_t mortar = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.55f);
            float dx = fx - lx, dy = fy - ly;
            float lit = 0.72f + 0.25f * expf(-(dx * dx + dy * dy) * 0.00003f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(mortar, shard, seam), lit));
        }
    }
    gk_blit(fb, w, h);
}
