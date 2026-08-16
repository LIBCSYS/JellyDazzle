/* pattern_339 — PLASMA LOZENGE (ground): a diamond-folded plasma — the
 * coordinates are reflected on a slowly rotating axis pair, so the field
 * reads as soft interlocking lozenges that breathe. */
#include "_gk336.h"

void pattern_339(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0036f;
    float hue0 = gk_sf(seed, 21) + t * 0.018f;
    float rot = t * 0.12f + gk_sf(seed, 22) * 6.28f;
    float cr = gk_cos(rot), sr = gk_sin(rot);
    float per = 55.0f + 30.0f * gk_sf(seed, 23);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * cr - fy * sr, v = fx * sr + fy * cr;
            float au = per * (0.5f + 0.5f * gk_cos(u * 3.14159f / per));
            float av = per * (0.5f + 0.5f * gk_cos(v * 3.14159f / per));
            float c = gk_sin(au * 0.05f + t) + gk_sin(av * 0.05f - t * 0.7f)
                    + gk_sin((au + av) * 0.03f + t * 0.4f) + 0.5f * gk_sin(sqrtf(fx * fx + fy * fy) * 0.02f - t);
            float b = 0.72f + 0.24f * gk_sin(c * 1.2f + t * 0.5f);
            gk_pix(y * GK_W + x, pal, hue0 + c * 0.08f, b);
        }
    }
    gk_blit(fb, w, h);
}
