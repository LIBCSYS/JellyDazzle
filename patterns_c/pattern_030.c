/* 030 Octa Facets — two trains of concentric octagon rings glide from wandering
 * centers and interfere into a faceted crystal lattice with bright ridge lines.
 * Port of lab/patterns/030_octa_facets/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

static uint8_t s_val030[257];           /* 0.14 + 0.86 ridge^1.3, 0..255 */
static int s_ready030;

static void s_init030(void) {
    if (s_ready030) return;
    for (int i = 0; i <= 256; i++) {
        float u = (float)i / 256.0f;
        int b = (int)((0.14f + 0.86f * powf(u, 1.3f)) * 255.0f + 0.5f);
        s_val030[i] = (uint8_t)(b > 255 ? 255 : b);
    }
    s_ready030 = 1;
}

static inline uint32_t s_shade030(uint32_t c, int v8) {
    uint32_t r = (((c >> 16) & 255u) * (uint32_t)v8) >> 8;
    uint32_t g = (((c >> 8) & 255u) * (uint32_t)v8) >> 8;
    uint32_t b = ((c & 255u) * (uint32_t)v8) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

/* octagonal norm: max(|dx|, |dy|, (|dx|+|dy|)/sqrt2) — straight jewel edges */
static inline int s_octd030(int dx, int dy) {
    int ax = dx < 0 ? -dx : dx;
    int ay = dy < 0 ? -dy : dy;
    int m = ax > ay ? ax : ay;
    int d = ((ax + ay) * 181) >> 8;
    return m > d ? m : d;
}

void pattern_030(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init030();
    const float sc = (float)w / 320.0f;
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    const float a = 0.0035f * tt;
    const int x1 = (int)(cx + 58.0f * sc * cosf(a));
    const int y1 = (int)(cy + 40.0f * sc * sinf(0.8f * a));
    const int x2 = (int)(cx - 58.0f * sc * cosf(0.9f * a));
    const int y2 = (int)(cy - 40.0f * sc * sinf(a + 1.3f));

    /* 0.055 rad/labpx; triangle period 2.0 == 4096 fixed units */
    const int KD = (int)(0.055f / sc * 2048.0f * 256.0f);   /* Q8 per screen px */
    const int q1 = (int)(-0.0060f * tt * 2048.0f) + (1 << 22);
    const int q2 = (int)( 0.0048f * tt * 2048.0f) + (1 << 22);
    const int drift = 19600 + (int)(tt * 0.5f) + (int)(seed & 4095u);

    for (int y = 0; y < h; y++) {
        int dy1 = y - y1, dy2 = y - y2;
        uint32_t *row = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int z1 = (((s_octd030(x - x1, dy1) * KD) >> 8) + q1) & 4095;
            int z2 = (((s_octd030(x - x2, dy2) * KD) >> 8) + q2) & 4095;
            int r1 = z1 - 2048; if (r1 < 0) r1 = -r1;      /* tri(), 0..2048 */
            int r2 = z2 - 2048; if (r2 < 0) r2 = -r2;
            int f = r1 + r2;                               /* 0..4096 == 0..2 */
            int ridge = f - 2048; if (ridge < 0) ridge = -ridge;   /* 0..2048 */
            int idx = drift + (((f - 2048) * 2500) >> 11);
            row[x] = s_shade030(pal[idx & JD_PAL_MASK], s_val030[ridge >> 3]);
        }
    }
}
