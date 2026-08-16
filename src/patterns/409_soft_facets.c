/* pattern_409 — SOFT FACETS (ground): a crumpled-foil field of large soft
 * facets — a Voronoi where each cell is a tilted plane, shaded by a moving
 * light, seams feathered so it reads as folded paper, not tiles. */
#include "_gk336.h"

void pattern_409(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 21) + t * 0.008f;
    float cell = 60.0f;
    float lx = gk_cos(t * 0.5f), ly = gk_sin(t * 0.5f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            /* weighted blend of the two nearest tilted planes for soft creases */
            float d1 = 1e9f, d2 = 1e9f; float n1x = 0, n1y = 0, n2x = 0, n2y = 0; uint32_t id1 = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + gk_hf(hh)) * cell, sy = ((float)(cy + j) + gk_hf(hh >> 5)) * cell;
                float dx = fx - sx, dy = fy - sy, d = sqrtf(dx * dx + dy * dy);
                float nx = (gk_hf(hh >> 9) - 0.5f) * 1.6f + 0.3f * gk_sin(t + gk_hf(hh >> 13) * 6.28f);
                float ny = (gk_hf(hh >> 17) - 0.5f) * 1.6f + 0.3f * gk_cos(t * 0.8f + gk_hf(hh >> 21) * 6.28f);
                if (d < d1) { d2 = d1; n2x = n1x; n2y = n1y; d1 = d; n1x = nx; n1y = ny; id1 = hh; }
                else if (d < d2) { d2 = d; n2x = nx; n2y = ny; }
            }
            float f = gk_sstep(0.0f, 8.0f, d2 - d1) * 0.5f + 0.5f;   /* 0.5 at seam, 1 inside */
            float nx = n1x * f + n2x * (1.0f - f), ny = n1y * f + n2y * (1.0f - f);
            float diff = gk_clamp01(0.55f + (nx * lx + ny * ly) * 0.6f);
            uint32_t c = gk_pal(pal, hue0 + gk_hf(id1) * 0.12f + diff * 0.1f + gk_n2(fx * 0.004f, fy * 0.004f) * 0.1f);
            gk_put(y * GK_W + x, gk_shade(c, 0.5f + 0.45f * diff));
        }
    }
    gk_blit(fb, w, h);
}
