/* pattern_457 — BOKEH FIELD (ground): out-of-focus lights — soft-edged
 * discs of many sizes drifting slowly across a graded blur, overlapping
 * translucently, each disc a little brighter at its rim like real bokeh. */
#include "_gk336.h"

void pattern_457(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 5) + t * 0.008f;
    float cell = 56.0f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x;
            float haze = gk_fbm3(fx * 0.006f, (float)y * 0.006f, t * 0.2f, 3);
            uint32_t col = gk_pal(pal, hue0 + fy * 0.15f + haze * 0.1f);
            float lit = 0.6f + 0.15f * haze;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = (int)floorf((fx + t * 20.0f) / cell) + i, cy = (int)floorf((float)y / cell) + j;
                uint32_t hh = gk_hash2(cx, cy, seed);
                float sx = ((float)cx + gk_hf(hh)) * cell - t * 20.0f, sy = ((float)cy + gk_hf(hh >> 5)) * cell + 8.0f * gk_sin(t + gk_hf(hh >> 9) * 6.28f);
                float R = 8.0f + 22.0f * gk_hf(hh >> 13);
                float dx = fx - sx, dy = (float)y - sy, d = sqrtf(dx * dx + dy * dy) / R;
                float disc = gk_sstep(1.0f, 0.85f, d);
                float ring = gk_sstep(0.6f, 0.95f, d) * disc;
                float a = disc * (0.25f + 0.35f * gk_hf(hh >> 17)) * (0.6f + 0.4f * gk_sin(t * 1.5f + gk_hf(hh >> 21) * 6.28f));
                uint32_t bc = gk_pal(pal, hue0 + 0.3f + gk_hf(hh >> 25) * 0.4f);
                col = gk_mix(col, bc, a);
                lit += a * (0.5f + 0.4f * ring);
            }
            gk_put(y * GK_W + x, gk_shade(col, lit));
        }
    }
    gk_blit(fb, w, h);
}
