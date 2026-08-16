/* 530 Corona Disc — an eclipse of lightning: a dark disc in the centre
 * ringed by a soft glowing limb, and from the limb sixteen streamers reach
 * outward in every direction, each on its own slow clock (growing over ~40
 * frames, wavering, retracting over ~50) and each with its own hue,
 * bright at the limb and thinning outward into another hue.  The limb
 * breathes; the streamers' base angles drift slowly round the disc.
 * Figure overlay, black inside the disc and beyond the streamers.
 * Repaint. */
#include "_trace509.h"

#define NS530 16

static gk g530;
static gk_bolt b530[NS530];
static int bi530[NS530];
static uint32_t bs530 = 0xFFFFFFFFu;

void pattern_530(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g530, w, h);
    gk_clear(&g530);
    if (seed != bs530) { for (int s = 0; s < NS530; s++) bi530[s] = -1; bs530 = seed; }
    float cw = (float)g530.cw, ch = (float)g530.ch, sc = g530.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float cx = cw * 0.5f, cy = ch * 0.5f;
    float R = (cw < ch ? cw : ch) * (0.20f + 0.008f * sinf(t * 0.011f));
    /* limb: two soft rings */
    float l0[3], l1[3];
    gk_col(pal, base + 2000, 0.25f, 0.55f, l0);
    gk_col(pal, base + 2800, 0.05f, 0.25f, l1);
    gk_ring(&g530, cx, cy, R, 2.5f * sc, l0);
    gk_ring(&g530, cx, cy, R + 6.0f * sc, 9.0f * sc, l1);
    for (int s = 0; s < NS530; s++) {
        int P = 150 + (int)(gk_hash((uint32_t)s * 77u + seed) * 90.0f);
        int ph = frame + (int)(gk_hash((uint32_t)s * 91u + seed) * (float)P);
        int idx = ph / P;
        float age = (float)(ph - idx * P);
        if (bi530[s] != idx) {
            gk_seed(&g530, seed ^ (uint32_t)(idx * 3457 + s * 6011));
            gk_bolt_gen(&g530, &b530[s], 1.0f, 0.0f, 1.0f + 0.5f + 0.7f * gk_rf(&g530), 0.25f * gk_rs(&g530), 0.15f, 5, 2, 0.35f);
            bi530[s] = idx;
        }
        float env = gk_env(age, 40.0f, 40.0f, 50.0f);
        if (env <= 0.0f) continue;
        float ang = GK_TAU * (float)s / (float)NS530 + t * 0.0015f + 0.15f * sinf(t * 0.007f + (float)s);
        int pi = base + (int)(gk_hash((uint32_t)idx * 13u + (uint32_t)s * 101u + seed) * 8000.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.42f * env, h0);
        gk_col(pal, pi + 2200, 0.05f, 0.30f * env, h1);
        gk_col(pal, pi + 300, 0.55f, 0.7f * env, c0);
        gk_col(pal, pi + 2500, 0.35f, 0.5f * env, c1);
        float prog = age / 40.0f;
        bx_draw_grad(&g530, &b530[s], cx, cy, ang, R, 1.0f, prog, 1.0f, h0, h1, 0.35f, 1.8f * sc, 6.0f * sc, 0.5f);
        bx_draw_grad(&g530, &b530[s], cx, cy, ang, R, 1.0f, prog, 1.0f, c0, c1, 0.35f, 0.8f * sc, 2.0f * sc, 0.25f);
        /* base flare on the limb */
        float fc[3];
        gk_col(pal, pi + 600, 0.3f, 0.5f * env, fc);
        gk_dot(&g530, cx + cosf(ang) * R, cy + sinf(ang) * R, fc, 3.0f * sc, 12.0f * sc, 0.6f);
    }
    gk_present(&g530, fb, w, h);
}
