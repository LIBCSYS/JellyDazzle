/* 491 Jacob's Ladder — two diverging rails rise from the bottom of the
 * frame; an arc strikes across the narrow gap, climbs, stretching as the
 * rails part, and thins out at the top while the next arc has already
 * struck below.  The arc's jag re-forms on a slow crossfade as it climbs.
 * Rails glow faintly.  Figure overlay, centre column.  Repaint pattern. */
#include "_hue469.h"

#define P491 190          /* climb period per arc */
#define NA491 2
#define PJ491 60          /* re-jag clock */

static gk g491;
static gk_bolt b491[NA491][2];
static int bi491[NA491][2];
static uint32_t bs491 = 0xFFFFFFFFu;

void pattern_491(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g491, w, h);
    gk_clear(&g491);
    if (seed != bs491) { memset(bi491, 0xFF, sizeof bi491); bs491 = seed; }
    float cw = (float)g491.cw, ch = (float)g491.ch, sc = g491.sc, t = (float)frame;
    float cx = cw * 0.5f;
    float ybot = ch * 0.92f, ytop = ch * 0.06f;
    float gbot = cw * 0.02f, gtop = cw * 0.30f;      /* half-gap at bottom / top */
    int base = (int)(t * 1.6f) + (int)(seed & 8191u);
    /* rails */
    float rc[3];
    gk_col(pal, base + 5000, 0.2f, 0.35f, rc);
    gk_seg(&g491, cx - gbot, ybot, cx - gtop, ytop, rc, 1.2f * sc, 4.0f * sc, 0.4f);
    gk_seg(&g491, cx + gbot, ybot, cx + gtop, ytop, rc, 1.2f * sc, 4.0f * sc, 0.4f);
    for (int a = 0; a < NA491; a++) {
        int ph = frame + a * (P491 / NA491);
        int idx = ph / P491;
        float age = (float)(ph - idx * P491);
        float u = age / (float)P491;                    /* 0 bottom .. 1 top */
        u = u * u * (0.6f + 0.4f * u) ;                  /* accelerates upward */
        float y = ybot + (ytop - ybot) * u;
        float gap = gbot + (gtop - gbot) * u;
        float env = gk_smooth(age / 12.0f) * gk_smooth((1.0f - u) * 6.0f);
        int pi = base + idx * 700 + a * 2000 + (int)(age * 10.0f);
        hk_style st;                      /* hue rail -> rail across the arc, warms as it climbs */
        hk_style_set(&st, 3000, 1000, 800,
                     0.55f * env, 1.8f * sc, 6.0f * sc, 0.5f,
                     0.40f, 0.85f * env, 0.8f * sc, 2.0f * sc, 0.25f);
        /* the arc bows upward: draw in unit space then shear via rot 0 and a
         * lift proportional to gap (jag handles the bow) */
        for (int q = 0; q < 2; q++) {
            int ph2 = frame + q * (PJ491 / 2) + a * 17;
            int j = ph2 / PJ491;
            float jage = (float)(ph2 - j * PJ491);
            if (bi491[a][q] != j) {
                gk_seed(&g491, seed ^ (uint32_t)(j * 2731 + a * 911 + q * 137));
                gk_bolt_gen(&g491, &b491[a][q], 0.0f, 0.0f, 1.0f, 0.0f, 0.18f, 5, 2, 0.35f);
                /* bow: shift every point up by a parabola */
                gk_bolt *b = &b491[a][q];
                for (int i = 0; i < b->n; i++) {
                    float m0 = b->s[i].x0, m1 = b->s[i].x1;
                    b->s[i].y0 -= 0.5f * (1.0f - (2.0f * m0 - 1.0f) * (2.0f * m0 - 1.0f));
                    b->s[i].y1 -= 0.5f * (1.0f - (2.0f * m1 - 1.0f) * (2.0f * m1 - 1.0f));
                }
                bi491[a][q] = j;
            }
            float je = gk_env(jage, 18.0f, 12.0f, 18.0f) * 1.15f;
            if (je <= 0.0f) continue;
            hk_bolt_xf(&g491, &b491[a][q], cx - gap, y, 0.0f, 2.0f * gap, 2.0f, je, pal, pi, &st);
        }
        /* contact points */
        float cp[3];
        gk_col(pal, pi, 0.5f, 0.8f * env, cp);
        gk_dot(&g491, cx - gap, y, cp, 1.5f * sc, 5.0f * sc, 0.5f);
        gk_col(pal, pi + 3000, 0.5f, 0.8f * env, cp);
        gk_dot(&g491, cx + gap, y, cp, 1.5f * sc, 5.0f * sc, 0.5f);
    }
    gk_present(&g491, fb, w, h);
}
