/* pattern_372 — WATER SURFACE (ground): a rippled surface seen at a low
 * angle — a height field of slow crossed waves shaded with a moving light
 * so the crests catch a highlight; colour from the surface normal. */
#include "_gk336.h"

void pattern_372(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.003f;
    float hue0 = gk_sf(seed, 17) + t * 0.01f;
    float lx = gk_cos(t * 0.3f) * 0.7f, ly = gk_sin(t * 0.3f) * 0.7f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y * 1.6f;     /* foreshortened */
            float e = 1.0f;
            #define HGT(X, Y) (gk_sin((X) * 0.03f + (Y) * 0.01f + t) + gk_sin((X) * 0.012f - (Y) * 0.025f - t * 0.7f) \
                             + 0.6f * gk_n3((X) * 0.02f, (Y) * 0.02f, t * 0.4f))
            float hx = HGT(fx + e, fy) - HGT(fx - e, fy);
            float hy = HGT(fx, fy + e) - HGT(fx, fy - e);
            #undef HGT
            float nx = -hx * 4.0f, ny = -hy * 4.0f;
            float nl = 1.0f / sqrtf(nx * nx + ny * ny + 1.0f);
            nx *= nl; ny *= nl;
            float diff = gk_clamp01(nx * lx + ny * ly + 0.55f);
            float spec = gk_clamp01(diff - 0.75f) * 4.0f;
            uint32_t c = gk_pal(pal, hue0 + nx * 0.12f + ny * 0.08f + (float)y * 0.0004f);
            gk_put(y * GK_W + x, gk_shade(gk_lift(c, spec * 0.4f), 0.62f + 0.36f * diff));
        }
    }
    gk_blit(fb, w, h);
}
