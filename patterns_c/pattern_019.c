/* pattern_019 — moire silk: two interference-ring sources orbiting inside an
 * 8-fold wedge (sixteen mirrored sources on screen) weaving silky moire fabric,
 * animated mostly by slow palette rotation.
 * Port of lab/patterns/019_moire_silk/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

#define LW 640
#define LH 480
#define SN 4096
#define SMASK 4095
#define R2I 651.8986469f

static float sn_tab[SN];
static float pxt[LW * LH], pyt[LW * LH];
static uint32_t lbuf[LW * LH];
static int ready = 0;

static float s_sin(float ph) { return sn_tab[(int)(ph * R2I) & SMASK]; }

static uint32_t lerp2(uint32_t a, uint32_t b, unsigned f) {
    unsigned g = 256u - f;
    uint32_t rb = (uint32_t)((((uint64_t)(a & 0xFF00FFu)) * g +
                              ((uint64_t)(b & 0xFF00FFu)) * f) >> 8);
    uint32_t gg = ((((a >> 8) & 0xFFu) * g + (((b >> 8) & 0xFFu) * f)) >> 8);
    return (rb & 0xFF00FFu) | ((gg & 0xFFu) << 8) | 0xFF000000u;
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

static float fold8(float ang) {
    const float w2 = 6.2831853f / 8.0f;
    float fa = fmodf(ang, w2);
    if (fa < 0.0f) fa += w2;
    if (fa > w2 * 0.5f) fa = w2 - fa;
    return fa;
}

static void setup(void) {
    for (int i = 0; i < SN; i++) sn_tab[i] = sinf((float)i * (6.2831853f / SN));
    const float sc = 320.0f / (float)LW;
    for (int y = 0; y < LH; y++) {
        float dy = ((float)y - LH * 0.5f) * sc;
        for (int x = 0; x < LW; x++) {
            float dx = ((float)x - LW * 0.5f) * sc;
            float r = sqrtf(dx * dx + dy * dy);
            float fa = fold8(atan2f(dy, dx));
            int i = y * LW + x;
            pxt[i] = r * cosf(fa);
            pyt[i] = r * sinf(fa);
        }
    }
}

void pattern_019(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!ready) { setup(); ready = 1; }
    const float t = (float)frame;

    /* both sources are themselves folded into the wedge */
    float ax = 72.0f * s_sin(t * 0.0030f), ay = 56.0f * s_sin(t * 0.0022f + 0.8f);
    float bx = 66.0f * s_sin(t * 0.0026f + 2.1f), by = 62.0f * s_sin(t * 0.0035f + 4.0f);
    float ra = sqrtf(ax * ax + ay * ay), fa = fold8(atan2f(ay, ax));
    float rb = sqrtf(bx * bx + by * by), fb2 = fold8(atan2f(by, bx));
    const float c1x = ra * cosf(fa), c1y = ra * sinf(fa);
    const float c2x = rb * cosf(fb2), c2y = rb * sinf(fb2);

    /* looping 256-stop silk ramp off the palette; rotation is the animation */
    uint32_t lut[256];
    int rot = (int)(t * 0.20f) + (int)(seed & 255u);
    for (int i = 0; i < 256; i++)
        lut[i] = pal[(((rot + i) & 255) * 128) & JD_PAL_MASK];

    const float ph1 = -t * 0.012f, ph2 = t * 0.010f;

    for (int i = 0; i < LW * LH; i++) {
        float px = pxt[i], py = pyt[i];
        float ux = px - c1x, uy = py - c1y;
        float vx = px - c2x, vy = py - c2y;
        float d1 = sqrtf(ux * ux + uy * uy);
        float d2 = sqrtf(vx * vx + vy * vy);
        float s = s_sin(d1 * 0.30f + ph1) + s_sin(d2 * 0.26f + ph2);
        int k = (int)((s + 2.0f) * 63.75f);
        if (k < 0) k = 0; else if (k > 255) k = 255;
        lbuf[i] = lut[k];
    }
    blit(fb, w, h);
}
