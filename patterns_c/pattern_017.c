/* pattern_017 — metaball kiss: four metaballs living in a 6-fold wedge, so each
 * ball fuses with its own reflections across the seams into soft lobed
 * mandalas that merge and split, with a glowing rim on the iso-contour.
 * Port of lab/patterns/017_metaball_kiss/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>

#define LW 480
#define LH 360
#define IN 8192                     /* 1/(d2+160) table, 32 units of d2 each  */
#define GN 4096                     /* field -> colour LUT entries            */
#define GMAX 24.0f

static float pxt[LW * LH], pyt[LW * LH];
static float invt[IN];
static uint32_t lbuf[LW * LH];
static int ready = 0;

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
    for (int i = 0; i < IN; i++) invt[i] = 1.0f / ((float)i * 32.0f + 160.0f);
    const float w2 = 6.2831853f / 6.0f;
    const float sc = 320.0f / (float)LW;
    for (int y = 0; y < LH; y++) {
        float dy = ((float)y - LH * 0.5f) * sc;
        for (int x = 0; x < LW; x++) {
            float dx = ((float)x - LW * 0.5f) * sc;
            float r = sqrtf(dx * dx + dy * dy);
            float a = atan2f(dy, dx);
            float fa = fmodf(a, w2);
            if (fa < 0.0f) fa += w2;
            if (fa > w2 * 0.5f) fa = w2 - fa;
            int i = y * LW + x;
            pxt[i] = r * cosf(fa);
            pyt[i] = r * sinf(fa);
        }
    }
}

void pattern_017(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!ready) { setup(); ready = 1; }
    const float t = (float)frame;
    const float w2 = 6.2831853f / 6.0f;

    /* wx, wy, phase x, phase y, amp x, amp y, strength */
    static const float B[4][7] = {
        { 0.0040f, 0.0030f, 0.0f, 1.2f, 105.0f, 82.0f, 2400.0f },
        { 0.0028f, 0.0038f, 2.4f, 0.4f,  88.0f, 96.0f, 2000.0f },
        { 0.0035f, 0.0025f, 4.1f, 2.8f, 116.0f, 70.0f, 2100.0f },
        { 0.0021f, 0.0045f, 1.7f, 4.6f,  72.0f,104.0f, 1700.0f },
    };
    float qx[4], qy[4], st[4];
    for (int k = 0; k < 4; k++) {
        float bx = B[k][4] * sinf(t * B[k][0] + B[k][2]);
        float by = B[k][5] * sinf(t * B[k][1] + B[k][3]);
        float ba = atan2f(by, bx), br = sqrtf(bx * bx + by * by);
        float fa = fmodf(ba, w2);
        if (fa < 0.0f) fa += w2;
        if (fa > w2 * 0.5f) fa = w2 - fa;
        qx[k] = br * cosf(fa); qy[k] = br * sinf(fa);
        st[k] = B[k][6];
    }

    /* one field -> colour LUT per frame; everything nonlinear lives here */
    uint32_t clut[GN];
    const float bph = -t * 0.009f;
    const float hbase = 0.50f + t * 0.0003f + (float)(seed & 1023u) * 0.000976f;
    for (int k = 0; k < GN; k++) {
        float g = (float)k * (GMAX / (float)GN);
        float inside = tanhf((g - 1.0f) * 2.2f) * 0.5f + 0.5f;
        float dd = g - 1.0f;
        float rim = expf(-(dd * dd) / 0.030f);
        float bands = 0.72f + 0.28f * sinf(logf(g + 0.2f) * 5.0f + bph);
        float body = (g - 0.55f) * 1.1f;
        if (body < 0.0f) body = 0.0f; else if (body > 1.0f) body = 1.0f;
        body = powf(body, 1.2f);
        float val = (body * bands + rim * 0.55f) * 1.30f;
        if (val < 0.0f) val = 0.0f; else if (val > 1.0f) val = 1.0f;
        float hue = hbase - inside * 0.17f;
        uint32_t c = pal[(int)(hue * 32768.0f) & JD_PAL_MASK];
        unsigned v = (unsigned)(val * 256.0f);
        uint32_t rr = ((((c >> 16) & 255u) * v) >> 8);
        uint32_t gg = ((((c >> 8) & 255u) * v) >> 8);
        uint32_t bb = (((c & 255u) * v) >> 8);
        unsigned wmix = (unsigned)(rim * 150.0f);       /* rim burns to white */
        rr += ((255u - rr) * wmix) >> 8;
        gg += ((255u - gg) * wmix) >> 8;
        bb += ((255u - bb) * wmix) >> 8;
        clut[k] = 0xFF000000u | (rr << 16) | (gg << 8) | bb;
    }

    const float gs = (float)GN / GMAX;
    for (int i = 0; i < LW * LH; i++) {
        float px = pxt[i], py = pyt[i];
        float g = 0.0f;
        for (int k = 0; k < 4; k++) {
            float ax = px - qx[k], ay = py - qy[k];
            float d2 = ax * ax + ay * ay;
            int j = (int)(d2 * (1.0f / 32.0f));
            if (j >= IN) j = IN - 1;
            g += st[k] * invt[j];
        }
        int k = (int)(g * gs);
        if (k >= GN) k = GN - 1; else if (k < 0) k = 0;
        lbuf[i] = clut[k];
    }
    blit(fb, w, h);
}
