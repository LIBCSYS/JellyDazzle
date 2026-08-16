/* pattern_455 — ISOBAR MAP (ground): a weather chart — highs and lows as
 * broad pressure hills, isobars as soft concentric contours around them,
 * the systems drifting across the frame at a crawl. */
#include "_gk336.h"

void pattern_455(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 65) + t * 0.008f;
    float sx[4], sy[4], sg[4];
    for (int i = 0; i < 4; i++) {
        sx[i] = GK_W * (gk_sf(seed, 70 + i) + t * 0.15f * (0.5f + 0.5f * gk_sf(seed, 80 + i)));
        sx[i] = fmodf(sx[i], GK_W * 1.4f) - GK_W * 0.2f;
        sy[i] = GK_H * (0.1f + 0.8f * gk_sf(seed, 90 + i)) + 20.0f * gk_sin(t + (float)i);
        sg[i] = (i & 1) ? 1.0f : -1.0f;
    }
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float p = gk_fbm3((float)x * 0.004f, (float)y * 0.004f, t * 0.2f, 3) * 0.3f;
            for (int i = 0; i < 4; i++) {
                float dx = (float)x - sx[i], dy = (float)y - sy[i];
                p += sg[i] * expf(-(dx * dx + dy * dy) * 0.00012f);
            }
            float lv = (p + 1.5f) * 6.0f, fr = gk_fract(lv);
            float line = 1.0f - gk_sstep(0.0f, 0.1f, fr) * gk_sstep(1.0f, 0.9f, fr);
            float pn = gk_clamp01(p * 0.5f + 0.5f);
            uint32_t c = gk_pal(pal, hue0 + pn * 0.4f);
            uint32_t ink = gk_shade(gk_pal(pal, hue0 + 0.6f), 0.7f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(c, ink, line * 0.7f), 0.65f + 0.3f * pn));
        }
    }
    gk_blit(fb, w, h);
}
