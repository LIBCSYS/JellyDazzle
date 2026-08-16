/* pattern_401 — HAMMERED METAL (ground): a hammered surface — packed
 * shallow dimples (a Voronoi of concave dishes) each catching a moving
 * light on one rim, warm metallic tint from the ramp. */
#include "_gk336.h"

void pattern_401(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 37) + t * 0.008f;
    float cell = 20.0f;
    float lx = gk_cos(t * 0.7f), ly = gk_sin(t * 0.7f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float d1 = 1e9f; float ox = 0, oy = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float sx = ((float)(cx + i) + gk_hf(hh)) * cell, sy = ((float)(cy + j) + gk_hf(hh >> 5)) * cell;
                float dx = fx - sx, dy = fy - sy, d = dx * dx + dy * dy;
                if (d < d1) { d1 = d; ox = dx; oy = dy; }
            }
            float r = sqrtf(d1) / cell;
            /* concave dish: normal points toward centre, steeper at rim */
            float nx = -ox / cell * 1.5f, ny = -oy / cell * 1.5f;
            float diff = gk_clamp01(0.5f + (nx * lx + ny * ly) * 0.9f);
            float spec = gk_clamp01(diff - 0.7f) * 3.0f;
            float tint = gk_n3(fx * 0.005f, fy * 0.005f, t * 0.2f);
            uint32_t c = gk_pal(pal, hue0 + tint * 0.12f + r * 0.03f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, spec * 0.5f), 0.5f + 0.4f * diff + 0.1f * (1.0f - r)));
        }
    }
    gk_blit(fb, w, h);
}
