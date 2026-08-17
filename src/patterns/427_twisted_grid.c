/* pattern_427 — TWISTED GRID (ground): a grid twisted about its centre —
 * rotation angle grows with radius — so the lines spiral outward; the
 * twist breathes and the centre drifts, cells lit by a colour wave. */
#include "_gk336.h"

void pattern_427(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 13) + t * 0.008f;
    float cx = GK_W * 0.5f + 25.0f * gk_sin(t * 0.3f), cy = GK_H * 0.5f + 20.0f * gk_cos(t * 0.4f);
    float tw = 0.006f + 0.004f * gk_sin(t * 0.4f);
    float cell = 30.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy);
            float ang = r * tw + t * 0.3f, ca = gk_cos(ang), sa = gk_sin(ang);
            float u = (dx * ca - dy * sa) / cell, v = (dx * sa + dy * ca) / cell;
            float gu = gk_absf(gk_fract(u) - 0.5f), gv = gk_absf(gk_fract(v) - 0.5f);
            float line = 1.0f - gk_sstep(0.36f, 0.5f, fmaxf(gu, gv));
            float wave = gk_n3(floorf(u) * 0.2f, floorf(v) * 0.2f, t * 0.4f);
            uint32_t fill = gk_pal(pal, hue0 + wave * 0.25f + r * 0.0006f);
            uint32_t ink = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.7f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(fill, ink, 1.0f - line), 0.6f + 0.25f * line + 0.15f * expf(-r * 0.008f)));
        }
    }
    gk_blit(fb, w, h);
}
