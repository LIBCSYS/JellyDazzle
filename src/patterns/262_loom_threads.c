/* pattern_262 — LOOM THREADS (field): a loom mid-weave — taut warp threads
 * running top to bottom, weft bands packed at the bottom and a shuttle's
 * worth of open threads above, threads humming softly; black between the
 * threads.  Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_262(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float nw = vk_seedr(seed, 1, 22.0f, 32.0f);
    const float fell = 0.55f + 0.12f * vk_sin(t * 0.0012f);   /* woven edge */
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh;
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = (float)x / (float)sw * 1.333f;
            float hum = 0.006f * vk_sin(v * 12.0f + t * 0.01f + u * 20.0f) * vk_sstep(fell, fell - 0.3f, v);
            float wu = (u + hum) * nw;
            int iw = (int)floorf(wu);
            float fw = vk_absf(vk_fract(wu) - 0.5f);
            float warp = vk_sstep(0.22f, 0.10f, fw);
            float m, ci;
            if (v > fell) {
                /* woven cloth: weft bands with over/under.
                 * TEMPORAL REVIEW 2.4.0 (docs/review/04_pattern_temporal.md,
                 * F-262): the over/under used to be a hard parity of
                 * floor(wv), and every canvas row shares the same fractional
                 * phase of wv (bands are an exact 2 canvas rows), so when
                 * `fell` drifted across a half-band ALL rows flipped parity
                 * in the same frame — the whole cloth swapped its brick
                 * pattern (delta 14.9 every ~87 frames).  Over-ness is now
                 * a continuous cosine of wv (1 at band centres of one
                 * parity, 0 at the other, 0.5 on the dark band edges), and
                 * the per-band colour is a continuous function of wv too,
                 * so the cloth translates smoothly with `fell`.  Band count
                 * 40 -> 33: at the 80-row canvas 40 bands is EXACTLY two
                 * rows per band, so every row sampled the same band phase
                 * and the whole cloth's contrast pulsed in step as the
                 * phase slid; a non-integer rows-per-band decorrelates it. */
                float wv = (v - fell) * 33.0f;
                float fv = vk_absf(vk_fract(wv) - 0.5f);
                float weft = vk_sstep(0.5f, 0.15f, fv);
                float ov = 0.5f + 0.5f * vk_cos(wv * 3.14159265f) * ((iw & 1) ? -1.0f : 1.0f);
                float m_over  = warp * 0.9f + (1.0f - warp) * weft * 0.7f;
                float m_under = weft * 0.9f;
                m = m_under + (m_over - m_under) * ov;
                ci = base + 1800.0f * (1.0f - ov) + wv * 83.0f + t * 0.5f;
                m *= vk_sstep(fell, fell + 0.03f, v);
            } else {
                float sheen = 0.6f + 0.4f * vk_sin(v * 8.0f - t * 0.004f + iw * 0.3f);
                m = warp * sheen * vk_sstep(0.0f, 0.05f, v);
                ci = base + vk_h2(iw, 1, seed) * 600.0f + v * 1200.0f + t * 0.5f;
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci, ci + 1500.0f, fw * 4.0f, m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
