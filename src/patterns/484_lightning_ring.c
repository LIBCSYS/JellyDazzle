/* 484 Lightning Ring — twelve electrodes on a slowly turning circle; arcs
 * jump between neighbours (and sometimes across the ring) so the circle is
 * traced by a shifting garland of discharges.  Each arc grows across in
 * ~20 frames and dims over ~50; the ring breathes in radius.  Figure
 * overlay, transparent inside and out.  Repaint pattern. */
#include "_hue469.h"

#define NE484 12
#define NA484 10
#define P484 150

static gk g484;
static gk_bolt a484[NA484];
static int ai484[NA484];
static uint32_t as484 = 0xFFFFFFFFu;
static int ea484[NA484], eb484[NA484];
static float hue484[NA484];

void pattern_484(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g484, w, h);
    gk_clear(&g484);
    if (seed != as484) { for (int i = 0; i < NA484; i++) ai484[i] = -1; as484 = seed; }
    float cw = (float)g484.cw, ch = (float)g484.ch, sc = g484.sc, t = (float)frame;
    float cx = cw * 0.5f, cy = ch * 0.5f;
    float R = (cw < ch ? cw : ch) * (0.36f + 0.03f * sinf(t * 0.011f));
    float rot = t * 0.0025f;
    int base = (int)(t * 1.9f) + (int)(seed & 8191u);
    float ex[NE484], ey[NE484], eg[NE484];
    for (int i = 0; i < NE484; i++) {
        float a = rot + GK_TAU * (float)i / (float)NE484;
        ex[i] = cx + cosf(a) * R; ey[i] = cy + sinf(a) * R; eg[i] = 0.0f;
    }
    for (int s = 0; s < NA484; s++) {
        int ph = frame + (s * 41) % P484;
        int idx = ph / P484;
        float age = (float)(ph - idx * P484);
        if (idx != ai484[s]) {
            gk_seed(&g484, seed ^ (uint32_t)(idx * 4457 + s * 6199));
            ea484[s] = (int)(gk_rf(&g484) * NE484) % NE484;
            int span = gk_rf(&g484) < 0.75f ? 1 : 2 + (int)(gk_rf(&g484) * 4.0f);
            eb484[s] = (ea484[s] + span) % NE484;
            /* unit arc, bowed outward a little via jag */
            gk_bolt_gen(&g484, &a484[s], 0.0f, 0.0f, 1.0f, 0.0f, 0.15f, 5, 2, 0.3f);
            hue484[s] = gk_rf(&g484);
            ai484[s] = idx;
        }
        float env = gk_env(age, 8.0f, 25.0f, 50.0f);
        if (env <= 0.0f) continue;
        int a = ea484[s], b = eb484[s];
        float dx = ex[b] - ex[a], dy = ey[b] - ey[a];
        float len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        int pi = base + (int)(hue484[s] * 6000.0f) + (int)(age * 12.0f);
        hk_style st;
        hk_style_set(&st, 4000, 1500, 800,
                     0.5f * env, 1.8f * sc, 6.0f * sc, 0.5f,
                     0.40f, 0.75f * env, 0.8f * sc, 2.0f * sc, 0.2f);
        hk_bolt_xf(&g484, &a484[s], ex[a], ey[a], ang, len, age / 20.0f, 1.0f, pal, pi, &st);
        eg[a] += env * 0.5f; eg[b] += env * 0.5f * gk_smooth(age / 20.0f - 0.8f);
    }
    for (int i = 0; i < NE484; i++) {
        float c[3];
        gk_col(pal, base + i * 600, 0.35f, 0.5f + 1.0f * eg[i], c);
        gk_dot(&g484, ex[i], ey[i], c, 2.0f * sc, (5.0f + 4.0f * eg[i]) * sc, 0.5f);
    }
    gk_present(&g484, fb, w, h);
}
