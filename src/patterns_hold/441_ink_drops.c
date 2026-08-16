/* pattern_441 — INK DROPS (ground): drops of ink spreading in water —
 * each drop a soft disc that grows and thins over ~15 s with a feathery
 * noise edge, several at once on staggered timers, on a tinted ground. */
#define GK_W 240
#define GK_H 180
#include "_gk336.h"

void pattern_441(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * (1.0f / 60.0f);
    float hue0 = gk_sf(seed, 25) + t * 0.002f;
    float cell = 80.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float water = gk_fbm3(fx * 0.006f, fy * 0.006f, t * 0.05f, 3);
            uint32_t col = gk_lift(gk_pal(pal, hue0 + water * 0.1f), 0.2f);
            float lit = 0.75f + 0.1f * water;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = (int)floorf(fx / cell) + i, cy = (int)floorf(fy / cell) + j;
                uint32_t hh = gk_hash2(cx, cy, seed);
                float per = 16.0f + 10.0f * gk_hf(hh);
                float age = fmodf(t + gk_hf(hh >> 5) * per, per) / per;
                float sx = ((float)cx + 0.2f + 0.6f * gk_hf(hh >> 9)) * cell, sy = ((float)cy + 0.2f + 0.6f * gk_hf(hh >> 13)) * cell;
                float dx = fx - sx, dy = fy - sy;
                float ang = atan2f(dy, dx);
                float feather = gk_n2(gk_cos(ang) * 1.6f + (float)cx * 5.3f + age * 2.0f, gk_sin(ang) * 1.6f + (float)cy * 7.1f) * 9.0f * age;
                float R = cell * (0.1f + 0.5f * sqrtf(age));
                float d = sqrtf(dx * dx + dy * dy) + feather;
                float alpha = gk_sstep(R, R * 0.5f, d) * (1.0f - age * age) * (0.4f + 0.6f * gk_sstep(0.0f, 0.1f, age));
                uint32_t ink = gk_pal(pal, hue0 + 0.3f + gk_hf(hh >> 17) * 0.4f);
                col = gk_mix(col, ink, alpha);
                lit += alpha * 0.15f;
            }
            gk_put(y * GK_W + x, gk_shade(col, lit));
        }
    }
    gk_blit(fb, w, h);
}
