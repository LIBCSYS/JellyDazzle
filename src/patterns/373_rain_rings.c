/* pattern_373 — RAIN RINGS (ground): expanding rings from many soft drops
 * on a still surface. Each cell owns one drop that re-arms on its own slow
 * timer; rings widen and fade over ~10 s, so nothing pops. */
#include "_gk336.h"

void pattern_373(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * (1.0f / 60.0f);
    float hue0 = gk_sf(seed, 21) + t * 0.002f;
    float cell = 80.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float hgt = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float per = 9.0f + 6.0f * gk_hf(hh);            /* seconds */
                float ph = fmodf(t + gk_hf(hh >> 5) * per, per); /* age */
                float sx = ((float)(cx + i) + 0.15f + 0.7f * gk_hf(hh >> 9)) * cell;
                float sy = ((float)(cy + j) + 0.15f + 0.7f * gk_hf(hh >> 13)) * cell;
                float dx = fx - sx, dy = fy - sy, r = sqrtf(dx * dx + dy * dy);
                float R = ph * 12.0f;                            /* px/s */
                float env = expf(-ph * 0.35f) * (1.0f - expf(-ph * 3.0f));
                float d = r - R;
                hgt += env * gk_sin(d * 0.25f) * expf(-d * d * 0.004f);
            }
            float base = gk_fbm3(fx * 0.006f, fy * 0.006f, t * 0.03f, 3);
            uint32_t c = gk_pal(pal, hue0 + base * 0.15f + hgt * 0.12f);
            gk_put(y * GK_W + x, gk_shade(c, 0.66f + 0.3f * hgt + 0.1f * base));
        }
    }
    gk_blit(fb, w, h);
}
