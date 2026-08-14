/* pattern_020 — feedback fractal: fold-aware video feedback. A glowing orbit
 * blob and a breathing ring are stamped into a 7-fold wedge; every generation
 * of the feedback re-mirrors, twists ~0.42 rad and shrinks toward the centre,
 * building a self-similar mandala of rings within rings. Hue tracks depth.
 * Port of lab/patterns/020_feedback_fractal/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

#define LW 320
#define LH 240
#define GLO (-3)
#define GHI 12                       /* generations GLO .. GHI-1              */
#define BN 512                       /* blob LUT, 8 units of d2 each          */
#define RN 896                       /* ring LUT, quarter-unit radius steps   */

static float pxt[LW * LH], pyt[LW * LH];
static unsigned short rid[LW * LH];
static float blut[BN];
static float acc[LW * LH * 3];
static uint32_t lbuf[LW * LH];
static int ready = 0;

static uint32_t lerp2(uint32_t a, uint32_t b, unsigned f) {
    unsigned g = 256u - f;
    uint32_t rb = (uint32_t)((((uint64_t)(a & 0xFF00FFu)) * g +
                              ((uint64_t)(b & 0xFF00FFu)) * f) >> 8);
    uint32_t gg = ((((a >> 8) & 0xFFu) * g + (((b >> 8) & 0xFFu) * f)) >> 8);
    return (rb & 0xFF00FFu) | ((gg & 0xFFu) << 8) | 0xFF000000u;
}

/* Horizontal half of the bilinear blit for one source row. Each 320x240 source
 * row feeds several output rows (four, at 960), so resampling it once and
 * caching costs one lerp2 per output pixel instead of three. Same arithmetic. */
static void hlerp_row(uint32_t *dst, const uint32_t *r, const int *xi,
                      const unsigned char *xf, int n) {
    for (int x = 0; x < n; x++) {
        int x0 = xi[x];
        dst[x] = lerp2(r[x0], r[x0 + 1], xf[x]);
    }
}

static void blit(uint32_t *fb, int w, int h) {
    static int cw = -1;
    static int xi[4096];
    static unsigned char xf[4096];
    static uint32_t hbuf[2][4096];
    uint32_t *p0 = hbuf[0], *p1 = hbuf[1];
    int cy = -2;
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
        if (y0 != cy) {
            if (y0 == cy + 1) { uint32_t *t = p0; p0 = p1; p1 = t; }
            else hlerp_row(p0, r0, xi, xf, wl);
            hlerp_row(p1, r1, xi, xf, wl);
            cy = y0;
        }
        for (int x = 0; x < wl; x++) d[x] = lerp2(p0[x], p1[x], fy);
        for (int x = wl; x < w; x++) d[x] = d[wl - 1];
    }
}

static float fold7(float ang) {
    const float w2 = 6.2831853f / 7.0f;
    float fa = fmodf(ang, w2);
    if (fa < 0.0f) fa += w2;
    if (fa > w2 * 0.5f) fa = w2 - fa;
    return fa;
}

static void setup(void) {
    for (int i = 0; i < BN; i++)
        blut[i] = expf(-((float)i * 8.0f) / (2.0f * 169.0f));
    const float sc = 320.0f / (float)LW;
    for (int y = 0; y < LH; y++) {
        float dy = ((float)y - LH * 0.5f) * sc;
        for (int x = 0; x < LW; x++) {
            float dx = ((float)x - LW * 0.5f) * sc;
            float r = sqrtf(dx * dx + dy * dy);
            float fa = fold7(atan2f(dy, dx));
            int i = y * LW + x;
            pxt[i] = r * cosf(fa);
            pyt[i] = r * sinf(fa);
            int ri = (int)(r * 4.0f);
            if (ri >= RN) ri = RN - 1;
            rid[i] = (unsigned short)ri;
        }
    }
}

static void neon(const uint32_t *pal, float hue, float *r, float *g, float *b) {
    uint32_t c = pal[(int)(hue * 32768.0f) & JD_PAL_MASK];
    float cr = (float)((c >> 16) & 255), cg = (float)((c >> 8) & 255),
          cb = (float)(c & 255);
    float m = cr > cg ? cr : cg; if (cb > m) m = cb;
    float k = (m > 1.0f) ? (1.0f / m) : 0.0f;
    *r = cr * k; *g = cg * k; *b = cb * k;
}

void pattern_020(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!ready) { setup(); ready = 1; }
    const float t = (float)frame;

    for (int i = 0; i < LW * LH * 3; i++) acc[i] = 0.0f;

    const float rot = 0.42f + 0.10f * sinf(t * 0.0015f);
    const float hbase = 0.55f + t * 0.00035f + (float)(seed & 1023u) * 0.000976f;

    float rlut[RN];
    for (int g = GLO; g < GHI; g++) {
        float tg = t - (float)g * 22.0f;
        float decay = powf(0.84f, (float)(g < 0 ? -g : g)) * (g < 0 ? 0.9f : 1.0f);
        float s = powf(1.26f, (float)g);
        float ang = rot * (float)g;
        float co = cosf(ang), si = sinf(ang);

        /* orbiting stamp, folded into the wedge, pulled back into P-space */
        float sx = 74.0f * sinf(tg * 0.0045f + 1.0f);
        float sy = 58.0f * sinf(tg * 0.0032f + 2.6f);
        float ba = atan2f(sy, sx), brr = sqrtf(sx * sx + sy * sy);
        float fb2 = fold7(ba);
        float qx = brr * cosf(fb2), qy = brr * sinf(fb2);
        float inv = 1.0f / s;
        float qpx = (qx * co + qy * si) * inv;
        float qpy = (-qx * si + qy * co) * inv;
        float s2 = s * s;

        /* breathing ring depends only on radius */
        float rr0 = 82.0f + 26.0f * sinf(tg * 0.0025f);
        for (int q = 0; q < RN; q++) {
            float d = (float)q * 0.25f * s - rr0;
            float e = (d * d) / 98.0f;
            rlut[q] = (e > 12.0f) ? 0.0f : expf(-e);
        }

        float h1 = hbase + (float)g * 0.075f;
        float c1r, c1g, c1b, c2r, c2g, c2b;
        neon(pal, h1, &c1r, &c1g, &c1b);
        neon(pal, h1 + 0.45f, &c2r, &c2g, &c2b);
        c1r *= decay; c1g *= decay; c1b *= decay;
        c2r *= decay * 0.60f; c2g *= decay * 0.60f; c2b *= decay * 0.60f;

        const float d2s = s2 * 0.125f;             /* -> blob LUT index scale */
        for (int i = 0; i < LW * LH; i++) {
            float ring = rlut[rid[i]];
            float ax = pxt[i] - qpx, ay = pyt[i] - qpy;
            float dq = (ax * ax + ay * ay) * d2s;
            float blob = 0.0f;
            if (dq < (float)BN) blob = blut[(int)dq];
            if (blob == 0.0f && ring == 0.0f) continue;
            float *o = acc + i * 3;
            o[0] += blob * c1r + ring * c2r;
            o[1] += blob * c1g + ring * c2g;
            o[2] += blob * c1b + ring * c2b;
        }
    }

    for (int i = 0; i < LW * LH; i++) {
        float *o = acc + i * 3;
        float r = o[0] * 0.92f, g = o[1] * 0.92f, b = o[2] * 0.92f;
        int ir = (int)(r * 255.0f), ig = (int)(g * 255.0f), ib = (int)(b * 255.0f);
        if (ir > 255) ir = 255; if (ig > 255) ig = 255; if (ib > 255) ib = 255;
        if (ir < 0) ir = 0; if (ig < 0) ig = 0; if (ib < 0) ib = 0;
        lbuf[i] = 0xFF000000u | ((uint32_t)ir << 16) | ((uint32_t)ig << 8)
                | (uint32_t)ib;
    }
    blit(fb, w, h);
}
