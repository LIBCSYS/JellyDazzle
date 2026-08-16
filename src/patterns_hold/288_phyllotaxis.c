/* pattern_288 — PHYLLOTAXIS (field): a sunflower head — florets on the
 * golden-angle spiral, each a soft disc that grows toward the rim, the head
 * turning imperceptibly and breathing; black between florets.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NPT 470
static float px_[NPT], py_[NPT], pr_[NPT];
void pattern_288(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float rot = t * 0.0002f;
    const float breathe = 1.0f + 0.04f * vk_sin(t * 0.003f);
    for (int i = 0; i < NPT; i++) {
        float r = 0.045f * sqrtf((float)i) * breathe;
        float a = (float)i * 2.39996323f + rot;
        px_[i] = r * vk_cos(a); py_[i] = r * vk_sin(a);
        pr_[i] = 0.02f + 0.024f * sqrtf(r);
    }
    /* grid bin the points for speed: 16x12 buckets */
    static int bucket[16 * 12][40]; static int bcount[16 * 12];
    for (int b = 0; b < 16 * 12; b++) bcount[b] = 0;
    for (int i = 0; i < NPT; i++) {
        int bx = (int)((px_[i] + 1.0f) * 8.0f), by = (int)((py_[i] + 0.75f) * 8.0f);
        for (int j = -1; j <= 1; j++) for (int k = -1; k <= 1; k++) {
            int cx = bx + k, cy = by + j;
            if (cx < 0 || cx >= 16 || cy < 0 || cy >= 12) continue;
            int b = cy * 16 + cx;
            if (bcount[b] < 40) bucket[b][bcount[b]++] = i;
        }
    }
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            int bx = (int)((u + 1.0f) * 8.0f), by = (int)((v + 0.75f) * 8.0f);
            if (bx < 0) bx = 0; if (bx > 15) bx = 15; if (by < 0) by = 0; if (by > 11) by = 11;
            int b = by * 16 + bx;
            float m = 0.0f, ci = base;
            for (int q = 0; q < bcount[b]; q++) {
                int i = bucket[b][q];
                float dx = u - px_[i], dy = v - py_[i];
                float d = sqrtf(dx * dx + dy * dy) / pr_[i];
                if (d > 1.0f) continue;
                float val = vk_sstep(1.0f, 0.6f, d) * (0.6f + 0.4f * (1.0f - d));
                if (val > m) { m = val; ci = base + sqrtf((float)i) * 120.0f + d * 600.0f; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1500.0f, 0.5f + 0.5f * vk_sin(u * 3.0f + v * 2.0f + t * 0.002f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
