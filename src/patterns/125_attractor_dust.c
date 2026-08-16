/* 125 Attractor Dust — a Clifford attractor drawn as a long exposure.
 *     x' = sin(a*y) + c*cos(a*x)      y' = sin(b*x) + d*cos(b*y)
 * 200k iterates of that map are splatted into a density field every frame. The
 * four parameters creep on four incommensurate sines, so the attractor's
 * filigree grows and folds instead of cutting; and because the walk starts from
 * the same seed point every frame, consecutive frames trace almost the same
 * grains — the dust drifts rather than boiling.
 * The density field is not shown directly: it is fed through a one-pole
 * temporal filter (1% new per frame, a ~100-frame memory), which is what turns a
 * sparse point cloud into a smooth, photographic long exposure and holds the
 * frame-to-frame delta near zero even when the attractor reorganises.
 * Colour comes from the palette indexed by density, so the sparse outer dust
 * and the dense caustic folds land in different parts of the ramp.
 * Overlay routine: most of the field is empty space. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P125_LW  512
#define P125_LH  384
#define P125_N   (P125_LW * P125_LH)
#define P125_PTS 200000
#define P125_TAU 6.283185307179586f

static float    p125_acc[P125_N];
static float    p125_ba[P125_N], p125_bb[P125_N];
static uint32_t p125_low[P125_N];
static float    p125_sin[2048];
static uint32_t p125_ramp[512];
static int      p125_ready;

static void p125_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p125_sin[i] = sinf((float)i * (P125_TAU / 2048.0f));
    for (i = 0; i < P125_N; i++) p125_acc[i] = 0.0f;
    p125_ready = 1;
}

/* one 1-2-1 separable pass, src -> dst (dst may not alias src) */
static void p125_blur(const float *src, float *dst)
{
    int x, y;
    for (y = 0; y < P125_LH; y++) {
        const float *s = src + y * P125_LW;
        float *d = p125_bb + y * P125_LW;
        d[0] = (3.0f * s[0] + s[1]) * 0.25f;
        for (x = 1; x < P125_LW - 1; x++)
            d[x] = (s[x - 1] + 2.0f * s[x] + s[x + 1]) * 0.25f;
        d[P125_LW - 1] = (s[P125_LW - 2] + 3.0f * s[P125_LW - 1]) * 0.25f;
    }
    for (y = 0; y < P125_LH; y++) {
        const float *u = p125_bb + (y > 0 ? y - 1 : y) * P125_LW;
        const float *m = p125_bb + y * P125_LW;
        const float *v = p125_bb + (y < P125_LH - 1 ? y + 1 : y) * P125_LW;
        float *d = dst + y * P125_LW;
        for (x = 0; x < P125_LW; x++)
            d[x] = (u[x] + 2.0f * m[x] + v[x]) * 0.25f;
    }
}

static void p125_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P125_LW << 16) / w);
    int fx0 = (int)(((long)P125_LW << 15) / w) - (1 << 15);
    int maxx = (P125_LW - 1) << 16, maxy = (P125_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P125_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * (long)w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P125_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p125_low + (long)y0 * P125_LW;
        r1 = p125_low + (long)y1 * P125_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P125_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx;
            unsigned sy = 256u - (unsigned)wy;
            uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            uint32_t trb = (((a & 0xFF00FFu) * sx + (b & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t tg  = (((a & 0x00FF00u) * sx + (b & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t brb = (((c & 0xFF00FFu) * sx + (d & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t bg  = (((c & 0x00FF00u) * sx + (d & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t orb = ((trb * sy + brb * (unsigned)wy) >> 8) & 0xFF00FFu;
            uint32_t og  = ((tg  * sy + bg  * (unsigned)wy) >> 8) & 0x00FF00u;
            dst[x] = 0xFF000000u | orb | og;
            fx += stepx;
        }
    }
}

void pattern_125(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    static float hit[P125_N];
    float t = (float)(frame & 0xFFFFF);
    float a, b, c, d, ka, kb, sc, cxp, cyp, ca, sa, spin;
    float px = 0.11f, py = 0.29f;
    int i, pb;
    (void)sl; (void)seed;

    if (!p125_ready) p125_init();

    a = -1.42f + 0.34f * sinf(t * 0.00031f);
    b =  1.66f + 0.28f * sinf(t * 0.00023f + 1.1f);
    c =  1.02f + 0.26f * sinf(t * 0.00017f + 2.3f);
    d =  0.71f + 0.24f * sinf(t * 0.00029f + 3.7f);
    ka = a * (2048.0f / P125_TAU);
    kb = b * (2048.0f / P125_TAU);

    spin = t * 0.00035f;
    ca = cosf(spin); sa = sinf(spin);
    sc  = (float)P125_LH * 0.250f;
    cxp = (float)P125_LW * 0.5f;
    cyp = (float)P125_LH * 0.5f;

    for (i = 0; i < P125_N; i++) hit[i] = 0.0f;

    for (i = 0; i < P125_PTS; i++) {
        float nx, ny, ux, uy, fxp, fyp, wxf, wyf;
        int xi, yi, o;
        nx = p125_sin[(int)(ka * py) & 2047] + c * p125_sin[(((int)(ka * px)) + 512) & 2047];
        ny = p125_sin[(int)(kb * px) & 2047] + d * p125_sin[(((int)(kb * py)) + 512) & 2047];
        px = nx; py = ny;
        if (i < 24) continue;
        ux = px * ca - py * sa;
        uy = px * sa + py * ca;
        fxp = cxp + ux * sc; fyp = cyp + uy * sc;
        xi = (int)fxp; yi = (int)fyp;
        if (xi < 0 || yi < 0 || xi >= P125_LW - 1 || yi >= P125_LH - 1) continue;
        wxf = fxp - (float)xi; wyf = fyp - (float)yi;
        o = yi * P125_LW + xi;
        hit[o]              += (1.0f - wxf) * (1.0f - wyf);
        hit[o + 1]          += wxf * (1.0f - wyf);
        hit[o + P125_LW]    += (1.0f - wxf) * wyf;
        hit[o + P125_LW + 1]+= wxf * wyf;
    }

    pb = (int)(t * 2.5f);
    for (i = 0; i < 512; i++) {
        uint32_t u = pal[(pb + i * 52) & JD_PAL_MASK];
        float s = (float)i * (1.0f / 511.0f);
        float g = powf(s, 0.72f);                     /* gentle lift off black */
        int r = (int)((float)((u >> 16) & 255) * g);
        int gg = (int)((float)((u >> 8) & 255) * g);
        int bb = (int)((float)(u & 255) * g);
        int wht = (int)(s * s * s * s * s * s * s * s * 120.0f);
        r += wht; gg += wht; bb += wht;
        if (r > 255) r = 255; if (gg > 255) gg = 255; if (bb > 255) bb = 255;
        p125_ramp[i] = ((uint32_t)r << 16) | ((uint32_t)gg << 8) | (uint32_t)bb;
    }

    for (i = 0; i < P125_N; i++)
        p125_acc[i] += (hit[i] - p125_acc[i]) * 0.010f;

    /* two 1-2-1 separable passes: the point cloud becomes grainless smoke */
    p125_blur(p125_acc, p125_ba);
    p125_blur(p125_ba, p125_ba);
    for (i = 0; i < P125_N; i++) {
        float av = p125_ba[i] * 0.36f;
        float v = av / (1.0f + av);               /* Reinhard: no hard clip  */
        int k = (int)(v * 638.0f);
        if (k < 0) k = 0; if (k > 511) k = 511;
        p125_low[i] = 0xFF000000u | p125_ramp[k];
    }
    p125_blit(fb, w, h);
}
