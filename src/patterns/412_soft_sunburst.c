/* pattern_412 — SOFT SUNBURST (ground): broad rays fanning from a slowly
 * wandering centre, each ray a smooth sine lobe (no hard wedges), the ray
 * count breathing, colour walking round the ramp with angle. */
#include "_gk336.h"

void pattern_412(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 1) + t * 0.008f;
    float nr = (float)(6 + (int)(gk_sf(seed, 2) * 5.0f)) * 2.0f;   /* even so nr/2 is whole */
    float cx = GK_W * (0.5f + 0.25f * gk_sin(t * 0.4f)), cy = GK_H * (0.5f + 0.25f * gk_cos(t * 0.3f));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float ray = gk_sin(a * nr + t * 0.8f) * 0.5f + 0.5f;
            float ray2 = gk_sin(a * nr * 0.5f - t * 0.5f + 1.0f) * 0.5f + 0.5f;
            float rr = ray * 0.7f + ray2 * 0.3f;
            float glow = expf(-r * 0.008f);
            uint32_t c = gk_pal(pal, hue0 + gk_sin(a + t * 0.2f) * 0.06f + rr * 0.15f + r * 0.0005f);
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.3f * rr + 0.2f * glow));
        }
    }
    gk_blit(fb, w, h);
}
