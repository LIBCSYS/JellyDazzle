/* pattern_429 — SOFT ARGYLE (ground): the diamond argyle — a lattice of
 * lozenges in alternating hues with a fine soft diagonal over-check,
 * the whole sheet gently rippling and slowly rotating. */
#include "_gk336.h"

void pattern_429(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0015f;
    float hue0 = gk_sf(seed, 21) + t * 0.008f;
    float ang = 0.785f + 0.08f * gk_sin(t * 0.3f), ca = gk_cos(ang), sa = gk_sin(ang);
    float cell = 44.0f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float fx = (float)x - GK_W * 0.5f, fy = (float)y - GK_H * 0.5f;
            fx += 6.0f * gk_sin(fy * 0.02f + t); fy += 6.0f * gk_sin(fx * 0.02f - t * 0.7f);
            float u = (fx * ca + fy * sa) / cell, v = (-fx * sa + fy * ca * 1.5f) / cell;
            int ui = (int)floorf(u), vi = (int)floorf(v);
            float pu = gk_fract(u) - 0.5f, pv = gk_fract(v) - 0.5f;
            int par = (ui + vi) & 1;
            float edge = gk_sstep(0.5f, 0.42f, fmaxf(gk_absf(pu), gk_absf(pv)));
            float over = gk_sstep(0.02f, 0.07f, gk_absf(gk_fract(u + 0.5f) - 0.5f)) * gk_sstep(0.02f, 0.07f, gk_absf(gk_fract(v + 0.5f) - 0.5f));
            float wave = gk_n3((float)x * 0.004f, (float)y * 0.004f, t * 0.2f);
            uint32_t a = gk_pal(pal, hue0 + wave * 0.1f), b = gk_pal(pal, hue0 + 0.33f + wave * 0.1f);
            uint32_t d = gk_pal(pal, hue0 + 0.66f);
            uint32_t c = gk_mix(par ? a : b, d, (1.0f - edge) * 0.6f);
            c = gk_mix(c, d, (1.0f - over) * 0.5f);
            gk_put(y * GK_W + x, gk_shade(c, 0.7f + 0.15f * wave + 0.1f * edge));
        }
    }
    gk_blit(fb, w, h);
}
