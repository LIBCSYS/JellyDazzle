/* 026 Spoke Moire — nine spiral spokes against eleven, counter-rotating, their
 * angular beat sweeping brightness lobes around a full rainbow pinwheel while
 * concentric rings breathe outward.
 * Port of lab/patterns/026_spoke_moire/proto.py. Repaint pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <stdlib.h>

static int16_t s_sin026[4096];          /* Q14 sine, full turn = 4096 */
static int s_ready026;

/* Frame-invariant polar map: the radius and the folded atan2 index are pure
 * functions of (x,y), so the per-pixel sqrtf and the atan2 divide are hoisted
 * out of the frame loop. Expressions below are copied verbatim from the inner
 * loop they replace, so the image is unchanged. */
static float   *s_rtab026;
static uint16_t *s_atab026;
static int s_tw026, s_th026;
static float    s_frow026[4096];        /* fallback if the alloc ever fails */
static uint16_t s_arow026[4096];

static void s_init026(void) {
    if (s_ready026) return;
    for (int i = 0; i < 4096; i++)
        s_sin026[i] = (int16_t)lrintf(16383.0f *
            sinf((float)i * (float)(6.283185307179586 / 4096.0)));
    s_ready026 = 1;
}

/* Shima's atan2 approximation, max error ~0.0015 rad — no libm call per pixel */
static inline float s_atan2_026(float y, float x) {
    float ay = fabsf(y) + 1e-10f, r, a;
    if (x >= 0.0f) { r = (x - ay) / (x + ay); a = 0.1963f * r * r * r - 0.9817f * r + 0.7853982f; }
    else           { r = (x + ay) / (ay - x); a = 0.1963f * r * r * r - 0.9817f * r + 2.3561945f; }
    return (y < 0.0f) ? -a : a;
}

static inline uint32_t s_shade026(uint32_t c, int v8) {
    uint32_t r = (((c >> 16) & 255u) * (uint32_t)v8) >> 8;
    uint32_t g = (((c >> 8) & 255u) * (uint32_t)v8) >> 8;
    uint32_t b = ((c & 255u) * (uint32_t)v8) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static void s_polrow026(float *rr, uint16_t *aa, float dy, float cx, int w) {
    float qy = dy * dy;
    if (w > 4096) w = 4096;
    for (int x = 0; x < w; x++) {
        float dx = (float)x - cx;
        rr[x] = sqrtf(qy + dx * dx);
        aa[x] = (uint16_t)(((int)(s_atan2_026(dy, dx) * 651.8986f + 8192.5f))
                           & 4095);
    }
}

static void s_map026(int w, int h) {
    if (s_tw026 == w && s_th026 == h && s_rtab026) return;
    free(s_rtab026); free(s_atab026);
    s_rtab026 = (float *)malloc(sizeof(float) * (size_t)w * (size_t)h);
    s_atab026 = (uint16_t *)malloc(sizeof(uint16_t) * (size_t)w * (size_t)h);
    if (!s_rtab026 || !s_atab026) {
        free(s_rtab026); free(s_atab026);
        s_rtab026 = 0; s_atab026 = 0; s_tw026 = 0; s_th026 = 0; return;
    }
    float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    for (int y = 0; y < h; y++)
        s_polrow026(s_rtab026 + (long)y * w, s_atab026 + (long)y * w,
                    (float)y - cy, cx, w);
    s_tw026 = w; s_th026 = h;
}

void pattern_026(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init026();
    s_map026(w, h);
    const float sc = (float)w / 320.0f;
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    const float KT1 = 0.020f / sc * 651.8986f;   /* spiral twist, arm set 1 */
    const float KT2 = 0.016f / sc * 651.8986f;   /* spiral twist, arm set 2 */
    const float KRG = 0.160f / sc * 651.8986f;   /* radial ring frequency */
    const int p1 = (int)( 0.0080f * tt * 651.8986f);
    const int p2 = (int)( 0.0068f * tt * 651.8986f);
    const int pr = (int)( 0.0280f * tt * 651.8986f);
    const int drift = (int)(tt * 0.6f) + (int)(seed & 32767u);

    for (int y = 0; y < h; y++) {
        float dy = (float)y - cy;
        uint32_t *row = fb + (long)y * w;
        const float *rr; const uint16_t *aa;
        if (s_rtab026) { rr = s_rtab026 + (long)y * w;
                         aa = s_atab026 + (long)y * w; }
        else { s_polrow026(s_frow026, s_arow026, dy, cx, w);
               rr = s_frow026; aa = s_arow026; }
        for (int x = 0; x < w; x++) {
            float r = rr[x];
            int ai = aa[x];
            int ri1 = (int)(r * KT1);
            int ri2 = (int)(r * KT2);
            int g1 = s_sin026[( 9 * ai + ri1 + p1) & 4095];
            int g2 = s_sin026[(-11 * ai - ri2 - p2) & 4095];
            int ring = s_sin026[((int)(r * KRG) - pr) & 4095];
            int f = (g1 + g2) >> 1;                        /* Q14, -1..1 */
            /* val = 0.15 + 0.85 * clip((0.5+0.5f)(0.55+0.45 ring)) */
            int A = 8192 + (f >> 1);
            int B = 9011 + ((ring * 7373) >> 14);
            int P = (A * B) >> 14;
            if (P > 16383) P = 16383;
            int v = 2458 + ((P * 13926) >> 14);
            int idx = drift + ai * 8 + ((ring * 3277) >> 14);
            row[x] = s_shade026(pal[idx & JD_PAL_MASK], v >> 6);
        }
    }
}
