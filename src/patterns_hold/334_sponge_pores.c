/* pattern_334 — SPONGE PORES (field): the surface of a sea sponge — soft
 * tissue riddled with round pores of many sizes, black holes in a glowing
 * body that swells and relaxes.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_334(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float hole = 0.0f;
            for (int L = 0; L < 2; L++) {
                float cells = L ? 14.0f : 6.0f;
                float uu = u * cells + 0.1f * L, vv = v * cells;
                int iu = (int)floorf(uu), iv = (int)floorf(vv);
                for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                    int cx = iu + i, cy = iv + j;
                    float h1 = vk_h2(cx, cy, seed + L * 91u), h2 = vk_h2(cx, cy, seed ^ (7u + L));
                    if (h2 < 0.25f) continue;
                    float sx = cx + 0.2f + 0.6f * h1, sy = cy + 0.2f + 0.6f * h2;
                    float dx = uu - sx, dy = vv - sy;
                    float d = sqrtf(dx * dx + dy * dy);
                    float rad = (L ? 0.25f : 0.34f) * (0.8f + 0.2f * vk_sin(t * 0.0025f + h1 * VK_TAU));
                    float hh = vk_sstep(rad + 0.10f, rad, d);
                    if (hh > hole) hole = hh;
                }
            }
            float body = 1.0f - hole;
            float swell = 0.5f + 0.5f * vk_sin(u * 2.5f + v * 2.0f + t * 0.002f);
            float tex = 0.8f + 0.2f * vk_noise2(u * 20.0f, v * 20.0f, seed);
            /* dim regions keep it from being a wall */
            float m = body * (0.4f + 0.6f * vk_sstep(0.25f, 0.8f, swell)) * tex;
            float ci = base + swell * 1800.0f + hole * 500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, tex, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
