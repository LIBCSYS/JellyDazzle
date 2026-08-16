/* pattern_411 — DELAUNAY BLOOM (ground): a Voronoi field where each cell
 * glows from its centre outward in its own colour, cells overlapping
 * softly (nearest-three blend), so the frame is a lattice of blooms. */
#include "_gk336.h"

void pattern_411(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    float cell = 44.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float rs = 0, gs = 0, bs = 0, ws = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + 0.5f + 0.35f * gk_sin(t + gk_hf(hh) * 6.28f)) * cell;
                float sy = ((float)(cy + j) + 0.5f + 0.35f * gk_cos(t * 0.7f + gk_hf(hh >> 5) * 6.28f)) * cell;
                float dx = fx - sx, dy = fy - sy;
                float wgt = expf(-(dx * dx + dy * dy) / (cell * cell * 0.25f));
                uint32_t c = gk_pal(pal, hue0 + gk_hf(hh >> 9) * 0.5f);
                rs += ((c >> 16) & 255) * wgt; gs += ((c >> 8) & 255) * wgt; bs += (c & 255) * wgt; ws += wgt;
            }
            float inv = 1.0f / (ws + 1e-4f);
            float lit = 0.6f + 0.4f * gk_clamp01(ws * 0.9f);
            gk_putf(y * GK_W + x, rs * inv / 255.0f * lit, gs * inv / 255.0f * lit, bs * inv / 255.0f * lit);
        }
    }
    gk_blit(fb, w, h);
}
