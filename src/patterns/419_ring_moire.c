/* pattern_419 — RING MOIRE (ground): two ring systems from two centres
 * drifting apart and together — the interference makes slow hyperbolic
 * bands sweep across the frame; rings soft, bands broad. */
#include "_gk336.h"

void pattern_419(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 33) + t * 0.008f;
    float sep = 60.0f + 50.0f * gk_sin(t * 0.4f);
    float ax = GK_W * 0.5f - sep, ay = GK_H * 0.5f + 20.0f * gk_sin(t * 0.3f);
    float bx = GK_W * 0.5f + sep, by = GK_H * 0.5f - 20.0f * gk_sin(t * 0.3f);
    float k = 0.12f + 0.04f * gk_sf(seed, 34);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dax = (float)x - ax, day = (float)y - ay, dbx = (float)x - bx, dby = (float)y - by;
            float ra = sqrtf(dax * dax + day * day), rb = sqrtf(dbx * dbx + dby * dby);
            float f = (gk_cos(ra * k - t) + gk_cos(rb * k + t)) * 0.5f;      /* -1..1 */
            float env = gk_cos((ra - rb) * k * 0.5f) ;                       /* moire envelope */
            float v = f * 0.5f + 0.5f;
            uint32_t c = gk_pal(pal, hue0 + env * 0.15f + v * 0.05f + (ra + rb) * 0.0004f);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.25f * v + 0.12f * env * env));
        }
    }
    gk_blit(fb, w, h);
}
