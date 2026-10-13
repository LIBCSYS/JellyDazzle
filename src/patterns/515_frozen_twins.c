/* 515 Frozen Twins — the whole segment is one strike in extreme slow
 * motion: a downward leader creeps from the top and an upward leader from
 * the ground, each unfurling its branches over ~780..1020 frames (seed picks) as they feel
 * toward a meeting point; when they touch, the junction blooms softly and
 * a slow return stroke brightens the joined channel over ~80 frames, after
 * which the whole thing hangs in the air, breathing, hue sliding down its
 * length, until the segment ends.  sl-clocked.  Sparse overlay.  Repaint. */
#include "_trace509.h"

static gk g515;
static gk_bolt bd515, bu515;       /* down leader, up leader */
static uint32_t bs515 = 0xFFFFFFFFu;
static float mx515, my515;
static float meet515 = 900.0f;     /* frames until the leaders touch      */
static int   pal515;               /* ramp start                          */

void pattern_515(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g515, w, h);
    gk_clear(&g515);
    float cw = (float)g515.cw, ch = (float)g515.ch, sc = g515.sc, t = (float)frame;
    if (seed != bs515 || sl < 2) {
        gk_seed(&g515, seed ^ 0x5157u);
        /* 3.6.5: wider meeting-point and root ranges, plus seed-picked
         * jaggedness, branchiness, approach time and a ramp start over the
         * whole palette (it was seed & 8191, a quarter of it).  The strike
         * kept landing in the same middle band in the same colours, so
         * appearances read as one picture (variety audit). */
        mx515 = cw * (0.22f + 0.56f * gk_rf(&g515));
        my515 = ch * (0.34f + 0.32f * gk_rf(&g515));
        float jag = 0.15f + 0.12f * gk_hash(seed ^ 0x5151u);
        int   nbr = 4 + (int)(gk_hash(seed ^ 0x5152u) * 5.0f);
        meet515 = 780.0f + 240.0f * gk_hash(seed ^ 0x5153u);
        pal515  = (int)(gk_hash(seed ^ 0x5154u) * 32768.0f);
        float tx = cw * (0.10f + 0.80f * gk_rf(&g515));
        gk_bolt_gen(&g515, &bd515, tx, -ch * 0.02f, mx515, my515, jag, 7, nbr, 0.45f);
        float bx = cw * (0.10f + 0.80f * gk_rf(&g515));
        gk_bolt_gen(&g515, &bu515, bx, ch * 1.02f, mx515, my515, jag, 7, nbr, 0.45f);
        bs515 = seed;
    }
    float s = (float)sl;
    int base = (int)(t * 0.9f) + pal515;
    float prog = s / meet515;                            /* both leaders creep in */
    float ret = gk_smooth((s - meet515) / 80.0f);        /* return stroke */
    float breathe = 0.85f + 0.15f * sinf(t * 0.017f);
    float amp = 0.55f + 0.7f * ret * breathe;
    float wt = 0.35f + 0.35f * ret;
    for (int k = 0; k < 2; k++) {
        const gk_bolt *b = k ? &bu515 : &bd515;
        int pi = base + k * 3500;
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.40f * amp, h0);
        gk_col(pal, pi + 1800, 0.05f, 0.40f * amp, h1);
        gk_col(pal, pi + 300, wt, 0.65f * amp, c0);
        gk_col(pal, pi + 2100, wt, 0.65f * amp, c1);
        float thick = 0.8f + 0.5f * ret;
        bx_draw_grad(&g515, b, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, prog, 1.0f, h0, h1, 0.0f, 2.0f * sc * thick, 7.0f * sc * thick, 0.5f);
        bx_draw_grad(&g515, b, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, prog, 1.0f, c0, c1, 0.0f, 0.9f * sc * thick, 2.4f * sc * thick, 0.25f);
        /* live leader tip while creeping: find the segment at prog */
        if (prog < 1.0f) {
            for (int i = 0; i < b->n; i++) {
                const gk_bseg *sg = &b->s[i];
                if (sg->wgt < 0.99f || sg->t0 > prog || sg->t1 <= prog) continue;
                float f = (prog - sg->t0) / (sg->t1 - sg->t0);
                float x = sg->x0 + (sg->x1 - sg->x0) * f, y = sg->y0 + (sg->y1 - sg->y0) * f;
                float tc[3];
                gk_col(pal, pi + 1500, 0.6f, 0.9f, tc);
                gk_dot(&g515, x, y, tc, 1.8f * sc, 7.0f * sc, 0.6f);
                break;
            }
        }
    }
    /* junction bloom */
    float jb = gk_env(s - (meet515 - 20.0f), 40.0f, 60.0f, 400.0f) * 0.9f + 0.25f * ret * breathe;
    if (jb > 0.0f) {
        float jc[3], jh[3];
        gk_col(pal, base + 1200, 0.5f, 1.0f * jb, jc);
        gk_col(pal, base + 2400, 0.1f, 0.5f * jb, jh);
        gk_dot(&g515, mx515, my515, jh, 10.0f * sc, 40.0f * sc, 0.6f);
        gk_dot(&g515, mx515, my515, jc, 3.0f * sc, 12.0f * sc, 0.6f);
    }
    gk_present(&g515, fb, w, h);
}
