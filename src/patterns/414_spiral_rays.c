/* pattern_414 — SPIRAL RAYS (ground): rays that curve — a sunburst whose
 * angle term is offset by radius, so the fan twists into a soft pinwheel
 * that turns very slowly and breathes its twist. */
#include "_gk336.h"

void pattern_414(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 9) + t * 0.008f;
    float nr = (float)(5 + (int)(gk_sf(seed, 10) * 5.0f));
    float twist = 0.012f + 0.006f * gk_sin(t * 0.4f);
    float cx = GK_W * 0.5f + 30.0f * gk_sin(t * 0.3f), cy = GK_H * 0.5f + 20.0f * gk_cos(t * 0.23f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx);
            float ph = a * nr + r * twist * nr + t;
            float ray = gk_sin(ph) * 0.5f + 0.5f;
            ray = ray * ray * (3.0f - 2.0f * ray);
            float ring = gk_sin(r * 0.03f - t * 2.0f) * 0.5f + 0.5f;
            uint32_t c = gk_pal(pal, hue0 + ray * 0.2f + r * 0.0008f + ring * 0.03f);
            gk_put(y * GK_W + x, gk_shade(c, 0.55f + 0.3f * ray + 0.15f * ring));
        }
    }
    gk_blit(fb, w, h);
}
