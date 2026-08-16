/* pattern_278 — VAULT RIBS (field): looking up into a ribbed vault —
 * arches spring from the sides and meet at the crown, the webs between ribs
 * lit softly and some left dark; the vault breathes.  Mirror symmetry.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_278(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const int nb = 6;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.0f;
            float m = 0.0f, ci = base;
            /* bays: repeating along v, each bay an arch in (u,v_local) */
            float bv = v * (float)nb * 0.5f + t * 0.0004f;
            float f = vk_fract(bv), ib = floorf(bv);
            float lv = (f - 0.5f) * 2.0f;                        /* -1..1 across the bay */
            /* arch: circle centred at (1, 0) in (u, lv) space of radius 1 -> u = 1 - sqrt(1-lv^2) */
            float ru = 1.0f - sqrtf(vk_absf(1.0f - lv * lv)) * 0.98f;
            float rib = vk_sstep(0.05f, 0.015f, vk_absf(u - ru)) ;
            /* diagonal ribs to the crown */
            float ru2 = 1.0f - vk_absf(lv);
            float rib2 = vk_sstep(0.045f, 0.012f, vk_absf(u - ru2)) * 0.9f;
            /* transverse rib at bay boundary */
            float rib3 = vk_sstep(0.05f, 0.015f, vk_absf(f - 0.5f) > 0.45f ? 0.0f : 1.0f) * 0.0f + vk_sstep(0.05f, 0.015f, vk_absf(vk_absf(lv) - 1.0f)) * 0.9f;
            float ribs = rib > rib2 ? rib : rib2; ribs = ribs > rib3 ? ribs : rib3;
            /* web panels: lit depending on bay & side */
            int panel = (u > ru2) + 2 * (u > ru);
            float lit = 0.5f + 0.5f * vk_sin(t * 0.0025f + ib * 1.7f + panel * 2.1f);
            float web = vk_sstep(0.3f, 0.7f, lit) * 0.45f * (0.7f + 0.3f * (1.0f - u));
            m = ribs > web ? ribs : web;
            ci = base + panel * 600.0f + u * 1500.0f + (ribs > web ? 2000.0f : 0.0f) + t * 0.5f;
            m *= vk_sstep(0.0f, 0.02f, u) + 0.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, lit, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
