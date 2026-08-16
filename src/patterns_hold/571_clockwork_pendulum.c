/* 571 Clockwork Pendulum — a long pendulum swings below a train of three
 * gears and DRIVES it: the crown wheel's angle follows the swing, so the
 * whole train accelerates through the bottom, eases to a standstill at each
 * end, and reverses (IDEAS 3, made mechanical).  Ratio-locked meshes,
 * per-tooth drifting hues, colour handed along the train each half-swing.
 * Figure overlay, repaint. */
#include "_fig541.h"

#define NG571 3
static gk g571;
static fg_gear gs571[NG571];
static uint32_t s571 = 0xFFFFFFFFu;

void pattern_571(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g571, w, h);
    gk_clear(&g571);
    float cw = (float)g571.cw, ch = (float)g571.ch, sc = g571.sc, t = (float)frame;
    int i;
    if (seed != s571) {
        s571 = seed;
        float mod = (4.0f + 1.0f * gk_hash(seed + 3u)) * sc;
        static const int nt[NG571] = { 30, 12, 22 };
        for (i = 0; i < NG571; i++)
            fg_gear_set(&gs571[i], 0.0f, 0.0f, mod, nt[i] + (int)(gk_hash(seed + 10u + (uint32_t)i) * 5.0f) - 2, i == 1 ? 1 : 2, seed, i);
    }
    float amp = gk_smooth((float)sl / 60.0f);
    /* pendulum: pivot at the crown wheel's centre, long rod, big slow swing */
    float px = cw * 0.5f + cw * 0.03f * sinf(t * 0.0009f), py = ch * 0.30f;
    float L = ch * 0.58f;
    float A = 0.30f + 0.08f * gk_hash(seed + 5u);
    float om = 0.011f;                                   /* ~9.5 s per full swing */
    float th = A * sinf(t * om + gk_hash(seed) * 6.0f);
    /* the crown wheel is keyed to the pendulum: it turns with the swing (geared up) */
    gs571[0].cx = px; gs571[0].cy = py;
    gs571[0].phase = th * 3.0f + gk_hash(seed + 1u) * 6.0f;
    float a01 = -0.35f + 0.10f * sinf(t * 0.0011f), a12 = -1.5707963f - 0.6f + 0.10f * sinf(t * 0.0008f + 1.0f);
    fg_place(&gs571[0], &gs571[1], a01);
    fg_place(&gs571[1], &gs571[2], a12);
    for (i = 0; i < NG571; i++) fg_gear_colour(&gs571[i], pal, t, 0.012f);
    fg_transfer(&gs571[0], &gs571[1], a01, 0.08f);
    fg_transfer(&gs571[1], &gs571[2], a12, 0.08f);
    float c[3];
    /* frame bar */
    float hb = fg_pick_sat(pal, gk_hash(seed + 9u) * 32768.0f, 6000.0f);
    fg_colv(pal, hb + 7000.0f, 1.1f, amp * 0.35f, c);
    {
        float top = 1e9f;
        for (i = 0; i < NG571; i++) { float y = gs571[i].cy - gs571[i].r - gs571[i].m * 2.0f; if (y < top) top = y; }
        gk_seg(&g571, px - cw * 0.25f, top, px + cw * 0.25f, top, c, 1.5f * sc, 4.0f * sc, 0.3f);
    }
    for (i = 0; i < NG571; i++) fg_gear_draw(&g571, &gs571[i], amp);
    /* pendulum rod and bob */
    float bx = px + sinf(th) * L, by = py + cosf(th) * L;
    fg_colv(pal, hb + 5000.0f, 1.1f, amp * 0.55f, c);
    gk_seg(&g571, px, py, bx, by, c, 1.0f * sc, 2.6f * sc, 0.25f);
    float R = 13.0f * sc;
    fg_colv(pal, hb, 1.4f, amp * 0.6f, c);
    gk_disc(&g571, bx, by, R, c);
    fg_colv(pal, hb + 1500.0f, 1.3f, amp * 0.5f, c);
    gk_ring(&g571, bx, by, R, 1.4f * sc, c);
    gk_col(pal, (int)(hb + 3000.0f), 0.4f, amp * 0.5f, c);
    gk_dot(&g571, bx - R * 0.3f, by - R * 0.3f, c, R * 0.2f, R * 0.5f, 0.3f);
    /* faint arc of the swing */
    fg_colv(pal, hb + 2500.0f, 1.2f, amp * 0.12f, c);
    {
        float lx = 0.0f, ly = 0.0f; int k;
        for (k = 0; k <= 24; k++) {
            float a = -A + 2.0f * A * (float)k / 24.0f;
            float x = px + sinf(a) * L, y = py + cosf(a) * L;
            if (k) gk_seg(&g571, lx, ly, x, y, c, 0.6f * sc, 1.8f * sc, 0.3f);
            lx = x; ly = y;
        }
    }
    for (i = 0; i < NG571; i++) {
        gk_col(pal, (int)gs571[i].rb + 2500, 0.4f, amp * 0.6f, c);
        gk_dot(&g571, gs571[i].cx, gs571[i].cy, c, 1.5f * sc, 4.0f * sc, 0.3f);
    }
    gk_present(&g571, fb, w, h);
}
