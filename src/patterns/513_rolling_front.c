/* 513 Rolling Front — a squall line seen obliquely: the shelf cloud's
 * leading edge runs from near-left (low, large) to far-right (high,
 * small), the deck above it a slow-rolling fbm mass; six strike slots
 * are staggered along the edge, each bolt sized by its distance, growing
 * over ~24 frames from the shelf edge to the ground and dying over ~70,
 * lighting the shelf above it.  Bolt hue differs per slot and along each
 * bolt.  Sky over the deck and ground below stay dark.  Field.  Repaint. */
#include "_trace509.h"

#define GW513 72
#define GH513 48
#define NS513 6
#define P513 300

static gk g513;
static float grid513[GW513 * GH513];
static float lut513[256 * 3];
static gk_bolt b513[NS513];
static int bi513[NS513] = { -1, -1, -1, -1, -1, -1 };
static uint32_t bs513 = 0xFFFFFFFFu;
static float sx513[NS513], hue513[NS513];

/* shelf edge height (0..1 of frame) at horizontal position u (0..1) */
static inline float edge513(float u) { return 0.74f - 0.34f * u; }
/* perspective scale at u: near = 1, far = 0.35 */
static inline float pers513(float u) { return 1.0f - 0.65f * u; }

void pattern_513(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g513, w, h);
    gk_clear(&g513);
    if (seed != bs513) { for (int s = 0; s < NS513; s++) bi513[s] = -1; bs513 = seed; }
    float cw = (float)g513.cw, ch = (float)g513.ch, sc = g513.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float litx[NS513], litv[NS513];
    for (int s = 0; s < NS513; s++) {
        int ph = frame + s * (P513 / NS513) + s * 11;
        int idx = ph / P513;
        float age = (float)(ph - idx * P513);
        if (idx != bi513[s]) {
            gk_seed(&g513, seed ^ (uint32_t)(idx * 3391 + s * 7877));
            sx513[s] = ((float)s + 0.15f + 0.7f * gk_rf(&g513)) / (float)NS513;
            hue513[s] = gk_rf(&g513);
            gk_bolt_gen(&g513, &b513[s], 0.0f, 0.0f, 0.15f * gk_rs(&g513), 1.0f, 0.18f, 7, 4, 0.35f);
            bi513[s] = idx;
        }
        float env = gk_env(age, 8.0f, 40.0f, 70.0f);
        litx[s] = sx513[s]; litv[s] = 0.0f;
        if (env <= 0.0f) continue;
        litv[s] = env;
        float u = sx513[s], p = pers513(u);
        float x0 = u * cw, y0 = edge513(u) * ch;
        float len = ch * (0.95f - edge513(u)) * (0.6f + 0.4f * p);   /* to the ground line */
        float prog = age / 24.0f;
        int pi = base + (int)(hue513[s] * 8000.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.45f * env, h0);
        gk_col(pal, pi + 1500, 0.05f, 0.35f * env, h1);
        gk_col(pal, pi + 300, 0.65f, 0.7f * env, c0);
        gk_col(pal, pi + 1800, 0.45f, 0.6f * env, c1);
        float thick = 0.4f + 0.6f * p;
        bx_draw_grad(&g513, &b513[s], x0, y0, 0.0f, len, 1.0f, prog, 1.0f, h0, h1, 0.15f, 2.0f * sc * thick, 7.0f * sc * thick, 0.5f);
        bx_draw_grad(&g513, &b513[s], x0, y0, 0.0f, len, 1.0f, prog, 1.0f, c0, c1, 0.15f, 0.9f * sc * thick, 2.4f * sc * thick, 0.25f);
        /* ground glow at the foot */
        float gg[3];
        gk_col(pal, pi + 600, 0.3f, 0.35f * env * gk_smooth(prog - 0.8f), gg);
        gk_dot(&g513, x0, y0 + len, gg, 6.0f * sc * thick, 26.0f * sc * thick, 0.6f);
    }
    /* the shelf deck: fbm masked above the edge, lit by the strikes */
    gk_grid_fbm(grid513, GW513, GH513, 0.11f, t * 0.0016f + (float)(seed & 255u), t * 0.0005f, 43u);
    for (int y = 0; y < GH513; y++)
        for (int x = 0; x < GW513; x++) {
            float xx = ((float)x + 0.5f) / (float)GW513, yy = ((float)y + 0.5f) / (float)GH513;
            float d = grid513[y * GW513 + x];
            float e = edge513(xx);
            float below = (yy - e) / 0.10f;              /* >0 under the shelf */
            float m = gk_smooth(1.0f - below + 0.8f * (d - 0.5f));
            float top = gk_smooth((yy - (e - 0.55f)) / 0.25f);   /* dark upper sky fades in */
            float lit = 0.0f;
            for (int s = 0; s < NS513; s++) {
                float dx = (xx - litx[s]) / (0.10f + 0.06f * (1.0f - litx[s]));
                float dy = (yy - e) / 0.12f;
                lit += litv[s] * expf(-(dx * dx + dy * dy));
            }
            grid513[y * GW513 + x] = m * top * (0.06f + 0.36f * d * d + 0.6f * lit) * (0.5f + 0.9f * d);
        }
    gk_lut_ramp(lut513, pal, base + 2200, 4500, 0.35f, 1.7f, 1.25f);
    gk_grid_fill(&g513, grid513, GW513, GH513, lut513);
    gk_present(&g513, fb, w, h);
}
