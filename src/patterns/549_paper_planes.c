/* 549 Paper Planes — dart-fold planes glide across on long shallow sine
 * paths, banking with the curve; each wing is its own palette offset and
 * the fold line catches light.  A plane enters dim from one side and fades
 * before the other.  Figure overlay, repaint. */
#include "_fig541.h"

#define NP549 5
#define P549 640.0f
static gk g549;

void pattern_549(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g549, w, h);
    gk_clear(&g549);
    float cw = (float)g549.cw, ch = (float)g549.ch, sc = g549.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i;
    for (i = 0; i < NP549; i++) {
        float ph = t + (float)i * (P549 / (float)NP549) + gk_hash(seed + (uint32_t)i) * 60.0f;
        int cyc = (int)floorf(ph / P549);
        float age = ph - (float)cyc * P549;
        uint32_t s = seed + (uint32_t)i * 419u + (uint32_t)cyc * 2039u;
        float life = fg_life(age, P549, 100.0f) * amp;
        if (life <= 0.0f) continue;
        float dir = gk_hash(s + 1u) < 0.5f ? 1.0f : -1.0f;
        float u = age / P549;
        float x = dir > 0.0f ? cw * (-0.1f + 1.2f * u) : cw * (1.1f - 1.2f * u);
        float y0 = ch * (0.15f + 0.7f * gk_hash(s + 2u));
        float wave = ch * (0.05f + 0.07f * gk_hash(s + 3u));
        float wf = 0.010f + 0.008f * gk_hash(s + 4u);
        float y = y0 + wave * sinf(age * wf) + ch * 0.06f * u;   /* slow sink */
        /* heading = tangent of the path */
        float vx = dir * cw * 1.2f / P549, vy = wave * wf * cosf(age * wf) + ch * 0.06f / P549;
        float hd = atan2f(vy, vx);
        float bank = 0.35f * sinf(age * wf + 1.2f);
        float ca = cosf(hd), sa = sinf(hd);
        float L = (38.0f + 20.0f * gk_hash(s + 5u)) * sc;
        float hb = fg_pick_sat(pal, gk_hash(s + 6u) * 32768.0f, 6000.0f) + 900.0f * sinf(t * 0.004f + (float)i);
        float c[3];
        /* local: nose at (L,0), tail at (-L*0.6, +-W); wing spans by bank */
        float W1 = L * 0.42f * (1.0f + bank), W2 = L * 0.42f * (1.0f - bank);
        float nx, ny, t1x, t1y, t2x, t2y, bx, by;
        fg_xf(L, 0.0f, ca, sa, x, y, &nx, &ny);
        fg_xf(-L * 0.6f, -W1, ca, sa, x, y, &t1x, &t1y);
        fg_xf(-L * 0.6f, W2, ca, sa, x, y, &t2x, &t2y);
        fg_xf(-L * 0.45f, 0.0f, ca, sa, x, y, &bx, &by);
        fg_colv(pal, hb, 1.3f, life * 0.55f * (0.75f + 0.25f * bank), c);
        fg_tri(&g549, nx, ny, t1x, t1y, bx, by, c);
        fg_colv(pal, hb + 2600.0f, 1.3f, life * 0.55f * (0.75f - 0.25f * bank), c);
        fg_tri(&g549, nx, ny, bx, by, t2x, t2y, c);
        /* fold line + edges */
        fg_colv(pal, hb + 5000.0f, 1.2f, life * 0.8f, c);
        gk_seg(&g549, nx, ny, bx, by, c, 0.9f * sc, 2.5f * sc, 0.3f);
        fg_colv(pal, hb + 1300.0f, 1.2f, life * 0.5f, c);
        gk_seg(&g549, nx, ny, t1x, t1y, c, 0.7f * sc, 2.0f * sc, 0.3f);
        gk_seg(&g549, nx, ny, t2x, t2y, c, 0.7f * sc, 2.0f * sc, 0.3f);
        /* faint slipstream */
        int k;
        for (k = 1; k <= 8; k++) {
            float f = 1.0f - (float)k / 9.0f;
            fg_colv(pal, hb + 4000.0f + (float)k * 300.0f, 1.2f, life * 0.25f * f, c);
            gk_dot(&g549, bx - ca * (float)k * L * 0.35f, by - sa * (float)k * L * 0.35f, c, 1.2f * sc, 4.0f * sc, 0.3f);
        }
    }
    gk_present(&g549, fb, w, h);
}
