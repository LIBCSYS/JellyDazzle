/* pattern_013 — tunnel bloom: off-centre flying tunnel behind a 4-fold mirror,
 * four mouths blooming, merging and drifting apart. Ice tones.
 * Port of lab/patterns/013_tunnel_bloom/proto.py. Repaint pattern. */
#include "../engine/jellydazzle.h"
#include <math.h>

#define LW 480
#define LH 360
#define SN 4096
#define SMASK 4095
#define R2I 651.8986469f

static float sn_tab[SN];
static float pxt[LW * LH], pyt[LW * LH];
static float shd[256];                 /* clip(x)^0.6 shade curve */
static uint32_t lbuf[LW * LH];
static int ready = 0;

static float s_sin(float ph) { return sn_tab[(int)(ph * R2I) & SMASK]; }

/* ~0.005 rad accurate atan2, plenty for an 8-band tunnel wall */
static float fast_atan2(float y, float x) {
    float ax = fabsf(x), ay = fabsf(y), a, z;
    if (ax >= ay) {
        z = y / (ax + 1e-9f);
        a = z / (1.0f + 0.28f * z * z);
        if (x < 0.0f) a = (y >= 0.0f) ? (3.14159265f - a) : (-3.14159265f - a);
        /* note: z already carries sign of y; for x<0 mirror about pi/2 */
        return a;
    }
    z = x / (ay + 1e-9f);
    a = 1.57079633f - z / (1.0f + 0.28f * z * z);
    return (y < 0.0f) ? -a : a;
}

static uint32_t shade(uint32_t c, unsigned v) {   /* v = 0..256 */
    uint32_t rb = (uint32_t)((((uint64_t)(c & 0xFF00FFu)) * v) >> 8);
    uint32_t g = ((((c >> 8) & 0xFFu) * v) >> 8);
    return (rb & 0xFF00FFu) | ((g & 0xFFu) << 8) | 0xFF000000u;
}

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
    for (int i = 0; i < 256; i++) shd[i] = powf((float)i / 255.0f, 0.6f);
    const float wseg = 6.2831853f / 4.0f;         /* 4-fold */
    const float sc = 320.0f / (float)LW;
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

void pattern_013(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!ready) { setup(); ready = 1; }
    const float t = (float)frame;

    const float ox = 55.0f * s_sin(t * 0.0026f);
    const float oy = 40.0f * s_sin(t * 0.0019f + 1.3f);
    const float fly = t * 0.10f;                    /* glide down the tunnel */
    const float rot = t * 0.002f;
    const float hueT = 0.52f + t * 0.00008f
                     + (float)(seed & 255u) * 0.00035f;

    for (int i = 0; i < LW * LH; i++) {
        float ddx = pxt[i] - ox, ddy = pyt[i] - oy;
        float rr = sqrtf(ddx * ddx + ddy * ddy) + 2.0f;
        float ang = fast_atan2(ddy, ddx);
        float v = 850.0f / rr + fly;
        float u = ang * 2.5464791f + rot;           /* 8/pi bands */
        float tex = s_sin(u * 3.14159265f) * s_sin(v * 0.32f);
        int si = (int)(rr * (255.0f / 55.0f));
        if (si > 255) si = 255; else if (si < 0) si = 0;
        float sh = shd[si];
        float val = sh * (0.50f + 0.50f * tex);
        if (val < 0.02f) val = 0.02f; else if (val > 1.0f) val = 1.0f;
        float hue = hueT + 0.065f * s_sin(v * 0.16f) + 0.035f * tex;
        int idx = (int)(hue * 32768.0f) & JD_PAL_MASK;
        lbuf[i] = shade(pal[idx], (unsigned)(val * 256.0f));
    }
    blit(fb, w, h);
}
