/* 554 Spinning Tops — four tops on an unseen floor, each leaning and
 * precessing slowly about its point: a cone body, a bright disc rim, a stem
 * and a smear of ground shadow.  The rim carries a spinning band of colour;
 * body panels drift.  Figure overlay, repaint. */
#include "_fig541.h"

#define NT554 4
static gk g554;

void pattern_554(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g554, w, h);
    gk_clear(&g554);
    float cw = (float)g554.cw, ch = (float)g554.ch, sc = g554.sc, t = (float)frame;
    float amp = gk_smooth((float)sl / 60.0f);
    int i, k;
    for (i = 0; i < NT554; i++) {
        uint32_t s = seed + (uint32_t)i * 337u;
        float px = cw * (0.15f + 0.7f * ((float)i + 0.5f) / (float)NT554) + cw * 0.05f * sinf(t * 0.0025f + gk_hash(s + 1u) * 6.0f);
        float py = ch * (0.55f + 0.25f * gk_hash(s + 2u)) + ch * 0.02f * sinf(t * 0.003f + (float)i);
        float H = (48.0f + 26.0f * gk_hash(s + 3u)) * sc;
        float lean = 0.18f + 0.14f * sinf(t * 0.0037f + gk_hash(s + 4u) * 6.0f);
        float prec = t * (0.006f + 0.005f * gk_hash(s + 5u)) * (gk_hash(s + 6u) < 0.5f ? -1.0f : 1.0f) + gk_hash(s + 7u) * 6.0f;
        float spin = t * (0.05f + 0.03f * gk_hash(s + 8u));
        /* axis direction on screen: leaning by lean, direction prec (foreshortened y) */
        float axx = sinf(lean) * cosf(prec), axy = -cosf(lean) + 0.0f * sinf(prec);
        float depthy = sinf(lean) * sinf(prec);          /* toward/away: shifts rim ellipse */
        axy += depthy * 0.35f;
        float hb = fg_pick_sat(pal, gk_hash(s + 9u) * 32768.0f, 6000.0f) + 900.0f * sinf(t * 0.004f + (float)i);
        float c[3];
        /* points along the axis */
        float dx = axx * H, dy = axy * H;               /* tip -> disc centre */
        float cxr = px + dx * 0.72f, cyr = py + dy * 0.72f;    /* rim centre */
        float sxx = px + dx * 1.15f, syy = py + dy * 1.15f;    /* stem top */
        float R = H * 0.42f;
        /* rim ellipse orientation: perpendicular to axis on screen; minor axis by lean+view */
        float rot = atan2f(dy, dx) + 1.5707963f;
        float minor = R * (0.32f + 0.35f * fabsf(sinf(lean) * sinf(prec)) + 0.15f);
        /* ground shadow */
        fg_colv(pal, hb + 8000.0f, 1.1f, amp * 0.10f, c);
        fg_ellipse(&g554, px + dx * 0.15f, py + H * 0.06f, R * 0.9f, R * 0.25f, 0.0f, c);
        /* cone body: tip to rim, two panels */
        float e1x = cxr + cosf(rot) * R, e1y = cyr + sinf(rot) * R;
        float e2x = cxr - cosf(rot) * R, e2y = cyr - sinf(rot) * R;
        fg_colv(pal, hb, 1.3f, amp * 0.5f, c);
        fg_tri(&g554, px, py, e1x, e1y, cxr, cyr, c);
        fg_colv(pal, hb + 1400.0f, 1.3f, amp * 0.5f, c);
        fg_tri(&g554, px, py, cxr, cyr, e2x, e2y, c);
        /* upper cap: from rim to stem base, dimmer */
        float bxx = px + dx * 0.95f, byy = py + dy * 0.95f;
        fg_colv(pal, hb + 2600.0f, 1.3f, amp * 0.36f, c);
        fg_tri(&g554, e1x, e1y, bxx, byy, cxr, cyr, c);
        fg_colv(pal, hb + 3400.0f, 1.3f, amp * 0.36f, c);
        fg_tri(&g554, e2x, e2y, cxr, cyr, bxx, byy, c);
        /* rim: ellipse with a spinning colour band (segments) */
        float cr = cosf(rot), sr = sinf(rot);
        float lx = 0.0f, ly = 0.0f;
        for (k = 0; k <= 40; k++) {
            float a = GK_TAU * (float)k / 40.0f;
            float u = cosf(a) * R, v = sinf(a) * minor;
            float x = cxr + u * cr - v * sr, y = cyr + u * sr + v * cr;
            if (k) {
                fg_colv(pal, hb + 4500.0f + 2500.0f * (0.5f + 0.5f * sinf(a * 3.0f + spin)), 1.3f, amp * 0.85f, c);
                gk_seg(&g554, lx, ly, x, y, c, 1.4f * sc, 4.0f * sc, 0.35f);
            }
            lx = x; ly = y;
        }
        /* stem and tip glints */
        fg_colv(pal, hb + 7000.0f, 1.2f, amp * 0.7f, c);
        gk_seg(&g554, bxx, byy, sxx, syy, c, 1.3f * sc, 3.0f * sc, 0.3f);
        gk_col(pal, (int)(hb + 7000.0f), 0.5f, amp * 0.9f, c);
        gk_dot(&g554, px, py, c, 1.5f * sc, 5.0f * sc, 0.3f);
        gk_dot(&g554, sxx, syy, c, 1.5f * sc, 4.0f * sc, 0.3f);
    }
    gk_present(&g554, fb, w, h);
}
