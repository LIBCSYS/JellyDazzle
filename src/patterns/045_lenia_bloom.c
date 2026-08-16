/* pattern_045 — Lenia Bloom
 * Port of lab/patterns/045_lenia_bloom/proto.py
 * 25 Gaussian-ring "orbium" cells (1 centre + rings of 6/6/12) orbiting and
 * breathing; their fields add and a tanh response fuses overlapping membranes
 * into amoebic super-cells. Rendered at 320x240 into a float field, colored
 * through the palette on a near-black ground, bilinearly upscaled. */
#include "../engine/jellydazzle.h"
#include <math.h>
#include <string.h>

#define P45_LW 320
#define P45_LH 240
#define P45_N  (P45_LW * P45_LH)
#define P45_TAU 6.28318531f

static float    p45_acc[P45_N];
static uint32_t p45_low[P45_N];
static float    p45_gauss[544];      /* exp(-z^2/2), z = i/64            */
static uint8_t  p45_resp[1025];      /* tanh(1.25*a) on a in 0..8, u8    */
static int      p45_ready = 0;

static void p45_init(void) {
    for (int i = 0; i < 544; i++) {
        float z = (float)i / 64.0f;
        p45_gauss[i] = expf(-0.5f * z * z);
    }
    for (int i = 0; i <= 1024; i++) {
        float a = (float)i * (8.0f / 1024.0f);
        p45_resp[i] = (uint8_t)(255.0f * tanhf(1.25f * a) + 0.5f);
    }
    p45_ready = 1;
}

/* add one Gaussian ring bump centred at (cx,cy), radius r0, width w */
static void p45_ring(float cx, float cy, float r0, float w, float amp) {
    float inv = 64.0f / w;
    float rmax = r0 + 4.2f * w;
    int x0 = (int)(cx - rmax), x1 = (int)(cx + rmax) + 1;
    int y0 = (int)(cy - rmax), y1 = (int)(cy + rmax) + 1;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > P45_LW) x1 = P45_LW; if (y1 > P45_LH) y1 = P45_LH;
    for (int y = y0; y < y1; y++) {
        float dy = (float)y - cy;
        float dy2 = dy * dy;
        float *row = p45_acc + (long)y * P45_LW;
        for (int x = x0; x < x1; x++) {
            float dx = (float)x - cx;
            float d = sqrtf(dx * dx + dy2);
            float z = (d - r0) * inv;
            int i = (int)(z < 0.0f ? -z : z);
            if (i < 544) row[x] += amp * p45_gauss[i];
        }
    }
}

static inline uint32_t p45_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p45_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P45_LH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fym = ((long)(P45_LH - 1)) << 16;
        if (fy > fym) fy = fym;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P45_LH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p45_low + (long)y0 * P45_LW;
        const uint32_t *r1 = p45_low + (long)y1 * P45_LW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P45_LW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxm = ((long)(P45_LW - 1)) << 16;
            if (fx > fxm) fx = fxm;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P45_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p45_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p45_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p45_lerp(t, b, wy);
        }
    }
}

void pattern_045(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p45_ready) p45_init();
    float t = (float)frame;
    const float cx = P45_LW * 0.5f, cy = P45_LH * 0.5f;
    memset(p45_acc, 0, sizeof p45_acc);

    /* central breathing cell */
    p45_ring(cx, cy, 26.0f + 9.0f * sinf(t * 0.004f), 8.0f, 1.0f);

    static const float ring_def[3][7] = {
        /* rad,  n,  spd,     wob,     ph,  cr,   cw  */
        {  62.0f, 6.0f,  0.0022f, 0.0045f, 0.0f, 16.0f, 6.0f },
        {  88.0f, 6.0f, -0.0013f, 0.0031f, 2.0f, 24.0f, 9.0f },
        { 128.0f, 12.0f, 0.0010f, 0.0038f, 0.9f, 17.0f, 6.5f },
    };
    for (int k = 0; k < 3; k++) {
        float rad = ring_def[k][0];
        int   n   = (int)ring_def[k][1];
        float spd = ring_def[k][2], wob = ring_def[k][3], ph = ring_def[k][4];
        float cr  = ring_def[k][5], cw = ring_def[k][6];
        float rr  = rad + 18.0f * sinf(t * wob + ph);
        for (int j = 0; j < n; j++) {
            float a = (float)j * P45_TAU / (float)n + t * spd + ph;
            p45_ring(cx + rr * cosf(a), cy + rr * sinf(a), cr, cw, 0.95f);
        }
    }
    /* faint connective halo */
    p45_ring(cx, cy, 96.0f + 14.0f * sinf(t * 0.0027f), 34.0f, 0.35f);

    int drift = (int)(t * 0.00028f * 32768.0f) + (int)(seed & 32767u);
    for (int i = 0; i < P45_N; i++) {
        float a = p45_acc[i];
        int q = (int)(a * 128.0f);                 /* 1024 entries over 0..8 */
        if (q > 1024) q = 1024; else if (q < 0) q = 0;
        unsigned L = p45_resp[q];                  /* 0..255 */
        uint32_t c = pal[(drift + (int)((L * 2600u) >> 8)) & JD_PAL_MASK];
        unsigned g = 12u + ((L * 243u) >> 8);      /* 0.05 + 0.95*L */
        unsigned hi = L > 185u ? ((L - 185u) * 3u) : 0u;   /* cream membranes */
        unsigned rr = ((((c >> 16) & 255) * g) >> 8) + hi;
        unsigned gg = ((((c >> 8) & 255) * g) >> 8) + hi;
        unsigned bb = (((c & 255) * g) >> 8) + hi;
        if (rr > 255) rr = 255;
        if (gg > 255) gg = 255;
        if (bb > 255) bb = 255;
        p45_low[i] = (rr << 16) | (gg << 8) | bb;
    }
    p45_blit(fb, w, h);
}
