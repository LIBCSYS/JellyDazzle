/* pattern_306 — PAISLEY FIELD (field): paisley teardrops on a staggered
 * grid, each with a curled tail and inner rings, alternating orientation,
 * black cloth between them; the print sways.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_306(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 2.0f, 2.8f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f * n;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f * n + 0.05f * vk_sin(v * 2.0f + t * 0.002f);
            float ry = floorf(v);
            float ox = ((int)ry & 1) ? 0.5f : 0.0f;
            float rx = floorf(u - ox) + ox;
            float dx = u - rx - 0.5f, dy = v - ry - 0.5f;
            int flip = ((int)rx + (int)ry) & 1;
            if (flip) dx = -dx;
            /* teardrop: circle whose radius shrinks toward the tail, tail curls */
            float ang = atan2f(dy, dx);
            float r = sqrtf(dx * dx + dy * dy);
            float body_r = 0.50f - 0.22f * vk_sstep(-1.0f, 1.0f, vk_cos(ang - 0.6f)) ;
            float curl = 0.06f * vk_sin(ang * 2.0f + t * 0.002f);
            float d = r / (body_r + curl);
            float body = vk_sstep(1.0f, 0.9f, d);
            float ring1 = vk_sstep(0.06f, 0.0f, vk_absf(d - 0.72f));
            float ring2 = vk_sstep(0.06f, 0.0f, vk_absf(d - 0.42f));
            float dots = vk_sstep(0.5f, 0.9f, 0.5f + 0.5f * vk_sin(ang * 12.0f)) * vk_sstep(0.05f, 0.0f, vk_absf(d - 0.9f));
            float fill = 0.45f + 0.3f * (1.0f - d);
            float m = body * (fill + 0.6f * (ring1 > ring2 ? ring1 : ring2) + 0.5f * dots);
            m = m > 1.0f ? 1.0f : m;
            float ci = base + vk_h2((int)(rx * 2.0f), (int)ry, seed) * 1200.0f + d * 1800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, ring1 + ring2, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
