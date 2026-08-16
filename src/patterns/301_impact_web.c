/* pattern_301 — IMPACT WEB (field): a pane shattered from one point —
 * radial cracks and concentric rings, the glass shards between them lit in
 * turn and others dark, the web flexing.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_301(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cx = 0.4f + 0.5f * vk_seedf(seed, 1) + 0.03f * vk_sin(t * 0.001f), cy = 0.3f + 0.4f * vk_seedf(seed, 2);
    const int nr = 14 + 2 * (int)(vk_seedf(seed, 3) * 4.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float dx = u - cx, dy = v - cy;
            float r = sqrtf(dx * dx + dy * dy) + 1e-4f;
            float ang = atan2f(dy, dx);
            /* radial cracks with wobble */
            float a = ang * nr / VK_TAU;
            float wob = 0.15f * vk_noise2(r * 6.0f, floorf(a) * 3.0f, seed) - 0.075f;
            float fa = vk_absf(vk_fract(a + wob) - 0.5f) * (VK_TAU / nr) * r;
            float crack = vk_sstep(0.012f, 0.003f, fa);
            /* rings, irregular spacing */
            float rr = r * 7.0f + 0.4f * vk_noise2(ang * 3.0f, floorf(r * 7.0f), seed ^ 3u);
            float fr = vk_absf(vk_fract(rr) - 0.5f) / 7.0f;
            float ring = vk_sstep(0.010f, 0.003f, fr) * vk_sstep(0.05f, 0.1f, r);
            /* shards: lit per cell */
            int ia = (int)floorf(a + wob), ir = (int)floorf(rr);
            float lit = 0.5f + 0.5f * vk_sin(t * 0.002f + vk_h2(ia, ir, seed) * VK_TAU);
            float shard = vk_sstep(0.35f, 0.65f, lit) * 0.55f * (0.6f + 0.4f * vk_h2(ia, ir, seed ^ 5u));
            float lines = crack > ring ? crack : ring;
            float m = lines > shard ? lines : shard;
            m *= vk_sstep(1.5f, 0.9f, r);
            float ci = base + vk_h2(ia, ir, seed ^ 9u) * 1200.0f + r * 1500.0f + (lines > shard ? 2000.0f : 0.0f) + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, lit, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
