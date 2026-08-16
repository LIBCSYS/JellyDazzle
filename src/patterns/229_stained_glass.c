/* pattern_229 — STAINED GLASS (field): irregular leaded panes, each glowing
 * a colour of its own, some panes unlit; light through the panes wanes and
 * waxes slowly, and the lead lines are black.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_229(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float cells = vk_seedr(seed, 1, 6.0f, 9.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * cells * 0.75f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * cells;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float d1 = 9.0f, d2 = 9.0f, id = 0.0f, id2 = 0.0f;
            for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
                int cx = iu + i, cy = iv + j;
                float sx = cx + 0.15f + 0.7f * vk_h2(cx, cy, seed ^ 5u);
                float sy = cy + 0.15f + 0.7f * vk_h2(cx, cy, seed ^ 6u);
                float dx = u - sx, dy = v - sy;
                float d = vk_absf(dx) + vk_absf(dy) * 0.9f + 0.25f * sqrtf(dx * dx + dy * dy);   /* angular panes */
                if (d < d1) { d2 = d1; d1 = d; id2 = id; id = vk_h2(cx, cy, seed); }
                else if (d < d2) { d2 = d; id2 = vk_h2(cx, cy, seed); }
            }
            float e = d2 - d1;
            float lead = vk_sstep(0.03f, 0.10f, e);
            float lit = 0.5f + 0.5f * vk_sin(t * 0.0025f + id * 12.0f);
            float pane = vk_sstep(0.35f, 0.60f, id * 0.6f + lit * 0.4f);   /* some panes dark */
            /* glass texture: soft streaks */
            float streak = 0.8f + 0.2f * vk_sin(u * 9.0f + v * 6.0f + id * 30.0f + t * 0.002f);
            float m = lead * pane * streak;
            float ci = base + id * 3600.0f + t * 0.4f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 700.0f + id2 * 500.0f, d1 * 1.5f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
