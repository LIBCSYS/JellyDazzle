/* pattern_342 — SOFT CHECKER PLASMA (ground): a wavy checkerboard whose
 * squares are drawn with sine edges (no hard steps), rotated slowly and
 * lit by a second plasma so the board undulates like a tablecloth. */
#include "_gk336.h"

void pattern_342(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0030f;
    float hue0 = gk_sf(seed, 51) + t * 0.015f;
    float rot = 0.3f + t * 0.06f;
    float cr = gk_cos(rot), sr = gk_sin(rot);
    float per = 0.09f + 0.05f * gk_sf(seed, 52);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            float u = fx * cr - fy * sr, v = fx * sr + fy * cr;
            u += 12.0f * gk_sin(v * 0.03f + t); v += 12.0f * gk_sin(u * 0.03f - t * 0.8f);
            float ch = gk_sin(u * per) * gk_sin(v * per);          /* -1..1 soft checker */
            float lit = gk_sin(fx * 0.015f + t * 0.7f) + gk_sin(fy * 0.02f - t * 0.5f);
            float b = 0.70f + 0.20f * ch + 0.10f * lit;
            gk_pix(y * GK_W + x, pal, hue0 + ch * 0.12f + lit * 0.04f, b);
        }
    }
    gk_blit(fb, w, h);
}
