/* pattern_226 — SPIRAL GALAXY (field): two broad logarithmic arms of glowing
 * gas turning around a soft core, dark dust lanes between the arms and black
 * space beyond.  2-fold symmetry.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_226(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float pitch = vk_seedr(seed, 1, 2.6f, 4.0f);
    const float arms = 2.0f + (float)(seed & 1);
    const float rot = t * 0.0006f;
    const float tilt = vk_seedr(seed, 2, 0.55f, 0.85f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f / tilt;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v) + 0.001f;
            float ang = atan2f(v, u) - rot;
            /* spiral phase */
            float ph = ang * arms - logf(r) * pitch * arms;
            float arm = 0.5f + 0.5f * vk_cos(ph);
            /* clumpy gas along the arm */
            float clump = vk_fbm2(u * 4.0f + t * 0.0004f, v * 4.0f, 3, seed);
            float lane = 0.5f + 0.5f * vk_cos(ph * 2.0f + 1.5f + clump * 2.0f);   /* dust lane */
            float armw = vk_sstep(0.18f, 0.80f, arm) * (0.55f + 0.45f * clump);
            armw *= 0.6f + 0.4f * lane;
            float core = vk_sstep(0.30f, 0.05f, r);
            float fade = vk_sstep(1.3f, 0.6f, r);
            float m = (armw * fade + core) * vk_sstep(0.0f, 0.06f, r);
            if (m > 1.0f) m = 1.0f;
            float ci = base + r * 2400.0f + arm * 800.0f + clump * 600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1900.0f, clump, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
