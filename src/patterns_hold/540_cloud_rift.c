/* 540 Cloud Rift — seen from above, a dim cloud deck fills the frame; a
 * jagged crack opens across it and light from the storm beneath pours up
 * through the gap: the rift is a long horizontal discharge whose halo
 * widens from a hairline to a broad glow over ~150 frames as it opens,
 * lighting the cloud tops on either side, while small bolts crawl along
 * its edges; it holds, then closes over ~150 as the next rift (new place,
 * new hue) begins to split elsewhere.  Rift hue slides along its length
 * and drifts; the deck carries the palette's dim end.  Full-frame field.
 * Repaint. */
#include "_trace509.h"

#define GW540 72
#define GH540 48
#define P540 400
#define NC540 4

static gk g540;
static float grid540[GW540 * GH540];
static float lut540[256 * 3];
static gk_bolt b540[2], cr540[2][NC540];
static int bi540[2] = { -1, -1 };
static uint32_t bs540 = 0xFFFFFFFFu;
static float ry540[2], rx0540[2], rx1540[2], rtilt540[2], hue540[2];
static float cu540[2][NC540], cs540[2][NC540];

void pattern_540(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g540, w, h);
    gk_clear(&g540);
    if (seed != bs540) { bi540[0] = bi540[1] = -1; bs540 = seed; }
    float cw = (float)g540.cw, ch = (float)g540.ch, sc = g540.sc, t = (float)frame;
    int base = (int)(t * 1.2f) + (int)(seed & 8191u);
    float lit[2] = { 0.0f, 0.0f };
    for (int k = 0; k < 2; k++) {
        int idx = frame / P540 - k;
        if (idx < 0) continue;
        int slot = idx & 1;
        float age = (float)(frame - idx * P540);
        if (bi540[slot] != idx) {
            gk_seed(&g540, seed ^ (uint32_t)(idx * 5147 + 99));
            ry540[slot] = ch * (0.3f + 0.4f * gk_rf(&g540));
            rx0540[slot] = cw * (0.05f + 0.15f * gk_rf(&g540));
            rx1540[slot] = cw * (0.8f + 0.15f * gk_rf(&g540));
            rtilt540[slot] = 0.18f * gk_rs(&g540);
            hue540[slot] = gk_rf(&g540);
            gk_bolt_gen(&g540, &b540[slot], 0.0f, 0.0f, 1.0f, 0.0f, 0.06f, 6, 3, 0.15f);
            for (int c = 0; c < NC540; c++) {
                cu540[slot][c] = 0.1f + 0.8f * gk_rf(&g540);
                cs540[slot][c] = gk_rf(&g540) < 0.5f ? -1.0f : 1.0f;
                gk_bolt_gen(&g540, &cr540[slot][c], 0.0f, 0.0f, 1.0f, 0.0f, 0.2f, 5, 2, 0.3f);
            }
            bi540[slot] = idx;
        }
        float open = gk_env(age, 150.0f, 130.0f, 150.0f);
        if (open <= 0.0f) continue;
        lit[slot] = open;
        float len = rx1540[slot] - rx0540[slot];
        float ang = rtilt540[slot];
        int pi = base + (int)(hue540[slot] * 8000.0f);
        float wide = 0.4f + 3.0f * open;                          /* halo grows as the rift opens */
        float c0[3], c1[3], h0[3], h1[3], g0[3], g1[3];
        gk_col(pal, pi, 0.05f, 0.35f * (0.4f + 0.6f * open), h0);
        gk_col(pal, pi + 2000, 0.05f, 0.35f * (0.4f + 0.6f * open), h1);
        gk_col(pal, pi + 300, 0.5f, 0.6f * (0.5f + 0.5f * open), c0);
        gk_col(pal, pi + 2300, 0.45f, 0.6f * (0.5f + 0.5f * open), c1);
        gk_col(pal, pi + 900, 0.15f, 0.22f * open, g0);
        gk_col(pal, pi + 2900, 0.15f, 0.22f * open, g1);
        float prog = age / 40.0f;                                  /* the crack runs across first */
        bx_draw_grad(&g540, &b540[slot], rx0540[slot], ry540[slot], ang, len, 1.0f, prog, 1.0f, g0, g1, 0.0f, 4.0f * sc * wide, 8.0f * sc * wide, 0.9f);
        bx_draw_grad(&g540, &b540[slot], rx0540[slot], ry540[slot], ang, len, 1.0f, prog, 1.0f, h0, h1, 0.0f, 2.0f * sc, 7.0f * sc, 0.5f);
        bx_draw_grad(&g540, &b540[slot], rx0540[slot], ry540[slot], ang, len, 1.0f, prog, 1.0f, c0, c1, 0.0f, 0.9f * sc, 2.4f * sc, 0.25f);
        /* edge crawlers, each on its own little clock while the rift is open */
        for (int c = 0; c < NC540; c++) {
            int P = 90 + c * 17;
            int ph = frame + c * 29;
            int ci = ph / P;
            float ca = (float)(ph - ci * P);
            float env = gk_env(ca, 12.0f, 20.0f, 40.0f) * open;
            if (env <= 0.0f) continue;
            float u = cu540[slot][c] + 0.15f * gk_hash((uint32_t)ci * 17u + (uint32_t)c);
            float ax = rx0540[slot] + cosf(ang) * len * u, ay = ry540[slot] + sinf(ang) * len * u;
            float a2 = ang + cs540[slot][c] * (0.9f + 0.6f * gk_hash((uint32_t)ci * 31u + (uint32_t)c)) * (gk_hash((uint32_t)ci * 7u + (uint32_t)c) < 0.5f ? 1.0f : -1.0f);
            float l2 = (20.0f + 30.0f * gk_hash((uint32_t)ci * 41u + (uint32_t)c)) * sc;
            float e0[3], e1[3];
            gk_col(pal, pi + 600, 0.4f, 0.55f * env, e0);
            gk_col(pal, pi + 1600, 0.2f, 0.4f * env, e1);
            bx_draw_grad(&g540, &cr540[slot][c], ax, ay, a2, l2, 1.0f, ca / 12.0f, 1.0f, e0, e1, 0.4f, 1.2f * sc, 4.0f * sc, 0.5f);
        }
    }
    /* cloud deck, lit near an open rift */
    gk_grid_fbm(grid540, GW540, GH540, 0.10f, t * 0.0011f + (float)(seed & 255u), t * 0.0006f, 91u);
    for (int y = 0; y < GH540; y++)
        for (int x = 0; x < GW540; x++) {
            float xx = ((float)x + 0.5f) / (float)GW540 * cw, yy = ((float)y + 0.5f) / (float)GH540 * ch;
            float d = grid540[y * GW540 + x];
            float l = 0.0f;
            for (int s = 0; s < 2; s++) {
                if (lit[s] <= 0.0f) continue;
                float dx = xx - rx0540[s], dy = yy - ry540[s];
                float ca = cosf(rtilt540[s]), sa = sinf(rtilt540[s]);
                float along = (dx * ca + dy * sa) / (rx1540[s] - rx0540[s]);
                float perp = (-dx * sa + dy * ca) / (ch * 0.16f);
                float inside = gk_smooth(along / 0.08f) * (1.0f - gk_smooth((along - 0.92f) / 0.08f));
                l += lit[s] * inside * expf(-perp * perp);
            }
            grid540[y * GW540 + x] = (0.05f + 0.22f * d * d) * (0.6f + 0.8f * d) + 0.5f * l * (0.5f + 0.7f * d);
        }
    gk_lut_ramp(lut540, pal, base + 2600, 4200, 0.3f, 1.25f, 1.15f);
    gk_grid_fill(&g540, grid540, GW540, GH540, lut540);
    gk_present(&g540, fb, w, h);
}
