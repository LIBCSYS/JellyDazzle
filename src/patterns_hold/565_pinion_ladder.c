/* 565 Pinion Ladder — a vertical zigzag of gears from top to bottom, big
 * wheel / small pinion alternating, each mesh counter-rotating at ratio;
 * the ladder sways gently as a whole.  The pinions carry colour between
 * the wheels above and below them.  Figure overlay, repaint. */
#include "_fig541.h"

#define NG565 6
static gk g565;
static fg_gear gs565[NG565];
static uint32_t s565 = 0xFFFFFFFFu;

void pattern_565(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g565, w, h);
    gk_clear(&g565);
    float cw = (float)g565.cw, ch = (float)g565.ch, sc = g565.sc, t = (float)frame;
    int i;
    if (seed != s565) {
        s565 = seed;
        float mod = (3.3f + 0.8f * gk_hash(seed + 3u)) * sc;
        for (i = 0; i < NG565; i++) {
            int n = (i & 1) ? 9 + (int)(gk_hash(seed + 10u + (uint32_t)i) * 4.0f)
                            : 24 + (int)(gk_hash(seed + 10u + (uint32_t)i) * 10.0f);
            fg_gear_set(&gs565[i], 0.0f, 0.0f, mod, n, (i & 1) ? 1 : 2, seed, i);
        }
        gs565[0].phase = gk_hash(seed) * GK_TAU;
    }
    float side = gk_hash(seed + 4u) < 0.5f ? 1.0f : -1.0f;
    float ang[NG565];
    for (i = 0; i + 1 < NG565; i++)
        ang[i] = 1.5707963f + side * ((i & 1) ? -0.55f : 0.55f) + 0.15f * sinf(t * 0.0011f + (float)i);
    gs565[0].cx = 0.0f; gs565[0].cy = 0.0f;
    for (i = 0; i + 1 < NG565; i++) fg_place(&gs565[i], &gs565[i + 1], ang[i]);
    float mny = 1e9f, mxy = -1e9f, mx = 0.0f;
    for (i = 0; i < NG565; i++) {
        float r = gs565[i].r + gs565[i].m;
        if (gs565[i].cy - r < mny) mny = gs565[i].cy - r; if (gs565[i].cy + r > mxy) mxy = gs565[i].cy + r;
        mx += gs565[i].cx;
    }
    mx /= (float)NG565;
    float ox = cw * 0.5f - mx + cw * 0.08f * sinf(t * 0.0009f), oy = ch * 0.5f - (mny + mxy) * 0.5f;
    for (i = 0; i < NG565; i++) { gs565[i].cx += ox; gs565[i].cy += oy; }
    float wv = 0.0055f + 0.0025f * sinf(t * 0.0019f);
    gs565[0].phase += wv;
    if (sl < 2) gs565[0].phase = gk_hash(seed) * GK_TAU;
    for (i = 0; i + 1 < NG565; i++) fg_mesh(&gs565[i], &gs565[i + 1], ang[i]);
    for (i = 0; i < NG565; i++) fg_gear_colour(&gs565[i], pal, t, 0.012f);
    for (i = 0; i + 1 < NG565; i++) fg_transfer(&gs565[i], &gs565[i + 1], ang[i], 0.08f);
    float amp = gk_smooth((float)sl / 60.0f);
    float c[3];
    for (i = 0; i < NG565; i++) {
        fg_gear_draw(&g565, &gs565[i], amp);
        gk_col(pal, (int)gs565[i].rb + 2500, 0.4f, amp * 0.6f, c);
        gk_dot(&g565, gs565[i].cx, gs565[i].cy, c, 1.4f * sc, 3.5f * sc, 0.3f);
    }
    gk_present(&g565, fb, w, h);
}
