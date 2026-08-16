/* pattern_423 — RING TUNNEL (ground): a soft tunnel — rings whose spacing
 * grows with radius (log spacing) so they read as depth, the far end a
 * bright glow, the tunnel bending as its vanishing point wanders. */
#include "_gk336.h"

void pattern_423(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 55) + t * 0.008f;
    float cx = GK_W * (0.5f + 0.25f * gk_sin(t * 0.3f)), cy = GK_H * (0.5f + 0.25f * gk_cos(t * 0.37f));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy) + 4.0f, a = atan2f(dy, dx);
            float depth = logf(r) * 3.0f - t * 1.5f;
            float ring = gk_cos(depth) * 0.5f + 0.5f;
            float idx = depth * (1.0f / 6.2832f);
            float glow = expf(-r * 0.02f);
            float wall = gk_sin(a * 6.0f + depth * 0.5f) * 0.5f + 0.5f;
            uint32_t c = gk_pal(pal, hue0 + idx * 0.05f + ring * 0.12f + wall * 0.05f);
            c = gk_mix(c, gk_lift(gk_pal(pal, hue0 + 0.4f), 0.4f), glow);
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.25f * ring + 0.1f * wall + 0.3f * glow));
        }
    }
    gk_blit(fb, w, h);
}
