/* pattern_012 — rotozoom kaleido: procedural 3-sine tile texture rotozoomed
 * inside an 8-fold mirror, breathing with the zoom pulse.
 * Port of lab/patterns/012_rotozoom_kaleido/proto.py. Repaint pattern.
 * Low-res wedge field (480x360) + bilinear upscale; palette-LUT color. */
#include "../jellydazzle.h"
#include <math.h>

#define LW 480
#define LH 360
#define SN 4096
#define SMASK 4095
#define R2I 651.8986469f          /* SN / 2pi */

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

static void setup(void) {
    for (int i = 0; i < SN; i++) sn_tab[i] = sinf((float)i * (6.2831853f / SN));
    const float wseg = 6.2831853f / 8.0f;     /* 8-fold */
    const float sc = 320.0f / (float)LW;      /* proto units per lo-res pixel */
    for (int y = 0; y < LH; y++) {
        float dy = ((float)y - LH * 0.5f) * sc;
        for (int x = 0; x < LW; x++) {
            float dx = ((float)x - LW * 0.5f) * sc;
            float r = sqrtf(dx * dx + dy * dy);
            float a = atan2f(dy, dx);
            float fa = fmodf(a, wseg);
            if (fa < 0.0f) fa += wseg;
            if (fa > wseg * 0.5f) fa = wseg - fa;
            int i = y * LW + x;
            pxt[i] = r * cosf(fa);
            pyt[i] = r * sinf(fa);
        }
    }
}

void pattern_012(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!ready) { setup(); ready = 1; }
    const float t = (float)frame;

    /* looping 256-stop ramp taken straight off the palette loop */
    uint32_t lut[256];
    int base = (int)(seed & 0x7FFFu);
    for (int i = 0; i < 256; i++) lut[i] = pal[(base + i * 72) & JD_PAL_MASK];

    const float th = t * 0.0012f;                              /* slow spin */
    const float k = 0.055f * (1.0f + 0.42f * s_sin(t * 0.0016f)); /* zoom pulse */
    const float co = cosf(th) * k, si = sinf(th) * k;
    const float du = t * 0.0030f, dv = t * 0.0022f;            /* texture drift */

    for (int i = 0; i < LW * LH; i++) {
        float px = pxt[i], py = pyt[i];
        float u = px * co - py * si + du;
        float v = px * si + py * co + dv;
        float f = s_sin(u * 2.2f) * s_sin(v * 2.2f)
                + 0.6f * s_sin(u + v)
                + 0.35f * s_sin(u * 0.7f - v * 0.9f);
        int idx = (int)((f + 1.95f) * (255.0f / 3.9f));
        if (idx < 0) idx = 0; else if (idx > 255) idx = 255;
        lbuf[i] = lut[idx];
    }
    blit(fb, w, h);
}
