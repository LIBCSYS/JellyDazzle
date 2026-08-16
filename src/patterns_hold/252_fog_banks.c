/* pattern_252 — FOG BANKS (field): layered banks of fog drifting across at
 * different speeds, each bank a soft horizontal mass with ragged edges,
 * clear black air between them.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NB 5
static float p_cy[NB], p_half[NB][640];
void pattern_252(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    /* per-bank, per-column geometry (independent of y) */
    for (int i = 0; i < NB; i++) {
        p_cy[i] = (i + 0.5f) / NB + 0.04f * vk_sin(t * 0.001f + i);
        float speed = 0.0008f * (0.5f + vk_seedf(seed, i));
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float ux = u + t * speed * (i & 1 ? 1.0f : -1.0f);
            float edge = 0.08f * vk_fbm2(ux * 3.0f, i * 5.0f + t * 0.0005f, 3, seed + i);
            p_half[i][x] = 0.035f + 0.04f * vk_noise2(ux * 1.5f, i * 3.0f, seed ^ 9u) + edge;
        }
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            uint32_t col = 0xFF000000u;
            for (int i = 0; i < NB; i++) {
                float d = vk_absf(v - p_cy[i]);
                float half = p_half[i][x];
                if (d > half) continue;
                float body = vk_sstep(half, half * 0.3f, d);
                float speed = 0.0008f * (0.5f + vk_seedf(seed, i));
                float ux = u + t * speed * (i & 1 ? 1.0f : -1.0f);
                float dens = vk_noise2(ux * 6.0f, v * 12.0f + i, seed + 3u * i);
                float m = body * (0.55f + 0.45f * dens);
                float ci = base + i * 700.0f + dens * 1200.0f + t * 0.4f;
                col = vk_max(col, vk_pc2(pal, ci, ci + 1600.0f, dens, m));
            }
            vk_putp(row + x * 3, col);
        }
    }
    vk_blit(&cv, fb, w, h);
}
