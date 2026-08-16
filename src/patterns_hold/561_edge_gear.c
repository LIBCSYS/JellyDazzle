/* 561 Edge Gear — one huge, slow gear whose centre sits beyond a frame edge
 * so only a broad arc of rim and teeth is in view, turning at a crawl and
 * easing to reverse on a long sine.  Its many teeth each carry a drifting
 * hue, so a band of colour creeps along the edge of the picture.  Figure
 * overlay, repaint. */
#include "_fig541.h"

static gk g561;
static fg_gear big561;
static uint32_t s561 = 0xFFFFFFFFu;
static int side561;
static float ph561;

void pattern_561(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g561, w, h);
    gk_clear(&g561);
    float cw = (float)g561.cw, ch = (float)g561.ch, t = (float)frame;
    if (seed != s561) {
        s561 = seed; ph561 = gk_hash(seed) * GK_TAU;
        side561 = (int)(gk_hash(seed + 1u) * 4.0f) & 3;
        int n = 56 + (int)(gk_hash(seed + 2u) * 40.0f);
        float R = (cw > ch ? cw : ch) * (0.55f + 0.25f * gk_hash(seed + 3u));
        fg_gear_set(&big561, 0.0f, 0.0f, R * 2.0f / (float)n, n, 2, seed, 0);
        big561.rim = big561.r * 0.87f;
        big561.hub = big561.r * 0.22f;
        big561.spokes = 8 + (int)(gk_hash(seed + 4u) * 8.0f);
    }
    /* centre beyond the chosen edge; the visible arc is ~30-45% of the frame */
    float in = 0.30f + 0.08f * sinf(t * 0.0007f + ph561);
    float ex = 0.0f, ey = 0.0f;
    switch (side561) {
        case 0: ex = cw * 0.5f + cw * 0.15f * sinf(t * 0.0005f); ey = -big561.r + ch * in; break;   /* top */
        case 1: ex = cw + big561.r - cw * in; ey = ch * 0.5f + ch * 0.15f * sinf(t * 0.0005f); break; /* right */
        case 2: ex = cw * 0.5f + cw * 0.15f * sinf(t * 0.0005f); ey = ch + big561.r - ch * in; break; /* bottom */
        default: ex = -big561.r + cw * in; ey = ch * 0.5f + ch * 0.15f * sinf(t * 0.0005f); break;   /* left */
    }
    big561.cx = ex; big561.cy = ey;
    float wv = 0.0016f * sinf(t * 0.0019f + ph561 * 2.0f);   /* eases to a crawl and reverses */
    big561.phase += wv;
    if (sl < 2) big561.phase = ph561;
    fg_gear_colour(&big561, pal, t, 0.01f);
    float amp = gk_smooth((float)sl / 60.0f);
    fg_gear_draw(&g561, &big561, amp * 0.8f);
    gk_present(&g561, fb, w, h);
}
