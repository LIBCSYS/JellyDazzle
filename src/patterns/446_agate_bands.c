/* pattern_446 — AGATE BANDS (ground): a polished agate slice — many
 * concentric bands following a lumpy, noise-distorted outline around an
 * off-centre core, band widths varying, translucent and glowing. */
#include "_gk336.h"

void pattern_446(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0008f;
    float hue0 = gk_sf(seed, 1) + t * 0.008f;
    float cx = GK_W * (0.4f + 0.2f * gk_sf(seed, 2)), cy = GK_H * (0.4f + 0.2f * gk_sf(seed, 3));
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float a = atan2f(dy, dx);
            float lump = gk_n3(gk_cos(a) * 1.5f, gk_sin(a) * 1.5f, t * 0.3f) * 0.3f + gk_n3(gk_cos(a) * 4.0f + 5.0f, gk_sin(a) * 4.0f, t * 0.4f) * 0.12f;
            float r = sqrtf(dx * dx + dy * dy) * (1.0f + lump) + gk_n3((float)x * 0.01f, (float)y * 0.01f, t * 0.3f) * 6.0f;
            float ph = r * 0.09f - t * 2.0f;
            float bands = gk_sin(ph) * 0.5f + 0.5f;
            float bands2 = gk_sin(ph * 0.31f + 1.0f) * 0.5f + 0.5f;
            float b = bands * 0.6f + bands2 * 0.4f;
            uint32_t c = gk_pal(pal, hue0 + bands2 * 0.25f + bands * 0.06f + r * 0.0005f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, bands * 0.15f), 0.6f + 0.35f * b));
        }
    }
    gk_blit(fb, w, h);
}
