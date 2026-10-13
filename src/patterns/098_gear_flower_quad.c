/* 098 Gear Flower Quad — port of lab/patterns/098_gear_flower_quad/proto.py
 * A 2x2 array of toothed gear/sunflower rosettes with dotted seed cores over
 * drifting diagonal stripes, stepped triangle borders top and bottom, and a
 * diamond chain creeping down the centre vertical. The teeth turn slowly while
 * the whole scheme swaps figure and ground on a ~26 s sine. 640x480 field. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define P98_LW 640
#define P98_LH 480
#define P98_N  (P98_LW * P98_LH)
#define P98_PI 3.14159265f

static uint32_t p98_low[P98_N];
static float    p98_sin[2048];
static int      p98_ready;

static const float p98_ctr[4][2] = { { 80.0f, 66.0f }, { 240.0f, 66.0f },
                                     { 80.0f, 174.0f }, { 240.0f, 174.0f } };
/* 3.6.5: it ignored its seed (identical every appearance) and its five
 * colours were fixed RGB constants only tinted 15% by the palette, so C
 * barely changed it (variety audit dPal 2.3). The five roles keep their old
 * names, but each is now read from the live palette at a seed-chosen spot
 * on the ramp; the seed also sets tooth count, gear spacing, stripe pitch
 * and slant, and which way the teeth turn. */
static float    p98_YEL[3], p98_GRN[3], p98_NVY[3], p98_CYN[3], p98_BLU[3];
static uint32_t p98_seed = 0xFFFFFFFFu;
static float    p98_teeth = 14.0f;      /* teeth per gear                    */
static float    p98_spr   = 0.0f;       /* gear spread from the quad centres */
static float    p98_sk    = 0.32f;      /* stripe spatial frequency          */
static float    p98_slx   = 1.0f;       /* stripe slant: x weight (y = 1)    */
static float    p98_sdir  = 1.0f;       /* tooth spin direction              */
static uint32_t p98_base;               /* where on the ramp the run starts  */

static uint32_t p98_h(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }

/* One palette role. Bright roles get the brightest channel lifted to ~190 so
 * a dark stretch of the ramp cannot turn a gear black; the navy role is the
 * same hue pulled down, keeping the old dark-rim/bright-face contrast. */
static void p98_role(float *o, const uint32_t *pal, uint32_t at, int dark)
{
    uint32_t u = pal[at & JD_PAL_MASK];
    float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255), b = (float)(u & 255);
    float mx = r > g ? r : g, k;
    if (b > mx) mx = b;
    if (mx < 8.0f) { r = g = b = mx = 8.0f; }
    k = dark ? 105.0f / mx : (mx < 190.0f ? 190.0f / mx : 1.0f);
    o[0] = r * k; o[1] = g * k; o[2] = b * k;
}

static void p98_init(void)
{
    int i;
    for (i = 0; i < 2048; i++)
        p98_sin[i] = sinf((float)i * (2.0f * P98_PI / 2048.0f));
    p98_ready = 1;
}

static inline float p98_lsin(float a)
{
    return p98_sin[((int)(a * 325.9493f + 2048.5f)) & 2047];
}
static inline float p98_lcos(float a) { return p98_lsin(a + 1.57079633f); }

static float p98_atan2(float y, float x)
{
    float ax = x < 0.0f ? -x : x, ay = y < 0.0f ? -y : y, a, s, r;
    if (ax < 1e-9f && ay < 1e-9f) return 0.0f;
    a = (ax > ay) ? ay / (ax + 1e-20f) : ax / (ay + 1e-20f);
    s = a * a;
    r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = 1.57079637f - r;
    if (x < 0.0f) r = 3.14159274f - r;
    if (y < 0.0f) r = -r;
    return r;
}

static void p98_buildpal(const uint32_t *pal)
{
    p98_role(p98_YEL, pal, p98_base,          0);
    p98_role(p98_GRN, pal, p98_base + 5000u,  0);
    p98_role(p98_CYN, pal, p98_base + 11000u, 0);
    p98_role(p98_BLU, pal, p98_base + 18000u, 0);
    p98_role(p98_NVY, pal, p98_base + 25000u, 1);
}

static void p98_blit(uint32_t *fb, int w, int h)
{
    int x, y;
    int stepx = (int)(((long)P98_LW << 16) / w);
    int fx0 = (int)(((long)P98_LW << 15) / w) - (1 << 15);
    int maxx = (P98_LW - 1) << 16, maxy = (P98_LH - 1) << 16;
    for (y = 0; y < h; y++) {
        int fy = (int)(((long)(2 * y + 1) * P98_LH << 15) / h) - (1 << 15);
        int y0, y1, wy, fx = fx0;
        const uint32_t *r0, *r1;
        uint32_t *dst = fb + (long)y * w;
        if (fy < 0) fy = 0; if (fy > maxy) fy = maxy;
        y0 = fy >> 16; y1 = y0 + 1 < P98_LH ? y0 + 1 : y0; wy = (fy >> 8) & 255;
        r0 = p98_low + (long)y0 * P98_LW;
        r1 = p98_low + (long)y1 * P98_LW;
        for (x = 0; x < w; x++) {
            int cx = fx < 0 ? 0 : (fx > maxx ? maxx : fx);
            int x0 = cx >> 16, x1 = x0 + 1 < P98_LW ? x0 + 1 : x0;
            unsigned wx = (unsigned)((cx >> 8) & 255), sx = 256u - wx, sy2 = 256u - (unsigned)wy;
            uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            uint32_t trb = (((a & 0xFF00FFu) * sx + (b & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t tg  = (((a & 0x00FF00u) * sx + (b & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t brb = (((c & 0xFF00FFu) * sx + (d & 0xFF00FFu) * wx) >> 8) & 0xFF00FFu;
            uint32_t bg  = (((c & 0x00FF00u) * sx + (d & 0x00FF00u) * wx) >> 8) & 0x00FF00u;
            uint32_t orb = ((trb * sy2 + brb * (unsigned)wy) >> 8) & 0xFF00FFu;
            uint32_t og  = ((tg  * sy2 + bg  * (unsigned)wy) >> 8) & 0x00FF00u;
            dst[x] = 0xFF000000u | orb | og;
            fx += stepx;
        }
    }
}

static uint32_t p98_pack(const float *c)
{
    int r = (int)c[0], g = (int)c[1], b = (int)c[2];
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static void p98_mix(float *o, const float *a, const float *b, float s)
{
    o[0] = a[0] * (1.0f - s) + b[0] * s;
    o[1] = a[1] * (1.0f - s) + b[1] * s;
    o[2] = a[2] * (1.0f - s) + b[2] * s;
}

void pattern_098(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float swap, tstripe, tdot, spin, chy, ctr[4][2];
    float do_[3], di[3], ga[3], gb[3], cco[3], cdo[3];
    uint32_t uga, ugb, ucore, udot, urim;
    int i, x, y;
    (void)sl;

    if (!p98_ready) p98_init();
    if (seed != p98_seed) {
        uint32_t a = p98_h(seed ^ 0x9898u), b = p98_h(a), c = p98_h(b);
        p98_teeth = (float)(9 + (int)(a % 10u));                   /* 9..18   */
        p98_spr   = -6.0f + (float)((a >> 8) % 1000u) * 0.014f;   /* -6..+8  */
        p98_sk    = 0.20f + (float)(b % 1000u) * 0.00026f;        /* .20..46 */
        p98_slx   = (b & 0x10000u) ? 1.0f : -1.0f;                /* / or \  */
        p98_sdir  = (c & 1u) ? 1.0f : -1.0f;
        p98_base  = p98_h(c) & JD_PAL_MASK;
        p98_seed  = seed;
    }
    p98_buildpal(pal);
    for (i = 0; i < 4; i++) {   /* push the quad apart or together a little */
        ctr[i][0] = p98_ctr[i][0] + (i & 1 ? p98_spr : -p98_spr);
        ctr[i][1] = p98_ctr[i][1] + (i & 2 ? p98_spr * 0.4f : -p98_spr * 0.4f);
    }
    swap = 0.5f + 0.5f * p98_lsin(t * 0.004f - 2.0f * P98_PI
           * floorf(t * 0.004f / (2.0f * P98_PI)));
    tstripe = t * 0.012f; tstripe -= 2.0f * P98_PI * floorf(tstripe / (2.0f * P98_PI));
    tdot    = t * 0.02f;  tdot    -= 2.0f * P98_PI * floorf(tdot / (2.0f * P98_PI));
    spin    = p98_sdir * t * 0.004f;
    chy     = t * 0.05f;  chy     -= 26.0f * floorf(chy / 26.0f);

    p98_mix(do_, p98_YEL, p98_BLU, swap);
    p98_mix(di,  p98_GRN, p98_CYN, swap);
    p98_mix(ga,  p98_BLU, p98_YEL, swap);
    p98_mix(gb,  p98_CYN, p98_GRN, swap);
    p98_mix(cco, p98_NVY, p98_YEL, swap);
    p98_mix(cdo, p98_YEL, p98_NVY, swap);
    uga = p98_pack(ga); ugb = p98_pack(gb);
    ucore = p98_pack(cco); udot = p98_pack(cdo);
    urim = p98_pack(p98_NVY);

    for (y = 0; y < P98_LH; y++) {
        float ly = ((float)y + 0.5f) * (240.0f / (float)P98_LH);
        uint32_t *out = p98_low + (long)y * P98_LW;
        float bflip = -1.0f;
        if (ly < 16.0f) bflip = ly;
        else if (ly >= 224.0f) bflip = 239.0f - ly;
        for (x = 0; x < P98_LW; x++) {
            float lx = ((float)x + 0.5f) * (320.0f / (float)P98_LW);
            out[x] = (p98_lsin((p98_slx * lx + ly) * p98_sk + tstripe) > 0.0f) ? uga : ugb;
            if (bflip >= 0.0f) {
                float q = lx + 5.0f * floorf(bflip * 0.25f);
                q -= 26.0f * floorf(q / 26.0f);
                if (q < 13.0f) out[x] = ucore;
            }
        }
        /* four gear rosettes */
        for (i = 0; i < 4; i++) {
            float cy = ctr[i][1], cx = ctr[i][0];
            float dy = ly - cy;
            int xa, xb;
            if (dy < -42.0f || dy > 42.0f) continue;
            xa = (int)((cx - 42.0f) * 2.0f); xb = (int)((cx + 42.0f) * 2.0f);
            if (xa < 0) xa = 0; if (xb > P98_LW) xb = P98_LW;
            for (x = xa; x < xb; x++) {
                float lx = ((float)x + 0.5f) * (320.0f / (float)P98_LW);
                float dx = lx - cx;
                float rr = sqrtf(dx * dx + dy * dy);
                float aa, tooth, u, col[3];
                if (rr > 42.0f) continue;
                aa = p98_atan2(dy, dx);
                tooth = 34.0f + 5.0f * p98_lcos(p98_teeth * (aa + spin));
                if (rr < tooth) {
                    u = rr * (1.0f / 36.0f); if (u > 1.0f) u = 1.0f;
                    p98_mix(col, di, do_, u);
                    out[x] = p98_pack(col);
                }
                if (rr - tooth < 2.2f && tooth - rr < 2.2f) out[x] = urim;
                if (rr < 15.0f) {
                    out[x] = ucore;
                    if (p98_lsin(rr * 1.35f - tdot) > 0.35f &&
                        p98_lsin(aa * 9.0f + rr * 0.6f) > 0.1f) out[x] = udot;
                }
            }
        }
        /* diamond chain on the centre vertical */
        {
            float m = ly + chy;
            float k;
            m -= 26.0f * floorf(m / 26.0f);
            k = m - 13.0f; if (k < 0.0f) k = -k;
            k = 1.0f - k / 10.0f;
            if (k > 0.0f) {
                float halfw = k * 7.0f;
                int xa = (int)((160.0f - halfw) * 2.0f), xb = (int)((160.0f + halfw) * 2.0f);
                if (xa < 0) xa = 0; if (xb > P98_LW) xb = P98_LW;
                for (x = xa; x < xb; x++) out[x] = udot;
            }
        }
    }
    p98_blit(fb, w, h);
}
