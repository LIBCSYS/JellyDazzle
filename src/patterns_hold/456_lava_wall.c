/* pattern_456 — LAVA WALL (ground): a full-frame lava lamp — big wax
 * blobs rising and sinking on slow independent clocks against a lit
 * gradient, blobs merging with a soft metaball threshold, no black. */
#include "_gk336.h"

void pattern_456(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 1) + t * 0.008f;
    float bx[7], by[7], br[7];
    for (int i = 0; i < 7; i++) {
        float ph = gk_sf(seed, 10 + i) * 6.28f, sp = 0.3f + 0.4f * gk_sf(seed, 20 + i);
        bx[i] = GK_W * (0.15f + 0.7f * gk_sf(seed, 30 + i)) + 25.0f * gk_sin(t * sp * 0.7f + ph);
        by[i] = GK_H * (0.5f + 0.42f * gk_sin(t * sp + ph * 1.3f));
        br[i] = 22.0f + 22.0f * gk_sf(seed, 40 + i);
    }
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float f = 0, hue = 0;
            for (int i = 0; i < 7; i++) {
                float dx = (float)x - bx[i], dy = (float)y - by[i];
                float g = br[i] * br[i] / (dx * dx + dy * dy + 1.0f);
                f += g; hue += g * (float)i;
            }
            hue /= (f + 1e-4f);
            float wax = gk_sstep(0.7f, 1.3f, f);
            float rim = expf(-(f - 1.0f) * (f - 1.0f) * 6.0f);
            uint32_t back = gk_pal(pal, hue0 + fy * 0.15f);
            uint32_t waxc = gk_pal(pal, hue0 + 0.35f + hue * 0.03f);
            uint32_t c = gk_mix(back, waxc, wax);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, rim * 0.25f), 0.6f + 0.2f * (1.0f - fy) + 0.2f * wax));
        }
    }
    gk_blit(fb, w, h);
}
