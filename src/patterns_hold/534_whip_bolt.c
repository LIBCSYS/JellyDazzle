/* 534 Whip Bolt — one long bolt hangs from an anchor near the top of the
 * frame and sways like a whip, swinging slowly through ±35 degrees on a
 * pendulum clock while its channel re-jags on a slow crossfade; a
 * persistence canvas keeps a faint fan of where it has been.  A small
 * crackle of sparks rides at the tip.  Hue runs from the anchor's colour
 * to a second hue at the tip and both drift; the fan trails cool into
 * the palette behind it.  Figure overlay.  Repaint with memory (decay). */
#include "_trace509.h"

#define P534 150

static gk g534, g534b;
static gk_bolt b534[2];
static int bi534[2] = { -1, -1 };
static gk_bolt tip534[3];
static int ti534[3] = { -1, -1, -1 };
static uint32_t bs534 = 0xFFFFFFFFu;

void pattern_534(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g534, w, h);
    if (seed != bs534) { bi534[0] = bi534[1] = -1; ti534[0] = ti534[1] = ti534[2] = -1; bs534 = seed; gk_clear(&g534); }
    gk_decay_snap(&g534, 0.93f);
    float cw = (float)g534.cw, ch = (float)g534.ch, sc = g534.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float ax = cw * 0.5f + cw * 0.04f * sinf(t * 0.0027f), ay = ch * 0.06f;
    float L = ch * 0.82f;
    float swing = 0.6f * sinf(t * 0.0125f) + 0.12f * sinf(t * 0.0071f + 1.0f);
    float ang = GK_TAU * 0.25f + swing;                    /* down = +90 deg */
    int idx = frame / P534;
    float u = (float)(frame - idx * P534) / (float)P534;
    int j = idx & 1;
    if (bi534[j] != idx) { gk_seed(&g534, seed ^ (uint32_t)(idx * 2687)); gk_bolt_gen(&g534, &b534[j], 0.0f, 0.0f, 1.0f, 0.0f, 0.10f, 7, 4, 0.3f); bi534[j] = idx; }
    if (bi534[j ^ 1] != idx - 1) { gk_seed(&g534, seed ^ (uint32_t)((idx - 1) * 2687)); gk_bolt_gen(&g534, &b534[j ^ 1], 0.0f, 0.0f, 1.0f, 0.0f, 0.10f, 7, 4, 0.3f); bi534[j ^ 1] = idx - 1; }
    float wa = gk_smooth(u), wb = 1.0f - wa;
    /* the persistent pass: dim halo only, so the fan is soft colour */
    float f0[3], f1[3];
    gk_col(pal, base + 2600, 0.05f, 0.022f, f0);
    gk_col(pal, base + 4200, 0.05f, 0.022f, f1);
    for (int k = 0; k < 2; k++) {
        float wgt = k == 0 ? wa : wb;
        bx_draw_grad(&g534, &b534[k == 0 ? j : j ^ 1], ax, ay, ang, L, 1.0f, 2.0f, wgt, f0, f1, 0.0f, 2.5f * sc, 8.0f * sc, 0.8f);
    }
    /* the live channel goes on a fresh copy */
    gk_setup(&g534b, w, h);
    memcpy(g534b.acc, g534.acc, sizeof(float) * (size_t)(g534.cw * g534.ch) * 3);
    float c0[3], c1[3], h0[3], h1[3];
    float br = 0.8f + 0.2f * gk_noise1(t * 0.02f, 9u + seed);
    gk_col(pal, base, 0.05f, 0.42f * br, h0);
    gk_col(pal, base + 2600, 0.05f, 0.42f * br, h1);
    gk_col(pal, base + 300, 0.55f, 0.7f * br, c0);
    gk_col(pal, base + 2900, 0.5f, 0.7f * br, c1);
    for (int k = 0; k < 2; k++) {
        float wgt = k == 0 ? wa : wb;
        const gk_bolt *b = &b534[k == 0 ? j : j ^ 1];
        bx_draw_grad(&g534b, b, ax, ay, ang, L, 1.0f, 2.0f, wgt, h0, h1, 0.15f, 1.6f * sc, 5.5f * sc, 0.5f);
        bx_draw_grad(&g534b, b, ax, ay, ang, L, 1.0f, 2.0f, wgt, c0, c1, 0.15f, 0.9f * sc, 2.4f * sc, 0.25f);
    }
    /* anchor and tip */
    float tx = ax + cosf(ang) * L, ty = ay + sinf(ang) * L;
    float kc[3];
    gk_col(pal, base + 300, 0.4f, 0.8f, kc);
    gk_dot(&g534b, ax, ay, kc, 3.0f * sc, 14.0f * sc, 0.6f);
    for (int c = 0; c < 3; c++) {
        int P = 70 + c * 19;
        int phs = frame + c * 29;
        int ci = phs / P;
        float age = (float)(phs - ci * P);
        if (ti534[c] != ci) { gk_seed(&g534, seed ^ (uint32_t)(ci * 811 + c * 97)); gk_bolt_gen(&g534, &tip534[c], 0.0f, 0.0f, 1.0f, 0.0f, 0.2f, 4, 1, 0.3f); ti534[c] = ci; }
        float env = gk_env(age, 10.0f, 15.0f, 25.0f);
        if (env <= 0.0f) continue;
        float a2 = ang + (gk_hash((uint32_t)ci * 37u + (uint32_t)c * 5u + seed) - 0.5f) * 2.4f;
        float len = (14.0f + 20.0f * gk_hash((uint32_t)ci * 53u + (uint32_t)c)) * sc;
        float sc0[3];
        gk_col(pal, base + 3200, 0.4f, 0.55f * env, sc0);
        bx_draw_grad(&g534b, &tip534[c], tx, ty, a2, len, 1.0f, age / 10.0f, 1.0f, sc0, sc0, 0.5f, 0.7f * sc, 2.5f * sc, 0.4f);
    }
    gk_present(&g534b, fb, w, h);
}
