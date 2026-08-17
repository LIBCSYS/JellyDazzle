/* pattern_216 — FROST FERNS (field): window frost — feathery dendrites
 * growing from several seeds, side-branches at 60 degrees, black glass
 * between them.  Growth breathes very slowly.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NS 9
void pattern_216(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float sx[NS], sy[NS], sa[NS];
    for (int i = 0; i < NS; i++) {
        sx[i] = vk_seedf(seed, i * 5 + 1) * 1.333f; sy[i] = vk_seedf(seed, i * 5 + 2);
        sa[i] = vk_seedf(seed, i * 5 + 3) * VK_TAU;
    }
    const float grow = 0.75f + 0.25f * vk_sin(t * 0.0012f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float m = 0.0f, ci = base;
            for (int i = 0; i < NS; i++) {
                float dx = u - sx[i], dy = v - sy[i];
                float r = sqrtf(dx * dx + dy * dy);
                if (r > 0.85f * grow) continue;
                float ang = atan2f(dy, dx) - sa[i];
                /* six main arms */
                float a6 = vk_absf(vk_fract(ang * 6.0f / VK_TAU + 0.5f) - 0.5f) * (VK_TAU / 6.0f);
                float along = r * vk_cos(a6), across = r * vk_sin(a6);
                float arm = vk_sstep(0.016f + 0.025f * (1.0f - r), 0.0f, across) * vk_sstep(0.85f * grow, 0.45f * grow, r);
                /* side branches every step along the arm, at 60 deg, length shrinking outward */
                float step = 0.055f;
                float k = floorf(along / step + 0.5f);
                float bx = along - k * step;                /* offset along arm from branch root */
                float blen = (0.09f + 0.06f * vk_noise2(k * 3.0f, i * 9.0f, seed)) * (1.0f - along / (0.9f * grow)) * grow;
                /* branch direction 60 deg from arm: parametrise */
                float pb = bx * 0.5f + across * 0.866f;     /* distance along branch */
                float qb = vk_absf(-bx * 0.866f + across * 0.5f);   /* distance from branch axis */
                float br = vk_sstep(0.014f, 0.0f, qb) * vk_sstep(0.0f, 0.005f, pb) * vk_sstep(blen, blen * 0.6f, pb);
                float bud = 0.6f + 0.4f * vk_sin(pb * 80.0f);
                float val = arm > br * bud ? arm : br * bud;
                if (val > m) { m = val; ci = base + r * 3000.0f + i * 500.0f; }
            }
            m *= 0.9f;
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, m, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
