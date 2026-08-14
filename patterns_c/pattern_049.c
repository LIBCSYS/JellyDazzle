/* pattern_049 — Voronoi Breath
 * Port of lab/patterns/049_voronoi_breath/proto.py
 * Nineteen Worley seeds (origin + three counter-rotating, breathing rings of 6)
 * paint stained-glass cells: golden-ratio hue per owner, dark leading where the
 * two nearest seeds tie, soft glow at each seed. 320x240, bilinear upscale. */
#include "../jellydazzle.h"
#include <math.h>

#define P49_LW 320
#define P49_LH 240
#define P49_N  (P49_LW * P49_LH)
#define P49_NS 19
#define P49_TAU 6.28318531f

static uint32_t p49_low[P49_N];
static uint8_t  p49_wall[512];    /* smoothstep((F2-F1)/10) -> brightness */
static uint8_t  p49_glow[512];    /* seed glow on F1                      */
static int      p49_ready = 0;

static void p49_init(void) {
    for (int i = 0; i < 512; i++) {
        float s = (float)i / 10.0f / 4.0f;      /* i = 4*(F2-F1) */
        if (s > 1.0f) s = 1.0f;
        s = s * s * (3.0f - 2.0f * s);
        p49_wall[i] = (uint8_t)(255.0f * (0.22f + 0.78f * s) + 0.5f);
        float d = (float)i / 4.0f;              /* i = 4*F1 */
        p49_glow[i] = (uint8_t)(255.0f * (0.80f + 0.20f * expf(-(d * d) / 800.0f)) + 0.5f);
    }
    p49_ready = 1;
}

static inline uint32_t p49_lerp(uint32_t a, uint32_t b, unsigned t) {
    unsigned s = 256u - t;
    uint32_t rb = (((a & 0xFF00FFu) * s + (b & 0xFF00FFu) * t) >> 8) & 0xFF00FFu;
    uint32_t g  = (((a & 0x00FF00u) * s + (b & 0x00FF00u) * t) >> 8) & 0x00FF00u;
    return rb | g;
}

static void p49_blit(uint32_t *fb, int w, int h) {
    for (int y = 0; y < h; y++) {
        long fy = (((long)(2 * y + 1) * P49_LH) << 15) / h - (1L << 15);
        if (fy < 0) fy = 0;
        long fym = ((long)(P49_LH - 1)) << 16;
        if (fy > fym) fy = fym;
        int y0 = (int)(fy >> 16), y1 = y0 + 1 < P49_LH ? y0 + 1 : y0;
        unsigned wy = (unsigned)((fy >> 8) & 255);
        const uint32_t *r0 = p49_low + (long)y0 * P49_LW;
        const uint32_t *r1 = p49_low + (long)y1 * P49_LW;
        uint32_t *dst = fb + (long)y * w;
        for (int x = 0; x < w; x++) {
            long fx = (((long)(2 * x + 1) * P49_LW) << 15) / w - (1L << 15);
            if (fx < 0) fx = 0;
            long fxm = ((long)(P49_LW - 1)) << 16;
            if (fx > fxm) fx = fxm;
            int x0 = (int)(fx >> 16), x1 = x0 + 1 < P49_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((fx >> 8) & 255);
            uint32_t t = p49_lerp(r0[x0], r0[x1], wx);
            uint32_t b = p49_lerp(r1[x0], r1[x1], wx);
            dst[x] = 0xFF000000u | p49_lerp(t, b, wy);
        }
    }
}

void pattern_049(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!p49_ready) p49_init();
    float t = (float)frame;
    float sx[P49_NS], sy[P49_NS];
    uint32_t col[P49_NS];
    int drift = (int)(t * 0.0004f * 32768.0f) + (int)(seed & 32767u);

    sx[0] = P49_LW * 0.5f; sy[0] = P49_LH * 0.5f;
    static const float rings[3][5] = {
        {  45.0f, 6.0f,  0.0016f, 0.0040f, 0.0f },
        {  95.0f, 6.0f, -0.0011f, 0.0030f, 0.5f },
        { 145.0f, 6.0f,  0.0008f, 0.0050f, 1.0f },
    };
    int k = 1;
    for (int r = 0; r < 3; r++) {
        float rad = rings[r][0], spd = rings[r][2], wob = rings[r][3], ph = rings[r][4];
        int n = (int)rings[r][1];
        float rr = rad + 14.0f * sinf(t * wob + ph);
        for (int j = 0; j < n; j++) {
            float a = (float)j * P49_TAU / (float)n + t * spd + ph;
            sx[k] = P49_LW * 0.5f + rr * cosf(a);
            sy[k] = P49_LH * 0.5f + rr * sinf(a);
            k++;
        }
    }
    for (int i = 0; i < P49_NS; i++)
        col[i] = pal[(drift + i * 12515) & JD_PAL_MASK];

    for (int y = 0; y < P49_LH; y++) {
        uint32_t *out = p49_low + (long)y * P49_LW;
        for (int x = 0; x < P49_LW; x++) {
            float f1 = 1e18f, f2 = 1e18f;
            int owner = 0;
            for (int s = 0; s < P49_NS; s++) {
                float dx = (float)x - sx[s], dy = (float)y - sy[s];
                float d2 = dx * dx + dy * dy;
                if (d2 < f1) { f2 = f1; f1 = d2; owner = s; }
                else if (d2 < f2) { f2 = d2; }
            }
            float d1 = sqrtf(f1), d2r = sqrtf(f2);
            int wi = (int)((d2r - d1) * 4.0f);
            if (wi > 511) wi = 511; else if (wi < 0) wi = 0;
            int gi = (int)(d1 * 4.0f);
            if (gi > 511) gi = 511;
            unsigned g = ((unsigned)p49_wall[wi] * (unsigned)p49_glow[gi]) >> 8;
            uint32_t c = col[owner];
            out[x] = ((((((c >> 16) & 255) * g) >> 8) << 16)
                    | (((((c >> 8) & 255) * g) >> 8) << 8)
                    |  (((c & 255) * g) >> 8));
        }
    }
    p49_blit(fb, w, h);
}
