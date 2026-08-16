/* pattern_268 — ICE SHARDS (field): long angular shards of ice lying
 * across each other, each catching light along its length, black between
 * them; the pile shifts imperceptibly.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NSH 32
void pattern_268(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float sx[NSH], sy[NSH], sa[NSH], sl_[NSH], swd[NSH];
    for (int i = 0; i < NSH; i++) {
        sx[i] = vk_seedf(seed, i * 5 + 1) * 1.333f + 0.02f * vk_sin(t * 0.001f + i);
        sy[i] = vk_seedf(seed, i * 5 + 2) + 0.02f * vk_cos(t * 0.0008f + i * 2.0f);
        sa[i] = vk_seedf(seed, i * 5 + 3) * VK_TAU + 0.02f * vk_sin(t * 0.0012f + i);
        sl_[i] = 0.25f + 0.35f * vk_seedf(seed, i * 5 + 4);
        swd[i] = 0.03f + 0.05f * vk_seedf(seed, i * 5 + 5);
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float m = 0.0f, ci = base;
            for (int i = 0; i < NSH; i++) {
                float dx = u - sx[i], dy = v - sy[i];
                float ca = vk_cos(sa[i]), sn = vk_sin(sa[i]);
                float a = dx * ca + dy * sn, b = -dx * sn + dy * ca;
                float half = swd[i] * (1.0f - vk_absf(a) / sl_[i]);      /* pointed ends */
                if (vk_absf(a) > sl_[i] || vk_absf(b) > half) continue;
                float edge = vk_sstep(half, half * 0.6f, vk_absf(b));
                float lit = 0.5f + 0.5f * vk_sin(a * 20.0f + t * 0.004f + i);
                float facet = 0.55f + 0.45f * (b > 0.0f ? 1.0f : 0.5f) * lit;
                float val = edge * facet;
                if (val > m) { m = val; ci = base + i * 190.0f + lit * 1400.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1600.0f, 0.5f + 0.5f * vk_sin(u * 4.0f + v * 3.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
