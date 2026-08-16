/* pattern_284 — FROST EDGES (field): frost creeping in from the four edges
 * of a window pane, feathery fronds reaching toward a clear black centre,
 * advancing and retreating with the cold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_284(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float reach = 0.30f + 0.08f * vk_sin(t * 0.001f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            /* distance to nearest edge and coordinate along it */
            float dl = u, dr = 1.333f - u, dt = v, db = 1.0f - v;
            float e = dl, along = v; int side = 0;
            if (dr < e) { e = dr; along = v; side = 1; }
            if (dt < e) { e = dt; along = u; side = 2; }
            if (db < e) { e = db; along = u; side = 3; }
            /* frond edge: reach modulated by noise along the edge; fern texture inside */
            float front = reach * (0.55f + 0.45f * vk_noise2(along * 6.0f, side * 3.0f + t * 0.0005f, seed))
                        + 0.05f * vk_noise2(along * 30.0f, side * 5.0f, seed ^ 3u);
            float inside = vk_sstep(front + 0.03f, front - 0.06f, e);
            /* dendrite texture: ridged noise oriented perpendicular to the edge */
            float n = vk_noise2(along * 40.0f + e * 8.0f, e * 30.0f + side * 7.0f, seed ^ 5u);
            float rid = 1.0f - vk_absf(n - 0.5f) * 3.0f;
            float n2 = vk_noise2(along * 12.0f, e * 12.0f + side, seed ^ 9u);
            float rid2 = 1.0f - vk_absf(n2 - 0.5f) * 3.0f;
            float tex = (rid > rid2 ? rid : rid2);
            tex = tex < 0.0f ? 0.0f : tex;
            float m = inside * (0.35f + 0.65f * vk_sstep(0.3f, 0.9f, tex)) * (0.6f + 0.4f * vk_sstep(front, 0.0f, e));
            float ci = base + e * 5000.0f + tex * 700.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1600.0f, tex, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
