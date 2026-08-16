/* 563 Corner Train — a big gear anchored in one corner (partly off-frame)
 * drives a chain of five ever-smaller gears that curl out along an arc
 * into the picture, each spinning faster than the last (ratio-locked).
 * Colour is injected at the corner and carried outward tooth by tooth.
 * The corner changes per segment.  Figure overlay, repaint. */
#include "_fig541.h"

#define NG563 6
static gk g563;
static fg_gear gs563[NG563];
static float an563[NG563];
static uint32_t s563 = 0xFFFFFFFFu;
static int corner563;

void pattern_563(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g563, w, h);
    gk_clear(&g563);
    float cw = (float)g563.cw, ch = (float)g563.ch, sc = g563.sc, t = (float)frame;
    int i;
    if (seed != s563) {
        s563 = seed;
        corner563 = (int)(gk_hash(seed + 1u) * 4.0f) & 3;
        float mod = (3.6f + 1.0f * gk_hash(seed + 3u)) * sc;
        static const int nt[NG563] = { 64, 30, 22, 16, 12, 9 };
        for (i = 0; i < NG563; i++) {
            int n = nt[i] + (int)(gk_hash(seed + 10u + (uint32_t)i) * 5.0f) - 2;
            fg_gear_set(&gs563[i], 0.0f, 0.0f, mod, n, n > 20 ? 2 : (n > 11 ? 1 : 0), seed, i);
        }
        gs563[0].phase = gk_hash(seed) * GK_TAU;
    }
    /* corner anchor: centre sits at the corner (a quarter of the big gear visible) */
    float sx = (corner563 & 1) ? 1.0f : -1.0f, sy = (corner563 & 2) ? 1.0f : -1.0f;
    float ax = (corner563 & 1) ? cw + gs563[0].r * 0.35f : -gs563[0].r * 0.35f;
    float ay = (corner563 & 2) ? ch + gs563[0].r * 0.35f : -gs563[0].r * 0.35f;
    ax += -sx * cw * 0.02f * sinf(t * 0.0009f); ay += -sy * ch * 0.02f * sinf(t * 0.0011f);
    gs563[0].cx = ax; gs563[0].cy = ay;
    /* headings: from the corner into the frame, curling round */
    float base = atan2f(-sy, -sx);      /* toward centre */
    for (i = 0; i + 1 < NG563; i++)
        an563[i] = base + (float)(i - 2) * 0.20f * (gk_hash(seed + 5u) < 0.5f ? 1.0f : -1.0f)
                 + 0.10f * sinf(t * 0.0008f + (float)i);
    /* driver */
    float wv = 0.0035f + 0.0015f * sinf(t * 0.0021f);
    gs563[0].phase += wv * (gk_hash(seed + 6u) < 0.5f ? -1.0f : 1.0f);
    if (sl < 2) gs563[0].phase = gk_hash(seed) * GK_TAU;
    for (i = 0; i + 1 < NG563; i++) fg_place(&gs563[i], &gs563[i + 1], an563[i]);
    for (i = 0; i < NG563; i++) fg_gear_colour(&gs563[i], pal, t, 0.012f);
    for (i = 0; i + 1 < NG563; i++) fg_transfer(&gs563[i], &gs563[i + 1], an563[i], 0.08f);
    float amp = gk_smooth((float)sl / 60.0f);
    float c[3];
    for (i = 0; i < NG563; i++) {
        fg_gear_draw(&g563, &gs563[i], amp * (i == 0 ? 0.8f : 1.0f));
        gk_col(pal, (int)gs563[i].rb + 2500, 0.4f, amp * 0.6f, c);
        gk_dot(&g563, gs563[i].cx, gs563[i].cy, c, 1.4f * sc, 3.5f * sc, 0.3f);
    }
    gk_present(&g563, fb, w, h);
}
