/* 485 Heat Lightning — a summer horizon.  A dark ragged land/cloud line
 * sits low in the frame; behind it, distant storms flash silently: broad
 * warm glows that swell over ~25 frames and fade over ~70, sometimes with
 * a hair-thin bolt visible for a moment just above the skyline.  The upper
 * sky carries a faint palette gradient; the land is black.  Full-width
 * field/ground.  Repaint pattern. */
#include "_hue469.h"

#define GW485 64
#define GH485 40
#define NF485 3
#define P485 210

static gk g485;
static float grid485[GW485 * GH485];
static float lut485[256 * 3];
static gk_bolt b485[NF485];
static int bi485[NF485];
static uint32_t bs485 = 0xFFFFFFFFu;
static float fx485[NF485], fw485[NF485];

void pattern_485(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g485, w, h);
    gk_clear(&g485);
    if (seed != bs485) { for (int i = 0; i < NF485; i++) bi485[i] = -1; bs485 = seed; }
    float cw = (float)g485.cw, ch = (float)g485.ch, sc = g485.sc, t = (float)frame;
    int base = (int)(t * 1.5f) + (int)(seed & 8191u);
    float sky = 0.72f;                        /* skyline height fraction */
    float fa[NF485];
    for (int k = 0; k < NF485; k++) {
        int ph = frame + k * (P485 / NF485) + (int)(seed % 53u);
        int idx = ph / P485;
        float age = (float)(ph - idx * P485);
        if (idx != bi485[k]) {
            gk_seed(&g485, seed ^ (uint32_t)(idx * 5101 + k * 7919));
            fx485[k] = 0.1f + 0.8f * gk_rf(&g485);
            fw485[k] = 0.12f + 0.15f * gk_rf(&g485);
            float x0 = fx485[k] * cw + cw * 0.05f * gk_rs(&g485);
            gk_bolt_gen(&g485, &b485[k], x0, ch * (sky - 0.28f - 0.1f * gk_rf(&g485)),
                        x0 + cw * 0.04f * gk_rs(&g485), ch * (sky - 0.02f), 0.16f, 5, 2, 0.35f);
            bi485[k] = idx;
        }
        fa[k] = gk_env(age, 25.0f, 10.0f, 70.0f) + 0.6f * gk_env(age - 40.0f, 15.0f, 5.0f, 50.0f);
        /* the thin distant bolt: only during the first swell, dim */
        float be = gk_env(age - 10.0f, 12.0f, 10.0f, 30.0f) * 0.5f;
        if (be > 0.0f) {
            hk_style st;
            hk_style_set(&st, 3000, 1000, 600,
                         0.35f * be, 1.4f * sc, 3.5f * sc, 0.5f,
                         0.40f, 0.9f * be, 0.7f * sc, 2.0f * sc, 0.4f);
            hk_bolt(&g485, &b485[k], (age - 10.0f) / 14.0f, 1.0f, pal, base + 2500 + k * 1500 + (int)(age * 10.0f), &st);
        }
    }
    for (int y = 0; y < GH485; y++)
        for (int x = 0; x < GW485; x++) {
            float xx = ((float)x + 0.5f) / (float)GW485, yy = ((float)y + 0.5f) / (float)GH485;
            /* skyline: noise ridge */
            float ridge = sky + 0.05f * (gk_noise1(xx * 6.0f + (float)(seed & 63u), 5u) - 0.5f)
                              + 0.02f * (gk_noise1(xx * 19.0f, 6u) - 0.5f);
            float land = gk_smooth((yy - ridge) * 40.0f);       /* 1 below skyline */
            float v = 0.05f + 0.06f * gk_smooth((yy - 0.2f) * 2.0f); /* sky base, brighter low */
            for (int k = 0; k < NF485; k++) {
                float dx = (xx - fx485[k]) / fw485[k];
                float dy = (ridge - yy) / 0.22f; if (dy < 0.0f) dy = 0.0f;
                v += fa[k] * 0.9f * expf(-dx * dx * 0.7f - dy * dy);
            }
            grid485[y * GW485 + x] = v * (1.0f - land);
        }
    gk_lut_ramp(lut485, pal, base, 5000, 0.22f, 1.8f, 1.3f);
    gk_grid_fill(&g485, grid485, GW485, GH485, lut485);
    gk_present(&g485, fb, w, h);
}
