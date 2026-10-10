/* 611 Jellyfish Drift (creature) — a few medusae rise slowly through dark
 * water.  Each is a pulsing bell drawn as stacked arcs with a ring of
 * trailing tentacles that lag the bell, so the swim reads as propulsion
 * rather than a sprite sliding upward.  The bell contracts, the tentacles
 * catch up a beat later, and the whole animal gains a little height on each
 * pulse.  Hue per animal from the live palette; nothing carries its own RGB.
 * Repaint pattern with a soft decay, so the water keeps a faint wake. */
#include "_spark572.h"

#define NJ611   5          /* animals on screen                            */
#define NT611   9          /* tentacles per animal                         */
#define TSEG611 7          /* segments per tentacle                        */

static gk g611;
static uint32_t bs611 = 0xFFFFFFFFu;
static int   base611;
static float jx611[NJ611], jy611[NJ611], jsc611[NJ611], jhue611[NJ611];
static float jph611[NJ611], jrate611[NJ611], jdrift611[NJ611];

void pattern_611(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k, s;
    gk_setup(&g611, w, h);
    gk_decay_snap(&g611, 0.82f);                  /* faint wake, not a smear */

    float cw = (float)g611.cw, ch = (float)g611.ch, sc = g611.sc;

    if (seed != bs611) {                          /* re-seed: new cast       */
        base611 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NJ611; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 7919);
            jx611[i]     = gk_hash(r + 1u) * cw;
            jy611[i]     = gk_hash(r + 2u) * ch;
            jsc611[i]    = (0.45f + 0.75f * gk_hash(r + 3u)) * sc;
            jhue611[i]   = gk_hash(r + 4u);
            jph611[i]    = gk_hash(r + 5u) * 6.2831853f;
            jrate611[i]  = 0.022f + 0.020f * gk_hash(r + 6u);
            jdrift611[i] = (gk_hash(r + 7u) - 0.5f) * 0.25f;
        }
        bs611 = seed;
    }

    float t = (float)frame;
    float col[3];

    for (i = 0; i < NJ611; i++) {
        /* One pulse cycle: 0 relaxed, 1 fully contracted. Thrust follows the
         * contraction, so the animal moves in beats instead of gliding. */
        float ph    = jph611[i] + t * jrate611[i];
        float pulse = 0.5f - 0.5f * sinf(ph);
        float thrust = pulse * pulse;

        jy611[i] -= (0.25f + 1.55f * thrust) * jsc611[i];
        jx611[i] += jdrift611[i] * jsc611[i]
                  + (gk_noise1(t * 0.004f + (float)i * 13.0f, 90u + (uint32_t)i) - 0.5f) * 0.6f * jsc611[i];

        if (jy611[i] < -70.0f * jsc611[i]) {      /* leaves the top, returns below */
            jy611[i] = ch + 50.0f * jsc611[i];
            jx611[i] = gk_hash((uint32_t)(t) ^ (uint32_t)(i * 4211)) * cw;
            jhue611[i] = gk_hash((uint32_t)(t) ^ (uint32_t)(i * 777));
        }
        if (jx611[i] < -60.0f) jx611[i] += cw + 120.0f;
        if (jx611[i] > cw + 60.0f) jx611[i] -= cw + 120.0f;

        float x = jx611[i], y = jy611[i], S = jsc611[i];
        float rx = (16.0f + 5.0f * pulse) * S;    /* bell widens as it relaxes */
        float ry = (11.0f - 3.0f * pulse) * S;

        /* BELL: stacked arcs, brightest at the crown. Drawn as short segments
         * so the rim stays soft at every scale. */
        for (k = 0; k < 4; k++) {
            float f  = (float)k / 3.0f;
            float ex = rx * (1.0f - 0.16f * f);
            float ey = ry * (1.0f - 0.16f * f);
            sk_col(pal, sk_hidx(base611, jhue611[i] + 0.04f * f),
                   0.30f + 0.28f * (1.0f - f), 0.55f, 1.0f, col);
            float px = x - ex, py = y;
            for (s = 1; s <= 12; s++) {
                float a  = 3.14159265f * (float)s / 12.0f;   /* pi..0, top half */
                float nx = x - ex * cosf(a);
                float ny = y - ey * sinf(a);
                gk_seg(&g611, px, py, nx, ny, col,
                       0.9f * S, 3.0f * S, 0.22f);
                px = nx; py = ny;
            }
        }

        /* TENTACLES: each lags the bell by a growing delay down its length, so
         * a contraction travels outward instead of the whole animal snapping. */
        for (k = 0; k < NT611; k++) {
            float u  = ((float)k / (float)(NT611 - 1)) * 2.0f - 1.0f;   /* -1..1 */
            float ax = x + u * rx * 0.82f;
            float ay = y + ry * 0.30f;
            sk_col(pal, sk_hidx(base611, jhue611[i] + 0.10f + 0.03f * (float)k),
                   0.16f, 0.45f, 0.80f, col);
            float px = ax, py = ay;
            for (s = 1; s <= TSEG611; s++) {
                float d    = (float)s / (float)TSEG611;
                float lag  = ph - d * 1.9f;                  /* the delay      */
                float sway = sinf(lag) * (5.0f + 9.0f * d) * S * 0.55f;
                float nx = ax + sway * 0.75f + u * 2.2f * S * d;
                float ny = ay + d * (26.0f + 16.0f * (1.0f - thrust)) * S;
                gk_seg(&g611, px, py, nx, ny, col,
                       0.55f * S, 2.0f * S, 0.13f * (1.0f - 0.55f * d));
                px = nx; py = ny;
            }
        }
    }

    gk_present(&g611, fb, w, h);
}
