/* pattern_400 — SPUN METAL (ground): circular brushed finish — grain in
 * concentric circles around a wandering centre, with the radial
 * highlight arms that spun aluminium shows, turning slowly. */
#include "_gk336.h"

void pattern_400(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 33) + t * 0.008f;
    float cx = GK_W * (0.5f + 0.2f * gk_sin(t * 0.4f)), cy = GK_H * (0.5f + 0.2f * gk_cos(t * 0.3f));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float grain = gk_n2(r * 0.8f, a * 3.0f) * 0.4f + gk_n2(r * 0.25f + 5.0f, a * 1.0f) * 0.4f;
            float arms = gk_absf(gk_sin(a * 1.0f + t)) * 0.5f + gk_absf(gk_sin(a * 2.0f - t * 0.7f + 1.0f)) * 0.5f;
            arms = arms * arms;
            float ring = gk_sin(r * 0.03f - t) * 0.5f + 0.5f;
            uint32_t c = gk_pal(pal, hue0 + r * 0.0018f + arms * 0.08f + ring * 0.06f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, arms * 0.15f), 0.55f + 0.1f * grain + 0.3f * arms + 0.1f * ring));
        }
    }
    gk_blit(fb, w, h);
}
