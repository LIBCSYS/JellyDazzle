/* pattern_267 — DRAGON SCALES (field): big keeled scales in mirrored rows
 * down a spine, each scale two-toned with a ridge, flexing as the beast
 * breathes; deep black shadow under every scale.  Mirror symmetry.
 * Repaint. */
#include "_veilkit.h"
static vk_canvas cv;
void pattern_267(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl; vk_init();
    int sw, sh; vk_size(w, h, 3, &sw, &sh);
    if (!vk_canvas_prep(&cv, sw, sh)) return;
    const float t = (float)frame;
    const float base = vk_base(pal, seed, 4000);
    const float n = vk_seedr(seed, 1, 4.0f, 6.0f);
    for (int y = 0; y < sh; y++) {
        float v = (float)y / (float)sh * n * 0.75f + 0.15f * vk_sin(t * 0.002f);
        uint8_t *row = cv.img + (size_t)y * sw * 3;
        for (int x = 0; x < sw; x++) {
            float u = vk_absf((float)x / (float)sw - 0.5f) * 2.0f * n * 0.6f;
            /* rows offset half a scale; scales tilt outward from the spine */
            float m = 0.0f, ci = base;
            for (int j = 0; j <= 1; j++) {
                float ry = floorf(v) - j;
                float ox = ((int)ry & 1) ? 0.5f : 0.0f;
                float cxr = floorf(u - ox + 0.5f) + ox;
                float dx = u - cxr, dy = v - ry;
                if (dy < 0.0f || dy > 1.4f) continue;
                /* pointed scale: width shrinks toward the tip (dy large).
                 * TEMPORAL REVIEW 2.4.0 (docs/review/04_pattern_temporal.md,
                 * F-267): the base used to hard-cut at dy<0.05 at full
                 * strength; the row drift moves that edge less than one
                 * canvas row per ~150 frames, so a whole row of scales
                 * flipped on at once whenever the edge crossed a row
                 * (delta 0.74 pops on a 0.012 median — the only motion the
                 * pattern has).  Fade the base in over 0..0.15 instead. */
                float wdt = 0.40f * (1.0f - vk_sstep(0.4f, 1.4f, dy) * 0.95f);
                float inside = vk_sstep(wdt, wdt * 0.8f, vk_absf(dx)) * vk_sstep(1.4f, 1.3f, dy)
                             * vk_sstep(0.0f, 0.15f, dy);
                float keel = 0.65f + 0.35f * vk_sstep(0.12f, 0.0f, vk_absf(dx));
                float shade = 0.5f + 0.5f * (dy / 1.4f);
                float val = inside * keel * shade;
                if (val > m) { m = val; ci = base + (dx > 0.0f ? 1600.0f : 0.0f) + dy * 900.0f + vk_h2((int)cxr, (int)ry, seed) * 400.0f; break; }
            }
            vk_putp(row + x * 3, vk_pc2(pal, ci + t * 0.5f, ci + 1400.0f, 0.5f + 0.5f * vk_sin(v * 2.0f - t * 0.003f), m));
        }
    }
    vk_blit(&cv, fb, w, h);
}
