/* 562 Gear Cascade — seven meshed gears wind down the frame in a serpentine
 * from top-left to bottom-right, tooth counts 8..38, speeds rising as the
 * gears shrink; the driver eases to a standstill and reverses on a long
 * sine (the whole cascade turning over together is the point).  Per-tooth
 * hues drift and hand off down the chain.  Figure overlay, repaint. */
#include "_fig541.h"

#define NG562 7
static gk g562;
static fg_gear gs562[NG562];
static float an562[NG562];
static uint32_t s562 = 0xFFFFFFFFu;

void pattern_562(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g562, w, h);
    gk_clear(&g562);
    float cw = (float)g562.cw, ch = (float)g562.ch, sc = g562.sc, t = (float)frame;
    int i;
    if (seed != s562) {
        s562 = seed;
        float mod = (3.6f + 0.9f * gk_hash(seed + 3u)) * sc;
        static const int nt[NG562] = { 38, 12, 30, 9, 24, 8, 18 };
        for (i = 0; i < NG562; i++) {
            int n = nt[i] + (int)(gk_hash(seed + 10u + (uint32_t)i) * 5.0f) - 2;
            fg_gear_set(&gs562[i], 0.0f, 0.0f, mod, n, n > 20 ? 2 : (n > 11 ? 1 : 0), seed, i);
            /* serpentine: headings alternate around the down-right diagonal */
            an562[i] = 0.78f + ((i & 1) ? 0.75f : -0.75f) + (gk_hash(seed + 20u + (uint32_t)i) - 0.5f) * 0.4f;
        }
        gs562[0].phase = gk_hash(seed) * GK_TAU;
    }
    /* place chain from a start point, then centre it */
    gs562[0].cx = 0.0f; gs562[0].cy = 0.0f;
    for (i = 0; i + 1 < NG562; i++) {
        float a = an562[i] + 0.12f * sinf(t * 0.0009f + (float)i * 1.3f);
        fg_place(&gs562[i], &gs562[i + 1], a);
    }
    float mnx = 1e9f, mxx = -1e9f, mny = 1e9f, mxy = -1e9f;
    for (i = 0; i < NG562; i++) {
        float r = gs562[i].r + gs562[i].m;
        if (gs562[i].cx - r < mnx) mnx = gs562[i].cx - r; if (gs562[i].cx + r > mxx) mxx = gs562[i].cx + r;
        if (gs562[i].cy - r < mny) mny = gs562[i].cy - r; if (gs562[i].cy + r > mxy) mxy = gs562[i].cy + r;
    }
    float ox = cw * 0.5f - (mnx + mxx) * 0.5f + cw * 0.02f * sinf(t * 0.0011f);
    float oy = ch * 0.5f - (mny + mxy) * 0.5f + ch * 0.02f * sinf(t * 0.0008f);
    for (i = 0; i < NG562; i++) { gs562[i].cx += ox; gs562[i].cy += oy; }
    /* driver: sine velocity — slows to a crawl, stops, reverses */
    float wv = 0.0075f * sinf(t * 0.0026f + gk_hash(seed + 5u) * 6.0f);
    gs562[0].phase += wv;
    if (sl < 2) gs562[0].phase = gk_hash(seed) * GK_TAU;
    for (i = 0; i + 1 < NG562; i++) {
        float a = an562[i] + 0.12f * sinf(t * 0.0009f + (float)i * 1.3f);
        fg_mesh(&gs562[i], &gs562[i + 1], a);
    }
    for (i = 0; i < NG562; i++) fg_gear_colour(&gs562[i], pal, t, 0.012f);
    for (i = 0; i + 1 < NG562; i++) {
        float a = an562[i] + 0.12f * sinf(t * 0.0009f + (float)i * 1.3f);
        fg_transfer(&gs562[i], &gs562[i + 1], a, 0.08f);
    }
    float amp = gk_smooth((float)sl / 60.0f);
    float c[3];
    for (i = 0; i < NG562; i++) {
        fg_gear_draw(&g562, &gs562[i], amp);
        gk_col(pal, (int)gs562[i].rb + 2500, 0.4f, amp * 0.6f, c);
        gk_dot(&g562, gs562[i].cx, gs562[i].cy, c, 1.4f * sc, 3.5f * sc, 0.3f);
    }
    gk_present(&g562, fb, w, h);
}
