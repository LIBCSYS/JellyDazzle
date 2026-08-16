/* pattern_435 — WATERCOLOUR BLOOMS (ground): wet-in-wet blooms — soft
 * pigment discs with darker feathered rims (the "cauliflower" edge), each
 * bloom slowly spreading and fading on its own clock, on a paper tint. */
#define GK_W 256
#define GK_H 192
#include "_gk336.h"

void pattern_435(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * (1.0f / 60.0f);
    float hue0 = gk_sf(seed, 1) + t * 0.002f;
    float cell = 72.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float paper = gk_fbm2(fx * 0.015f, fy * 0.015f, 2) * 0.05f;
            uint32_t col = gk_lift(gk_pal(pal, hue0 + paper), 0.3f);
            float lit = 0.9f + paper;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = (int)floorf(fx / cell) + i, cy = (int)floorf(fy / cell) + j;
                uint32_t hh = gk_hash2(cx, cy, seed);
                float per = 14.0f + 10.0f * gk_hf(hh);
                float age = fmodf(t + gk_hf(hh >> 5) * per, per) / per;   /* 0..1 */
                float sx = ((float)cx + 0.5f + 0.5f * (gk_hf(hh >> 9) - 0.5f)) * cell, sy = ((float)cy + 0.5f + 0.5f * (gk_hf(hh >> 13) - 0.5f)) * cell;
                float dx = fx - sx, dy = fy - sy;
                float wob = gk_n2(dx * 0.02f + (float)cx * 3.1f, dy * 0.02f + (float)cy * 2.7f) * 8.0f;
                float R = cell * (0.25f + 0.45f * age);
                float d = sqrtf(dx * dx + dy * dy) + wob;
                float alpha = gk_sstep(R, R - 12.0f, d) * (1.0f - age) * (0.3f + 0.7f * gk_sstep(0.0f, 0.15f, age));
                float rim = expf(-(d - R + 5.0f) * (d - R + 5.0f) * 0.03f) * alpha;
                uint32_t pig = gk_pal(pal, hue0 + 0.15f + gk_hf(hh >> 17) * 0.6f);
                col = gk_mix(col, pig, alpha * 0.7f);
                lit -= rim * 0.25f;
            }
            gk_put(y * GK_W + x, gk_shade(col, lit));
        }
    }
    gk_blit(fb, w, h);
}
