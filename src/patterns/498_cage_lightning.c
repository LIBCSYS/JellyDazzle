/* 498 Cage Lightning — a wireframe cube tumbling slowly in perspective,
 * every edge a jagged discharge that re-forms on its own slow crossfade,
 * the vertices glowing as electrodes.  Nearer edges are brighter and
 * thicker.  Figure overlay, transparent through the faces.  Repaint. */
#include "_hue469.h"

#define NE498 12
#define P498 120

static gk g498;
static gk_bolt e498[NE498][2];
static int ei498[NE498][2];
static uint32_t es498 = 0xFFFFFFFFu;

static const int edges498[NE498][2] = {
    {0,1},{1,3},{3,2},{2,0}, {4,5},{5,7},{7,6},{6,4}, {0,4},{1,5},{2,6},{3,7}
};

void pattern_498(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g498, w, h);
    gk_clear(&g498);
    if (seed != es498) { memset(ei498, 0xFF, sizeof ei498); es498 = seed; }
    float sc = g498.sc, t = (float)frame;
    int base = (int)(t * 1.6f) + (int)(seed & 8191u);
    gk_cam cam;
    gk_cam_set(&cam, &g498, t * 0.004f, 0.5f + 0.4f * sinf(t * 0.0023f), 4.0f, 130.0f);
    float vx[8], vy[8], vd[8];
    for (int i = 0; i < 8; i++) {
        float x = (i & 1) ? 1.0f : -1.0f, y = (i & 2) ? 1.0f : -1.0f, z = (i & 4) ? 1.0f : -1.0f;
        gk_project(&cam, x, y, z, &vx[i], &vy[i], &vd[i]);
    }
    for (int e = 0; e < NE498; e++) {
        int a = edges498[e][0], b = edges498[e][1];
        float dx = vx[b] - vx[a], dy = vy[b] - vy[a];
        float len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        float depth = 0.5f * (vd[a] + vd[b]);          /* ~0.8 far .. 1.3 near */
        float near = gk_smooth((depth - 0.75f) * 1.8f);
        float amp = 0.3f + 0.7f * near;
        /* edge hue grades from vertex a's colour to vertex b's */
        int pi = base + a * 300;
        float ws = 0.6f + 0.6f * near;
        hk_style st;
        hk_style_set(&st, (b - a) * 300 + 1500, 900, 700,
                     0.45f * amp, 1.8f * sc * ws, 6.0f * sc * ws, 0.5f,
                     0.40f, 0.65f * amp, 0.8f * sc * ws, 2.0f * sc * ws, 0.25f);
        for (int q = 0; q < 2; q++) {
            int ph = frame + q * (P498 / 2) + e * 13;
            int idx = ph / P498;
            float age = (float)(ph - idx * P498);
            if (ei498[e][q] != idx) {
                gk_seed(&g498, seed ^ (uint32_t)(idx * 4451 + e * 809 + q * 61));
                gk_bolt_gen(&g498, &e498[e][q], 0.0f, 0.0f, 1.0f, 0.0f, 0.10f, 5, 2, 0.3f);
                ei498[e][q] = idx;
            }
            float env = gk_env(age, 35.0f, 24.0f, 35.0f) * 1.15f;
            if (env <= 0.0f) continue;
            hk_bolt_xf(&g498, &e498[e][q], vx[a], vy[a], ang, len, 2.0f, env, pal, pi + (int)(age * 5.0f), &st);
        }
    }
    for (int i = 0; i < 8; i++) {
        float c[3];
        float near = gk_smooth((vd[i] - 0.75f) * 1.8f);
        gk_col(pal, base + 3000 + i * 300, 0.5f, 0.6f + 0.8f * near, c);
        gk_dot(&g498, vx[i], vy[i], c, (1.5f + 1.5f * near) * sc, (5.0f + 5.0f * near) * sc, 0.5f);
    }
    gk_present(&g498, fb, w, h);
}
