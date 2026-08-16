/* pattern_371 — CELL CAUSTICS (ground): a true Voronoi light net — cell
 * edges glow, cell interiors take a colour from the ramp by cell id, the
 * seed points wander on slow orbits so the net flexes without jumping. */
#include "_gk336.h"

void pattern_371(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0025f;
    float hue0 = gk_sf(seed, 13) + t * 0.01f;
    float cell = 44.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            int cx = (int)floorf(fx / cell), cy = (int)floorf(fy / cell);
            float d1 = 1e9f, d2 = 1e9f; uint32_t id = 0;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                uint32_t hh = gk_hash2(cx + i, cy + j, seed);
                float ph = gk_hf(hh) * 6.28f, ph2 = gk_hf(hh >> 7) * 6.28f;
                float sx = ((float)(cx + i) + 0.5f + 0.35f * gk_sin(t + ph)) * cell;
                float sy = ((float)(cy + j) + 0.5f + 0.35f * gk_cos(t * 0.8f + ph2)) * cell;
                float dx = fx - sx, dy = fy - sy, d = sqrtf(dx * dx + dy * dy);
                if (d < d1) { d2 = d1; d1 = d; id = hh; } else if (d < d2) d2 = d;
            }
            float edge = gk_clamp01((d2 - d1) / 12.0f);
            float glow = 1.0f - edge; glow = glow * glow;
            uint32_t inner = gk_pal(pal, hue0 + gk_hf(id >> 11) * 0.22f);
            uint32_t light = gk_lift(gk_pal(pal, hue0 + 0.4f), 0.4f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(inner, light, glow * 0.85f), 0.62f + 0.35f * glow));
        }
    }
    gk_blit(fb, w, h);
}
