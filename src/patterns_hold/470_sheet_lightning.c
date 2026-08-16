/* 470 Sheet Lightning — a night cloud deck lit from inside.  Slow fbm cloud
 * bodies drift; every few seconds a flash swells somewhere behind them over
 * ~20 frames and dies over ~50, lighting the cloud bellies with the palette.
 * No visible bolt: just the wash.  Dark cloud tops stay near-black so lower
 * layers show through the gaps.  Full-frame field.  Repaint pattern. */
#include "_glow469.h"

static gk g470;
static float grid470[64 * 48];
static float lut470[256 * 3];

#define GW470 64
#define GH470 48
#define NF470 3
#define PF470 170

void pattern_470(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g470, w, h);
    gk_clear(&g470);
    float t = (float)frame;
    /* flashes: NF470 slots, each striking every PF470 frames at a hashed spot */
    float fx[NF470], fy[NF470], fa[NF470], fr[NF470];
    for (int k = 0; k < NF470; k++) {
        int ph = frame + k * (PF470 / NF470) + (int)(seed % 97u);
        int idx = ph / PF470;
        float age = (float)(ph - idx * PF470);
        uint32_t hs = (uint32_t)idx * 0x9E3779B1u + (uint32_t)k * 0x85EBCA6Bu + seed;
        fx[k] = gk_hash(hs) * (float)GW470;
        fy[k] = gk_hash(hs + 1u) * (float)GH470 * 0.8f;
        fr[k] = (0.35f + 0.5f * gk_hash(hs + 2u)) * (float)GW470;
        /* second peak in the same spot: sheet lightning stutters softly */
        float e = gk_env(age, 18.0f, 8.0f, 55.0f) + 0.5f * gk_env(age - 30.0f, 12.0f, 4.0f, 40.0f);
        fa[k] = e * (0.7f + 0.3f * gk_hash(hs + 3u));
    }
    /* cloud density */
    gk_grid_fbm(grid470, GW470, GH470, 0.075f, t * 0.0011f + (float)(seed & 255u), t * 0.0004f, 11u);
    for (int y = 0; y < GH470; y++)
        for (int x = 0; x < GW470; x++) {
            float d = grid470[y * GW470 + x];             /* 0..1 density */
            float cloud = gk_smooth((d - 0.35f) * 2.6f);  /* where cloud is */
            float lit = 0.05f + 0.10f * (1.0f - (float)y / (float)GH470);
            for (int k = 0; k < NF470; k++) {
                float dx = (float)x - fx[k], dy = ((float)y - fy[k]) * 1.3f;
                float r2 = (dx * dx + dy * dy) / (fr[k] * fr[k]);
                if (r2 < 4.0f) lit += fa[k] * expf(-r2 * 1.6f);
            }
            /* thin cloud shows the flash better than dense; dense glows dimly */
            float v = lit * (0.25f + 0.9f * cloud) * (1.15f - 0.55f * cloud);
            grid470[y * GW470 + x] = v;
        }
    int base = (int)(t * 2.3f) + (int)(seed & 8191u);
    gk_lut_ramp(lut470, pal, base, 9000, 0.35f, 2.2f, 1.25f);
    gk_grid_fill(&g470, grid470, GW470, GH470, lut470);
    gk_present(&g470, fb, w, h);
}
