/* pattern_383 — SATIN RIPPLES (ground): satin — a glossy fabric with fine
 * parallel weave ripples riding on big soft undulations; the gloss band
 * is broad and drifts across the frame. */
#include "_gk336.h"

void pattern_383(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0022f;
    float hue0 = gk_sf(seed, 9) + t * 0.01f;
    float ang = 0.4f + gk_sf(seed, 10) * 2.3f, ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float u = fx * ca + fy * sa;
            float big = gk_n3(fx * 0.007f, fy * 0.007f, t * 0.2f);
            float big2 = gk_n3(fx * 0.004f + 9.0f, fy * 0.004f, t * 0.15f);
            float rip = gk_sin(u * 0.25f + big * 8.0f) * 0.5f + 0.5f;
            float gloss = expf(-(big2 - 0.15f * gk_sin(t)) * (big2 - 0.15f * gk_sin(t)) * 14.0f);
            uint32_t c = gk_pal(pal, hue0 + big * 0.12f + rip * 0.02f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, gloss * 0.45f), 0.55f + 0.15f * rip + 0.25f * gloss + 0.1f * big));
        }
    }
    gk_blit(fb, w, h);
}
