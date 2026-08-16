/* pattern_050 — Turing Spots
 * Port of lab/patterns/050_turing_spots/proto.py
 * Two counter-rotating hexagonal lattices (big cells + freckles) thresholded by
 * a breathing smoothstep; a radial palette fan sweeps the rainbow across the
 * field. Repaint; the six lattice phases are walked incrementally along each
 * scanline (no per-pixel multiplies). 640x480 internal, bilinear upscale. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P50_LW 640
#define P50_LH 480
#define P50_N  (P50_LW * P50_LH)

static int16_t  p50_sin[4096];
static uint16_t p50_rad[P50_N];     /* radial palette offset */
static uint32_t p50_low[P50_N];
static int      p50_ready = 0;

static void p50_init(void) {
    for (int i = 0; i < 4096; i++)
        p50_sin[i] = (int16_t)lrintf(16383.0f * sinf((float)i * 6.28318531f / 4096.0f));
    for (int y = 0; y < P50_LH; y++) {
        float ly = ((float)y - P50_LH * 0.5f) * 0.5f;
        for (int x = 0; x < P50_LW; x++) {
            float lx = ((float)x - P50_LW * 0.5f) * 0.5f;
            float r = sqrtf(lx * lx + ly * ly);
            p50_rad[y * P50_LW + x] = (uint16_t)(r * 0.0009f * 32768.0f);
        }
    }
    p50_ready = 1;
}

static inline uint32_t p50_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p50_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P50_LH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fym = ((long)(P50_LH - 1)) << 16;
        if (fy > fym) fy = fym;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P50_LH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p50_low + (long)y0 * P50_LW;
        const uint32_t *r1 = p50_low + (long)y1 * P50_LW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P50_LW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxm = ((long)(P50_LW - 1)) << 16;
            if (fx > fxm) fx = fxm;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P50_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p50_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p50_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p50_lerp(t, b, wy);
        }
    }
}

void pattern_050(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p50_ready) p50_init();
    float t = (float)frame;
    const float U = 4096.0f / 6.28318531f;      /* rad -> angle units */
    const float S = 0.5f;                        /* internal px -> lab px */

    /* six phase channels: 3 per lattice, linear in (x,y), Q8 angle units */
    int cx[6], cy[6];
    {
        const float kk[2] = { 0.10f, 0.23f };
        const float ang[2] = { t * 0.00045f, -t * 0.0003f + 1.0f };
        for (int L = 0; L < 2; L++) {
            float k = kk[L], ca = cosf(ang[L]), sa = sinf(ang[L]);
            float axx =  ca,          axy = -sa;
            float bxx =  0.5f * ca + 0.8660254f * sa;
            float bxy = -0.5f * sa + 0.8660254f * ca;
            float dxx =  0.5f * ca - 0.8660254f * sa;
            float dxy = -0.5f * sa - 0.8660254f * ca;
            float m = k * U * S * 256.0f;
            cx[L * 3 + 0] = (int)lrintf(axx * m); cy[L * 3 + 0] = (int)lrintf(axy * m);
            cx[L * 3 + 1] = (int)lrintf(bxx * m); cy[L * 3 + 1] = (int)lrintf(bxy * m);
            cx[L * 3 + 2] = (int)lrintf(dxx * m); cy[L * 3 + 2] = (int)lrintf(dxy * m);
        }
    }
    /* breathing threshold -> two response tables */
    float thr = 0.15f * sinf(t * 0.0035f);
    static uint8_t r1t[512], r2t[512];
    for (int i = 0; i < 512; i++) {
        float f = ((float)(i * 64) - 16384.0f) / 16384.0f;
        float a = 2.5f * (f - thr) * 0.5f + 0.5f;
        float b = 2.5f * (f + thr) * 0.5f + 0.5f;
        if (a < 0.0f) a = 0.0f; else if (a > 1.0f) a = 1.0f;
        if (b < 0.0f) b = 0.0f; else if (b > 1.0f) b = 1.0f;
        r1t[i] = (uint8_t)(255.0f * a * a * (3.0f - 2.0f * a));
        r2t[i] = (uint8_t)(255.0f * b * b * (3.0f - 2.0f * b));
    }
    int drift = (int)(t * 0.00018f * 32768.0f) + (int)(seed & 32767u);
    const int hx = P50_LW / 2, hy = P50_LH / 2;

    for (int y = 0; y < P50_LH; y++) {
        int p[6];
        for (int j = 0; j < 6; j++)
            p[j] = cx[j] * (-hx) + cy[j] * (y - hy) + (1024 << 8);   /* +pi/2 => cos */
        const uint16_t *rad = p50_rad + (long)y * P50_LW;
        uint32_t *out = p50_low + (long)y * P50_LW;
        for (int x = 0; x < P50_LW; x++) {
            int f1 = (p50_sin[(p[0] >> 8) & 4095] + p50_sin[(p[1] >> 8) & 4095]
                    + p50_sin[(p[2] >> 8) & 4095]) / 3;
            int f2 = (p50_sin[(p[3] >> 8) & 4095] + p50_sin[(p[4] >> 8) & 4095]
                    + p50_sin[(p[5] >> 8) & 4095]) / 3;
            for (int j = 0; j < 6; j++) p[j] += cx[j];
            int i1 = (f1 + 16384) >> 6; if (i1 < 0) i1 = 0; else if (i1 > 511) i1 = 511;
            int i2 = (f2 + 16384) >> 6; if (i2 < 0) i2 = 0; else if (i2 > 511) i2 = 511;
            unsigned v1 = r1t[i1], v2 = r2t[i2];
            int idx = (drift + (int)(v1 * 24u) + (int)(v2 * 12u) + (int)rad[x]) & JD_PAL_MASK;
            uint32_t c = pal[idx];
            unsigned g = 178u + ((v1 * 77u) >> 8);
            out[x] = ((((((c >> 16) & 255) * g) >> 8) << 16)
                    | (((((c >> 8) & 255) * g) >> 8) << 8)
                    |  (((c & 255) * g) >> 8));
        }
    }
    p50_blit(fb, w, h);
}
