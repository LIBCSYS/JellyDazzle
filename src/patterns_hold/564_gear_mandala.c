/* 564 Gear Mandala — a sun gear with six planets meshed round it, and six
 * smaller satellites meshed to the planets further out: a 6-fold rosette
 * of clockwork, every ring counter-rotating against the one inside it at
 * its ratio.  Hue flows outward from the sun and back.  The whole rosette
 * breathes and turns very slowly.  Figure overlay, repaint. */
#include "_fig541.h"

static gk g564;
static fg_gear sun564, pl564[6], sat564[6];
static uint32_t s564 = 0xFFFFFFFFu;
static float ph564;

void pattern_564(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g564, w, h);
    gk_clear(&g564);
    float cw = (float)g564.cw, ch = (float)g564.ch, sc = g564.sc, t = (float)frame;
    int i;
    int ns = 24 + 6 * (int)(gk_hash(seed + 2u) * 2.0f);   /* 24 or 30 */
    int np = 12, nq = 8;
    float R = (cw < ch ? cw : ch) * 0.46f;
    /* outer reach = r_s + 2 r_p + 2 r_q (+tip) = mod*(ns/2 + np + nq) + tip */
    float mod = R / ((float)ns * 0.5f + (float)np + (float)nq + 1.2f);
    if (seed != s564) {
        s564 = seed; ph564 = gk_hash(seed) * GK_TAU;
        fg_gear_set(&sun564, 0.0f, 0.0f, mod, ns, 2, seed, 0);
        for (i = 0; i < 6; i++) fg_gear_set(&pl564[i], 0.0f, 0.0f, mod, np, 1, seed, 1 + i);
        for (i = 0; i < 6; i++) fg_gear_set(&sat564[i], 0.0f, 0.0f, mod, nq, 0, seed, 7 + i);
    }
    float cx = cw * 0.5f + cw * 0.02f * sinf(t * 0.0009f + ph564);
    float cy = ch * 0.5f + ch * 0.02f * sinf(t * 0.0007f + ph564 * 2.0f);
    sun564.cx = cx; sun564.cy = cy;
    float wv = 0.0045f * sinf(t * 0.0024f + ph564);      /* sun eases to a stop and reverses */
    sun564.phase += wv;
    if (sl < 2) sun564.phase = ph564;
    float rose = ph564 + 0.0006f * t;                    /* the rosette itself creeps round */
    float sway = 0.18f * sinf(t * 0.0013f);
    for (i = 0; i < 6; i++) {
        float th = rose + GK_TAU * (float)i / 6.0f;
        fg_place(&sun564, &pl564[i], th);
        fg_place(&pl564[i], &sat564[i], th + sway);
    }
    fg_gear_colour(&sun564, pal, t, 0.012f);
    for (i = 0; i < 6; i++) {
        float th = rose + GK_TAU * (float)i / 6.0f;
        fg_gear_colour(&pl564[i], pal, t, 0.012f);
        fg_gear_colour(&sat564[i], pal, t, 0.012f);
        fg_transfer(&sun564, &pl564[i], th, 0.07f);
        fg_transfer(&pl564[i], &sat564[i], th + sway, 0.07f);
    }
    float amp = gk_smooth((float)sl / 60.0f);
    fg_gear_draw(&g564, &sun564, amp);
    for (i = 0; i < 6; i++) { fg_gear_draw(&g564, &pl564[i], amp); fg_gear_draw(&g564, &sat564[i], amp); }
    float c[3];
    gk_col(pal, (int)sun564.rb + 2500, 0.4f, amp * 0.6f, c);
    gk_dot(&g564, cx, cy, c, 2.0f * sc, 5.0f * sc, 0.3f);
    for (i = 0; i < 6; i++) {
        gk_dot(&g564, pl564[i].cx, pl564[i].cy, c, 1.4f * sc, 3.5f * sc, 0.3f);
        gk_dot(&g564, sat564[i].cx, sat564[i].cy, c, 1.2f * sc, 3.0f * sc, 0.3f);
    }
    gk_present(&g564, fb, w, h);
}
