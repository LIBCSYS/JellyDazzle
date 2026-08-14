/* 027 Wedge Ripples — two wave sources ripple inside one kaleidoscope wedge and
 * the 8-fold fold mirrors them into sixteen phantom emitters; petal seams grow
 * lens-shaped interference eyes while the whole rosette revolves.
 * Port of lab/patterns/027_wedge_ripples/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

static int16_t s_sin027[4096];          /* Q14 sine, full turn = 4096 */
static int s_ready027;

/* Frame-invariant polar map: radius and the fixed-point atan2 index depend only
 * on (x,y), so hoist the per-pixel sqrtf + fdiv out of the frame loop. Built
 * once per resolution; the expressions are byte-identical to the inline ones
 * they replace, so the image does not change. */
static float   *s_rtab027;
static uint16_t *s_atab027;
static int s_tw027, s_th027;
static float    s_frow027[4096];        /* fallback if the alloc ever fails */
static uint16_t s_arow027[4096];

static void s_init027(void) {
    if (s_ready027) return;
    for (int i = 0; i < 4096; i++)
        s_sin027[i] = (int16_t)lrintf(16383.0f *
            sinf((float)i * (float)(6.283185307179586 / 4096.0)));
    s_ready027 = 1;
}

static inline float s_atan2_027(float y, float x) {
    float ay = fabsf(y) + 1e-10f, r, a;
    if (x >= 0.0f) { r = (x - ay) / (x + ay); a = 0.1963f * r * r * r - 0.9817f * r + 0.7853982f; }
    else           { r = (x + ay) / (ay - x); a = 0.1963f * r * r * r - 0.9817f * r + 2.3561945f; }
    return (y < 0.0f) ? -a : a;
}

static inline uint32_t s_shade027(uint32_t c, int v8) {
    uint32_t r = (((c >> 16) & 255u) * (uint32_t)v8) >> 8;
    uint32_t g = (((c >> 8) & 255u) * (uint32_t)v8) >> 8;
    uint32_t b = ((c & 255u) * (uint32_t)v8) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static void s_map027(int w, int h) {
    if (s_tw027 == w && s_th027 == h && s_rtab027 && s_atab027) return;
    free(s_rtab027); free(s_atab027);
    s_rtab027 = (float *)malloc(sizeof(float) * (size_t)w * (size_t)h);
    s_atab027 = (uint16_t *)malloc(sizeof(uint16_t) * (size_t)w * (size_t)h);
    if (!s_rtab027 || !s_atab027) {
        free(s_rtab027); free(s_atab027);
        s_rtab027 = 0; s_atab027 = 0; s_tw027 = 0; s_th027 = 0; return;
    }
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    for (int y = 0; y < h; y++) {
        float dy = (float)y - cy;
        float qy = dy * dy;
        float *rr = s_rtab027 + (long)y * w;
        uint16_t *aa = s_atab027 + (long)y * w;
        for (int x = 0; x < w; x++) {
            float dx = (float)x - cx;
            rr[x] = sqrtf(qy + dx * dx);
            aa[x] = (uint16_t)(int)(s_atan2_027(dy, dx) * 651.8986f + 8192.5f);
        }
    }
    s_tw027 = w; s_th027 = h;
}

void pattern_027(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    s_init027();
    s_map027(w, h);
    const float sc = (float)w / 320.0f;
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    const float tt = (float)frame;

    /* the two sources live in WEDGE space (lab px), off the fold axis */
    const float s1x = (62.0f + 26.0f * cosf(0.0045f * tt)) * sc;
    const float s1y = (26.0f + 16.0f * sinf(0.0058f * tt)) * sc;
    const float s2x = (18.0f + 10.0f * sinf(0.0039f * tt)) * sc;
    const float s2y = (62.0f + 20.0f * sinf(0.0033f * tt + 1.0f)) * sc;

    const float K1 = 0.300f / sc * 651.8986f;
    const float K2 = 0.260f / sc * 651.8986f;
    const float KE = 0.075f / sc * 651.8986f;   /* interference-eye envelope */
    const float KR = 0.012f / sc * 651.8986f;   /* radial hue wave */
    const int p1 = (int)(-0.026f * tt * 651.8986f);
    const int p2 = (int)( 0.020f * tt * 651.8986f);
    const int pr = (int)(-0.0065f * tt * 651.8986f);
    const int spin = (int)(0.0035f * tt * 651.8986f);
    const int drift = 6600 + (int)(tt * 0.5f) + (int)(seed & 2047u);

    for (int y = 0; y < h; y++) {
        float dy = (float)y - cy;
        float qy = dy * dy;
        uint32_t *row = fb + (long)y * w;
        const float *rr; const uint16_t *aa;
        if (s_rtab027) {
            rr = s_rtab027 + (long)y * w; aa = s_atab027 + (long)y * w;
        } else {                                  /* alloc failed: row scratch */
            int n = w > 4096 ? 4096 : w;
            for (int x = 0; x < n; x++) {
                float dx = (float)x - cx;
                s_frow027[x] = sqrtf(qy + dx * dx);
                s_arow027[x] = (uint16_t)(int)(s_atan2_027(dy, dx)
                                               * 651.8986f + 8192.5f);
            }
            rr = s_frow027; aa = s_arow027;
        }
        for (int x = 0; x < w; x++) {
            float r = rr[x];
            int ai = aa[x];
            int fa = ((ai + spin) & 511) - 256;      /* 8-fold fold, +-256 */
            if (fa < 0) fa = -fa;                    /* 0..256 == 0..pi/8 */
            float u = r * (float)s_sin027[(fa + 1024) & 4095] * (1.0f / 16384.0f);
            float v = r * (float)s_sin027[fa & 4095] * (1.0f / 16384.0f);
            float ax = u - s1x, ay2 = v - s1y;
            float bx = u - s2x, by = v - s2y;
            float d1 = sqrtf(ax * ax + ay2 * ay2);
            float d2 = sqrtf(bx * bx + by * by);
            int f = s_sin027[((int)(d1 * K1) + p1) & 4095]
                  + s_sin027[((int)(d2 * K2) + p2) & 4095];      /* Q14, -2..2 */
            int env = s_sin027[((int)((d1 - d2) * KE) + 1024) & 4095];
            int env2 = (env * env) >> 14;
            int rw = s_sin027[((int)(r * KR) + pr) & 4095];
            /* val = 0.20 + 0.60*(0.5 + 0.25 f) + 0.20 env^2 */
            int vv = 8192 + ((f * 2458) >> 14) + ((env2 * 3277) >> 14);
            if (vv < 0) vv = 0;
            if (vv > 16383) vv = 16383;
            int idx = drift + ((f * 1150) >> 14) + ((rw * 2600) >> 14);
            row[x] = s_shade027(pal[idx & JD_PAL_MASK], vv >> 6);
        }
    }
}
