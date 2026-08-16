/* pattern_413 — TWIN SUNBURSTS (ground): two ray fans from two centres
 * that orbit each other, their rays interfering into a soft moire where
 * they cross; each fan carries its own hue family. */
#include "_gk336.h"

void pattern_413(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 5) + t * 0.008f;
    float ax = GK_W * (0.5f + 0.3f * gk_sin(t * 0.5f)), ay = GK_H * (0.5f + 0.3f * gk_cos(t * 0.5f));
    float bx = GK_W * (0.5f - 0.3f * gk_sin(t * 0.5f)), by = GK_H * (0.5f - 0.3f * gk_cos(t * 0.5f));
    float nr = (float)(7 + (int)(gk_sf(seed, 6) * 4.0f));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dax = (float)x - ax, day = (float)y - ay, dbx = (float)x - bx, dby = (float)y - by;
            float aa = atan2f(day, dax), ab = atan2f(dby, dbx);
            float ra = sqrtf(dax * dax + day * day), rb = sqrtf(dbx * dbx + dby * dby);
            float f1 = gk_sin(aa * nr + t) * 0.5f + 0.5f, f2 = gk_sin(ab * nr - t * 0.8f) * 0.5f + 0.5f;
            float wa = rb / (ra + rb + 1.0f);          /* nearer to A -> A dominates */
            float rr = f1 * wa + f2 * (1.0f - wa);
            uint32_t ca = gk_pal(pal, hue0 + f1 * 0.15f), cb = gk_pal(pal, hue0 + 0.4f + f2 * 0.15f);
            uint32_t c = gk_mix(cb, ca, wa);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.35f * rr));
        }
    }
    gk_blit(fb, w, h);
}
