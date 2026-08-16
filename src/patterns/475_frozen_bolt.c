/* 475 Frozen Bolt — one enormous bolt drawn over the whole segment, a
 * crack spreading through glass in slow motion: the leader tip creeps
 * downward across ~1500 frames, its branches unfurling as it passes, and
 * everything it has drawn stays lit with a slow breathing halo, hue sliding
 * along the channel.  Segment-clocked (sl) so a fresh bolt begins each
 * segment; sparse overlay, black elsewhere.  Repaint pattern. */
#include "_hue469.h"

static gk g475;
static gk_bolt b475;
static uint32_t bs475 = 0xFFFFFFFFu;

#define GROW475 1500.0f

void pattern_475(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g475, w, h);
    gk_clear(&g475);
    float cw = (float)g475.cw, ch = (float)g475.ch, sc = g475.sc;
    if (seed != bs475) {
        gk_seed(&g475, seed * 3u + 17u);
        float x0 = cw * (0.3f + 0.4f * gk_rf(&g475));
        gk_bolt_gen(&g475, &b475, x0, -ch * 0.02f, x0 + cw * 0.3f * gk_rs(&g475), ch * 1.02f,
                    0.24f, 8, 9, 0.5f);
        bs475 = seed;
    }
    /* first 40 frames of a segment fade in from black so the previous
     * segment's finished bolt hands over softly */
    float fade = gk_smooth((float)sl / 40.0f);
    float prog = (float)sl / GROW475;
    int base = (int)(seed & 8191u) + (int)(frame * 0.6f);
    float breathe = 0.8f + 0.2f * sinf((float)frame * 0.017f);
    for (int i = 0; i < b475.n; i++) {
        const gk_bseg *s = &b475.s[i];
        if (s->t0 >= prog) continue;
        float x1 = s->x1, y1 = s->y1, tipf = 0.0f;
        if (s->t1 > prog) {
            float f = (prog - s->t0) / (s->t1 - s->t0);
            x1 = s->x0 + (s->x1 - s->x0) * f; y1 = s->y0 + (s->y1 - s->y0) * f;
            tipf = 1.0f;
        }
        float c[3], hc[3];
        int pi = base + (int)(s->t0 * 9000.0f) + (int)((1.0f - s->wgt) * 1500.0f);
        float wa = (0.35f + 0.65f * s->wgt) * fade, ws = 0.5f + 0.5f * s->wgt;
        hk_col(pal, pi + 900, 0.05f, 0.7f * wa * breathe, hc);
        hk_col(pal, pi, 0.42f * (0.6f + 0.4f * s->wgt), 0.95f * wa, c);
        gk_seg(&g475, s->x0, s->y0, x1, y1, hc, 2.4f * sc * ws, 8.0f * sc * ws, 0.5f);
        gk_seg(&g475, s->x0, s->y0, x1, y1, c, 1.0f * sc * ws, 2.6f * sc * ws, 0.25f);
        if (tipf > 0.0f) {                     /* leader head glow */
            float hc2[3];
            hk_col(pal, pi, 0.5f, 1.2f * fade, hc2);
            gk_dot(&g475, x1, y1, hc2, 1.5f * sc, 6.0f * sc, 0.6f);
        }
    }
    gk_present(&g475, fb, w, h);
}
