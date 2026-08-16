/* 496 Storm Horizon — a wide, low view of a storm far off across a plain:
 * a graded night sky, a bank of distant cumulus along the horizon lit
 * from within by slow flashes, and every so often a small distant bolt
 * dropping from the cloud bank to the horizon line.  Ground below the
 * horizon is black; upper sky is dim.  Full-frame field.  Repaint. */
#include "_hue469.h"

#define GW496 80
#define GH496 64
#define NF496 4
#define P496 240

static gk g496;
static float grid496[GW496 * GH496];
static float lut496[256 * 3];
static gk_bolt b496[NF496];
static int bi496[NF496];
static uint32_t bs496 = 0xFFFFFFFFu;
static float fx496[NF496];

void pattern_496(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g496, w, h);
    gk_clear(&g496);
    if (seed != bs496) { for (int i = 0; i < NF496; i++) bi496[i] = -1; bs496 = seed; }
    float cw = (float)g496.cw, ch = (float)g496.ch, sc = g496.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float hz = 0.68f;
    float fa[NF496];
    for (int k = 0; k < NF496; k++) {
        int ph = frame + k * (P496 / NF496) + (int)(seed % 71u);
        int idx = ph / P496;
        float age = (float)(ph - idx * P496);
        if (idx != bi496[k]) {
            gk_seed(&g496, seed ^ (uint32_t)(idx * 6421 + k * 3011));
            fx496[k] = 0.05f + 0.9f * gk_rf(&g496);
            float x0 = fx496[k] * cw;
            gk_bolt_gen(&g496, &b496[k], x0, ch * (hz - 0.13f - 0.05f * gk_rf(&g496)), x0 + cw * 0.03f * gk_rs(&g496), ch * hz, 0.16f, 5, 2, 0.35f);
            bi496[k] = idx;
        }
        fa[k] = gk_env(age, 20.0f, 12.0f, 60.0f) + 0.5f * gk_env(age - 35.0f, 12.0f, 6.0f, 45.0f);
        float be = gk_env(age - 14.0f, 10.0f, 12.0f, 40.0f) * (gk_hash((uint32_t)idx * 5u + (uint32_t)k) < 0.6f ? 0.6f : 0.0f);
        if (be > 0.0f) {
            hk_style st;
            hk_style_set(&st, 3000, 1000, 600,
                         0.35f * be, 1.2f * sc, 3.5f * sc, 0.5f,
                         0.40f, 0.9f * be, 0.6f * sc, 2.0f * sc, 0.4f);
            hk_bolt(&g496, &b496[k], (age - 14.0f) / 12.0f, 1.0f, pal, base + 3000 + k * 1500 + (int)(age * 10.0f), &st);
        }
    }
    gk_grid_fbm(grid496, GW496, GH496, 0.09f, t * 0.0009f + (float)(seed & 255u), t * 0.0002f, 41u);
    for (int y = 0; y < GH496; y++)
        for (int x = 0; x < GW496; x++) {
            float xx = ((float)x + 0.5f) / (float)GW496, yy = ((float)y + 0.5f) / (float)GH496;
            float d = grid496[y * GW496 + x];
            /* cloud bank: dense near the horizon, tops ragged by noise */
            float top = hz - 0.10f - 0.14f * d;
            float bank = gk_smooth((yy - top) * 7.0f) * gk_smooth((hz - yy) * 18.0f);
            float sky = 0.03f + 0.07f * gk_smooth((yy - 0.1f) * 1.6f);   /* dim gradient */
            float lit = 0.06f;
            for (int k = 0; k < NF496; k++) {
                float dx = (xx - fx496[k]) * 3.0f;
                lit += fa[k] * expf(-dx * dx) * (0.7f + 0.6f * (yy - top) / 0.24f);
                /* the flash also washes the sky just above the cloud */
                sky += fa[k] * 0.35f * expf(-dx * dx * 0.7f) * expf(-(top - yy) * (top - yy) * 60.0f);
            }
            float v = (yy < hz) ? sky * (1.0f - bank) + bank * lit * (0.5f + 0.8f * d) : 0.0f;
            grid496[y * GW496 + x] = v;
        }
    gk_lut_ramp(lut496, pal, base, 6000, 0.22f, 2.0f, 1.2f);
    gk_grid_fill(&g496, grid496, GW496, GH496, lut496);
    gk_present(&g496, fb, w, h);
}
