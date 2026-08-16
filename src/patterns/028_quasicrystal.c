/* 028 Quasicrystal — seven plane waves at seventh-turn angles superposing into
 * an aperiodic seven-fold rosette lattice that assembles and dissolves.
 * Port of lab/patterns/028_quasicrystal/proto.py. Repaint pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>

static int16_t s_sin028[4096];          /* Q14 sine, full turn = 4096 */
static int16_t s_tanh028[512];          /* tanh(2.6 f), f in -1..1, Q14 */
static int s_ready028;

static void s_init028(void) {
    if (s_ready028) return;
    for (int i = 0; i < 4096; i++)
        s_sin028[i] = (int16_t)lrintf(16383.0f *
            sinf((float)i * (float)(6.283185307179586 / 4096.0)));
    for (int i = 0; i < 512; i++) {
        float f = ((float)i - 255.5f) * (1.0f / 255.5f);
        s_tanh028[i] = (int16_t)lrintf(16383.0f * tanhf(2.6f * f));
    }
    s_ready028 = 1;
}

static inline uint32_t s_shade028(uint32_t c, int v8) {
    uint32_t r = (((c >> 16) & 255u) * (uint32_t)v8) >> 8;
    uint32_t g = (((c >> 8) & 255u) * (uint32_t)v8) >> 8;
    uint32_t b = ((c & 255u) * (uint32_t)v8) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

void pattern_028(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init028();
    const float sc = (float)w / 320.0f;
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    const float kk = 0.30f / sc * 651.8986f;     /* k=0.30 rad/labpx -> idx/px */
    const float rot = 0.00085f * tt;             /* whole wave-star rotation */
    const float drp = 0.019f * tt * 651.8986f;   /* counter-drifting phases */

    /* per-wave: x/y index steps (Q6) and row-start constant */
    int stx[7], sty[7], ph[7];
    for (int i = 0; i < 7; i++) {
        float th = rot + (float)i * 0.4487990f;      /* pi/7 */
        stx[i] = (int)(kk * cosf(th) * 64.0f);
        sty[i] = (int)(kk * sinf(th) * 64.0f);
        ph[i] = (int)(((i & 1) ? -drp : drp) * 64.0f) + (1024 << 6); /* cos */
    }
    const int drift = 4200 + (int)(tt * 0.55f) + (int)(seed & 8191u);

    for (int y = 0; y < h; y++) {
        float dy = (float)y - cy;
        int acc[7];
        for (int i = 0; i < 7; i++)
            acc[i] = (int)((-cx * (float)stx[i] / 64.0f + dy * (float)sty[i] / 64.0f) * 64.0f)
                   + ph[i];
        uint32_t *row = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            int s = s_sin028[(acc[0] >> 6) & 4095] + s_sin028[(acc[1] >> 6) & 4095]
                  + s_sin028[(acc[2] >> 6) & 4095] + s_sin028[(acc[3] >> 6) & 4095]
                  + s_sin028[(acc[4] >> 6) & 4095] + s_sin028[(acc[5] >> 6) & 4095]
                  + s_sin028[(acc[6] >> 6) & 4095];
            acc[0] += stx[0]; acc[1] += stx[1]; acc[2] += stx[2]; acc[3] += stx[3];
            acc[4] += stx[4]; acc[5] += stx[5]; acc[6] += stx[6];
            /* s in -114688..114688 -> f in -16384..16384 -> tanh LUT slot */
            int ti = ((s * 2341) >> 19) + 256;        /* s/7/32768*512 */
            if (ti < 0) ti = 0;
            if (ti > 511) ti = 511;
            int g = s_tanh028[ti];                    /* Q14, -1..1 */
            int v = 2294 + ((g + 16384) * 7045 >> 14);   /* 0.14 + 0.86(0.5+g/2) */
            if (v > 16383) v = 16383;
            int idx = drift + ((g * 3600) >> 14);
            row[x] = s_shade028(pal[idx & JD_PAL_MASK], v >> 6);
        }
    }
}
