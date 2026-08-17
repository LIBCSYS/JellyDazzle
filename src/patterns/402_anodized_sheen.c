/* pattern_402 — ANODIZED SHEEN (ground): the rainbow interference sheen of
 * anodised titanium — a smooth height field whose colour is read straight
 * off the ramp by height, with a broad soft specular sweep. */
#include "_gk336.h"

void pattern_402(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 41) + t * 0.008f;
    float lx = gk_cos(t * 0.5f) * 0.8f, ly = gk_sin(t * 0.5f) * 0.8f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            #define HA(X, Y) (gk_n3((X) * 0.005f, (Y) * 0.005f, t * 0.2f) + 0.35f * gk_n3((X) * 0.012f + 4.0f, (Y) * 0.012f, t * 0.3f))
            float h0 = HA(fx, fy);
            float dxh = HA(fx + 2.0f, fy) - h0, dyh = HA(fx, fy + 2.0f) - h0;
            #undef HA
            float nx = -dxh * 15.0f, ny = -dyh * 15.0f;
            float nl = 1.0f / sqrtf(nx * nx + ny * ny + 1.0f); nx *= nl; ny *= nl;
            float diff = gk_clamp01(nx * lx + ny * ly + 0.6f);
            float spec = gk_clamp01(diff - 0.8f) * 5.0f;
            uint32_t c = gk_pal(pal, hue0 + h0 * 0.35f + diff * 0.05f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, spec * 0.4f), 0.55f + 0.4f * diff));
        }
    }
    gk_blit(fb, w, h);
}
