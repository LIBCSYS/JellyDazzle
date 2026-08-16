/* pattern_279 — CELL KALEIDO (field): living cells folded through a
 * five-fold mirror — soft cell bodies with glowing nuclei, dark walls, the
 * whole colony turning slowly.  5-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_279(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u0 = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u0 * u0 + v * v);
            float ang = atan2f(v, u0) + t * 0.0003f;
            float wa = VK_TAU / 10.0f;
            float fa = fmodf(ang, 2.0f * wa); if (fa < 0.0f) fa += 2.0f * wa;
            if (fa > wa) fa = 2.0f * wa - fa;
            float u = r * vk_cos(fa) * 5.0f + 0.5f, vv = r * vk_sin(fa) * 5.0f + 0.5f;
            int iu = (int)floorf(u), iv = (int)floorf(vv);
            float d1 = 9.0f, d2 = 9.0f, id = 0.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float ph = vk_h2(cx, cy, seed) * VK_TAU;
                float sx = cx + 0.5f + 0.3f * vk_sin(t * 0.003f + ph), sy = cy + 0.5f + 0.3f * vk_cos(t * 0.0025f + ph * 1.3f);
                float dx = u - sx, dy = vv - sy, d = dx * dx + dy * dy;
                if (d < d1) { d2 = d1; d1 = d; id = vk_h2(cx, cy, seed ^ 7u); }
                else if (d < d2) d2 = d;
            }
            float e = sqrtf(d2) - sqrtf(d1);
            float wall = vk_sstep(0.06f, 0.22f, e);
            float glow = 1.0f - vk_sstep(0.0f, 0.6f, sqrtf(d1));
            float alive = vk_sstep(0.25f, 0.45f, id);
            float m = wall * alive * (0.4f + 0.6f * glow) * vk_sstep(1.35f, 1.0f, r);
            float ci = base + id * 2000.0f + glow * 1000.0f + r * 500.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, glow, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
