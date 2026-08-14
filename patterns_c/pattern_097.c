/* 097 Magenta Fireworks — port of lab/patterns/097_magenta_fireworks/proto.py
 * Radial particle bursts pop every nine frames over a hot magenta flood; the
 * thin trails droop under gravity and stay forever, so the field slowly
 * saturates into a dense multicolour fibre wash. The only asymmetric routine.
 * Accumulator: flood when sl < 2, then extend every live burst by one step. */
#include "../jellydazzle.h"
#include <math.h>

#define P97_LW 640
#define P97_LH 480
#define P97_N  (P97_LW * P97_LH)
#define P97_NP 46
#define P97_MAXB 16
#define P97_LIFE 55
#define P97_SPAWN 9

static uint32_t p97_low[P97_N];
static float    p97_hue[256][3];

static struct {
    float cx, cy;
    float ca[P97_NP], sa[P97_NP], v[P97_NP];
    uint32_t c1, c2;
    int age, live;
} p97_b[P97_MAXB];

static int      p97_inited = -1;
static uint32_t p97_seed;
static int      p97_next;

static uint32_t p97_rs;
static uint32_t p97_rnd(void)
{
    p97_rs ^= p97_rs << 13; p97_rs ^= p97_rs >> 17; p97_rs ^= p97_rs << 5;
    return p97_rs;
}
static float p97_r01(void) { return (float)(p97_rnd() >> 8) * (1.0f / 16777216.0f); }

static void p97_buildhue(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i << 7) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255);
        float mx = r > g ? r : g, mn = r < g ? r : g, k;
        if (b > mx) mx = b; if (b < mn) mn = b;
        if (mx < 20.0f) mx = 20.0f;
        k = (mn / mx) * 0.75f;
        p97_hue[i][0] = (r / mx - k) / (1.0f - k);
        p97_hue[i][1] = (g / mx - k) / (1.0f - k);
        p97_hue[i][2] = (b / mx - k) / (1.0f - k);
    }
}

static uint32_t p97_col(int hi, float v)
{
    int r = (int)(p97_hue[hi & 255][0] * v);
    int g = (int)(p97_hue[hi & 255][1] * v);
    int b = (int)(p97_hue[hi & 255][2] * v);
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static void p97_flood(void)
{
    int i;
    uint32_t c = 0xFFB90876u;                     /* hot magenta ground */
    for (i = 0; i < P97_N; i++) p97_low[i] = c;
    for (i = 0; i < P97_MAXB; i++) p97_b[i].live = 0;
    p97_next = 0;
}

static void p97_spawn(int k)
{
    int slot = -1, i;
    for (i = 0; i < P97_MAXB; i++)
        if (!p97_b[i].live) { slot = i; break; }
    if (slot < 0) slot = k % P97_MAXB;
    p97_rs = ((uint32_t)k * 2654435761u) ^ p97_seed ^ 0x9E3779B9u;
    if (!p97_rs) p97_rs = 1u;
    p97_rnd(); p97_rnd();
    p97_b[slot].cx = 25.0f + p97_r01() * 270.0f;
    p97_b[slot].cy = 18.0f + p97_r01() * (240.0f * 0.66f - 18.0f);
    for (i = 0; i < P97_NP; i++) {
        float a = p97_r01() * 6.28318531f;
        p97_b[slot].ca[i] = cosf(a);
        p97_b[slot].sa[i] = sinf(a);
        p97_b[slot].v[i] = 0.9f + p97_r01() * 1.4f;
    }
    p97_b[slot].c1 = p97_col((int)(p97_r01() * 256.0f), 255.0f);
    p97_b[slot].c2 = p97_col((int)(p97_r01() * 256.0f), 255.0f);
    p97_b[slot].age = 0;
    p97_b[slot].live = 1;
}

static void p97_plot(float px, float py, uint32_t c)
{
    int ix, iy;
    if (px < 0.0f) px = 0.0f; else if (px > 319.0f) px = 319.0f;
    if (py < 0.0f) py = 0.0f; else if (py > 239.0f) py = 239.0f;
    ix = (int)(px * 2.0f); iy = (int)(py * 2.0f);
    if (ix > P97_LW - 1) ix = P97_LW - 1;
    if (iy > P97_LH - 1) iy = P97_LH - 1;
    p97_low[(long)iy * P97_LW + ix] = c;
}

static void p97_step(int bi)
{
    int i;
    float s = (float)p97_b[bi].age;
    float so = s - 4.0f;
    float dec = s * (1.0f - s * 0.004f);
    float deco = so * (1.0f - so * 0.004f);
    float gy = 0.018f * s * s, gyo = 0.018f * so * so;
    for (i = 0; i < P97_NP; i++) {
        uint32_t base = (i & 1) ? p97_b[bi].c2 : p97_b[bi].c1;
        float vv = p97_b[bi].v[i];
        uint32_t tip = 0xFF000000u
            | ((((base >> 16) & 255) * 90 / 256 + 165) << 16)
            | ((((base >> 8) & 255) * 90 / 256 + 165) << 8)
            |  (((base & 255) * 90 / 256) + 165);
        if (so >= 0.0f)
            p97_plot(p97_b[bi].cx + p97_b[bi].ca[i] * vv * deco,
                     p97_b[bi].cy + p97_b[bi].sa[i] * vv * deco + gyo, base);
        p97_plot(p97_b[bi].cx + p97_b[bi].ca[i] * vv * dec,
                 p97_b[bi].cy + p97_b[bi].sa[i] * vv * dec + gy, tip);
    }
}

void pattern_097(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i, x, y;
    uint32_t stepx;
    (void)frame;

    p97_buildhue(pal);
    if (sl < 2 || p97_inited < 0 || seed != p97_seed) {
        p97_seed = seed;
        p97_flood();
        p97_inited = 1;
    }
    if ((sl % P97_SPAWN) == 0) p97_spawn(p97_next++);
    for (i = 0; i < P97_MAXB; i++) {
        if (!p97_b[i].live) continue;
        p97_step(i);
        p97_b[i].age++;
        if (p97_b[i].age > P97_LIFE) p97_b[i].live = 0;
    }

    stepx = ((uint32_t)P97_LW << 16) / (uint32_t)w;
    for (y = 0; y < h; y++) {
        int sy = (int)(((long)y * P97_LH) / h);
        const uint32_t *src = p97_low + (long)sy * P97_LW;
        uint32_t *dst = fb + (long)y * w;
        uint32_t acc = 0;
        for (x = 0; x < w; x++) { dst[x] = src[acc >> 16]; acc += stepx; }
    }
}
