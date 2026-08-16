/* pattern_410 — WARPED MESH (ground): a triangle mesh bent by a slow
 * domain warp — struts drawn soft, facets filled with a colour wave — so
 * the lattice flexes like a net under water. */
#include "_gk336.h"

void pattern_410(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0018f;
    float hue0 = gk_sf(seed, 25) + t * 0.008f;
    float S = 30.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float wx = gk_n3(fx * 0.007f, fy * 0.007f, t * 0.3f) * 25.0f, wy = gk_n3(fx * 0.007f + 5.0f, fy * 0.007f + 3.0f, t * 0.3f) * 25.0f;
            float u = (fx + wx) / S, v = (fy + wy) / (S * 0.866f);
            int vi = (int)floorf(v); float fv = v - (float)vi;
            float us = u - (vi & 1) * 0.5f;
            int ui = (int)floorf(us); float fu = us - (float)ui;
            int upper = fu + fv > 1.0f;
            float e1 = fv, e2 = fu, e3 = 1.0f - fu - fv;
            float ed = upper ? fminf(fminf(1.0f - e1, 1.0f - e2), -e3) : fminf(fminf(e1, e2), e3);
            float strut = gk_sstep(0.0f, 0.1f, ed);
            float wave = gk_n3((float)ui * 0.15f, (float)vi * 0.15f, t * 0.4f);
            uint32_t facet = gk_pal(pal, hue0 + wave * 0.5f + (upper ? 0.03f : 0.0f));
            uint32_t line = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.7f);
            gk_put(y * GK_W + x, gk_shade(gk_mix(line, facet, strut), 0.6f + 0.15f * wave + 0.2f * (1.0f - strut) + 0.1f * ed));
        }
    }
    gk_blit(fb, w, h);
}
