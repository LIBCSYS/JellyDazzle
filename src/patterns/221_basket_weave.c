/* pattern_221 — BASKET WEAVE (field): broad flat splints woven three-over,
 * three-under, the whole basket flexing gently; open slots between splints
 * are black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_221(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 3, 7.0f, 10.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            /* flex: gentle 2-D warp */
            float a = u + 0.06f * vk_sin(v * 3.0f + t * 0.002f), b = v + 0.06f * vk_sin(u * 2.5f - t * 0.0016f);
            float wa = a * n, wb = b * n;
            int ia = (int)floorf(wa), ib = (int)floorf(wb);
            float fa = vk_fract(wa), fbb = vk_fract(wb);
            /* which is on top: 3-over-3 pattern */
            int over = (((ia + 30) / 3 + (ib + 30) / 3) & 1);
            /* splint shading: cylinder highlight across, plus over/under shadow */
            float sa = vk_sstep(0.14f, 0.28f, fa) * vk_sstep(0.86f, 0.72f, fa);
            float sb = vk_sstep(0.14f, 0.28f, fbb) * vk_sstep(0.86f, 0.72f, fbb);
            float shadeA = 0.55f + 0.45f * vk_sin((fa - 0.1f) * 3.9f);
            float shadeB = 0.55f + 0.45f * vk_sin((fbb - 0.1f) * 3.9f);
            float m, ci;
            if (over) { m = sa * shadeA * (0.7f + 0.3f * vk_sstep(0.0f, 0.3f, fbb) * vk_sstep(1.0f, 0.7f, fbb)); ci = base + fa * 900.0f + (float)(ia & 3) * 300.0f; }
            else      { m = sb * shadeB * (0.7f + 0.3f * vk_sstep(0.0f, 0.3f, fa) * vk_sstep(1.0f, 0.7f, fa)); ci = base + 2200.0f + fbb * 900.0f + (float)(ib & 3) * 300.0f; }
            /* the black slots: dropped every third splint per direction */
            int slotA = ((ia + 30) % 3) == 2, slotB = ((ib + 30) % 3) == 2;
            if (over && slotA) m = 0.0f;
            if (!over && slotB) m = 0.0f;
            /* let the other direction show in the slot */
            if (m == 0.0f) {
                if (over && !slotB) m = sb * shadeB * 0.6f, ci = base + 2200.0f + fbb * 900.0f;
                else if (!over && !slotA) m = sa * shadeA * 0.6f, ci = base + fa * 900.0f;
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin((a + b) * 3.0f + t * 0.003f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
