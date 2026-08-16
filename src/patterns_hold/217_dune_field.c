/* pattern_217 — DUNE FIELD (field): wind-ripples across sand, each crest
 * lit from one side, troughs falling to black shadow; the ripples migrate
 * slowly and their line wanders.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_217(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float k = vk_seedr(seed, 1, 14.0f, 22.0f);
    const float ang = vk_seedr(seed, 2, -0.5f, 0.5f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            /* ripples wander with a large-scale noise and slowly migrate */
            float wander = 1.2f * vk_noise2(u * 1.5f, v * 1.5f + t * 0.0004f, seed)
                         + 0.4f * vk_noise2(u * 5.0f, v * 5.0f - t * 0.0006f, seed ^ 3u);
            float p = (v + u * ang) * k + wander * 2.0f - t * 0.0035f;
            float f = vk_fract(p);
            /* asymmetric slope: gentle windward, steep lee — light comes from windward */
            float lit = f < 0.65f ? f / 0.65f : 1.0f - (f - 0.65f) / 0.35f;
            float m = vk_sstep(0.32f, 0.9f, lit);
            /* perspective: dunes shrink toward the top */
            float sheen = 0.5f + 0.5f * vk_sin(u * 30.0f + v * 20.0f + wander * 6.0f);
            float ci = base + lit * 1400.0f + v * 2400.0f + wander * 400.0f + t * 0.5f;
            float cj = ci + 1800.0f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, cj, sheen * 0.6f, m * (0.85f + 0.15f * sheen)));
        }
    }
    vk_blit(&cv, fb, w, h);
}
