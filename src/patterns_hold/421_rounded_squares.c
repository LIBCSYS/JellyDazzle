/* pattern_421 — ROUNDED SQUARES (ground): concentric rounded squares
 * (superellipse rings) that turn slowly and breathe between square and
 * circle; each ring its own ramp stop, edges soft. */
#include "_gk336.h"

void pattern_421(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 41) + t * 0.008f;
    float rot = t * 0.3f, cr = gk_cos(rot), sr = gk_sin(rot);
    float p = 2.6f + 1.4f * gk_sin(t * 0.5f);            /* superellipse power */
    float cx = GK_W * 0.5f + 20.0f * gk_sin(t * 0.7f), cy = GK_H * 0.5f + 15.0f * gk_cos(t * 0.6f);
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float u = gk_absf(dx * cr - dy * sr), v = gk_absf(dx * sr + dy * cr);
            float r = powf(powf(u, p) + powf(v, p), 1.0f / p);
            float ph = r * 0.06f - t * 1.5f;
            float ring = gk_cos(ph) * 0.5f + 0.5f;
            float idx = ph * (1.0f / 6.2832f);
            float band = gk_sstep(0.35f, 0.65f, ring);
            uint32_t c = gk_mix(gk_pal(pal, hue0 + idx * 0.08f), gk_pal(pal, hue0 + idx * 0.08f + 0.3f), band);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.3f * ring + 0.1f * expf(-r * 0.008f)));
        }
    }
    gk_blit(fb, w, h);
}
