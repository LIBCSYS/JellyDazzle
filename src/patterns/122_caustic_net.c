/* 122 Caustic Net — the light web on the floor of a swimming pool.
 * A water surface h(x,y,t) is built from four slow plane waves. Light entering
 * it is deflected by grad h, so the map floor(p) = p + k*grad h has Jacobian
 * I + k*Hess(h); brightness on the floor is 1/|det|, which blows up along the
 * curves where the deflected rays cross. Those curves are the caustic net.
 * Every second derivative of a plane wave is the same sine times a constant,
 * so one table read per wave gives all three Hessian terms: the inner loop is
 * four table reads, twelve multiply-adds, one determinant and one reciprocal.
 * Hue follows the surface height itself, so colour swims with the swell.
 * Overlay routine: the floor between the filaments stays near black. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P122_LW  640
#define P122_LH  480
#define P122_N   (P122_LW * P122_LH)
#define P122_NW  5
#define P122_TAU 6.283185307179586f

static uint32_t p122_low[P122_N];
static float    p122_sin[1024];
static uint32_t p122_ramp[256];
static int      p122_ready;

static void p122_init(void)
{
    int i;
    for (i = 0; i < 1024; i++)
        p122_sin[i] = sinf((float)i * (P122_TAU / 1024.0f));
    p122_ready = 1;
}

static void p122_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P122_LW << 16) / w);
    int fx0 = (int)(((long)P122_LW << 15) / w) - (1 << 15);
    int maxx = (P122_LW - 1) << 16, maxy = (P122_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P122_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * (long)w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P122_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p122_low + (long)y0 * P122_LW;
        r1 = p122_low + (long)y1 * P122_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P122_LW ? x0 + 1 : x0;
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

void pattern_122(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    static const float len[P122_NW] = { 232.0f, 168.0f, 279.0f, 131.0f, 437.0f };
    static const float amp[P122_NW] = { 0.79f, 0.68f, 0.58f, 0.45f, 0.52f };
    static const float th0[P122_NW] = { 0.0f, 1.31f, 2.55f, 4.02f, 5.31f };
    static const float spd[P122_NW] = { 0.0130f, -0.0098f, 0.0079f, -0.0142f, 0.0051f };
    float t = (float)(frame & 0xFFFFF);
    float cxx[P122_NW], cyy[P122_NW], cxy[P122_NW], ha[P122_NW], swell, ga;
    uint32_t sx[P122_NW], sy[P122_NW], b0[P122_NW];
    int i, x, y, pb, gxi, gyi;
    (void)sl; (void)seed;

    if (!p122_ready) p122_init();

    swell = 0.80f + 0.20f * sinf(t * 0.00037f);
    ga    = t * 0.000077f;                      /* hue-band drift direction  */
    gxi   = (int)(cosf(ga) * 112.0f);
    gyi   = (int)(sinf(ga) * 112.0f);
    pb = (int)(t * 5.0f);
    for (i = 0; i < 256; i++) {
        uint32_t c = pal[(pb + i * 128) & JD_PAL_MASK];
        int cr = (int)((c >> 16) & 255), cg = (int)((c >> 8) & 255), cb = (int)(c & 255);
        int mx = cr > cg ? cr : cg; if (cb > mx) mx = cb;
        if (mx < 40) mx = 40;                    /* keep the tint vivid      */
        cr = cr * 255 / mx; cg = cg * 255 / mx; cb = cb * 255 / mx;
        p122_ramp[i] = ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | (uint32_t)cb;
    }

    for (i = 0; i < P122_NW; i++) {
        float a  = th0[i] + t * 0.000041f * (float)(1 + (i & 1) * 2);
        float k  = P122_TAU / len[i];
        float ca = cosf(a), sa = sinf(a);
        float c  = amp[i] * swell;
        cxx[i] = c * ca * ca; cyy[i] = c * sa * sa; cxy[i] = c * ca * sa;
        ha[i]  = amp[i] * 0.42f;
        sx[i]  = (uint32_t)(int32_t)(k * ca * (1024.0f / P122_TAU) * 16777216.0f);
        sy[i]  = (uint32_t)(int32_t)(k * sa * (1024.0f / P122_TAU) * 16777216.0f);
        b0[i]  = (uint32_t)(int32_t)(t * spd[i] * (1024.0f / P122_TAU) * 16777216.0f);
    }

    for (y = 0; y < P122_LH; y++) {
        uint32_t ph[P122_NW];
        uint32_t *dst = p122_low + (long)y * P122_LW;
        int hb = gyi * y;
        for (i = 0; i < P122_NW; i++) ph[i] = b0[i] + sy[i] * (uint32_t)y;
        for (x = 0; x < P122_LW; x++, hb += gxi) {
            float Sxx = 0.0f, Syy = 0.0f, Sxy = 0.0f, hh = 0.0f, det, iv;
            int r, g, b, hot, hi;
            uint32_t c;
            for (i = 0; i < P122_NW; i++) {
                float s = p122_sin[(ph[i] >> 22) & 1023];
                ph[i] += sx[i];
                Sxx += cxx[i] * s; Syy += cyy[i] * s; Sxy += cxy[i] * s;
                hh  += ha[i] * s;
            }
            det = (1.0f - Sxx) * (1.0f - Syy) - Sxy * Sxy;
            if (det < 0.0f) det = -det;
            iv = 21.0f / (0.026f + det) - 17.0f;
            if (iv <= 1.0f) { dst[x] = 0xFF000000u; continue; }
            if (iv > 255.0f) iv = 255.0f;
            hi  = (int)(hh * 62.0f) + (hb >> 8);
            c   = p122_ramp[hi & 255];
            r   = (int)iv;
            hot = (r * r) >> 12;
            b   = (int)(((c & 255) * r) >> 8) + hot;
            g   = (int)((((c >> 8) & 255) * r) >> 8) + hot;
            r   = (int)((((c >> 16) & 255) * r) >> 8) + hot;
            if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
            dst[x] = 0xFF000000u | ((uint32_t)r << 16) |
                     ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
    p122_blit(fb, w, h);
}
