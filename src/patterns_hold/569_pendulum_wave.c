/* 569 Pendulum Wave — fifteen pendulums in a row, each a little shorter
 * than the last so its period is a little quicker; started together they
 * drift into travelling waves, split into two groups, scramble, and fall
 * back into line on a long cycle.  Bobs carry a hue that runs along the row
 * and drifts.  Figure overlay, repaint. */
#include "_fig541.h"

#define NP569 15
static gk g569;

void pattern_569(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g569, w, h);
    gk_clear(&g569);
    float cw = (float)g569.cw, ch = (float)g569.ch, sc = g569.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i;
    float hb0 = fg_pick_sat(pal, gk_hash(seed + 4u) * 32768.0f, 6000.0f), hdrift = 1000.0f * sinf(t * 0.003f);
    float hb = hb0 + hdrift;
    float A = 0.30f + 0.10f * gk_hash(seed + 2u);
    float T0 = 2600.0f;                                  /* full realignment cycle, frames */
    float base = 20.0f + 4.0f * gk_hash(seed + 3u);       /* oscillations of the longest per cycle */
    float pivy = ch * 0.06f;
    float c[3];
    /* pivot beam */
    fg_colv(pal, hb + 7000.0f, 1.2f, amp * 0.4f, c);
    gk_seg(&g569, cw * 0.06f, pivy, cw * 0.94f, pivy, c, 1.6f * sc, 5.0f * sc, 0.3f);
    for (i = 0; i < NP569; i++) {
        float pivx = cw * (0.10f + 0.80f * (float)i / (float)(NP569 - 1));
        float n = base + (float)i;                        /* oscillations per cycle */
        float om = GK_TAU * n / T0;
        float L = ch * 0.78f * (base * base) / (n * n);   /* T ~ sqrt(L) */
        float th = A * sinf(t * om + gk_hash(seed) * 0.5f);
        float x = pivx + sinf(th) * L, y = pivy + cosf(th) * L;
        float hue = fg_pick_sat(pal, hb0 + (float)i * 900.0f, 3000.0f) + hdrift;
        float R = 7.0f * sc;
        /* rod */
        fg_colv(pal, hue + 5000.0f, 1.1f, amp * 0.35f, c);
        gk_seg(&g569, pivx, pivy, x, y, c, 0.6f * sc, 1.6f * sc, 0.25f);
        /* bob */
        fg_colv(pal, hue, 1.5f, amp * 0.55f, c);
        gk_disc(&g569, x, y, R, c);
        fg_colv(pal, hue + 1500.0f, 1.4f, amp * 0.35f, c);
        gk_dot(&g569, x, y, c, R * 0.8f, R * 2.6f, 0.4f);
        gk_col(pal, (int)(hue + 3000.0f), 0.3f, amp * 0.3f, c);
        gk_dot(&g569, x - R * 0.3f, y - R * 0.3f, c, R * 0.22f, R * 0.5f, 0.3f);
        /* pivot pin */
        fg_colv(pal, hue + 5000.0f, 1.1f, amp * 0.5f, c);
        gk_dot(&g569, pivx, pivy, c, 1.0f * sc, 2.5f * sc, 0.3f);
    }
    gk_present(&g569, fb, w, h);
}
