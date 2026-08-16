/* 483 Sea Lightning — bolts over open water.  A horizon line sits at 58%;
 * bolts strike from the top toward it, and each one is mirrored below the
 * horizon as a rippled reflection: the reflected copy is drawn as a stack
 * of short horizontal slivers whose x-offset wobbles with depth, dimmer and
 * more blurred the further down.  Repaint pattern; dark sky and dark water
 * stay transparent. */
#include "_hue469.h"

#define NS483 3
#define P483 230

static gk g483;
static gk_bolt b483[NS483];
static int bi483[NS483] = { -1, -1, -1 };
static uint32_t bs483 = 0xFFFFFFFFu;
static float hue483[NS483];

void pattern_483(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g483, w, h);
    gk_clear(&g483);
    if (seed != bs483) { for (int i = 0; i < NS483; i++) bi483[i] = -1; bs483 = seed; }
    float cw = (float)g483.cw, ch = (float)g483.ch, sc = g483.sc, t = (float)frame;
    float hy = ch * 0.58f;
    int base = (int)(t * 2.0f) + (int)(seed & 8191u);
    for (int s = 0; s < NS483; s++) {
        int ph = frame + s * (P483 / NS483);
        int idx = ph / P483;
        float age = (float)(ph - idx * P483);
        if (idx != bi483[s]) {
            gk_seed(&g483, seed ^ (uint32_t)(idx * 3877 + s * 8009));
            float x0 = cw * (0.1f + 0.8f * gk_rf(&g483));
            gk_bolt_gen(&g483, &b483[s], x0, -ch * 0.02f, x0 + cw * 0.15f * gk_rs(&g483),
                        hy * (0.9f + 0.1f * gk_rf(&g483)), 0.2f, 6, 4, 0.4f);
            hue483[s] = gk_rf(&g483);
            bi483[s] = idx;
        }
        float env = gk_env(age, 10.0f, 30.0f, 70.0f);
        if (env <= 0.0f) continue;
        float prog = age / 26.0f;
        int pi = base + (int)(hue483[s] * 7000.0f) + (int)(age * 11.0f);
        hk_style st;
        hk_style_set(&st, 5000, 1800, 900,
                     0.6f * env, 2.0f * sc, 7.0f * sc, 0.5f,
                     0.40f, 1.0f * env, 0.9f * sc, 2.4f * sc, 0.25f);
        hk_bolt(&g483, &b483[s], prog, 1.0f, pal, pi, &st);
        /* reflection: every segment mirrored about hy, sheared by a wave */
        for (int i = 0; i < b483[s].n; i++) {
            const gk_bseg *sg = &b483[s].s[i];
            if (sg->t0 >= prog) continue;
            float x1 = sg->x1, y1 = sg->y1;
            if (sg->t1 > prog) { float f = (prog - sg->t0) / (sg->t1 - sg->t0); x1 = sg->x0 + (sg->x1 - sg->x0) * f; y1 = sg->y0 + (sg->y1 - sg->y0) * f; }
            float ry0 = 2.0f * hy - sg->y0, ry1 = 2.0f * hy - y1;
            if (ry0 < hy && ry1 < hy) continue;
            float d0 = (ry0 - hy) / ch, d1 = (ry1 - hy) / ch;
            float wob0 = 6.0f * sc * sinf(ry0 * 0.15f / sc + t * 0.03f) * (0.3f + d0 * 3.0f);
            float wob1 = 6.0f * sc * sinf(ry1 * 0.15f / sc + t * 0.03f) * (0.3f + d1 * 3.0f);
            float k = (0.35f + 0.65f * sg->wgt) * (0.55f - 0.9f * (d0 + d1) * 0.5f);
            if (k <= 0.0f) continue;
            /* reflection takes the same along-channel hue as the segment */
            float hc[3];
            gk_col(pal, pi + 900 + (int)(sg->t0 * 5000.0f) + (int)((1.0f - sg->wgt) * 1800.0f), 0.05f, 0.6f * env * k, hc);
            gk_seg(&g483, sg->x0 + wob0, ry0, x1 + wob1, ry1, hc, 2.5f * sc, (6.0f + 12.0f * d0) * sc, 0.6f);
        }
    }
    /* horizon glimmer */
    float hz[3];
    gk_col(pal, base + 3000, 0.1f, 0.12f, hz);
    gk_seg(&g483, 0.0f, hy, cw, hy, hz, 1.0f * sc, 3.0f * sc, 0.5f);
    gk_present(&g483, fb, w, h);
}
