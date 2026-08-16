/* 555 Pendulums — four long pendulums hang from the top edge and swing at
 * their own periods (longer rod, slower swing), each bob a glowing sphere
 * that leaves a short soft arc of its recent path.  Colours per pendulum,
 * drifting; the swing itself is the eased motion the client likes: fast
 * through the bottom, a standstill at each end.  Figure overlay, repaint. */
#include "_fig541.h"

#define NP555 4
static gk g555;

void pattern_555(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g555, w, h);
    gk_clear(&g555);
    float cw = (float)g555.cw, ch = (float)g555.ch, sc = g555.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NP555; i++) {
        uint32_t s = seed + (uint32_t)i * 449u;
        float pivx = cw * (0.15f + 0.7f * ((float)i + 0.5f) / (float)NP555) + cw * 0.02f * sinf(t * 0.002f + (float)i);
        float pivy = ch * -0.02f;
        float L = ch * (0.45f + 0.4f * gk_hash(s + 1u));
        float A = 0.28f + 0.22f * gk_hash(s + 2u);
        float om = 0.042f / sqrtf(L / (ch * 0.5f));           /* longer = slower */
        float ph0 = gk_hash(s + 3u) * GK_TAU;
        float hb = fg_pick_sat(pal, gk_hash(s + 4u) * 32768.0f, 6000.0f) + 900.0f * sinf(t * 0.004f + (float)i);
        float R = (9.0f + 5.0f * gk_hash(s + 5u)) * sc;
        float c[3];
        /* trail: the recent arc of the bob, as a fading ribbon */
        {
            float lx = 0.0f, ly = 0.0f;
            for (k = 16; k >= 0; k--) {
                float th = A * sinf((t - (float)k * 2.5f) * om + ph0);
                float x = pivx + sinf(th) * L, y = pivy + cosf(th) * L;
                float f = 1.0f - (float)k / 17.0f;
                if (k < 16) {
                    fg_colv(pal, hb + 2000.0f + (float)k * 250.0f, 1.3f, amp * 0.3f * f * f, c);
                    gk_seg(&g555, lx, ly, x, y, c, R * 0.5f * f + 0.5f, R * 1.4f, 0.35f);
                }
                lx = x; ly = y;
            }
        }
        float th = A * sinf(t * om + ph0);
        float x = pivx + sinf(th) * L, y = pivy + cosf(th) * L;
        /* rod */
        fg_colv(pal, hb + 6000.0f, 1.1f, amp * 0.5f, c);
        gk_seg(&g555, pivx, pivy, x, y, c, 0.7f * sc, 2.0f * sc, 0.25f);
        /* pivot */
        gk_dot(&g555, pivx, pivy + 3.0f * sc, c, 1.5f * sc, 4.0f * sc, 0.3f);
        /* bob: disc + hot glint */
        fg_colv(pal, hb, 1.4f, amp * 0.55f, c);
        gk_disc(&g555, x, y, R, c);
        fg_colv(pal, hb + 1200.0f, 1.3f, amp * 0.45f, c);
        gk_ring(&g555, x, y, R, 1.2f * sc, c);
        gk_col(pal, (int)(hb + 3500.0f), 0.4f, amp * 0.5f, c);
        gk_dot(&g555, x - R * 0.3f, y - R * 0.3f, c, R * 0.2f, R * 0.5f, 0.3f);
    }
    /* beam along the top */
    float c[3];
    float hb = fg_pick_sat(pal, gk_hash(seed + 77u) * 32768.0f, 6000.0f);
    fg_colv(pal, hb, 1.2f, amp * 0.35f, c);
    gk_seg(&g555, cw * 0.08f, ch * -0.02f, cw * 0.92f, ch * -0.02f, c, 2.0f * sc, 6.0f * sc, 0.3f);
    gk_present(&g555, fb, w, h);
}
