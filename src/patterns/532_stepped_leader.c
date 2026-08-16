/* 532 Stepped Leader — the descent of a leader shown step by step: the
 * channel advances in a dozen discrete steps of ~12 frames (each step
 * eases in over its first half and rests over the second, a bulb of light
 * swelling at the tip while it rests), branches unfurling as it passes;
 * on reaching the ground the return stroke brightens the whole channel
 * over ~20 frames, holds, and cools over ~110.  The leader is dim and
 * one hue; the return stroke is hot and shifts toward another, hue
 * varying along the channel.  Two slots on staggered clocks.  Sparse
 * overlay.  Repaint. */
#include "_trace509.h"

#define P532 340
#define NSTEP532 12
#define STEPF532 12.0f

static gk g532;
static gk_bolt b532[2];
static int bi532[2] = { -1, -1 };
static uint32_t bs532 = 0xFFFFFFFFu;
static float hue532[2];

void pattern_532(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g532, w, h);
    gk_clear(&g532);
    if (seed != bs532) { bi532[0] = bi532[1] = -1; bs532 = seed; }
    float cw = (float)g532.cw, ch = (float)g532.ch, sc = g532.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    for (int s = 0; s < 2; s++) {
        int ph = frame + s * (P532 / 2);
        int idx = ph / P532;
        float age = (float)(ph - idx * P532);
        if (idx != bi532[s]) {
            gk_seed(&g532, seed ^ (uint32_t)(idx * 4519 + s * 7481));
            float x0 = cw * (0.2f + 0.6f * gk_rf(&g532));
            gk_bolt_gen(&g532, &b532[s], x0, -ch * 0.02f, x0 + cw * 0.25f * gk_rs(&g532), ch * 0.9f, 0.2f, 7, 5, 0.4f);
            hue532[s] = gk_rf(&g532);
            bi532[s] = idx;
        }
        float lead_t = NSTEP532 * STEPF532;                    /* frames to reach the ground */
        float prog, bulb = 0.0f;
        if (age < lead_t) {
            float k = floorf(age / STEPF532), f = (age - k * STEPF532) / STEPF532;   /* step index, phase */
            float ease = gk_smooth(f / 0.5f);                    /* advance in the first half */
            prog = (k + ease) / (float)NSTEP532;
            bulb = gk_smooth(f / 0.5f) * (1.0f - gk_smooth((f - 0.75f) / 0.25f));
        } else prog = 1.0f;
        float ret = gk_smooth((age - lead_t) / 20.0f);
        float cool = gk_smooth((age - lead_t - 60.0f) / 110.0f);
        float env = (1.0f - cool);
        if (env <= 0.0f) continue;
        int pi = base + (int)(hue532[s] * 8000.0f);
        float amp = 0.45f + 0.9f * ret;
        float wt = 0.25f + 0.45f * ret;
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.35f * amp * env, h0);
        gk_col(pal, pi + 1400 + (int)(ret * 900.0f), 0.05f, 0.35f * amp * env, h1);
        gk_col(pal, pi + 200 + (int)(ret * 600.0f), wt, 0.55f * amp * env, c0);
        gk_col(pal, pi + 1700 + (int)(ret * 900.0f), wt, 0.55f * amp * env, c1);
        float th = 0.8f + 0.5f * ret;
        bx_draw_grad(&g532, &b532[s], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, prog, 1.0f, h0, h1, 0.1f, 2.0f * sc * th, 7.0f * sc * th, 0.5f);
        bx_draw_grad(&g532, &b532[s], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, prog, 1.0f, c0, c1, 0.1f, 0.9f * sc * th, 2.4f * sc * th, 0.25f);
        /* tip bulb during the descent */
        if (bulb > 0.0f && prog < 1.0f) {
            for (int i = 0; i < b532[s].n; i++) {
                const gk_bseg *sg = &b532[s].s[i];
                if (sg->wgt < 0.99f || sg->t0 > prog || sg->t1 < prog) continue;
                float f = (prog - sg->t0) / (sg->t1 - sg->t0);
                float x = sg->x0 + (sg->x1 - sg->x0) * f, y = sg->y0 + (sg->y1 - sg->y0) * f;
                float bc[3], bh[3];
                gk_col(pal, pi + 800, 0.5f, 0.9f * bulb, bc);
                gk_col(pal, pi + 1200, 0.1f, 0.4f * bulb, bh);
                gk_dot(&g532, x, y, bh, 5.0f * sc, 18.0f * sc, 0.6f);
                gk_dot(&g532, x, y, bc, 2.0f * sc, 7.0f * sc, 0.6f);
                break;
            }
        }
        /* ground flash */
        float gf = ret * env;
        if (gf > 0.0f) {
            float gx = b532[s].s[0].x0, gy = ch * 0.9f;
            for (int i = 0; i < b532[s].n; i++) if (b532[s].s[i].wgt > 0.99f && b532[s].s[i].t1 >= 0.999f) { gx = b532[s].s[i].x1; break; }
            float gc[3];
            gk_col(pal, pi + 500, 0.4f, 0.6f * gf, gc);
            gk_dot(&g532, gx, gy, gc, 6.0f * sc, 28.0f * sc, 0.6f);
        }
    }
    gk_present(&g532, fb, w, h);
}
