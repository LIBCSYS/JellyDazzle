/* 502 Thunder Cell — one towering cumulonimbus in the middle of the frame:
 * a soft billowing tower (fbm shaped into a stacked-anvil silhouette) that
 * pulses from inside with slow flashes, and drops a bolt from its base
 * every so often.  Black sky around the tower.  Field overlay.  Repaint. */
#include "_hue469.h"

#define GW502 56
#define GH502 56
#define NF502 3
#define P502 220

static gk g502;
static float grid502[GW502 * GH502];
static float lut502[256 * 3];
static gk_bolt b502[NF502];
static int bi502[NF502];
static uint32_t bs502 = 0xFFFFFFFFu;
static float fx502[NF502], fy502[NF502];

void pattern_502(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g502, w, h);
    gk_clear(&g502);
    if (seed != bs502) { for (int i = 0; i < NF502; i++) bi502[i] = -1; bs502 = seed; }
    float cw = (float)g502.cw, ch = (float)g502.ch, sc = g502.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    float fa[NF502];
    for (int k = 0; k < NF502; k++) {
        int ph = frame + k * (P502 / NF502);
        int idx = ph / P502;
        float age = (float)(ph - idx * P502);
        if (idx != bi502[k]) {
            gk_seed(&g502, seed ^ (uint32_t)(idx * 5303 + k * 2917));
            fx502[k] = 0.5f + 0.22f * gk_rs(&g502);
            fy502[k] = 0.25f + 0.4f * gk_rf(&g502);
            float x0 = cw * (0.5f + 0.18f * gk_rs(&g502));
            gk_bolt_gen(&g502, &b502[k], x0, ch * 0.66f, x0 + cw * 0.1f * gk_rs(&g502), ch * 0.98f, 0.18f, 6, 3, 0.4f);
            bi502[k] = idx;
        }
        fa[k] = gk_env(age, 20.0f, 10.0f, 60.0f) + 0.5f * gk_env(age - 30.0f, 12.0f, 6.0f, 45.0f);
        float be = gk_env(age - 8.0f, 10.0f, 22.0f, 50.0f) * (gk_hash((uint32_t)idx * 3u + (uint32_t)k + seed) < 0.55f ? 1.0f : 0.0f);
        if (be > 0.0f) {
            hk_style st;
            hk_style_set(&st, 4000, 1400, 800,
                         0.5f * be, 1.8f * sc, 6.0f * sc, 0.5f,
                         0.40f, 0.95f * be, 0.8f * sc, 2.2f * sc, 0.25f);
            hk_bolt(&g502, &b502[k], (age - 8.0f) / 22.0f, 1.0f, pal, base + 3000 + k * 1500 + (int)(age * 10.0f), &st);
        }
    }
    gk_grid_fbm(grid502, GW502, GH502, 0.12f, t * 0.0012f + (float)(seed & 255u), t * -0.0006f, 51u);
    for (int y = 0; y < GH502; y++)
        for (int x = 0; x < GW502; x++) {
            float xx = ((float)x + 0.5f) / (float)GW502, yy = ((float)y + 0.5f) / (float)GH502;
            float d = grid502[y * GW502 + x];
            /* tower silhouette: wide anvil at top (yy~0.15), narrower body,
             * flat base at 0.72 */
            float anvil = gk_smooth((0.30f - yy) * 6.0f) * 0.32f;
            float body = 0.13f + 0.10f * gk_smooth((yy - 0.25f) * 3.0f) * (1.0f - gk_smooth((yy - 0.6f) * 6.0f));
            float halfw = body + anvil + 0.08f * (d - 0.5f);
            float inside = gk_smooth((halfw - fabsf(xx - 0.5f)) * 8.0f)
                         * gk_smooth((yy - 0.06f) * 8.0f) * gk_smooth((0.74f - yy) * 14.0f);
            float lit = 0.10f + 0.06f * (1.0f - yy);
            for (int k = 0; k < NF502; k++) {
                float dx = (xx - fx502[k]) * 4.0f, dy = (yy - fy502[k]) * 3.0f;
                lit += fa[k] * 0.9f * expf(-dx * dx - dy * dy);
            }
            grid502[y * GW502 + x] = inside * lit * (0.35f + 1.1f * d * d);
        }
    gk_lut_ramp(lut502, pal, base, 5000, 0.35f, 2.4f, 1.2f);
    gk_grid_fill(&g502, grid502, GW502, GH502, lut502);
    gk_present(&g502, fb, w, h);
}
