/* pattern_461 — CURTAIN FOLDS (ground): a hanging curtain — vertical
 * folds as a sum of slow sines in x, shaded by their slope with a broad
 * light that pans across; the fabric colour drifts and a hem line breathes. */
#include "_gk336.h"

void pattern_461(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 21) + t * 0.008f;
    float lx = gk_sin(t * 0.4f);
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x;
            float sway = 6.0f * gk_sin(fy * 3.0f + t) * fy;
            float u = fx + sway;
            float slope = gk_cos(u * 0.06f + t * 0.3f) * 0.6f + gk_cos(u * 0.15f - t * 0.2f + 1.0f) * 0.3f + gk_cos(u * 0.025f + 2.0f) * 0.4f;
            float diff = gk_clamp01(0.55f + slope * lx * 0.5f);
            float depth = gk_sin(u * 0.06f + t * 0.3f) * 0.5f + 0.5f;
            float tint = gk_n3(fx * 0.004f, fy * 2.0f, t * 0.2f);
            uint32_t c = gk_pal(pal, hue0 + tint * 0.15f + depth * 0.05f + fy * 0.05f);
            gk_put(y * GK_W + x, gk_shade(c, 0.5f + 0.4f * diff + 0.1f * depth));
        }
    }
    gk_blit(fb, w, h);
}
