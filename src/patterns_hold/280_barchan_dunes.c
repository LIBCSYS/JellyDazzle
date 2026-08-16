/* pattern_280 — BARCHAN DUNES (field): crescent dunes marching across a
 * dark plain, each lit on its windward slope with a black slip face, the
 * horns trailing downwind.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_280(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = 3.5f;
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells + t * 0.0004f;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float m = 0.0f, ci = base;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float h1 = vk_h2(cx, cy, seed), h2 = vk_h2(cx, cy, seed ^ 3u);
                if (h2 < 0.05f) continue;
                float sx = cx + 0.3f + 0.4f * h1, sy = cy + 0.3f + 0.4f * h2;
                float dx = u - sx, dy = v - sy;
                float sz = 0.6f + 0.25f * vk_h2(cx, cy, seed ^ 9u);
                /* crescent: big disc minus offset disc (offset downwind = +x) */
                float d0 = sqrtf(dx * dx + dy * dy * 1.3f) / sz;
                float d1 = sqrtf((dx - sz * 0.5f) * (dx - sz * 0.5f) + dy * dy * 1.3f) / sz;
                float cres = vk_sstep(1.0f, 0.9f, d0) * vk_sstep(0.80f, 0.92f, d1);
                /* windward slope shading: bright toward -x */
                float slope = 0.45f + 0.55f * vk_sstep(0.5f, -0.9f, dx / sz);
                float ripple = 0.85f + 0.15f * vk_sin(dx * 40.0f + dy * 10.0f + t * 0.004f);
                float val = cres * slope * ripple;
                if (val > m) { m = val; ci = base + h1 * 800.0f + d0 * 1500.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(u * 2.0f + v), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
