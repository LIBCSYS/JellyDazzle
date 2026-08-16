/* 486 Volcanic Lightning — an ash plume rising from the bottom centre,
 * drawn as a soft dark-cored column of noise lit from within, with slow
 * bolts arcing inside and around it and embers drifting up and out.  Bolts
 * begin low in the plume and climb.  Field overlay: black outside the
 * plume.  Repaint pattern. */
#include "_hue469.h"

#define GW486 48
#define GH486 48
#define NS486 4
#define NE486 40
#define P486 200

static gk g486;
static float grid486[GW486 * GH486];
static float lut486[256 * 3];
static gk_bolt b486[NS486];
static int bi486[NS486];
static uint32_t bs486 = 0xFFFFFFFFu;
static float bx486[NS486], by486[NS486];

void pattern_486(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g486, w, h);
    gk_clear(&g486);
    if (seed != bs486) { for (int i = 0; i < NS486; i++) bi486[i] = -1; bs486 = seed; }
    float cw = (float)g486.cw, ch = (float)g486.ch, sc = g486.sc, t = (float)frame;
    int base = (int)(t * 1.7f) + (int)(seed & 8191u);
    float lx[NS486], ly[NS486], la[NS486]; int nl = 0;
    for (int s = 0; s < NS486; s++) {
        int ph = frame + s * (P486 / NS486);
        int idx = ph / P486;
        float age = (float)(ph - idx * P486);
        if (idx != bi486[s]) {
            gk_seed(&g486, seed ^ (uint32_t)(idx * 3391 + s * 9001));
            float y0 = ch * (0.35f + 0.45f * gk_rf(&g486));
            float halfw = 0.08f + 0.22f * (1.0f - y0 / ch);
            float x0 = cw * (0.5f + halfw * gk_rs(&g486));
            float x1 = cw * (0.5f + (halfw + 0.12f) * gk_rs(&g486));
            float y1 = y0 - ch * (0.15f + 0.3f * gk_rf(&g486));
            gk_bolt_gen(&g486, &b486[s], x0, y0, x1, y1, 0.22f, 6, 4, 0.4f);
            bx486[s] = x0 / cw; by486[s] = (y0 + y1) * 0.5f / ch;
            bi486[s] = idx;
        }
        float env = gk_env(age, 8.0f, 25.0f, 60.0f);
        if (env <= 0.0f) continue;
        lx[nl] = bx486[s]; ly[nl] = by486[s]; la[nl] = env; nl++;
        int pi = base + s * 1500 + (int)(age * 12.0f);
        hk_style st;
        hk_style_set(&st, 4500, 1600, 900,
                     0.6f * env, 1.8f * sc, 6.0f * sc, 0.5f,
                     0.40f, 0.95f * env, 0.8f * sc, 2.2f * sc, 0.25f);
        hk_bolt(&g486, &b486[s], age / 26.0f, 1.0f, pal, pi, &st);
    }
    /* plume: column widening upward, fbm texture rising */
    gk_grid_fbm(grid486, GW486, GH486, 0.13f, (float)(seed & 255u), -t * 0.004f, 31u);
    for (int y = 0; y < GH486; y++)
        for (int x = 0; x < GW486; x++) {
            float xx = ((float)x + 0.5f) / (float)GW486, yy = ((float)y + 0.5f) / (float)GH486;
            float d = grid486[y * GW486 + x];
            /* column that flares into a billowing head above 40% */
            float head = gk_smooth((0.45f - yy) * 4.0f);
            float halfw = 0.06f + 0.16f * (1.0f - yy) + 0.22f * head + 0.16f * (d - 0.5f);
            float inside = gk_smooth((halfw - fabsf(xx - 0.5f)) * 7.0f) * gk_smooth((yy - 0.02f) * 8.0f);
            float glow = 0.05f + 0.16f * gk_smooth((yy - 0.55f) * 3.0f);   /* vent glow at the base */
            for (int k = 0; k < nl; k++) {
                float dx = (xx - lx[k]) * 3.5f, dy = (yy - ly[k]) * 3.5f;
                glow += la[k] * 0.7f * expf(-dx * dx - dy * dy);
            }
            grid486[y * GW486 + x] = inside * glow * (0.25f + 1.3f * d * d);
        }
    gk_lut_ramp(lut486, pal, base + 2000, 4500, 0.3f, 2.4f, 1.2f);
    gk_grid_fill(&g486, grid486, GW486, GH486, lut486);
    /* embers rising from the vent, drifting outward */
    for (int e = 0; e < NE486; e++) {
        float ph = fmodf(t * (0.0022f + 0.0015f * gk_hash((uint32_t)e * 3u + seed)) + gk_hash((uint32_t)e + seed), 1.0f);
        float yy = 0.95f - 0.95f * ph;
        float xx = 0.5f + (gk_hash((uint32_t)e * 7u + seed) - 0.5f) * (0.1f + 0.7f * ph)
                 + 0.03f * sinf(t * 0.02f + (float)e);
        float amp = gk_smooth(ph * 8.0f) * gk_smooth((1.0f - ph) * 3.0f) * 0.8f;
        float c[3];
        gk_col(pal, base + 3500 + e * 90, 0.35f, amp, c);
        gk_dot(&g486, xx * cw, yy * ch, c, 1.0f * sc, 3.5f * sc, 0.5f);
    }
    gk_present(&g486, fb, w, h);
}
