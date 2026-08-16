/* pattern_399 — BRUSHED METAL (ground): fine linear brush grain with a
 * broad anisotropic highlight band that slides across, tinted by the ramp
 * so it reads as coloured anodised steel rather than grey. */
#include "_gk336.h"

void pattern_399(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.002f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    float ang = gk_sf(seed, 30) * 3.14f, ca = gk_cos(ang), sa = gk_sin(ang);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            float grain = gk_n2(u * 0.02f, v * 0.9f) * 0.5f + gk_n2(u * 0.05f + 9.0f, v * 0.5f) * 0.3f;
            float band = gk_sin(v * 0.012f + t * 1.3f) * 0.5f + 0.5f;
            float band2 = gk_sin(v * 0.03f - t * 0.8f + 2.0f) * 0.5f + 0.5f;
            float hi = band * band * 0.6f + band2 * 0.2f;
            float tint = gk_n3(fx * 0.004f, fy * 0.004f, t * 0.2f);
            uint32_t c = gk_pal(pal, hue0 + tint * 0.3f + hi * 0.08f + v * 0.0004f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, hi * 0.2f), 0.55f + 0.1f * grain + 0.35f * hi));
        }
    }
    gk_blit(fb, w, h);
}
