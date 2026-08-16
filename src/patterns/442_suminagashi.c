/* pattern_442 — SUMINAGASHI (ground): floating-ink marbling — many thin
 * concentric rings from a few centres, pushed around by a slow flow so
 * they stretch into the classic feathered whorls; two inks alternating. */
#include "_gk336.h"

void pattern_442(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.001f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    float cx[3], cy[3];
    for (int i = 0; i < 3; i++) { cx[i] = GK_W * (0.2f + 0.6f * gk_sf(seed, 30 + i)); cy[i] = GK_H * (0.2f + 0.6f * gk_sf(seed, 40 + i)); }
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float wx = gk_fbm3(fx * 0.005f, fy * 0.005f, t * 0.4f, 3) * 60.0f, wy = gk_fbm3(fx * 0.005f + 8.0f, fy * 0.005f + 3.0f, t * 0.4f, 3) * 60.0f;
            float px = fx + wx, py = fy + wy;
            float dmin = 1e9f; int which = 0;
            for (int i = 0; i < 3; i++) {
                float dx = px - cx[i], dy = py - cy[i], d = sqrtf(dx * dx + dy * dy);
                if (d < dmin) { dmin = d; which = i; }
            }
            float ring = gk_sin(dmin * 0.25f + (float)which * 2.0f) * 0.5f + 0.5f;
            float ink = gk_sstep(0.35f, 0.65f, ring);
            uint32_t a = gk_lift(gk_pal(pal, hue0 + wx * 0.001f), 0.35f);
            uint32_t b = gk_pal(pal, hue0 + 0.4f + (float)which * 0.08f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(a, b, ink), 0.72f + 0.2f * (1.0f - ink) + 0.05f * gk_sin(dmin * 0.02f)));
        }
    }
    gk_blit(fb, w, h);
}
