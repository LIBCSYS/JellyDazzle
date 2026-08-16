/* 567 Rolling Pinions — a big sun gear turns at a crawl while two pinions
 * roll round its rim on a slow orbit, their spin solved from the mesh so
 * teeth stay interlocked at every instant (a pinion rolling round a fixed
 * wheel spins at 1 + n_s/n_p orbits per orbit).  A faint orbit ring marks
 * the path.  Colour picked up at the mesh rides round on the pinions.
 * Figure overlay, repaint. */
#include "_fig541.h"

static gk g567;
static fg_gear sun567, p567[2];
static uint32_t s567 = 0xFFFFFFFFu;
static float ph567;

void pattern_567(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g567, w, h);
    gk_clear(&g567);
    float cw = (float)g567.cw, ch = (float)g567.ch, sc = g567.sc, t = (float)frame;
    int i;
    if (seed != s567) {
        s567 = seed; ph567 = gk_hash(seed) * GK_TAU;
        float R = (cw < ch ? cw : ch) * 0.28f;
        int ns = 36 + (int)(gk_hash(seed + 2u) * 12.0f);
        float mod = R * 2.0f / (float)ns;
        fg_gear_set(&sun567, 0.0f, 0.0f, mod, ns, 2, seed, 0);
        fg_gear_set(&p567[0], 0.0f, 0.0f, mod, 9 + (int)(gk_hash(seed + 3u) * 4.0f), 1, seed, 1);
        fg_gear_set(&p567[1], 0.0f, 0.0f, mod, 12 + (int)(gk_hash(seed + 4u) * 6.0f), 1, seed, 2);
    }
    float cx = cw * 0.5f + cw * 0.03f * sinf(t * 0.0011f + ph567);
    float cy = ch * 0.5f + ch * 0.03f * sinf(t * 0.0008f + ph567 * 2.0f);
    sun567.cx = cx; sun567.cy = cy;
    float wv = 0.0018f * sinf(t * 0.0017f + ph567);          /* sun: crawl, reverse */
    sun567.phase += wv;
    if (sl < 2) sun567.phase = ph567;
    float orb0 = ph567 + t * 0.0032f + 0.3f * sinf(t * 0.0009f), orb1 = orb0 + 3.14159265f + 0.7f * sinf(t * 0.0013f + 2.0f);
    fg_place(&sun567, &p567[0], orb0);
    fg_place(&sun567, &p567[1], orb1);
    fg_gear_colour(&sun567, pal, t, 0.012f);
    for (i = 0; i < 2; i++) fg_gear_colour(&p567[i], pal, t, 0.012f);
    fg_transfer(&sun567, &p567[0], orb0, 0.08f);
    fg_transfer(&sun567, &p567[1], orb1, 0.08f);
    float amp = gk_smooth((float)sl / 60.0f);
    float c[3];
    /* orbit rings */
    for (i = 0; i < 2; i++) {
        fg_colv(pal, p567[i].rb + 1000.0f, 1.2f, amp * 0.14f, c);
        gk_ring(&g567, cx, cy, sun567.r + p567[i].r, 1.2f * sc, c);
    }
    fg_gear_draw(&g567, &sun567, amp);
    for (i = 0; i < 2; i++) fg_gear_draw(&g567, &p567[i], amp);
    /* carrier arms to the pinions */
    gk_col(pal, (int)sun567.rb + 2500, 0.4f, amp * 0.5f, c);
    for (i = 0; i < 2; i++) {
        gk_seg(&g567, cx, cy, p567[i].cx, p567[i].cy, c, 1.0f * sc, 3.0f * sc, 0.3f);
        gk_dot(&g567, p567[i].cx, p567[i].cy, c, 1.4f * sc, 3.5f * sc, 0.3f);
    }
    gk_dot(&g567, cx, cy, c, 2.0f * sc, 5.0f * sc, 0.3f);
    gk_present(&g567, fb, w, h);
}
