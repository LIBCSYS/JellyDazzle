/* pattern_388 — ROSE WINDOW (ground): a cathedral rose — radial petals in
 * concentric rings, each cell a pane of coloured glass, soft leading, the
 * whole window turning slowly and its light breathing. */
#include "_gk336.h"

void pattern_388(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; gk_begin(pal);
    float t = (float)frame * 0.0012f;
    float hue0 = gk_sf(seed, 29) + t * 0.008f;
    int nseg = 8 + 2 * (int)(gk_sf(seed, 30) * 4.0f);
    float cx = GK_W * 0.5f, cy = GK_H * 0.5f;
    for (int y = 0; y < GK_H; y++) {
        for (int x = 0; x < GK_W; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float r = sqrtf(dx * dx + dy * dy), a = atan2f(dy, dx) + t;
            float ring = r / 30.0f;
            int ri = (int)floorf(ring);
            float segs = (float)nseg * (float)(ri + 1) * 0.5f;
            if (segs < (float)nseg) segs = (float)nseg;
            float sa = a * segs / 6.2832f + (ri & 1) * 0.5f;
            int si = (int)floorf(sa);
            float fr = ring - (float)ri, fs = sa - (float)si;
            float er = gk_sstep(0.0f, 0.12f, fr) * gk_sstep(1.0f, 0.88f, fr);
            float es = gk_sstep(0.0f, 0.15f, fs) * gk_sstep(1.0f, 0.85f, fs);
            float lead = er * es;
            uint32_t hh = gk_hash2(ri, si, seed);
            float glow = 0.5f + 0.5f * gk_sin(t * 2.0f + (float)ri * 0.8f + (float)si * 0.3f);
            uint32_t pane = gk_pal(pal, hue0 + gk_hf(hh) * 0.5f + fr * 0.03f);
            uint32_t leadc = gk_shade(gk_pal(pal, hue0 + 0.5f), 0.35f);
            uint32_t c = gk_mix(leadc, pane, lead);
            gk_put(y * GK_W + x, gk_shade(c, 0.6f + 0.25f * glow + 0.15f * (1.0f - fr)));
        }
    }
    gk_blit(fb, w, h);
}
