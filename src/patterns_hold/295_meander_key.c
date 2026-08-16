/* pattern_295 — MEANDER KEY (field): a Greek-key meander running in bands
 * across the frame, the path drawn as a thick soft line with black between,
 * bands sliding against one another.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_295(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nb = 5.0f;
    /* meander as a 4x4 cell bitmap tile (1 = line) */
    static const unsigned char tile[4][4] = { {1,1,1,1}, {0,0,0,1}, {1,1,0,1}, {1,0,0,0} };
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * nb;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            int ib = (int)floorf(v);
            float fv = vk_fract(v);
            float u = (float)x / (float)sw * 1.333f * nb + t * 0.0012f * ((ib & 1) ? 1.0f : -1.0f) + 0.04f * vk_sin(v * 3.0f + t * 0.002f);
            /* band occupies 0.1..0.9 of its row: 4 cells tall */
            float cy = (fv - 0.1f) / 0.8f * 4.0f, cx = u * 4.0f;
            float m = 0.0f;
            if (cy >= 0.0f && cy < 4.0f) {
                int ix = (int)floorf(cx), iy = (int)floorf(cy);
                int mx = ((ix % 4) + 4) % 4, my = iy;
                float lit = tile[my][mx] ? 1.0f : 0.0f;
                /* soften: sample neighbours for anti-alias-ish softness */
                float fx = vk_fract(cx) - 0.5f, fy = vk_fract(cy) - 0.5f;
                int nx = ((ix + (fx > 0 ? 1 : -1)) % 4 + 4) % 4, ny = iy + (fy > 0 ? 1 : -1);
                float lx = (ny >= 0 && ny < 4) ? (tile[my][nx] ? 1.0f : 0.0f) : lit;
                float ly = (ny >= 0 && ny < 4) ? (tile[ny][mx] ? 1.0f : 0.0f) : 0.0f;
                float wx = vk_sstep(0.3f, 0.5f, vk_absf(fx)), wy = vk_sstep(0.3f, 0.5f, vk_absf(fy));
                m = lit * (1.0f - wx * 0.5f) + lx * wx * 0.5f;
                m = m * (1.0f - wy * 0.5f) + ly * wy * 0.5f;
                m *= 0.8f + 0.2f * vk_sin(cx * 0.5f + t * 0.003f);
            }
            float ci = base + vk_h2(ib, 0, seed) * 2000.0f + fv * 600.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1400.0f, fv, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
