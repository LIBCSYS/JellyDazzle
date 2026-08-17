/* 560 Gear Silhouettes — four small clockwork clusters (a meshed pair or
 * triple each) float apart on the frame like parts laid out on a bench,
 * each drifting and turning at its own pace; hub holes and spokes let the
 * layers beneath show through.  Per-tooth hues drift and trade at each
 * mesh.  Figure overlay, repaint. */
#include "_fig541.h"

#define NC560 4
static gk g560;
static fg_gear gs560[NC560][3];
static int cnt560[NC560];
static float ang560[NC560][2];
static uint32_t s560 = 0xFFFFFFFFu;

void pattern_560(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g560, w, h);
    gk_clear(&g560);
    float cw = (float)g560.cw, ch = (float)g560.ch, sc = g560.sc, t = (float)frame;
    int i, j;
    if (seed != s560) {
        s560 = seed;
        for (i = 0; i < NC560; i++) {
            uint32_t s = seed + (uint32_t)i * 977u;
            float mod = (3.2f + 1.4f * gk_hash(s + 1u)) * sc;
            cnt560[i] = 2 + (gk_hash(s + 2u) < 0.5f ? 1 : 0);
            for (j = 0; j < cnt560[i]; j++) {
                int n = 8 + (int)(gk_hash(s + 10u + (uint32_t)j) * 22.0f);
                fg_gear_set(&gs560[i][j], 0.0f, 0.0f, mod, n, n > 18 ? 2 : (n > 11 ? 1 : 0), seed, i * 3 + j);
            }
            ang560[i][0] = gk_hash(s + 3u) * GK_TAU;
            ang560[i][1] = ang560[i][0] + (gk_hash(s + 4u) - 0.5f) * 1.8f;
            gs560[i][0].phase = gk_hash(s + 5u) * GK_TAU;
        }
    }
    float amp = gk_smooth((float)sl / 60.0f);
    float c[3];
    for (i = 0; i < NC560; i++) {
        uint32_t s = seed + (uint32_t)i * 977u;
        float bx = cw * (0.25f + 0.5f * (float)(i & 1)) + cw * 0.06f * sinf(t * 0.0013f + gk_hash(s + 6u) * 6.0f);
        float by = ch * (0.28f + 0.44f * (float)(i >> 1)) + ch * 0.06f * sinf(t * 0.0010f + gk_hash(s + 7u) * 6.0f);
        gs560[i][0].cx = bx; gs560[i][0].cy = by;
        float wv = (0.006f + 0.006f * gk_hash(s + 8u)) * (gk_hash(s + 9u) < 0.5f ? -1.0f : 1.0f) * (0.7f + 0.3f * sinf(t * 0.002f + (float)i));
        gs560[i][0].phase += wv;
        if (sl < 2) gs560[i][0].phase = gk_hash(s + 5u) * GK_TAU;
        float a0 = ang560[i][0] + 0.25f * sinf(t * 0.0009f + (float)i), a1 = ang560[i][1] + 0.25f * sinf(t * 0.0007f + (float)i);
        fg_place(&gs560[i][0], &gs560[i][1], a0);
        if (cnt560[i] > 2) fg_place(&gs560[i][1], &gs560[i][2], a1);
        for (j = 0; j < cnt560[i]; j++) fg_gear_colour(&gs560[i][j], pal, t, 0.014f);
        fg_transfer(&gs560[i][0], &gs560[i][1], a0, 0.07f);
        if (cnt560[i] > 2) fg_transfer(&gs560[i][1], &gs560[i][2], a1, 0.07f);
        for (j = 0; j < cnt560[i]; j++) {
            fg_gear_draw(&g560, &gs560[i][j], amp);
            gk_col(pal, (int)gs560[i][j].rb + 2500, 0.4f, amp * 0.6f, c);
            gk_dot(&g560, gs560[i][j].cx, gs560[i][j].cy, c, 1.3f * sc, 3.5f * sc, 0.3f);
        }
    }
    gk_present(&g560, fb, w, h);
}
