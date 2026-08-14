/* pattern_011 — plasma mandala: 4-sine plasma folded 6 ways, breathing petals.
 * Port of lab/patterns/011_plasma_mandala/proto.py.
 * Repaint pattern: low-res 320x240 field, bilinear upscale, palette color. */
#include "../jellydazzle.h"
#include <math.h>
#include <string.h>

#define LW 320
#define LH 240
#define SN 4096
#define SMASK 4095
#define RAD2IDX 651.8986469f /* SN / 2pi */

static uint32_t lbuf[LW * LH];
static float sn_tab[SN];
static float c6[SN], s6[SN];              /* cos/sin of 6-fold folded angle */
static unsigned short atab[LW * LH];      /* angle index 0..4095 */
static float rtab[LW * LH];               /* radius */
static int init_done = 0;

static float s_sin(float ph) { return sn_tab[(int)(ph * RAD2IDX) & SMASK]; }
static float s_cos(float ph) { return sn_tab[((int)(ph * RAD2IDX) + 1024) & SMASK]; }

static int fold15(int v) { v &= 0xFFFF; return v < 32768 ? v : 65535 - v; }

static uint32_t lerp_(uint32_t a, uint32_t b, unsigned f) {
    unsigned g = 256 - f;
    uint64_t rb = (((uint64_t)(a & 0xFF00FFu)) * g + ((uint64_t)(b & 0xFF00FFu)) * f) >> 8;
    uint32_t gg = (((a >> 8) & 0xFFu) * g + ((b >> 8) & 0xFFu) * f) >> 8;
    return ((uint32_t)rb & 0xFF00FFu) | (gg << 8) | 0xFF000000u;
}

static uint32_t shade_(uint32_t c, unsigned v) { /* v = 0..256 */
    uint64_t rb = ((uint64_t)(c & 0xFF00FFu) * v) >> 8;
    uint32_t g = (((c >> 8) & 0xFFu) * v) >> 8;
    return ((uint32_t)rb & 0xFF00FFu) | (g << 8) | 0xFF000000u;
}

static void upscale_(uint32_t *fb, int w, int h) {
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
            uint32_t t0 = lerp_(r0[x0], r0[x0 + 1], fx);
            uint32_t t1 = lerp_(r1[x0], r1[x0 + 1], fx);
            d[x] = lerp_(t0, t1, fy);
        }
        for (int x = wl; x < w; x++) d[x] = d[wl - 1];
    }
}

static void init_(void) {
    for (int i = 0; i < SN; i++) sn_tab[i] = sinf((float)i * (6.2831853f / SN));
    const float wseg = 6.2831853f / 6.0f;
    for (int k = 0; k < SN; k++) {
        float ang = (float)k * (6.2831853f / SN);
        float fa = fmodf(ang, wseg);
        if (fa > wseg * 0.5f) fa = wseg - fa;
        c6[k] = cosf(fa); s6[k] = sinf(fa);
    }
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++) {
            float dx = (float)x - LW / 2, dy = (float)y - LH / 2;
            int i = y * LW + x;
            rtab[i] = sqrtf(dx * dx + dy * dy);
            atab[i] = (unsigned short)((int)(atan2f(dy, dx) * RAD2IDX) & SMASK);
        }
}

void pattern_011(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!init_done) { init_(); init_done = 1; }
    float t = (float)frame;
    int ri = (int)(t * 0.0022f * RAD2IDX);
    float t1 = t * 0.0105f, t2 = t * 0.0085f, t3 = t * 0.0065f, t4 = t * 0.0095f;
    float tv1 = t * 0.0055f, tv2 = t * 0.0040f;
    float hueT = t * 0.0005f + (float)(seed & 1023u) * 0.000976f;
    for (int i = 0; i < LW * LH; i++) {
        float r = rtab[i];
        int ai = (atab[i] + ri) & SMASK;
        float u = r * c6[ai], v = r * s6[ai];
        float c = s_sin(u * 0.046f + t1) + s_sin(v * 0.071f - t2)
                + s_sin((u + v) * 0.038f + t3) + s_sin(r * 0.052f - t4);
        float val = 0.46f + 0.36f * s_cos(c * 1.15f - tv1)
                  + 0.16f * s_cos(r * 0.03f - tv2);
        if (val < 0.06f) val = 0.06f;
        if (val > 1.0f) val = 1.0f;
        int idx = fold15((int)((c * 0.10f + hueT) * 65536.0f));
        lbuf[i] = shade_(pal[idx & JD_PAL_MASK], (unsigned)(val * 256.0f));
    }
    upscale_(fb, w, h);
}
