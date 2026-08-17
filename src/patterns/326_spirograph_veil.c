/* pattern_326 — SPIROGRAPH VEIL (field): hypotrochoid curves layered into a
 * lacy rosette — many soft-edged loops of ribbon overlapping around the
 * centre, turning slowly; black in the openings.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
#define NSEG 1000
static float lx_[NSEG], ly_[NSEG];
void pattern_326(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 4, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    /* curve: (R-r)cos(th) + d cos((R-r)/r th), etc; parameters from seed */
    const float R = 1.0f, r = 0.28f + 0.14f * vk_seedf(seed, 1), d = 0.35f + 0.25f * vk_seedf(seed, 2) + 0.03f * vk_sin(t * 0.0012f);
    const float rot = t * 0.0004f;
    const float k = (R - r) / r;
    for (int i = 0; i < NSEG; i++) {
        float th = (float)i / NSEG * VK_TAU * 7.0f;
        float px = (R - r) * vk_cos(th) + d * vk_cos(k * th), py = (R - r) * vk_sin(th) - d * vk_sin(k * th);
        float c = vk_cos(rot), s = vk_sin(rot);
        lx_[i] = (px * c - py * s) * 0.65f; ly_[i] = (px * s + py * c) * 0.65f;
    }
    /* draw the curve into a coverage buffer with soft thickness: splat */
    static float acc[640 * 480];
    memset(acc, 0, sizeof(float) * (size_t)sw * sh);
    float thick = 8.5f * sw / 320.0f;
    for (int i = 0; i < NSEG; i++) {
        float x0 = (lx_[i] * 0.5f + 0.5f) * sw, y0 = (ly_[i] * 0.667f + 0.5f) * sh;
        int R0 = (int)thick + 1;
        for (int dy = -R0; dy <= R0; dy++) for (int dx = -R0; dx <= R0; dx++) {
            int px = (int)x0 + dx, py = (int)y0 + dy;
            if (px < 0 || py < 0 || px >= sw || py >= sh) continue;
            float ddx = px + 0.5f - x0, ddy = py + 0.5f - y0;
            float dd = sqrtf(ddx * ddx + ddy * ddy) / thick;
            if (dd > 1.0f) continue;
            float val = 1.0f - dd;
            float *a = &acc[py * sw + px];
            if (val > *a) *a = val;
        }
    }
    for (int y = 0; y < sh; y++) {
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float a = acc[y * sw + x];
            float u = ((float)x / (float)sw - 0.5f) * 2.0f, v = ((float)y / (float)sh - 0.5f) * 1.5f;
            float rr = sqrtf(u * u + v * v);
            float m = vk_sstep(0.0f, 0.5f, a) * (0.7f + 0.3f * a);
            float ci = base + rr * 2500.0f + a * 700.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, a, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
