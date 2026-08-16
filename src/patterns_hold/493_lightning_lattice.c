/* 493 Lightning Lattice — a 6x5 grid of nodes, slightly breathing, whose
 * edges are jagged arcs; a slow diagonal wave of charge sweeps the lattice
 * so bands of edges brighten and dim in turn, and each edge re-jags on its
 * own slow crossfade.  Reads as a full-frame electric mesh with black
 * cells — a field-density overlay.  Repaint pattern. */
#include "_hue469.h"

#define GX493 6
#define GY493 5
#define NE493 (GX493 * (GY493 - 1) + (GX493 - 1) * GY493)
#define P493 110

static gk g493;
static gk_bolt e493[NE493][2];
static int ei493[NE493][2];
static uint32_t es493 = 0xFFFFFFFFu;

void pattern_493(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g493, w, h);
    gk_clear(&g493);
    if (seed != es493) { memset(ei493, 0xFF, sizeof ei493); es493 = seed; }
    float cw = (float)g493.cw, ch = (float)g493.ch, sc = g493.sc, t = (float)frame;
    int base = (int)(t * 1.6f) + (int)(seed & 8191u);
    float nx[GX493 * GY493], ny[GX493 * GY493], ng[GX493 * GY493];
    for (int j = 0; j < GY493; j++)
        for (int i = 0; i < GX493; i++) {
            int k = j * GX493 + i;
            nx[k] = cw * ((float)i + 0.5f) / (float)GX493 + cw * 0.025f * sinf(t * 0.009f + (float)(i * 3 + j * 5));
            ny[k] = ch * ((float)j + 0.5f) / (float)GY493 + ch * 0.025f * cosf(t * 0.011f + (float)(i * 7 + j * 2));
            ng[k] = 0.0f;
        }
    int e = 0;
    for (int j = 0; j < GY493; j++)
        for (int i = 0; i < GX493; i++) {
            for (int dir = 0; dir < 2; dir++) {
                int a = j * GX493 + i, b;
                if (dir == 0) { if (i + 1 >= GX493) continue; b = a + 1; }
                else          { if (j + 1 >= GY493) continue; b = a + GX493; }
                float mxx = 0.5f * (nx[a] + nx[b]) / cw, myy = 0.5f * (ny[a] + ny[b]) / ch;
                /* diagonal wave */
                float ph = mxx * 1.3f + myy * 0.9f - t * 0.0035f;
                float wave = 0.5f + 0.5f * sinf(ph * GK_TAU);
                float str = 0.12f + 0.88f * wave * wave * wave;
                float dx = nx[b] - nx[a], dy = ny[b] - ny[a];
                float len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
                int pi = base + (int)(wave * 2500.0f) + dir * 1200;
                hk_style st;
                hk_style_set(&st, 1800, 900, 700,
                             0.4f * str, 1.5f * sc, 5.0f * sc, 0.5f,
                             0.40f, 0.55f * str, 0.7f * sc, 1.8f * sc, 0.2f);
                for (int q = 0; q < 2; q++) {
                    int p2 = frame + q * (P493 / 2) + e * 11;
                    int idx = p2 / P493;
                    float age = (float)(p2 - idx * P493);
                    if (ei493[e][q] != idx) {
                        gk_seed(&g493, seed ^ (uint32_t)(idx * 2371 + e * 611 + q * 71));
                        gk_bolt_gen(&g493, &e493[e][q], 0.0f, 0.0f, 1.0f, 0.0f, 0.09f, 4, 1, 0.3f);
                        ei493[e][q] = idx;
                    }
                    float env = gk_env(age, 32.0f, 24.0f, 32.0f) * 1.15f;
                    if (env <= 0.0f) continue;
                    hk_bolt_xf(&g493, &e493[e][q], nx[a], ny[a], ang, len, 2.0f, env, pal, pi + (int)(age * 5.0f), &st);
                }
                ng[a] += str * 0.3f; ng[b] += str * 0.3f;
                e++;
            }
        }
    for (int k = 0; k < GX493 * GY493; k++) {
        float c[3];
        gk_col(pal, base + 3000, 0.4f, 0.4f + 0.6f * ng[k], c);
        gk_dot(&g493, nx[k], ny[k], c, 2.0f * sc, (5.0f + 4.0f * ng[k]) * sc, 0.5f);
    }
    gk_present(&g493, fb, w, h);
}
