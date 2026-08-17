/* 559 Planetary Set — a sun gear drives three planets inside a fixed ring
 * gear; the carrier turns at n_s/(n_s+n_r) of the sun, exactly as an
 * epicyclic must (the ring phase is solved through the mesh chain every
 * frame and stays put).  Per-tooth hues drift; colour flows sun -> planets
 * -> ring and back.  Figure overlay, repaint. */
#include "_fig541.h"

static gk g559;
static fg_gear sun559, pl559[3], ring559;
static uint32_t s559 = 0xFFFFFFFFu;
static float ph559;

void pattern_559(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g559, w, h);
    gk_clear(&g559);
    float cw = (float)g559.cw, ch = (float)g559.ch, sc = g559.sc, t = (float)frame;
    int i;
    /* n_r = n_s + 2 n_p, and (n_s + n_r) divisible by 3 for equal spacing */
    int ns = 15 + 3 * (int)(gk_hash(seed + 2u) * 3.0f);       /* 15,18,21 */
    int np = 9 + 3 * (int)(gk_hash(seed + 3u) * 3.0f);        /* 9,12,15  */
    int nr = ns + 2 * np;
    float R = (cw < ch ? cw : ch) * 0.40f;
    float mod = R / ((float)nr * 0.5f + 2.6f);
    if (seed != s559) {
        s559 = seed; ph559 = gk_hash(seed) * GK_TAU;
        fg_gear_set(&sun559, 0.0f, 0.0f, mod, ns, 1, seed, 0);
        for (i = 0; i < 3; i++) fg_gear_set(&pl559[i], 0.0f, 0.0f, mod, np, 1, seed, 1 + i);
        fg_ring_set(&ring559, 0.0f, 0.0f, mod, nr, mod * 0.9f, seed, 4);
    }
    float cx = cw * 0.5f + cw * 0.03f * sinf(t * 0.0011f + ph559);
    float cy = ch * 0.5f + ch * 0.03f * sinf(t * 0.0008f + ph559 * 2.0f);
    sun559.cx = cx; sun559.cy = cy; ring559.cx = cx; ring559.cy = cy;
    /* sun drive: slow, breathing, occasionally reversing on a long sine */
    float wv = 0.009f * sinf(t * 0.0031f + ph559);
    sun559.phase += wv;
    if (sl < 2) sun559.phase = ph559;
    float carrier = ph559 * 0.5f + sun559.phase * (float)ns / (float)(ns + nr);
    for (i = 0; i < 3; i++) {
        float th = carrier + GK_TAU * (float)i / 3.0f;
        fg_place(&sun559, &pl559[i], th);
    }
    fg_mesh(&pl559[0], &ring559, carrier);   /* fixed ring: this phase is constant */
    fg_gear_colour(&sun559, pal, t, 0.012f);
    fg_gear_colour(&ring559, pal, t, 0.012f);
    for (i = 0; i < 3; i++) {
        float th = carrier + GK_TAU * (float)i / 3.0f;
        fg_gear_colour(&pl559[i], pal, t, 0.012f);
        fg_transfer(&sun559, &pl559[i], th, 0.06f);
        fg_transfer(&pl559[i], &ring559, th, 0.06f);
    }
    float amp = gk_smooth((float)sl / 60.0f);
    fg_gear_draw(&g559, &ring559, amp * 0.75f);
    fg_gear_draw(&g559, &sun559, amp);
    for (i = 0; i < 3; i++) fg_gear_draw(&g559, &pl559[i], amp);
    /* carrier arms */
    float c[3];
    gk_col(pal, (int)sun559.rb + 2000, 0.3f, amp * 0.5f, c);
    for (i = 0; i < 3; i++)
        gk_seg(&g559, cx, cy, pl559[i].cx, pl559[i].cy, c, 1.2f * sc, 3.5f * sc, 0.3f);
    for (i = 0; i < 3; i++) gk_dot(&g559, pl559[i].cx, pl559[i].cy, c, 1.6f * sc, 4.0f * sc, 0.3f);
    gk_dot(&g559, cx, cy, c, 2.0f * sc, 5.0f * sc, 0.3f);
    gk_present(&g559, fb, w, h);
}
