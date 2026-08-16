/* pattern_329 — FERN WALL (field): fern fronds overlapping across the
 * frame, each a curved rachis with rows of rounded leaflets, swaying; black
 * between the leaflets.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NFR 14
void pattern_329(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 5, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    float ox[NFR], oy[NFR], oa[NFR], ok[NFR], ol[NFR], oc[NFR], os_[NFR];
    for (int i = 0; i < NFR; i++) {
        ox[i] = 0.1f + 1.1f * vk_seedf(seed, i);
        oy[i] = 1.1f + 0.1f * vk_seedf(seed, i + 10);
        oa[i] = -1.5708f + (vk_seedf(seed, i + 20) - 0.5f) * 1.6f + 0.03f * vk_sin(t * 0.002f + i);
        ok[i] = (vk_seedf(seed, i + 30) - 0.5f) * 1.2f;
        ol[i] = 0.7f + 0.5f * vk_seedf(seed, i + 40);
        oc[i] = vk_cos(oa[i]); os_[i] = vk_sin(oa[i]);
    }
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float m = 0.0f, ci = base;
            for (int i = 0; i < NFR; i++) {
                float dx = u - ox[i], dy = v - oy[i];
                if (dy > 0.05f) continue;
                float a = dx * oc[i] + dy * os_[i], b = -dx * os_[i] + dy * oc[i];
                if (a < 0.0f || a > ol[i]) continue;
                b -= ok[i] * a * a * 0.5f + 0.02f * vk_sin(a * 8.0f + t * 0.004f + i);
                float ab = vk_absf(b);
                float rach = vk_sstep(0.012f, 0.004f, ab);
                /* leaflets: ellipses along a, both sides, shrinking to the tip */
                float step = 0.08f;
                float k = floorf(a / step + 0.5f);
                float la = a - k * step;
                float pos = k * step / ol[i];
                float len = 0.34f * (1.0f - pos * 0.85f) * vk_sstep(0.0f, 0.15f, pos);
                float lw = 0.07f * (1.0f - pos * 0.6f);
                /* leaflet axis at ~55 deg forward */
                float pa = la * 0.57f + ab * 0.82f, pb = -la * 0.82f + ab * 0.57f;
                float ex = (pa - len * 0.5f) / (len * 0.5f + 1e-3f), ey = pb / (lw + 1e-3f);
                float leaflet = vk_sstep(1.0f, 0.75f, ex * ex + ey * ey) * vk_sstep(0.0f, 0.02f, pa);
                float mid = 0.8f + 0.2f * vk_sstep(0.3f, 0.0f, vk_absf(ey));
                float val = rach > leaflet * mid ? rach : leaflet * mid;
                val *= 0.75f + 0.25f * vk_sin(a * 5.0f + t * 0.003f + i);
                if (val > m) { m = val; ci = base + i * 350.0f + pos * 1500.0f + pa * 1500.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(u * 3.0f + v * 2.0f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
