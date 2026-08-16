/* 501 St Elmo's Fire — a ship's mast and yards in faint silhouette lines,
 * every tip and yard-end wearing a soft brush discharge: a corona that
 * breathes and a few tiny wavering streamers, each on its own slow clock.
 * The rig sways gently.  Sparse overlay.  Repaint pattern. */
#include "_hue469.h"

#define NTIP501 9
#define NST501 3
#define P501 110

static gk g501;
static gk_bolt st501[NTIP501][NST501];
static int si501[NTIP501][NST501];
static uint32_t ss501 = 0xFFFFFFFFu;

void pattern_501(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g501, w, h);
    gk_clear(&g501);
    if (seed != ss501) { memset(si501, 0xFF, sizeof si501); ss501 = seed; }
    float cw = (float)g501.cw, ch = (float)g501.ch, sc = g501.sc, t = (float)frame;
    int base = (int)(t * 1.5f) + (int)(seed & 8191u);
    float sway = 0.03f * sinf(t * 0.009f);
    float cx = cw * 0.5f, deck = ch * 0.95f, top = ch * 0.10f;
    /* mast + 3 yards: endpoints computed with sway (rotation about deck) */
    float pts[NTIP501][2];
    float mx = cx + (top - deck) * sinf(sway), my = deck + (top - deck) * cosf(sway);
    pts[0][0] = mx; pts[0][1] = my;                     /* masthead */
    float yh[3] = { 0.30f, 0.50f, 0.70f }, yw[3] = { 0.16f, 0.24f, 0.30f };
    for (int i = 0; i < 3; i++) {
        float yy = deck + (top - deck) * (1.0f - yh[i]);
        float ycx = cx + (yy - deck) * sinf(sway), ycy = deck + (yy - deck) * cosf(sway);
        pts[1 + i * 2][0] = ycx - cw * yw[i] * cosf(sway); pts[1 + i * 2][1] = ycy - cw * yw[i] * -sinf(sway);
        pts[2 + i * 2][0] = ycx + cw * yw[i] * cosf(sway); pts[2 + i * 2][1] = ycy + cw * yw[i] * -sinf(sway);
    }
    pts[7][0] = cx + (ch * 0.5f - deck) * sinf(sway) - cw * 0.35f; pts[7][1] = ch * 0.62f;  /* stay ends */
    pts[8][0] = cx + (ch * 0.5f - deck) * sinf(sway) + cw * 0.35f; pts[8][1] = ch * 0.62f;
    /* rig lines, faint */
    float rc[3];
    gk_col(pal, base + 5000, 0.1f, 0.14f, rc);
    gk_seg(&g501, cx, deck, mx, my, rc, 1.0f * sc, 3.0f * sc, 0.3f);
    for (int i = 0; i < 3; i++)
        gk_seg(&g501, pts[1 + i * 2][0], pts[1 + i * 2][1], pts[2 + i * 2][0], pts[2 + i * 2][1], rc, 0.9f * sc, 2.5f * sc, 0.3f);
    gk_seg(&g501, mx, my, pts[7][0], pts[7][1], rc, 0.7f * sc, 2.0f * sc, 0.3f);
    gk_seg(&g501, mx, my, pts[8][0], pts[8][1], rc, 0.7f * sc, 2.0f * sc, 0.3f);
    /* the fire */
    for (int i = 0; i < NTIP501; i++) {
        float bx = pts[i][0], by = pts[i][1];
        float br = (0.8f + 0.2f * sinf(t * 0.02f + (float)i * 1.7f)) * (i == 0 ? 1.4f : 1.0f);
        int pi = base + i * 400;
        float c[3], hal[3];
        gk_col(pal, pi, 0.5f, 1.0f * br, c);
        gk_col(pal, pi + 600, 0.05f, 0.5f * br, hal);
        gk_dot(&g501, bx, by, hal, 4.0f * sc, 18.0f * sc, 0.7f);
        gk_dot(&g501, bx, by, c, 2.0f * sc, 6.0f * sc, 0.4f);
        for (int s = 0; s < NST501; s++) {
            int ph = frame + s * (P501 / NST501) + i * 17;
            int idx = ph / P501;
            float age = (float)(ph - idx * P501);
            if (si501[i][s] != idx) {
                gk_seed(&g501, seed ^ (uint32_t)(idx * 3331 + i * 509 + s * 83));
                float a = -GK_TAU * 0.25f + gk_rs(&g501) * 1.4f;
                float len = 1.0f + 1.0f * gk_rf(&g501);
                gk_bolt_gen(&g501, &st501[i][s], 0.0f, 0.0f, cosf(a) * len, sinf(a) * len, 0.25f, 4, 1, 0.4f);
                si501[i][s] = idx;
            }
            float env = gk_env(age, 20.0f, 25.0f, 40.0f) * 0.7f;
            if (env <= 0.0f) continue;
            /* streamers: hue runs tip -> end and drifts with age */
            hk_style st;
            hk_style_set(&st, 2000, 800, 600,
                         0.3f * env, 1.4f * sc, 4.0f * sc, 0.4f,
                         0.40f, 0.9f * env, 0.8f * sc, 2.6f * sc, 0.3f);
            hk_bolt_xf(&g501, &st501[i][s], bx, by, 0.0f, 20.0f * sc, age / 22.0f, 1.0f, pal, pi + s * 500 + (int)(age * 10.0f), &st);
        }
    }
    gk_present(&g501, fb, w, h);
}
