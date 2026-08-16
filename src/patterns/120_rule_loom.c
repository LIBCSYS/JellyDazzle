/* 120 Rule Loom — elementary cellular automata, woven slowly.
 * A 512-cell toroidal row is stepped by a Wolfram elementary rule (the eight
 * bits of the rule number are looked up by the 3-cell neighbourhood) and the
 * history is stacked as cloth. The loom advances one row every eighteen frames
 * and the stack is drawn with a fractional vertical offset, so the fabric
 * creeps rather than steps. The rule changes every 90 rows, and because a new
 * rule only affects rows entering at the top the weave changes character —
 * Sierpinski triangles to woven diagonals to nested combs — while the older
 * cloth below is undisturbed. Live cells take a hue that drifts along both the
 * row and the column, dead cells a dark version of the same, so the whole bolt
 * of cloth also slowly changes colour. Dense and structured: a ground layer.
 */
#include "../engine/jellydazzle.h"
#include "_upsample.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
static jd_up p120_up;

#define P120_W 512
#define P120_ROWS 512
#define P120_LH 384

static unsigned char p120_row[P120_ROWS][P120_W];
static int p120_head, p120_init, p120_made, p120_rule = 90;
static float p120_ph;
static int p120_lastf = -1;
static unsigned char p120_img[P120_W * P120_LH * 3];
static float p120_ramp[256][3];
static uint32_t p120_rs = 0x5EED1234u;

/* every rule here maps the empty neighbourhood to 0: a rule that does not
 * (105, for one) inverts the whole row each generation, and a cloth of
 * alternating rows blows the motion budget the moment it scrolls */
static const int p120_rules[8] = { 90, 150, 18, 22, 60, 26, 146, 182 };

static uint32_t p120_rnd(void)
{
    p120_rs ^= p120_rs << 13; p120_rs ^= p120_rs >> 17; p120_rs ^= p120_rs << 5;
    return p120_rs;
}

static void p120_ramp_build(const uint32_t *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t u = pal[(i * 128) & JD_PAL_MASK];
        float r = (float)((u >> 16) & 255), g = (float)((u >> 8) & 255);
        float b = (float)(u & 255), mx = r > g ? r : g;
        if (b > mx) mx = b;
        if (mx < 8.0f) mx = 8.0f;
        p120_ramp[i][0] = r / mx; p120_ramp[i][1] = g / mx; p120_ramp[i][2] = b / mx;
    }
}

static void p120_step(void)
{
    const unsigned char *src = p120_row[p120_head];
    unsigned char *dst;
    int i, alive = 0;
    p120_head = (p120_head + 1) & (P120_ROWS - 1);
    dst = p120_row[p120_head];
    for (i = 0; i < P120_W; i++) {
        int l = src[(i - 1) & (P120_W - 1)];
        int c = src[i];
        int r = src[(i + 1) & (P120_W - 1)];
        int n = (l << 2) | (c << 1) | r;
        dst[i] = (unsigned char)((p120_rule >> n) & 1);
        alive += dst[i];
    }
    /* a dead or saturated loom is re-seeded with a single thread */
    if (alive == 0 || alive == P120_W)
        dst[p120_rnd() % (unsigned)P120_W] ^= 1;
    p120_made++;
    if (p120_made % 90 == 0)
        p120_rule = p120_rules[(p120_made / 90) & 7];
}

void pattern_120(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    float t = (float)frame;
    float sp = (float)(seed & 1023) * 0.006136f;
    int x, y, i, c, hbase;
    (void)sl;

    p120_ramp_build(pal);
    if (!p120_init) {
        memset(p120_row, 0, sizeof p120_row);
        p120_row[0][P120_W / 2] = 1;
        p120_head = 0;
        p120_init = 1;
        for (i = 0; i < P120_ROWS - 2; i++) p120_step();
    }
    if (p120_lastf != frame) {
        p120_lastf = frame;
        p120_ph += 0.055f;                 /* one new row every ~18 frames */
        while (p120_ph >= 1.0f) { p120_ph -= 1.0f; p120_step(); }
    }
    /* TEMPORAL REVIEW 2.4.0 (docs/review/04_pattern_temporal.md, F-120):
     * hbase steps one ramp entry every ~28 frames, and the whole cloth's
     * colour used to snap with it (metronomic delta 1.9 on a 0.75 median).
     * Carry the fraction and lerp between the two ramp entries so the hue
     * glides through each step. */
    float hbf = t * 0.036f + sp * 30.0f;
    hbase = (int)hbf;
    float hfr = hbf - (float)hbase;

    for (y = 0; y < P120_LH; y++) {
        /* age of the row shown at this scanline; the +1-ph term is what makes
         * the step of the loom and the sub-row scroll cancel exactly */
        float sy = (float)y + 1.0f - p120_ph;
        int r0 = (int)sy;
        float fr = sy - (float)r0;
        int i0 = (p120_head - r0) & (P120_ROWS - 1);
        int i1 = (p120_head - r0 - 1) & (P120_ROWS - 1);
        const unsigned char *a = p120_row[i0], *b = p120_row[i1];
        unsigned char *dst = p120_img + (size_t)y * P120_W * 3;
        int hy = hbase + (int)((float)(p120_made - r0) * 0.42f);
        for (x = 0; x < P120_W; x++) {
            float va = a[x] ? 1.0f : 0.34f;
            float vb = b[x] ? 1.0f : 0.34f;
            float v = va + (vb - va) * fr;
            int hk = hy + (int)((float)x * 0.30f);
            const float *c0 = p120_ramp[hk & 255];
            const float *c1 = p120_ramp[(hk + 1) & 255];
            for (c = 0; c < 3; c++) {
                float q = (c0[c] + (c1[c] - c0[c]) * hfr) * v * 235.0f;
                dst[x * 3 + c] = q >= 255.0f ? 255 : (unsigned char)q;
            }
        }
    }

    jd_up_blit(&p120_up, fb, w, h, p120_img, P120_W, P120_LH);
}
