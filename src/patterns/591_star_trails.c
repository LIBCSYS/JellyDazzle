/* 591 Star Trails — a long-exposure sky: hundreds of stars wheel very slowly
 * about a seed-placed pole and the canvas forgets slowly, so each draws a
 * short coloured arc behind it that lengthens as the segment runs.  Stars
 * carry hues across a wide palette window; the pole star breathes.
 * ACCUMULATOR (slow decay, cleared at segment start). */
#include "_spark572.h"

#define NP591 320

static gk g591;
static float rr591[NP591], ra591[NP591], rh591[NP591], rs591[NP591], rp591[NP591];
static uint32_t bs591 = 0xFFFFFFFFu;
static int base591;
static float polex591, poley591, omega591;

void pattern_591(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g591, w, h);
    float cw = (float)g591.cw, ch = (float)g591.ch, sc = g591.sc;
    if (seed != bs591 || sl < 2) {
        gk_clear(&g591);
        base591 = (int)(seed & 0x7FFFu);
        polex591 = cw * (0.25f + 0.5f * gk_hash(seed ^ 1u));
        poley591 = ch * (0.25f + 0.5f * gk_hash(seed ^ 2u));
        omega591 = (gk_hash(seed ^ 3u) < 0.5f ? 1.0f : -1.0f) * 0.0011f;
        float rmax = sqrtf(cw * cw + ch * ch);
        for (i = 0; i < NP591; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 4451);
            rr591[i] = rmax * sqrtf(gk_hash(r + 1u)) * 0.75f + 4.0f;
            ra591[i] = gk_hash(r + 2u) * GK_TAU;
            rh591[i] = gk_hash(r + 3u);
            rs591[i] = (0.6f + 1.2f * gk_hash(r + 4u)) * sc;
            rp591[i] = gk_hash(r + 5u) * 100.0f;
        }
        bs591 = seed;
    }
    gk_decay_snap(&g591, 0.994f);
    float t = (float)frame;
    float col[3];
    for (i = 0; i < NP591; i++) {
        float a = ra591[i] + t * omega591;
        float x = polex591 + cosf(a) * rr591[i], y = poley591 + sinf(a) * rr591[i];
        if (x < -4.0f || y < -4.0f || x > cw + 4.0f || y > ch + 4.0f) continue;
        float tw = 0.75f + 0.25f * gk_noise1(t * 0.01f + rp591[i], (uint32_t)i);
        sk_col(pal, sk_hidx(base591, rh591[i] + t * 0.00002f), 0.3f, 0.5f, 0.045f * tw * (0.6f + 0.4f * rs591[i] / sc), col);
        gk_dot(&g591, x, y, col, rs591[i] * 0.8f, rs591[i] * 2.5f, 0.3f);
    }
    /* pole star, faint and steady */
    sk_col(pal, sk_hidx(base591, gk_hash(seed ^ 9u) + t * 0.00002f), 0.4f, 0.5f, 0.02f + 0.006f * gk_noise1(t * 0.01f, 3u), col);
    gk_dot(&g591, polex591, poley591, col, 1.5f * sc, 6.0f * sc, 0.5f);
    gk_present(&g591, fb, w, h);
}
