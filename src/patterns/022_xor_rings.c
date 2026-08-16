/* 022 XOR Rings — hard-fringe XOR moire of two drifting ring fields.
 * Port of lab/patterns/022_xor_rings/proto.py. Repaint pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>

void pattern_022(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    const float sc = (float)w / 320.0f;
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    const float a1 = 0.0022f * tt;
    const float a2 = -0.0019f * tt + 1.7f;
    const float x1 = cx + 55.0f * sc * cosf(a1);
    const float y1 = cy + 40.0f * sc * sinf(1.3f * a1);
    const float x2 = cx - 55.0f * sc * cosf(a2);
    const float y2 = cy - 40.0f * sc * sinf(a2);

    const float kd = 1.6f / sc;               /* ring density per screen px */
    const float p1 = 0.085f * tt;
    const float p2 = -0.068f * tt;

    /* 64-entry fringe LUT: indigo floor -> gold peaks along the ramp,
     * slow global drift; one palette read per band. */
    uint32_t lut[64];
    int base = (int)(tt * 0.6f) + (int)(seed & 511u);
    for (int i = 0; i < 64; i++) {
        int idx = base + i * 154;             /* 0..~9800 dark->bright arc */
        lut[i] = pal[idx & JD_PAL_MASK];
    }

    for (int y = 0; y < h; y++) {
        float dy1 = (float)y - y1, dy2 = (float)y - y2;
        float qy1 = dy1 * dy1, qy2 = dy2 * dy2;
        uint32_t *row = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            float dx1 = (float)x - x1, dx2 = (float)x - x2;
            int d1 = (int)(sqrtf(qy1 + dx1 * dx1) * kd + p1);
            int d2 = (int)(sqrtf(qy2 + dx2 * dx2) * kd + p2);
            row[x] = lut[(d1 ^ d2) & 63];
        }
    }
}
