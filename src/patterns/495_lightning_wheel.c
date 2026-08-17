/* 495 Lightning Wheel — a slowly turning wheel whose spokes are bolts:
 * seven jagged spokes from a bright hub to a faint rim, each spoke re-
 * jagging on its own slow crossfade, and a charge that runs round the rim
 * lighting spokes as it passes.  Figure overlay, transparent outside the
 * rim.  Repaint pattern. */
#include "_hue469.h"

#define NS495 7
#define P495 130

static gk g495;
static gk_bolt sp495[NS495][2];
static int si495[NS495][2];
static uint32_t ss495 = 0xFFFFFFFFu;

void pattern_495(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g495, w, h);
    gk_clear(&g495);
    if (seed != ss495) { memset(si495, 0xFF, sizeof si495); ss495 = seed; }
    float cw = (float)g495.cw, ch = (float)g495.ch, sc = g495.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.5f;
    float R = (cw < ch ? cw : ch) * 0.40f;
    float rot = t * 0.0028f;
    int base = (int)(t * 1.7f) + (int)(seed & 8191u);
    float rim[3];
    gk_col(pal, base + 4000, 0.15f, 0.25f, rim);
    gk_ring(&g495, cx, cy, R, 2.0f * sc, rim);
    float charge = fmodf(t * 0.006f, 1.0f);              /* position around rim, 0..1 */
    for (int s = 0; s < NS495; s++) {
        float u = (float)s / (float)NS495;
        float ang = rot + u * GK_TAU;
        float d = fabsf(fmodf(u - charge + 1.5f, 1.0f) - 0.5f);   /* 0..0.5 distance around */
        float lit = expf(-d * d * 60.0f);
        float amp = 0.35f + 0.9f * lit;
        int pi = base + s * 900;
        hk_style st;                                  /* hue hub -> rim */
        hk_style_set(&st, 3500, 1200, 700,
                     0.4f * amp, 1.8f * sc, 6.0f * sc, 0.5f,
                     0.40f, 0.55f * amp, 0.8f * sc, 2.0f * sc, 0.25f);
        for (int q = 0; q < 2; q++) {
            int ph = frame + q * (P495 / 2) + s * 19;
            int idx = ph / P495;
            float age = (float)(ph - idx * P495);
            if (si495[s][q] != idx) {
                gk_seed(&g495, seed ^ (uint32_t)(idx * 3557 + s * 733 + q * 97));
                gk_bolt_gen(&g495, &sp495[s][q], 0.08f, 0.0f, 1.0f, 0.0f, 0.12f, 5, 2, 0.3f);
                si495[s][q] = idx;
            }
            float env = gk_env(age, 38.0f, 26.0f, 38.0f) * 1.15f;
            if (env <= 0.0f) continue;
            hk_bolt_xf(&g495, &sp495[s][q], cx, cy, ang, R, 2.0f, env, pal, pi + (int)(age * 5.0f), &st);
        }
        float rc[3];
        gk_col(pal, pi, 0.4f, 0.4f + 0.8f * lit, rc);
        gk_dot(&g495, cx + cosf(ang) * R, cy + sinf(ang) * R, rc, 2.0f * sc, 7.0f * sc, 0.5f);
    }
    /* the charge itself on the rim */
    float cc[3];
    gk_col(pal, base + 2500, 0.6f, 1.2f, cc);
    gk_dot(&g495, cx + cosf(rot + charge * GK_TAU) * R, cy + sinf(rot + charge * GK_TAU) * R, cc, 2.5f * sc, 10.0f * sc, 0.6f);
    float hub[3];
    gk_col(pal, base + 1500, 0.5f, 1.4f, hub);
    gk_dot(&g495, cx, cy, hub, 4.0f * sc, 18.0f * sc, 0.5f);
    gk_present(&g495, fb, w, h);
}
