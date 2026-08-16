/* pattern_296 — TRUCHET RIBBONS (field): quarter-circle Truchet tiles
 * joining into winding ribbons, each ribbon a soft two-tone band with black
 * between; tiles flip one at a time on slow clocks.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_296(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 5.0f, 8.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * n * 0.75f + t * 0.0004f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * n;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float fu = vk_fract(u), fv = vk_fract(v);
            /* tile orientation: per-tile slow flip via smooth blend of two arcs */
            float ph = vk_h2(iu, iv, seed) * VK_TAU;
            float flip = vk_sstep(-0.3f, 0.3f, vk_sin(t * 0.0015f + ph));
            /* arcs A: centred at (0,0) and (1,1); arcs B: at (1,0) and (0,1) */
            float dA = vk_absf(sqrtf(fu * fu + fv * fv) - 0.5f);
            float dA2 = vk_absf(sqrtf((1 - fu) * (1 - fu) + (1 - fv) * (1 - fv)) - 0.5f);
            float dB = vk_absf(sqrtf((1 - fu) * (1 - fu) + fv * fv) - 0.5f);
            float dB2 = vk_absf(sqrtf(fu * fu + (1 - fv) * (1 - fv)) - 0.5f);
            float da = dA < dA2 ? dA : dA2, db = dB < dB2 ? dB : dB2;
            float d = da * (1.0f - flip) + db * flip;
            float wdt = 0.20f;
            float m = vk_sstep(wdt, wdt * 0.5f, d);
            float edge = 0.6f + 0.4f * vk_cos(d / wdt * 3.0f);
            float ci = base + vk_h2(iu, iv, seed ^ 3u) * 400.0f + (u + v) * 250.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, d / wdt, m * edge));
        }
    }
    vk_blit(&cv, fb, w, h);
}
