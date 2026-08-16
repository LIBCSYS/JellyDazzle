/* 558 Gear Train — five meshed gears in a gentle serpentine across the
 * frame, tooth counts 9..40, every mesh counter-rotating at its ratio.
 * Per-tooth drifting hues; colour handed from tooth to tooth at every mesh
 * so a tint injected at the driver walks the whole train.  The driver's
 * speed breathes (never stops).  Figure overlay, repaint. */
#include "_fig541.h"

#define NG558 5
static gk g558;
static fg_gear gs558[NG558];
static float th558[NG558];
static uint32_t s558 = 0xFFFFFFFFu;

void pattern_558(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g558, w, h);
    gk_clear(&g558);
    float cw = (float)g558.cw, ch = (float)g558.ch, sc = g558.sc, t = (float)frame;
    int i;
    if (seed != s558) {
        s558 = seed;
        float mod = (4.3f + 1.3f * gk_hash(seed + 3u)) * sc;
        static const int nt[NG558] = { 34, 11, 26, 9, 40 };
        for (i = 0; i < NG558; i++) {
            int n = nt[i] + (int)(gk_hash(seed + 10u + (uint32_t)i) * 5.0f) - 2;
            fg_gear_set(&gs558[i], 0.0f, 0.0f, mod, n, n > 20 ? 2 : (n > 12 ? 1 : 0), seed, i);
            th558[i] = (gk_hash(seed + 20u + (uint32_t)i) - 0.5f) * 0.9f;   /* mesh direction wobble */
        }
        gs558[0].phase = gk_hash(seed) * GK_TAU;
    }
    /* layout: gear 0 at left, each next placed along +x with a wobble angle */
    float sumr = 0.0f;
    for (i = 0; i + 1 < NG558; i++) sumr += (gs558[i].r + gs558[i + 1].r) * cosf(th558[i]);
    float x0 = cw * 0.5f - sumr * 0.5f + cw * 0.03f * sinf(t * 0.0013f);
    float y0 = ch * 0.5f + ch * 0.05f * sinf(t * 0.0009f + 1.0f);
    gs558[0].cx = x0; gs558[0].cy = y0;
    for (i = 0; i + 1 < NG558; i++) {
        float ang = th558[i] * (0.6f + 0.4f * sinf(t * 0.0011f + (float)i));
        if (i & 1) ang = -ang;
        fg_place(&gs558[i], &gs558[i + 1], ang);
    }
    /* keep the chain vertically centred */
    float my = 0.0f; for (i = 0; i < NG558; i++) my += gs558[i].cy; my /= (float)NG558;
    for (i = 0; i < NG558; i++) gs558[i].cy += y0 - my;
    /* driver */
    float wv = 0.0055f + 0.0035f * sinf(t * 0.0023f);
    gs558[0].phase += wv;
    if (sl < 2) gs558[0].phase = gk_hash(seed) * GK_TAU;
    for (i = 0; i + 1 < NG558; i++) {
        float ang = th558[i] * (0.6f + 0.4f * sinf(t * 0.0011f + (float)i));
        if (i & 1) ang = -ang;
        fg_mesh(&gs558[i], &gs558[i + 1], ang);
    }
    for (i = 0; i < NG558; i++) fg_gear_colour(&gs558[i], pal, t, 0.012f);
    for (i = 0; i + 1 < NG558; i++) {
        float ang = th558[i] * (0.6f + 0.4f * sinf(t * 0.0011f + (float)i));
        if (i & 1) ang = -ang;
        fg_transfer(&gs558[i], &gs558[i + 1], ang, 0.07f);
    }
    float amp = gk_smooth((float)sl / 60.0f);
    for (i = 0; i < NG558; i++) fg_gear_draw(&g558, &gs558[i], amp);
    /* axle pins */
    float c[3];
    for (i = 0; i < NG558; i++) {
        gk_col(pal, (int)gs558[i].rb + 3000, 0.4f, amp * 0.7f, c);
        gk_dot(&g558, gs558[i].cx, gs558[i].cy, c, 1.5f * sc, 4.0f * sc, 0.3f);
    }
    gk_present(&g558, fb, w, h);
}
