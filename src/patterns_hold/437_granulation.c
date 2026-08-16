/* pattern_437 — GRANULATION (ground): pigment granulating into the paper
 * tooth — a mid-frequency mottle riding under smooth colour fields, the
 * mottle strongest where the wash is densest; fields drift slowly. */
#include "_gk336.h"

void pattern_437(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 9) + t * 0.008f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x, fy = (float)y;
            float field = gk_fbm3(fx * 0.005f, fy * 0.005f, t * 0.3f, 3);
            float dens = gk_sstep(-0.4f, 0.4f, field);
            float tooth = gk_fbm2(fx * 0.03f, fy * 0.03f, 2);
            float gran = tooth * dens * 0.35f;
            uint32_t light = gk_lift(gk_pal(pal, hue0 + field * 0.1f), 0.35f);
            uint32_t deep = gk_pal(pal, hue0 + 0.3f + field * 0.15f);
            uint32_t c = gk_mix(light, deep, dens);
            gk_put(y * GK_W + x, gk_shade(c, 0.8f - gran + 0.05f * tooth));
        }
    }
    gk_blit(fb, w, h);
}
