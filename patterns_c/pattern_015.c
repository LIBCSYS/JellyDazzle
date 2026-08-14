/* pattern_015 — twister star: the demoscene twister run over radius instead of
 * scanlines. Five-armed braid of four jewel-toned ribbon faces, sinusoidally
 * twisted along the radius and lit from above.
 * Port of lab/patterns/015_twister_star/proto.py. Repaint pattern. */
#include "../jellydazzle.h"
#include <math.h>
#include <stdlib.h>

#define SN 4096
#define SMASK 4095
#define R2I 651.8986469f
#define LT 512                      /* face-lighting LUT resolution           */

static float sn_tab[SN];
static float lite[LT];              /* sin(pi*fp)^0.7                         */
static int ready = 0;

static float s_sin(float ph) { return sn_tab[(int)(ph * R2I) & SMASK]; }

static float fast_atan2(float y, float x) {
    float ax = fabsf(x), ay = fabsf(y), a, z;
    if (ax >= ay) {
        z = y / (ax + 1e-9f);
        a = z / (1.0f + 0.28f * z * z);
        if (x < 0.0f) a = (y >= 0.0f) ? (3.14159265f - a) : (-3.14159265f - a);
        return a;
    }
    z = x / (ay + 1e-9f);
    a = 1.57079633f - z / (1.0f + 0.28f * z * z);
    return (y < 0.0f) ? -a : a;
}

static uint32_t shade(uint32_t c, unsigned v) {
    uint32_t rb = (uint32_t)((((uint64_t)(c & 0xFF00FFu)) * v) >> 8);
    uint32_t g = ((((c >> 8) & 0xFFu) * v) >> 8);
    return (rb & 0xFF00FFu) | ((g & 0xFFu) << 8) | 0xFF000000u;
}

/* Frame-invariant polar map. r and the fast_atan2 angle depend only on (x,y);
 * caching them lifts one sqrtf and one divide per pixel out of the frame loop.
 * The stored expressions are verbatim copies, so the image is unchanged. */
static float *p15_r, *p15_a;
static int p15_tw, p15_th;
static float p15_rrow[4096], p15_arow[4096];   /* fallback if alloc fails */

static void p15_polrow(float *rr, float *aa, float dy, float cx, float sc,
                       int w) {
    if (w > 4096) w = 4096;
    for (int x = 0; x < w; x++) {
        float dx = ((float)x - cx) * sc;
        rr[x] = sqrtf(dx * dx + dy * dy);
        aa[x] = fast_atan2(dy, dx);
    }
}

static void p15_map(int w, int h) {
    if (p15_tw == w && p15_th == h && p15_r) return;
    free(p15_r); free(p15_a);
    p15_r = (float *)malloc(sizeof(float) * (size_t)w * (size_t)h);
    p15_a = (float *)malloc(sizeof(float) * (size_t)w * (size_t)h);
    if (!p15_r || !p15_a) {
        free(p15_r); free(p15_a); p15_r = 0; p15_a = 0;
        p15_tw = 0; p15_th = 0; return;
    }
    float sc = 320.0f / (float)w;
    float cx = 0.5f * (float)w, cy = 0.5f * (float)h;
    for (int y = 0; y < h; y++)
        p15_polrow(p15_r + (long)y * w, p15_a + (long)y * w,
                   ((float)y - cy) * sc, cx, sc, w);
    p15_tw = w; p15_th = h;
}

static void setup(void) {
    for (int i = 0; i < SN; i++) sn_tab[i] = sinf((float)i * (6.2831853f / SN));
    for (int i = 0; i < LT; i++) {
        float fp = ((float)i + 0.5f) / (float)LT;
        lite[i] = powf(sinf(fp * 3.14159265f), 0.7f);
    }
}

void pattern_015(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal) {
    (void)sl;
    if (!ready) { setup(); ready = 1; }
    p15_map(w, h);
    const float t = (float)frame;
    const float sc = 320.0f / (float)w;          /* screen px -> proto units */
    const float cx = 0.5f * (float)w, cy = 0.5f * (float)h;

    /* four jewel hues, evenly spread around the palette loop */
    static const float hues[4] = { 0.985f, 0.09f, 0.35f, 0.62f };
    const float hbase = t * 0.0003f + (float)(seed & 1023u) * 0.000976f;
    const float twist = -t * 0.0045f;            /* radial twist phase drift  */
    const float spin  =  t * 0.0025f;            /* whole-star rotation       */

    for (int y = 0; y < h; y++) {
        float dy = ((float)y - cy) * sc;
        uint32_t *row = fb + (long)y * w;
        const float *rr, *aa;
        if (p15_r) { rr = p15_r + (long)y * w; aa = p15_a + (long)y * w; }
        else { p15_polrow(p15_rrow, p15_arow, dy, cx, sc, w);
               rr = p15_rrow; aa = p15_arow; }
        for (int x = 0; x < w; x++) {
            float r = rr[x];
            float a = aa[x];
            float phi = 1.6f * s_sin(r * 0.028f + twist) + spin;
            /* face coordinate in quarter-turn units */
            float q = (a * 5.0f + phi) * 0.63661977f + 1024.0f;
            int qi = (int)q;
            float fp = q - (float)qi;
            int fi = qi & 3;
            float light = lite[(int)(fp * (float)LT) & (LT - 1)];

            float radial = 1.0f - r * (1.0f / 195.0f);
            if (radial < 0.0f) radial = 0.0f;
            float soft = r * (1.0f / 12.0f);
            if (soft > 1.0f) soft = 1.0f;
            float val = light * (0.22f + 0.78f * radial) * soft;
            if (val > 1.0f) val = 1.0f;

            float hue = hues[fi] + hbase + r * 0.0006f;
            int idx = (int)(hue * 32768.0f) & JD_PAL_MASK;
            row[x] = shade(pal[idx], (unsigned)(val * 256.0f));
        }
    }
}
