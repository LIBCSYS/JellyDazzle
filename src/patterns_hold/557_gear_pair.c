/* 557 Gear Pair — a big spoked wheel and a small pinion, meshed and
 * counter-rotating at the tooth ratio (wA*nA = -wB*nB).  Every tooth owns a
 * drifting palette hue and the engaged teeth trade colour at the mesh, so
 * the pinion carries the wheel's colours round and hands them back.  The
 * pair drifts slowly on the frame; the driver eases to a crawl and reverses
 * on a long sine (IDEAS 3).  Figure overlay, repaint. */
#include "_fig541.h"

static gk g557;
static fg_gear a557, b557;
static uint32_t s557 = 0xFFFFFFFFu;
static float ph557;

void pattern_557(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g557, w, h);
    gk_clear(&g557);
    float cw = (float)g557.cw, ch = (float)g557.ch, sc = g557.sc, t = (float)frame;
    float mod = (7.5f + 2.5f * gk_hash(seed + 3u)) * sc;
    int na = 30 + (int)(gk_hash(seed + 4u) * 14.0f), nb = 9 + (int)(gk_hash(seed + 5u) * 6.0f);
    if (seed != s557) {
        s557 = seed; ph557 = gk_hash(seed) * GK_TAU;
        fg_gear_set(&a557, 0.0f, 0.0f, mod, na, 2, seed, 0);
        fg_gear_set(&b557, 0.0f, 0.0f, mod, nb, 1, seed, 1);
    }
    /* slow wander of the pair */
    float ox = cw * 0.42f + cw * 0.05f * sinf(t * 0.0021f + ph557);
    float oy = ch * 0.52f + ch * 0.06f * sinf(t * 0.0017f + ph557 * 2.0f);
    a557.cx = ox; a557.cy = oy;
    /* driver: velocity is a slow sine so it slows to a crawl and reverses */
    float wv = 0.010f * sinf(t * 0.0045f + ph557 * 3.0f);
    a557.phase += wv;
    if (frame == 0 || sl < 2) a557.phase = ph557;
    float th = 0.35f * sinf(t * 0.0013f) - 0.2f;
    fg_place(&a557, &b557, th);
    fg_gear_colour(&a557, pal, t, 0.015f);
    fg_gear_colour(&b557, pal, t, 0.015f);
    fg_transfer(&a557, &b557, th, 0.06f);
    float amp = gk_smooth((float)sl / 60.0f);
    fg_gear_draw(&g557, &a557, amp);
    fg_gear_draw(&g557, &b557, amp);
    /* axle dots */
    float c[3];
    gk_col(pal, (int)a557.base + 20000, 0.3f, 0.8f * amp, c);
    gk_ring(&g557, a557.cx, a557.cy, a557.hub * 0.5f, 1.5f * sc, c);
    gk_col(pal, (int)b557.base + 20000, 0.3f, 0.8f * amp, c);
    gk_ring(&g557, b557.cx, b557.cy, b557.hub * 0.5f, 1.5f * sc, c);
    gk_present(&g557, fb, w, h);
}
