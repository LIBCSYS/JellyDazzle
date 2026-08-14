/* pattern_018 — kefrens spiral: Kefrens/Alcatraz bars with the scanline axis
 * swapped for radius. A never-cleared angular line buffer is stamped as the
 * radius grows, smearing the bar's wander into taffy tendrils, mirrored 4 ways.
 * Hue is frozen in at stamp time, so the burst reads as a peacock starburst.
 * Port of lab/patterns/018_kefrens_spiral/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

#define LW 480
#define LH 360
#define ABINS 360
#define RMAX 205

static uint32_t polar[LW * LH];      /* r*ABINS + bin, per lo-res pixel       */
static unsigned char radf[LW * LH];  /* radial falloff, 0..255                */
static unsigned char mv[RMAX * ABINS];
static unsigned char mh[RMAX * ABINS];
static unsigned pw[256];             /* v^0.85 -> 0..256                      */
static uint32_t lbuf[LW * LH];
static int ready = 0;

static uint32_t lerp2(uint32_t a, uint32_t b, unsigned f) {
    unsigned g = 256u - f;
    uint32_t rb = (uint32_t)((((uint64_t)(a & 0xFF00FFu)) * g +
                              ((uint64_t)(b & 0xFF00FFu)) * f) >> 8);
    uint32_t gg = ((((a >> 8) & 0xFFu) * g + (((b >> 8) & 0xFFu) * f)) >> 8);
    return (rb & 0xFF00FFu) | ((gg & 0xFFu) << 8) | 0xFF000000u;
}

static uint32_t shade(uint32_t c, unsigned v) {
    uint32_t rb = (uint32_t)((((uint64_t)(c & 0xFF00FFu)) * v) >> 8);
    uint32_t g = ((((c >> 8) & 0xFFu) * v) >> 8);
    return (rb & 0xFF00FFu) | ((g & 0xFFu) << 8) | 0xFF000000u;
}

static void blit(uint32_t *fb, int w, int h) {
    static int cw = -1;
    static int xi[4096];
    static unsigned char xf[4096];
    int wl = w > 4096 ? 4096 : w;
    if (w != cw) {
        for (int x = 0; x < wl; x++) {
            int64_t sx = (((int64_t)x * LW) << 16) / w;
            int x0 = (int)(sx >> 16);
            unsigned f = (unsigned)((sx >> 8) & 255);
            if (x0 >= LW - 1) { x0 = LW - 2; f = 255; }
            xi[x] = x0; xf[x] = (unsigned char)f;
        }
        cw = w;
    }
    for (int y = 0; y < h; y++) {
        int64_t sy = (((int64_t)y * LH) << 16) / h;
        int y0 = (int)(sy >> 16);
        unsigned fy = (unsigned)((sy >> 8) & 255);
        if (y0 >= LH - 1) { y0 = LH - 2; fy = 255; }
        const uint32_t *r0 = lbuf + y0 * LW, *r1 = r0 + LW;
        uint32_t *d = fb + (long)y * w;
        for (int x = 0; x < wl; x++) {
            int x0 = xi[x]; unsigned fx = xf[x];
            d[x] = lerp2(lerp2(r0[x0], r0[x0 + 1], fx),
                         lerp2(r1[x0], r1[x0 + 1], fx), fy);
        }
        for (int x = wl; x < w; x++) d[x] = d[wl - 1];
    }
}

static void setup(void) {
    for (int i = 0; i < 256; i++)
        pw[i] = (unsigned)(powf((float)i / 255.0f, 0.85f) * 256.0f);
    const float w2 = 6.2831853f / 4.0f;      /* 4-fold */
    const float hw = w2 * 0.5f;
    const float sc = 320.0f / (float)LW;
    for (int y = 0; y < LH; y++) {
        float dy = ((float)y - LH * 0.5f) * sc;
        for (int x = 0; x < LW; x++) {
            float dx = ((float)x - LW * 0.5f) * sc;
            float r = sqrtf(dx * dx + dy * dy);
            float a = atan2f(dy, dx);
            float fa = fmodf(a, w2);
            if (fa < 0.0f) fa += w2;
            if (fa > hw) fa = w2 - fa;
            int b = (int)((fa / hw) * (float)(ABINS - 1));
            if (b < 0) b = 0; else if (b >= ABINS) b = ABINS - 1;
            int ri = (int)r;
            if (ri < 0) ri = 0; else if (ri >= RMAX) ri = RMAX - 1;
            int i = y * LW + x;
            polar[i] = (uint32_t)(ri * ABINS + b);
            float rad = 1.0f - r / 230.0f;
            if (rad < 0.0f) rad = 0.0f;
            radf[i] = (unsigned char)((0.45f + 0.55f * rad) * 255.0f);
        }
    }
}

void pattern_018(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!ready) { setup(); ready = 1; }
    const float t = (float)frame;
    const float w2 = 6.2831853f / 4.0f;
    const float hw = w2 * 0.5f;

    /* --- run the Kefrens line buffer outward over radius --- */
    static float melt[ABINS];
    static unsigned char hbuf[ABINS];
    for (int b = 0; b < ABINS; b++) { melt[b] = 0.0f; hbuf[b] = 0; }

    const float p1 = t * 0.00045f, p2 = -t * 0.00028f, p3 = t * 0.00016f;
    for (int r = 0; r < RMAX; r++) {
        float th = 2.2f * sinf((float)r * 0.045f + p1)
                 + 1.4f * sinf((float)r * 0.023f + p2) + p3;
        float fth = fmodf(th, w2);
        if (fth < 0.0f) fth += w2;
        if (fth > hw) fth = w2 - fth;
        float cb = (fth / hw) * (float)(ABINS - 1);

        float hv = fmodf((float)r * 0.004f + t * 0.0004f, 1.0f);
        unsigned char hb = (unsigned char)(hv * 255.0f);

        int lo = (int)ceilf(cb - 14.6f), hi = (int)(cb + 14.6f);
        if (lo < 0) lo = 0;
        if (hi >= ABINS) hi = ABINS - 1;
        for (int b = 0; b < lo; b++) melt[b] *= 0.9865f;
        for (int b = hi + 1; b < ABINS; b++) melt[b] *= 0.9865f;
        for (int b = lo; b <= hi; b++) {
            float d = fabsf((float)b - cb);
            float bump = 1.0f - d / 15.0f;
            if (bump <= 0.03f) { melt[b] *= 0.9865f; continue; }
            if (bump > melt[b]) melt[b] = bump;
            if (bump > 0.55f) hbuf[b] = hb;
        }
        unsigned char *dv = mv + r * ABINS, *dh = mh + r * ABINS;
        for (int b = 0; b < ABINS; b++) {
            float m = melt[b];
            if (m > 1.0f) m = 1.0f;
            dv[b] = (unsigned char)(m * 255.0f);
            dh[b] = hbuf[b];
        }
    }

    /* --- resolve to screen through the polar fold map --- */
    const int hoff = (int)(seed & 0x7FFFu);
    for (int i = 0; i < LW * LH; i++) {
        uint32_t k = polar[i];
        unsigned v = (pw[mv[k]] * (unsigned)radf[i]) >> 8;
        v += 8;                                  /* faint floor, never black */
        if (v > 256u) v = 256u;
        uint32_t c = pal[(hoff + (int)mh[k] * 128) & JD_PAL_MASK];
        lbuf[i] = shade(c, v);
    }
    blit(fb, w, h);
}
