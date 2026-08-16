/* pattern_392 — TERRAZZO (ground): chips of coloured stone in a tinted
 * matrix — soft-edged blobs (per-cell rounded polygons) at two sizes,
 * each chip its own ramp colour, the slab lit by a slowly wandering light. */
#include "_gk336.h"

static float chip(float fx, float fy, float cell, uint32_t seed, uint32_t *idout, float rad)
{
    int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
    float best = 0; uint32_t bid = 0;
    for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
        uint32_t hh = gk_hash2(cx + i, cy + j, seed);
        float sx = ((float)(cx + i) + gk_hf(hh)) * cell, sy = ((float)(cy + j) + gk_hf(hh >> 5)) * cell;
        float dx = fx - sx, dy = fy - sy;
        float a = gk_hf(hh >> 9) * 3.14f, ca = gk_cos(a), sa = gk_sin(a);
        float u = dx * ca + dy * sa, v = -dx * sa + dy * ca;
        float rr = rad * cell * (0.6f + 0.6f * gk_hf(hh >> 13));
        float d = sqrtf(u * u * (0.7f + 0.6f * gk_hf(hh >> 17)) + v * v) / rr;
        float cov = gk_sstep(1.0f, 0.85f, d);
        if (cov > best) { best = cov; bid = hh; }
    }
    *idout = bid; return best;
}

void pattern_392(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 1) + t * 0.008f;
    float lx = GK_W * (0.5f + 0.4f * gk_sin(t * 0.5f)), ly = GK_H * (0.5f + 0.4f * gk_cos(t * 0.37f));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            uint32_t id1, id2;
            float c1 = chip(fx, fy, 34.0f, seed, &id1, 0.42f);
            float c2 = chip(fx + 500.0f, fy + 300.0f, 16.0f, seed ^ 0x55u, &id2, 0.4f);
            float dx = fx - lx, dy = fy - ly;
            float lit = 0.75f + 0.25f * expf(-(dx * dx + dy * dy) * 0.00003f);
            uint32_t matrix = gk_pal(pal, hue0 + gk_n2(fx * 0.01f, fy * 0.01f) * 0.04f);
            uint32_t k1 = gk_pal(pal, hue0 + 0.15f + gk_hf(id1) * 0.5f);
            uint32_t k2 = gk_pal(pal, hue0 + 0.15f + gk_hf(id2) * 0.5f);
            uint32_t c = gk_mix(matrix, k1, c1);
            c = gk_mix(c, k2, c2 * (1.0f - c1 * 0.5f));
            gk_put(y * GK_W + x, gk_shade(c, lit * (0.85f + 0.15f * (c1 + c2))));
        }
    }
    gk_blit(fb, w, h);
}
