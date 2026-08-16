/* 474 Storm Front — a full-width squall line.  A rolling cloud deck (fbm)
 * fills the upper half, its underside lit by whichever bolts are burning;
 * five strike slots stagger down from the deck to a faint ground line, each
 * bolt growing over ~24 frames and dying over ~60.  Sky above the deck and
 * ground below stay dark, so it reads as a mid-layer field.  Repaint. */
#include "_hue469.h"

#define GW474 48
#define GH474 36
#define NS474 5
#define P474 190

static gk g474;
static float grid474[GW474 * GH474];
static float lut474[256 * 3];
static gk_bolt b474[NS474];
static int bi474[NS474];
static uint32_t bs474 = 0xFFFFFFFFu;
static float bx474[NS474], bh474[NS474];

static void gen474(int s, int idx, uint32_t seed)
{
    gk_seed(&g474, seed ^ (uint32_t)(idx * 4409 + s * 15013));
    float cw = (float)g474.cw, ch = (float)g474.ch;
    float x0 = cw * ((float)s + 0.15f + 0.7f * gk_rf(&g474)) / (float)NS474;
    float y0 = ch * (0.30f + 0.15f * gk_rf(&g474));
    gk_bolt_gen(&g474, &b474[s], x0, y0, x0 + cw * 0.12f * gk_rs(&g474), ch * 0.9f,
                0.18f, 6, 3 + (int)(gk_rf(&g474) * 3.0f), 0.4f);
    bx474[s] = x0; bh474[s] = gk_rf(&g474);
    bi474[s] = idx;
}

void pattern_474(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g474, w, h);
    gk_clear(&g474);
    if (seed != bs474) { for (int i = 0; i < NS474; i++) bi474[i] = -1; bs474 = seed; }
    float cw = (float)g474.cw, sc = g474.sc, t = (float)frame;
    int base = (int)(t * 1.9f) + (int)(seed & 8191u);
    /* strikes */
    float lx[NS474], la[NS474];
    int nl = 0;
    for (int s = 0; s < NS474; s++) {
        int ph = frame + s * 37 + (s * s) * 11;
        int idx = ph / P474;
        float age = (float)(ph - idx * P474);
        if (idx != bi474[s]) gen474(s, idx, seed);
        float env = gk_env(age, 8.0f, 24.0f, 60.0f);
        if (env <= 0.0f) continue;
        lx[nl] = bx474[s] / cw * (float)GW474; la[nl] = env; nl++;
        int pi = base + (int)(bh474[s] * 6000.0f) + (int)(age * 12.0f);
        hk_style st;
        hk_style_set(&st, 5000, 1800, 900,
                     0.7f * env, 2.0f * sc, 7.0f * sc, 0.5f,
                     0.40f, 1.3f * env, 0.9f * sc, 2.4f * sc, 0.25f);
        hk_bolt(&g474, &b474[s], age / 24.0f, 1.0f, pal, pi, &st);
    }
    /* cloud deck: fbm density, shaped to a band with a ragged lower edge */
    gk_grid_fbm(grid474, GW474, GH474, 0.11f, t * 0.0016f + (float)(seed & 255u), t * 0.0003f, 21u);
    for (int y = 0; y < GH474; y++)
        for (int x = 0; x < GW474; x++) {
            float d = grid474[y * GW474 + x];
            float yy = (float)y / (float)GH474;
            float edge = 0.30f + 0.18f * d;                  /* deck bottom */
            float band = gk_smooth((edge - yy) * 7.0f) * gk_smooth((yy - 0.02f) * 6.0f);
            float under = expf(-(edge - yy) * (edge - yy) * 90.0f);  /* belly */
            float lit = 0.10f + 0.12f * under;
            for (int k = 0; k < nl; k++) {
                float dx = ((float)x - lx[k]) / (float)GW474 * 3.2f;
                lit += la[k] * 0.9f * expf(-dx * dx) * (0.3f + under);
            }
            /* ground line glow */
            float gl = expf(-(yy - 0.90f) * (yy - 0.90f) * 400.0f) * 0.06f;
            grid474[y * GW474 + x] = band * lit * (0.5f + 0.7f * d) + gl;
        }
    gk_lut_ramp(lut474, pal, base + 2000, 4000, 0.3f, 2.4f, 1.2f);
    gk_grid_fill(&g474, grid474, GW474, GH474, lut474);
    gk_present(&g474, fb, w, h);
}
