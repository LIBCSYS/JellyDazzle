/* pattern_300 — EYELET LACE (field): broderie anglaise — a lit cloth
 * punched with rows of eyelets, each hole black with an embroidered rim, the
 * holes swelling and shrinking on slow clocks; the cloth ripples.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_300(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 5.0f, 8.0f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float a = (u + 0.04f * vk_sin(v * 4.0f + t * 0.002f)) * n, b = (v + 0.04f * vk_sin(u * 3.0f - t * 0.0016f)) * n;
            /* staggered rows */
            float ry = floorf(b);
            float ox = ((int)ry & 1) ? 0.5f : 0.0f;
            float rx = floorf(a - ox) + ox;
            float dx = a - rx - 0.5f, dy = b - ry - 0.5f;
            float d = sqrtf(dx * dx + dy * dy);
            float ph = vk_h2((int)(rx * 2.0f), (int)ry, seed) * VK_TAU;
            float hole = 0.28f + 0.10f * vk_sin(t * 0.003f + ph);
            /* petal-shaped hole: 6 lobes */
            float lobe = 1.0f + 0.15f * vk_cos(atan2f(dy, dx) * 6.0f + ph);
            float hd = d / lobe;
            float cloth = vk_sstep(hole, hole + 0.05f, hd);
            float rim = vk_sstep(0.09f, 0.03f, vk_absf(hd - hole - 0.04f));
            /* cloth weave shading with soft ripples */
            float sheen = 0.55f + 0.45f * vk_sin((a + b) * 1.5f + t * 0.003f);
            float m = cloth * (0.45f + 0.35f * sheen) + rim * 0.55f;
            m = m > 1.0f ? 1.0f : m;
            /* also drop some cloth panels dark to keep transparency */
            float panel = vk_sstep(0.4f, 0.7f, 0.5f + 0.5f * vk_sin(a * 0.4f + b * 0.3f + t * 0.0025f));
            m *= 0.15f + 0.85f * panel;
            float ci = base + sheen * 1400.0f + rim * 1600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, rim, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
