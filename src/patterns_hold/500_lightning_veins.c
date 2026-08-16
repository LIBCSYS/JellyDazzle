/* 500 Lightning Veins — a full-frame vascular net of discharge: a fixed
 * skeleton of long trunks with recursive side veins (regenerated only when
 * the segment's seed changes, so it never pops), through which slow pulses
 * of brightness travel outward from a wandering heart.  Cells between the
 * veins stay black.  Field-density overlay.  Repaint pattern. */
#include "_hue469.h"

#define NT500 5

static gk g500;
static gk_bolt tr500[NT500];
static uint32_t bs500 = 0xFFFFFFFFu;
static float hx500, hy500;

void pattern_500(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g500, w, h);
    gk_clear(&g500);
    float cw = (float)g500.cw, ch = (float)g500.ch, sc = g500.sc, t = (float)frame;
    if (seed != bs500) {
        gk_seed(&g500, seed * 5u + 1u);
        hx500 = cw * (0.35f + 0.3f * gk_rf(&g500));
        hy500 = ch * (0.35f + 0.3f * gk_rf(&g500));
        for (int i = 0; i < NT500; i++) {
            float a = GK_TAU * ((float)i + 0.3f * gk_rf(&g500)) / (float)NT500;
            float len = (cw > ch ? cw : ch) * (0.55f + 0.25f * gk_rf(&g500));
            gk_bolt_gen(&g500, &tr500[i], hx500, hy500, hx500 + cosf(a) * len, hy500 + sinf(a) * len, 0.18f, 6, 6, 0.55f);
        }
        bs500 = seed;
    }
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    /* whole-net breathing so it never sits dead still */
    float breathe = 0.85f + 0.15f * sinf(t * 0.011f);
    for (int i = 0; i < NT500; i++) {
        const gk_bolt *b = &tr500[i];
        for (int k = 0; k < b->n; k++) {
            const gk_bseg *s = &b->s[k];
            /* pulses: brightness wave along arc-time, two wavelengths */
            float u = s->t0 * 3.0f - t * 0.006f + (float)i * 0.4f;
            float pulse = 0.5f + 0.5f * sinf(u * GK_TAU);
            pulse = pulse * pulse;
            float amp = (0.25f + 0.75f * pulse) * (0.35f + 0.65f * s->wgt) * breathe;
            int pi = base + (int)(s->t0 * 2500.0f) + (int)((1.0f - s->wgt) * 1200.0f) + i * 800;
            float c[3], hc[3];
            hk_col(pal, pi + 700, 0.05f, 0.45f * amp, hc);
            hk_col(pal, pi, 0.40f, 0.6f * amp, c);
            float ws = 0.5f + 0.5f * s->wgt;
            gk_seg(&g500, s->x0, s->y0, s->x1, s->y1, hc, 1.8f * sc * ws, 6.0f * sc * ws, 0.5f);
            gk_seg(&g500, s->x0, s->y0, s->x1, s->y1, c, 0.8f * sc * ws, 2.0f * sc * ws, 0.25f);
        }
    }
    float hc2[3];
    gk_col(pal, base + 4000, 0.5f, 1.2f * breathe, hc2);
    gk_dot(&g500, hx500, hy500, hc2, 4.0f * sc, 20.0f * sc, 0.5f);
    gk_present(&g500, fb, w, h);
}
