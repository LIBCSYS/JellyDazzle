/* pattern_381 — SILK FOLDS (ground): draped silk — a height field of long
 * soft folds (stretched noise) with anisotropic sheen: the highlight runs
 * along the fold direction and slides as the light moves. */
#include "_gk336.h"

void pattern_381(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.002f;
    float hue0 = gk_sf(seed, 1) + t * 0.01f;
    float ang = gk_sf(seed, 2) * 3.14f, ca = gk_cos(ang), sa = gk_sin(ang);
    float lx = gk_cos(t * 0.5f), ly = gk_sin(t * 0.5f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float u = fx * ca + fy * sa, v = -fx * sa + fy * ca;
            #define HF(U, V) (gk_n3((U) * 0.012f, (V) * 0.0025f, t * 0.2f) + 0.4f * gk_n3((U) * 0.03f + 5.0f, (V) * 0.006f, t * 0.3f))
            float h0 = HF(u, v);
            float du = HF(u + 2.0f, v) - h0, dv = HF(u, v + 2.0f) - h0;
            #undef HF
            float nx = -du * ca + dv * sa, ny = -du * sa - dv * ca;   /* back to screen */
            float diff = gk_clamp01(0.55f + (nx * lx + ny * ly) * 6.0f);
            float sheen = gk_clamp01(1.0f - gk_absf(du) * 12.0f) * 0.35f;
            uint32_t c = gk_pal(pal, hue0 + h0 * 0.12f + diff * 0.06f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, sheen * diff), 0.5f + 0.45f * diff));
        }
    }
    gk_blit(fb, w, h);
}
