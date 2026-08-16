/* pattern_349 — AURORA SHEETS (ground): tall wavering curtains hanging from
 * the top of the frame over a deep gradient — the curtain edge is a slow
 * noise-driven line, the glow falls off below it. Full frame, no black. */
#include "_gk336.h"

void pattern_349(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0022f;
    float hue0 = gk_sf(seed, 5) + t * 0.012f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_W;
            float glow = 0;
            float hue = hue0;
            for (int k = 0; k < 3; k++) {
                float top = 0.15f + 0.25f * (float)k + 0.18f * gk_n3(fx * 2.5f + (float)k * 9.0f, t * 0.3f, (float)k);
                float wob = 0.05f * gk_n3(fx * 8.0f, fy * 2.0f + t * 0.2f, (float)k * 3.0f + 5.0f);
                float d = fy - top + wob;
                float band = expf(-d * d * 60.0f) * 0.9f + gk_sstep(0.0f, 0.4f, d) * (1.0f - gk_sstep(0.4f, 1.0f, d)) * 0.35f;
                glow += band; hue += band * 0.09f * (float)(k + 1);
            }
            uint32_t base = gk_pal(pal, hue0 + 0.5f + fy * 0.15f);
            uint32_t cur = gk_pal(pal, hue + fx * 0.05f);
            uint32_t c = gk_mix(base, cur, gk_clamp01(glow * 0.9f));
            gk_put(y * GK_W + x, gk_shade(c, 0.60f + 0.35f * gk_clamp01(glow)));
        }
    }
    gk_blit(fb, w, h);
}
