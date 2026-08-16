/* pattern_397 — PEBBLE MOSAIC (ground): rounded pebbles packed in mortar,
 * each a soft dome lit from a light that drifts around, colours from the
 * ramp by a slow wave so patches of pebbles share a family. */
#include "_gk336.h"

void pattern_397(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 21) + t * 0.008f;
    float cell = 22.0f;
    float lx = gk_cos(t * 0.6f), ly = gk_sin(t * 0.6f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float best = 0, bnx = 0, bny = 0; uint32_t bid = 0; float bx = 0, by = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + 0.2f + 0.6f * gk_hf(hh)) * cell, sy = ((float)(cy + j) + 0.2f + 0.6f * gk_hf(hh >> 5)) * cell;
                float a = gk_hf(hh >> 9) * 3.14f, ca = gk_cos(a), sa = gk_sin(a);
                float dx = fx - sx, dy = fy - sy;
                float u = (dx * ca + dy * sa) / (cell * (0.5f + 0.15f * gk_hf(hh >> 13))), v = (-dx * sa + dy * ca) / (cell * 0.42f);
                float d2 = u * u + v * v;
                float dome = gk_clamp01(1.0f - d2);
                if (dome > best) { best = dome; bid = hh; bnx = -u * ca + v * sa; bny = -u * sa - v * ca; bx = sx; by = sy; }
            }
            float dome = sqrtf(best);
            float diff = gk_clamp01(0.55f + (bnx * lx + bny * ly) * 0.6f);
            float wave = gk_n3(bx * 0.008f, by * 0.008f, t * 0.4f);
            uint32_t peb = gk_pal(pal, hue0 + wave * 0.3f + gk_hf(bid) * 0.05f);
            uint32_t mortar = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.8f);
            float cov = gk_sstep(0.0f, 0.2f, dome);
            gk_put(y * GK_W + x, gk_shade(gk_mix(mortar, peb, cov), 0.55f + 0.35f * diff * cov + 0.1f * dome));
        }
    }
    gk_blit(fb, w, h);
}
