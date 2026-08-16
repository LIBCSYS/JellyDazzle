/* 024 Munch Frost — rotating/breathing munching-squares XOR lattice rendered
 * as a rainbow Sierpinski frost.
 * Port of lab/patterns/024_munch_frost/proto.py. Repaint pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>

static uint8_t s_val024[128];           /* 0.18 + 0.82 u^0.75, 0..255 */
static int s_ready024;

static void s_init024(void) {
    if (s_ready024) return;
    for (int i = 0; i < 128; i++) {
        float u = (float)i / 127.0f;
        float v = 0.18f + 0.82f * powf(u, 0.75f);
        int b = (int)(v * 255.0f + 0.5f);
        s_val024[i] = (uint8_t)(b > 255 ? 255 : b);
    }
    s_ready024 = 1;
}

static inline uint32_t s_shade024(uint32_t c, int v8) {
    uint32_t r = (((c >> 16) & 255u) * (uint32_t)v8) >> 8;
    uint32_t g = (((c >> 8) & 255u) * (uint32_t)v8) >> 8;
    uint32_t b = ((c & 255u) * (uint32_t)v8) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

void pattern_024(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init024();
    const float sc = (float)w / 320.0f;
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    /* three slow clocks: frame rotation, zoom breath, band cycling */
    const float rot = 0.00040f * tt;
    const float zs  = 1.0f + 0.13f * sinf(0.00075f * tt);
    /* TEMPORAL REVIEW 2.4.0 (docs/review/04_pattern_temporal.md, F-024):
     * the band clock used to be an integer, (int)(0.17*t): every ~6 frames
     * every pixel's LUT index advanced by one whole entry — 256 palette
     * units and one step of the value curve, all at once — a periodic
     * colour STUTTER (delta 6.2 spikes every 6 frames on a 0.86 median).
     * Keep the integer part for the index and fold the fraction into the
     * LUT itself: entry k is lerp(lut[k], lut[k+1], frac), so each pixel's
     * colour now glides through the band boundary over the six frames. */
    const float tbf   = 0.17f * tt;
    const int   tband = (int)floorf(tbf);
    const int   tfrac = (int)((tbf - (float)tband) * 256.0f);   /* 0..255 */

    /* screen px -> lattice cell (one lab px = sc screen px) */
    const float cs = cosf(rot) * zs / sc;
    const float sn = sinf(rot) * zs / sc;

    /* 128-entry rainbow band LUT: full palette wheel + value curve baked in */
    uint32_t lut0[128], lut[128];
    int base = (int)(tt * 0.5f) + (int)(seed & 4095u);
    for (int i = 0; i < 128; i++)
        lut0[i] = s_shade024(pal[(base + i * 256) & JD_PAL_MASK], s_val024[i]);
    for (int i = 0; i < 128; i++) {
        uint32_t a = lut0[i], b = lut0[(i + 1) & 127];
        uint32_t r = (((a >> 16) & 255u) * (256u - (uint32_t)tfrac) + ((b >> 16) & 255u) * (uint32_t)tfrac) >> 8;
        uint32_t g = (((a >>  8) & 255u) * (256u - (uint32_t)tfrac) + ((b >>  8) & 255u) * (uint32_t)tfrac) >> 8;
        uint32_t bl = ((a & 255u) * (256u - (uint32_t)tfrac) + (b & 255u) * (uint32_t)tfrac) >> 8;
        lut[i] = 0xFF000000u | (r << 16) | (g << 8) | bl;
    }

    const int dX = (int)(cs * 65536.0f);
    const int dY = (int)(sn * 65536.0f);
    const int bias = 4096 << 16;

    for (int y = 0; y < h; y++) {
        float dy = (float)y - cy;
        int X = (int)((-cx * cs - dy * sn) * 65536.0f) + bias;
        int Y = (int)((-cx * sn + dy * cs) * 65536.0f) + bias;
        uint32_t *row = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            row[x] = lut[(((X >> 16) ^ (Y >> 16)) + tband) & 127];
            X += dX; Y += dY;
        }
    }
}
