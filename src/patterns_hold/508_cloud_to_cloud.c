/* 508 Cloud to Cloud — two soft cloud masses drift near the left and right
 * edges; every so often a long horizontal bolt crosses the gap between
 * them, growing from one cloud to the other over ~40 frames, and both
 * clouds glow from inside as the charge passes.  Wide figure overlay,
 * dark above and below.  Repaint pattern. */
#include "_hue469.h"

#define GW508 64
#define GH508 40
#define P508 260

static gk g508;
static float grid508[GW508 * GH508];
static float lut508[256 * 3];
static gk_bolt b508[2];
static int bi508[2] = { -1, -1 };
static uint32_t bs508 = 0xFFFFFFFFu;
static int dir508[2];

void pattern_508(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g508, w, h);
    gk_clear(&g508);
    if (seed != bs508) { bi508[0] = bi508[1] = -1; bs508 = seed; }
    float cw = (float)g508.cw, ch = (float)g508.ch, sc = g508.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    float lx = 0.16f + 0.04f * sinf(t * 0.004f), rx = 0.84f + 0.04f * cosf(t * 0.0035f);
    float ly = 0.45f + 0.06f * sinf(t * 0.003f + 1.0f), ry = 0.5f + 0.06f * cosf(t * 0.0027f);
    float glowL = 0.0f, glowR = 0.0f;
    for (int s = 0; s < 2; s++) {
        int ph = frame + s * (P508 / 2);
        int idx = ph / P508;
        float age = (float)(ph - idx * P508);
        if (idx != bi508[s]) {
            gk_seed(&g508, seed ^ (uint32_t)(idx * 3079 + s * 8837));
            dir508[s] = gk_rf(&g508) < 0.5f ? 1 : -1;
            gk_bolt_gen(&g508, &b508[s], 0.0f, 0.0f, 1.0f, 0.0f, 0.15f, 7, 5, 0.35f);
            bi508[s] = idx;
        }
        float env = gk_env(age, 8.0f, 50.0f, 80.0f);
        if (env <= 0.0f) continue;
        float ax = (dir508[s] > 0 ? lx : rx) * cw, ay = (dir508[s] > 0 ? ly : ry) * ch;
        float bx = (dir508[s] > 0 ? rx : lx) * cw, by = (dir508[s] > 0 ? ry : ly) * ch;
        float dx = bx - ax, dy = by - ay, len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        float prog = age / 40.0f;
        int pi = base + s * 3000 + (int)(age * 10.0f);
        hk_style st;                      /* hue crosses cloud to cloud */
        hk_style_set(&st, 5500, 1800, 800,
                     0.5f * env, 2.0f * sc, 7.0f * sc, 0.5f,
                     0.40f, 0.85f * env, 0.9f * sc, 2.4f * sc, 0.25f);
        hk_bolt_xf(&g508, &b508[s], ax, ay, ang, len, prog, 1.0f, pal, pi, &st);
        float src = env, dst = env * gk_smooth(prog - 0.9f);
        if (dir508[s] > 0) { glowL += src; glowR += dst; } else { glowR += src; glowL += dst; }
    }
    gk_grid_fbm(grid508, GW508, GH508, 0.14f, t * 0.0013f + (float)(seed & 255u), t * 0.0004f, 61u);
    for (int y = 0; y < GH508; y++)
        for (int x = 0; x < GW508; x++) {
            float xx = ((float)x + 0.5f) / (float)GW508, yy = ((float)y + 0.5f) / (float)GH508;
            float d = grid508[y * GW508 + x];
            float dl = ((xx - lx) * (xx - lx) * 1.0f + (yy - ly) * (yy - ly) * 2.2f) / 0.05f;
            float dr = ((xx - rx) * (xx - rx) * 1.0f + (yy - ry) * (yy - ry) * 2.2f) / 0.05f;
            float ml = gk_smooth((1.4f + 1.2f * (d - 0.5f) - dl) * 0.9f);
            float mr = gk_smooth((1.4f + 1.2f * (d - 0.5f) - dr) * 0.9f);
            float v = ml * (0.14f + 0.7f * glowL * expf(-dl * 0.4f)) + mr * (0.14f + 0.7f * glowR * expf(-dr * 0.4f));
            grid508[y * GW508 + x] = v * (0.4f + 1.0f * d);
        }
    gk_lut_ramp(lut508, pal, base + 1500, 4500, 0.35f, 2.2f, 1.2f);
    gk_grid_fill(&g508, grid508, GW508, GH508, lut508);
    gk_present(&g508, fb, w, h);
}
