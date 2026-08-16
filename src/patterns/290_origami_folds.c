/* pattern_290 — ORIGAMI FOLDS (field): a sheet folded into a field of
 * triangular facets, light raking across so facets glow or fall dark, black
 * crease lines between; the folds flex.  4-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_290(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 4.0f, 6.0f);
    const float lx = vk_cos(t * 0.0015f), ly = vk_sin(t * 0.0011f);
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f * n;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f * n;
            int iu = (int)floorf(u), iv = (int)floorf(v);
            float fu = vk_fract(u), fv = vk_fract(v);
            /* square split into 4 triangles by its diagonals */
            int tri = fu > fv ? (fu + fv > 1.0f ? 0 : 3) : (fu + fv > 1.0f ? 1 : 2);
            /* facet normal: per square a random tilt, per triangle a fixed direction */
            float tx = (tri == 0 ? 1.0f : tri == 2 ? -1.0f : 0.0f), ty = (tri == 1 ? 1.0f : tri == 3 ? -1.0f : 0.0f);
            float jit = vk_h2(iu, iv, seed) - 0.5f;
            float lit = 0.5f + 0.42f * (tx * lx + ty * ly) + 0.25f * jit * vk_sin(t * 0.002f + jit * 9.0f);
            /* creases: distance to diagonals and edges */
            float d1 = vk_absf(fu - fv), d2 = vk_absf(fu + fv - 1.0f);
            float d3 = vk_absf(fu - 0.5f) > vk_absf(fv - 0.5f) ? 0.5f - vk_absf(fu - 0.5f) : 0.5f - vk_absf(fv - 0.5f);
            float crease = vk_sstep(0.0f, 0.05f, d1) * vk_sstep(0.0f, 0.05f, d2) * vk_sstep(0.0f, 0.04f, d3);
            float m = vk_sstep(0.25f, 0.65f, lit) * crease;
            float ci = base + vk_h2(iu, iv, seed ^ 3u) * 1000.0f + lit * 1800.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, lit, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
