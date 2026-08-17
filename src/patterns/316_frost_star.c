/* pattern_316 — FROST STAR (field): one great snow crystal filling the
 * frame — six dendritic arms with side branches at sixty degrees, plates
 * at the tips, growing and receding slowly; black between the arms.
 * 6-fold.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_316(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float grow = 0.85f + 0.15f * vk_sin(t * 0.001f);
    const float rot = t * 0.0002f;
    for (int y = 0; y < sh; y++) {
        float v = ((float)y / (float)sh - 0.5f) * 1.5f;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = ((float)x / (float)sw - 0.5f) * 2.0f;
            float r = sqrtf(u * u + v * v);
            float ang = atan2f(v, u) + rot;
            float a6 = vk_absf(vk_fract(ang * 6.0f / VK_TAU + 0.5f) - 0.5f) * (VK_TAU / 6.0f);   /* 0..30deg */
            float along = r * vk_cos(a6), across = r * vk_sin(a6);
            float L = 1.1f * grow;
            /* main arm */
            float arm = vk_sstep(0.07f + 0.04f * (1.0f - along / L), 0.0f, across) * vk_sstep(L, L - 0.05f, along);
            /* side branches at 60 degrees, every step, length falling toward the tip */
            float step = 0.11f;
            float k = floorf(along / step + 0.5f);
            float bx = along - k * step;
            float blen = 0.36f * (1.0f - k * step / L) * (0.7f + 0.3f * vk_h2((int)k, 0, seed));
            float pb = bx * 0.5f + across * 0.866f, qb = vk_absf(-bx * 0.866f + across * 0.5f);
            float br = vk_sstep(0.045f * (1.0f - pb / (blen + 1e-3f)) + 0.008f, 0.0f, qb) * vk_sstep(0.0f, 0.01f, pb) * vk_sstep(blen, blen * 0.7f, pb) * vk_sstep(0.05f, 0.1f, k * step) * (k * step < L - 0.05f ? 1.0f : 0.0f);
            /* sub-branches on the branches */
            float k2 = floorf(pb / 0.06f + 0.5f);
            float bx2 = pb - k2 * 0.06f;
            float blen2 = 0.11f * (1.0f - pb / (blen + 1e-3f));
            float pb2 = bx2 * 0.5f + qb * 0.866f, qb2 = vk_absf(-bx2 * 0.866f + qb * 0.5f);
            float br2 = vk_sstep(0.02f, 0.0f, qb2) * vk_sstep(0.0f, 0.005f, pb2) * vk_sstep(blen2, blen2 * 0.6f, pb2) * vk_sstep(0.03f, 0.06f, pb) * (pb < blen ? 1.0f : 0.0f);
            /* central hexagonal plate */
            float hexd = vk_absf(u) * 0.866f + vk_absf(v) * 0.5f; if (vk_absf(v) > hexd) hexd = vk_absf(v);
            float plate = vk_sstep(0.16f, 0.13f, hexd) * 0.7f + vk_sstep(0.02f, 0.0f, vk_absf(hexd - 0.15f)) * 0.5f;
            /* faint hexagonal plate glowing behind the dendrites, ridged toward the arms */
            float hex2 = vk_absf(along) ; (void)hex2;
            float plate2 = vk_sstep(1.05f * grow, 0.6f * grow, hexd / 0.866f) * (0.30f + 0.18f * vk_sstep(0.25f, 0.0f, across)) * vk_sstep(0.16f, 0.22f, hexd);
            float m = arm; m = m > br ? m : br; m = m > plate2 ? m : plate2; m = m > br2 * 0.8f ? m : br2 * 0.8f; m = m > plate ? m : plate;
            m *= 0.75f + 0.25f * vk_sin(r * 15.0f - t * 0.004f);
            float ci = base + r * 2600.0f + across * 3000.0f + t * 0.5f;
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, along / L, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
