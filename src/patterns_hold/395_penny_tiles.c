/* pattern_395 — PENNY TILES (ground): round tiles in a staggered lattice
 * on a grout field, each disc softly domed and coloured by a drifting
 * field, a broad light sweeping diagonally so the domes catch it in turn. */
#include "_gk336.h"

void pattern_395(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.002f;
    float hue0 = gk_sf(seed, 13) + t * 0.008f;
    float cell = 18.0f + 6.0f * gk_sf(seed, 14);
    float lx = gk_cos(t * 0.5f), ly = gk_sin(t * 0.5f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float rowf = fy / (cell * 0.866f); int row = (int)floorf(rowf);
            float ox = (row & 1) ? cell * 0.5f : 0.0f;
            int col = (int)floorf((fx - ox) / cell);
            float cx = ((float)col + 0.5f) * cell + ox, cy = ((float)row + 0.5f) * cell * 0.866f;
            float dx = fx - cx, dy = fy - cy;
            float rad = cell * 0.44f;
            float d = sqrtf(dx * dx + dy * dy) / rad;
            float disc = gk_sstep(1.0f, 0.85f, d);
            float dome = sqrtf(gk_clamp01(1.0f - d * d));
            float diff = 0.5f + 0.5f * (dx * lx + dy * ly) / rad * (1.0f - dome * 0.5f);
            float field = gk_n3(cx * 0.01f, cy * 0.01f, t * 0.4f);
            uint32_t tile = gk_pal(pal, hue0 + field * 0.3f + gk_hf(gk_hash2(col, row, seed)) * 0.03f);
            uint32_t grout = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.55f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(grout, tile, disc), 0.6f + 0.35f * diff * disc + 0.05f * dome));
        }
    }
    gk_blit(fb, w, h);
}
