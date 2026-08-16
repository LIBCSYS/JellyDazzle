/* 487 Aurora Bolts — soft vertical curtains of light (a coarse grid of
 * slow-waving columns, brightest along a rippling lower edge) with, now and
 * then, a hair-thin bolt sliding down one of the curtain folds and dying
 * away.  Curtains are transparent between folds; the lower half of the
 * frame is dark.  Field overlay.  Repaint pattern. */
#include "_hue469.h"

#define GW487 96
#define GH487 40
#define NS487 3
#define P487 260

static gk g487;
static float grid487[GW487 * GH487];
static float lut487[256 * 3];
static gk_bolt b487[NS487];
static int bi487[NS487];
static uint32_t bs487 = 0xFFFFFFFFu;

void pattern_487(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g487, w, h);
    gk_clear(&g487);
    if (seed != bs487) { for (int i = 0; i < NS487; i++) bi487[i] = -1; bs487 = seed; }
    float cw = (float)g487.cw, ch = (float)g487.ch, sc = g487.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    for (int y = 0; y < GH487; y++)
        for (int x = 0; x < GW487; x++) {
            float xx = ((float)x + 0.5f) / (float)GW487, yy = ((float)y + 0.5f) / (float)GH487;
            /* fold intensity: drifting noise along x, warped by height so
             * the folds lean and ripple like a curtain seen edge-on */
            float warp = 0.9f * (gk_noise1(yy * 3.0f + t * 0.0012f, 8u) - 0.5f);
            float fold = 0.55f * gk_noise1(xx * 7.0f + warp + t * 0.0016f, 3u + (seed & 63u))
                       + 0.30f * gk_noise1(xx * 17.0f - warp * 0.5f - t * 0.0011f, 4u)
                       + 0.15f * gk_noise1(xx * 41.0f + t * 0.002f, 5u);
            fold = gk_smooth((fold - 0.30f) * 2.6f);
            fold = fold * (0.4f + 0.6f * fold);
            /* lower edge ripples; curtain fades upward */
            float edge = 0.55f + 0.08f * sinf(xx * 7.0f + t * 0.01f) + 0.06f * gk_noise1(xx * 5.0f + t * 0.003f, 9u);
            float below = gk_smooth((edge - yy) * 12.0f);          /* 1 above the edge */
            float rim = expf(-(yy - edge) * (yy - edge) * 200.0f);
            float up = 0.10f + 0.9f * gk_smooth((yy - 0.0f) * 1.8f);
            grid487[y * GW487 + x] = fold * (below * 0.45f * up + rim * 0.8f);
        }
    gk_lut_ramp(lut487, pal, base, 7000, 0.25f, 2.6f, 1.0f);
    gk_grid_fill(&g487, grid487, GW487, GH487, lut487);
    for (int s = 0; s < NS487; s++) {
        int ph = frame + s * (P487 / NS487);
        int idx = ph / P487;
        float age = (float)(ph - idx * P487);
        if (idx != bi487[s]) {
            gk_seed(&g487, seed ^ (uint32_t)(idx * 2887 + s * 6007));
            float x0 = cw * (0.1f + 0.8f * gk_rf(&g487));
            gk_bolt_gen(&g487, &b487[s], x0, ch * 0.05f, x0 + cw * 0.06f * gk_rs(&g487), ch * 0.6f, 0.12f, 6, 2, 0.3f);
            bi487[s] = idx;
        }
        float env = gk_env(age, 12.0f, 20.0f, 60.0f) * 0.7f;
        if (env <= 0.0f) continue;
        hk_style st;
        hk_style_set(&st, 4000, 1400, 800,
                     0.4f * env, 1.6f * sc, 6.0f * sc, 0.5f,
                     0.40f, 0.95f * env, 0.6f * sc, 1.8f * sc, 0.2f);
        hk_bolt(&g487, &b487[s], age / 40.0f, 1.0f, pal, base + 3000 + s * 1200 + (int)(age * 10.0f), &st);
    }
    gk_present(&g487, fb, w, h);
}
