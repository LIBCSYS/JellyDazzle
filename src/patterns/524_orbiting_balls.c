/* 524 Orbiting Balls — two globes of ball lightning circle each other on
 * a tilted orbit, one swinging in front and growing as it nears, the
 * other shrinking behind, a jagged arc always strung between them (its
 * jag re-forming on a slow crossfade); each ball wears a soft corona of
 * short crackle and leaves a faint comet trail on a persistence canvas.
 * The two balls carry different hues that drift; the arc runs from one
 * hue to the other.  Figure overlay.  Repaint with memory (decay). */
#include "_trace509.h"

#define P524 140

static gk g524;      /* persistent: balls + trails */
static gk g524b;     /* fresh copy: arc + crackle   */
static gk_bolt b524[2];
static int bi524[2] = { -1, -1 };
static gk_bolt cr524[2][3];      /* crackle per ball */
static int ci524[2][3];
static uint32_t bs524 = 0xFFFFFFFFu;

void pattern_524(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g524, w, h);
    if (seed != bs524) { bi524[0] = bi524[1] = -1; for (int i = 0; i < 6; i++) ci524[i / 3][i % 3] = -1; bs524 = seed; gk_clear(&g524); }
    gk_decay_snap(&g524, 0.90f);
    float cw = (float)g524.cw, ch = (float)g524.ch, sc = g524.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    float cx = cw * 0.5f + cw * 0.05f * sinf(t * 0.0021f), cy = ch * 0.5f + ch * 0.04f * cosf(t * 0.0017f);
    float R = (cw < ch ? cw : ch) * 0.28f;
    float ph = t * 0.011f;
    float tilt = 0.5f + 0.15f * sinf(t * 0.0013f);
    float px[2], py[2], pz[2], ps[2];
    for (int b = 0; b < 2; b++) {
        float a = ph + (float)b * GK_TAU * 0.5f;
        float ox = cosf(a) * R, oz = sinf(a) * R;                 /* orbit plane */
        px[b] = cx + ox;
        py[b] = cy + oz * tilt;
        pz[b] = oz / R;                                             /* -1 (far) .. 1 (near) */
        ps[b] = 1.0f + 0.35f * pz[b];                              /* apparent size */
    }
    /* balls, far one first */
    for (int o = 0; o < 2; o++) {
        int b = pz[0] < pz[1] ? o : 1 - o;
        int pi = base + b * 3800;
        float s = ps[b];
        float k0[3], k1[3], k2[3];
        gk_col(pal, pi + 300, 0.55f, 0.9f, k0);
        gk_col(pal, pi + 900, 0.2f, 0.4f, k1);
        gk_col(pal, pi + 1600, 0.05f, 0.12f, k2);
        gk_dot(&g524, px[b], py[b], k0, 4.0f * sc * s, 11.0f * sc * s, 0.6f);
        gk_dot(&g524, px[b], py[b], k1, 9.0f * sc * s, 22.0f * sc * s, 0.6f);
        gk_dot(&g524, px[b], py[b], k2, 20.0f * sc * s, 40.0f * sc * s, 0.4f);
    }
    /* everything that must not smear goes on a fresh copy */
    gk_setup(&g524b, w, h);
    memcpy(g524b.acc, g524.acc, sizeof(float) * (size_t)(g524.cw * g524.ch) * 3);
    /* arc between them, crossfading geometries */
    {
        int idx = frame / P524;
        float u = (float)(frame - idx * P524) / (float)P524;
        int j = idx & 1;
        if (bi524[j] != idx) { gk_seed(&g524b, seed ^ (uint32_t)(idx * 3221)); gk_bolt_gen(&g524b, &b524[j], 0.0f, 0.0f, 1.0f, 0.0f, 0.16f, 6, 3, 0.3f); bi524[j] = idx; }
        if (bi524[j ^ 1] != idx - 1) { gk_seed(&g524b, seed ^ (uint32_t)((idx - 1) * 3221)); gk_bolt_gen(&g524b, &b524[j ^ 1], 0.0f, 0.0f, 1.0f, 0.0f, 0.16f, 6, 3, 0.3f); bi524[j ^ 1] = idx - 1; }
        float wa = gk_smooth(u), wb = 1.0f - wa;
        float dx = px[1] - px[0], dy = py[1] - py[0], len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        float c0[3], c1[3], h0[3], h1[3];
        float amp = 0.7f + 0.3f * gk_noise1(t * 0.03f, 31u);
        gk_col(pal, base, 0.05f, 0.40f * amp, h0);
        gk_col(pal, base + 3800, 0.05f, 0.40f * amp, h1);
        gk_col(pal, base + 300, 0.5f, 0.6f * amp, c0);
        gk_col(pal, base + 4100, 0.5f, 0.6f * amp, c1);
        for (int k = 0; k < 2; k++) {
            float wgt = k == 0 ? wa : wb;
            const gk_bolt *b = &b524[k == 0 ? j : j ^ 1];
            bx_draw_grad(&g524b, b, px[0], py[0], ang, len, 1.0f, 2.0f, wgt, h0, h1, 0.0f, 2.2f * sc, 7.5f * sc, 0.5f);
            bx_draw_grad(&g524b, b, px[0], py[0], ang, len, 1.0f, 2.0f, wgt, c0, c1, 0.0f, 1.0f * sc, 2.6f * sc, 0.25f);
        }
    }
    for (int o = 0; o < 2; o++) {
        int b = o;
        int pi = base + b * 3800;
        float s = ps[b];
        /* crackle: three short filaments on their own clocks */
        for (int c = 0; c < 3; c++) {
            int P = 90 + c * 23 + b * 11;
            int phs = frame + c * 31 + b * 47;
            int idx = phs / P;
            float age = (float)(phs - idx * P);
            if (ci524[b][c] != idx) { gk_seed(&g524, seed ^ (uint32_t)(idx * 977 + b * 131 + c * 17)); gk_bolt_gen(&g524, &cr524[b][c], 0.0f, 0.0f, 1.0f, 0.0f, 0.2f, 4, 1, 0.3f); ci524[b][c] = idx; }
            float env = gk_env(age, 12.0f, 20.0f, 30.0f);
            if (env <= 0.0f) continue;
            float ang = gk_hash((uint32_t)idx * 53u + (uint32_t)(b * 7 + c) + seed) * GK_TAU + t * 0.004f;
            float len = (18.0f + 22.0f * gk_hash((uint32_t)idx * 91u + (uint32_t)c)) * sc * s;
            float cc[3], hc[3];
            gk_col(pal, pi + 700, 0.4f, 0.55f * env, cc);
            gk_col(pal, pi + 1400, 0.05f, 0.30f * env, hc);
            bx_draw_grad(&g524b, &cr524[b][c], px[b], py[b], ang, len, 1.0f, age / 12.0f, 1.0f, hc, hc, 0.4f, 1.3f * sc, 4.0f * sc, 0.5f);
            bx_draw_grad(&g524b, &cr524[b][c], px[b], py[b], ang, len, 1.0f, age / 12.0f, 1.0f, cc, cc, 0.4f, 0.6f * sc, 1.5f * sc, 0.25f);
        }
    }
    gk_present(&g524b, fb, w, h);
}
