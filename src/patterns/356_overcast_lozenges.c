/* pattern_356 — OVERCAST LOZENGES (ground): a mackerel sky — rows of soft
 * cloud lozenges in a staggered lattice, each puff a smooth bump, the whole
 * sheet drifting and the puffs slowly swelling and shrinking. */
#include "_gk336.h"

void pattern_356(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0022f;
    float hue0 = gk_sf(seed, 33) + t * 0.01f;
    float sx = 0.16f + 0.05f * gk_sf(seed, 34), sy = sx * 0.6f;
    for (int y = 0; y < GK_H; y++) {
        float fy = (float)y / GK_H;
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x / GK_H + t * 0.08f;
            float n = gk_n3(fx * 2.0f, fy * 2.0f, t * 0.3f) * 0.03f;
            float px = fx + n, py = fy + n * 0.7f;
            int row = (int)floorf(py / sy);
            float ox = (row & 1) ? sx * 0.5f : 0.0f;
            float cx = (floorf((px - ox) / sx) + 0.5f) * sx + ox, cy = ((float)row + 0.5f) * sy;
            float dx = (px - cx) / sx, dy = (py - cy) / sy;
            float swell = 0.85f + 0.15f * gk_sin(t + (float)row * 0.7f + cx * 5.0f);
            float bump = expf(-(dx * dx + dy * dy * 1.3f) * 9.0f / swell);
            float sheet = gk_fbm3(fx * 1.0f, fy * 1.0f, t * 0.2f, 3);
            uint32_t base = gk_pal(pal, hue0 + fy * 0.15f + sheet * 0.08f);
            uint32_t puff = gk_pal(pal, hue0 + 0.3f + (float)row * 0.01f);
            uint32_t c = gk_mix(base, puff, bump * 0.75f);
            gk_put(y * GK_W + x, gk_shade(c, 0.60f + 0.35f * bump + 0.08f * sheet));
        }
    }
    gk_blit(fb, w, h);
}
