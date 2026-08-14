/* pattern_016 — shadebob rosette: one Lissajous shadebob whose additive trail
 * is folded six ways, so twelve synchronised bobs weave a glowing rosette with
 * white-hot knots where the path crosses itself.
 * Port of lab/patterns/016_shadebob_rosette/proto.py.
 * ACCUMULATOR: the trail field is cleared at sl<2 and built up from there. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define LW 320
#define LH 240
#define BR 22                       /* blob half-footprint (3.1 sigma)        */
#define BD (BR * 2 + 1)
#define TN 512                      /* tone LUT entries                       */
#define TSCALE 12.8f                /* field -> tone index                    */

static float acc[LW * LH];          /* persistent trail field                 */
static float blob[BD * BD];         /* gaussian stamp, sigma = 7              */
static int   tidx[TN];              /* tone -> palette offset                 */
static unsigned tval[TN];           /* tone -> brightness 0..256              */
static uint32_t lbuf[LW * LH];
static int ready = 0;
static int primed = 0;

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
    for (int j = -BR; j <= BR; j++)
        for (int i = -BR; i <= BR; i++)
            blob[(j + BR) * BD + (i + BR)] =
                0.105f * expf(-(float)(i * i + j * j) / (2.0f * 49.0f));
    for (int k = 0; k < TN; k++) {
        float f = (float)k / TSCALE;
        float g = tanhf(f);
        float hue = 0.78f - g * 0.26f;
        float val = powf(g, 0.75f);
        tidx[k] = (int)(hue * 32768.0f);
        tval[k] = (unsigned)(val * 256.0f);
    }
}

static void stamp(float cx, float cy) {
    int x0 = (int)(cx + 0.5f) - BR, y0 = (int)(cy + 0.5f) - BR;
    int sx = 0, sy = 0, ex = BD, ey = BD;
    if (x0 < 0) { sx = -x0; }
    if (y0 < 0) { sy = -y0; }
    if (x0 + BD > LW) ex = LW - x0;
    if (y0 + BD > LH) ey = LH - y0;
    for (int j = sy; j < ey; j++) {
        const float *s = blob + j * BD;
        float *d = acc + (long)(y0 + j) * LW + x0;
        for (int i = sx; i < ex; i++) d[i] += s[i];
    }
}

void pattern_016(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    if (!ready) { setup(); ready = 1; primed = 0; }
    if (sl < 2 || !primed) { memset(acc, 0, sizeof acc); primed = 1; }

    const float t = (float)frame;

    /* decay: trail history spans roughly 500 frames */
    for (int i = 0; i < LW * LH; i++) acc[i] *= 0.9940f;

    /* Lissajous bob, folded into the 6-fold wedge, stamped into all 12 images */
    const float bx = 112.0f * sinf(t * 0.0123f);
    const float by = 88.0f * sinf(t * 0.0177f + 1.1f);
    const float w2 = 6.2831853f / 6.0f;
    float ba = atan2f(by, bx);
    float br = sqrtf(bx * bx + by * by);
    float fa = fmodf(ba, w2);
    if (fa < 0.0f) fa += w2;
    if (fa > w2 * 0.5f) fa = w2 - fa;
    for (int k = 0; k < 6; k++) {
        float a0 = fa + (float)k * w2;
        float a1 = -fa + (float)k * w2;
        stamp(LW * 0.5f + br * cosf(a0), LH * 0.5f + br * sinf(a0));
        stamp(LW * 0.5f + br * cosf(a1), LH * 0.5f + br * sinf(a1));
    }

    /* resolve through the tone LUT */
    const int hueT = (int)(t * 0.0005f * 32768.0f) + (int)(seed & 2047u);
    for (int i = 0; i < LW * LH; i++) {
        int k = (int)(acc[i] * TSCALE);
        if (k < 0) k = 0; else if (k >= TN) k = TN - 1;
        lbuf[i] = shade(pal[(tidx[k] + hueT) & JD_PAL_MASK], tval[k]);
    }
    blit(fb, w, h);
}
