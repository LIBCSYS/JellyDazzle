/* 582 Rising Embers — sparse glints climbing from the bottom of the frame
 * over black, swaying on a slow noise wind, each with its own palette hue
 * that cools as it rises and a soft halo; they fade out before the top.
 * Repaint pattern (nothing persists), so it is transparent to the layers
 * beneath wherever it is not drawing. */
#include "_spark572.h"

#define NP582 150

static gk g582;
static sk_part p582[NP582];
static uint32_t bs582 = 0xFFFFFFFFu;
static int base582;
static float hue0582;

static void born582(sk_part *p, uint32_t r, float cw, float ch, float sc, int pre)
{
    p->x = gk_hash(r + 1u) * cw;
    p->y = ch + 6.0f * sc;
    p->vx = 0.0f;
    p->vy = -(0.25f + 0.55f * gk_hash(r + 2u)) * sc;
    p->life = 320.0f + 380.0f * gk_hash(r + 3u);
    p->age = pre ? gk_hash(r + 4u) * p->life : 0.0f;
    if (pre) p->y = ch - gk_hash(r + 5u) * ch;
    p->hue = hue0582 + gk_hash(r + 6u) * 0.14f;
    p->size = (1.0f + 1.9f * gk_hash(r + 7u)) * sc;
    p->amp = 0.6f + 0.6f * gk_hash(r + 8u);
    p->ph = gk_hash(r + 9u) * 100.0f;
    p->alive = 1;
}

void pattern_582(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g582, w, h);
    gk_clear(&g582);
    float cw = (float)g582.cw, ch = (float)g582.ch, sc = g582.sc;
    if (seed != bs582) {
        base582 = (int)(seed & 0x7FFFu);
        hue0582 = gk_hash(seed ^ 0x582u);
        for (i = 0; i < NP582; i++) born582(&p582[i], seed ^ (uint32_t)(i * 7919), cw, ch, sc, 1);
        bs582 = seed;
    }
    float t = (float)frame;
    float col[3];
    for (i = 0; i < NP582; i++) {
        sk_part *p = &p582[i];
        p->age += 1.0f;
        if (p->age >= p->life || p->y < -8.0f) born582(p, seed ^ (uint32_t)(i * 7919 + frame * 131), cw, ch, sc, 0);
        float wind = (gk_noise2(p->x * 0.004f / sc, t * 0.004f + p->ph, 7u) - 0.5f) * 0.6f * sc
                   + (gk_noise1(t * 0.02f + p->ph, (uint32_t)i) - 0.5f) * 0.3f * sc;
        p->vx += (wind - p->vx) * 0.03f;
        p->x += p->vx; p->y += p->vy;
        float u = p->age / p->life;
        float e = sk_bump(p->age, p->life, 60.0f, 120.0f);
        /* slow breathing glint */
        float br = 0.7f + 0.3f * gk_noise1(t * 0.03f + p->ph, (uint32_t)i + 99u);
        float hue = p->hue + u * 0.10f + t * 0.00003f;
        sk_col(pal, sk_hidx(base582, hue), 0.35f * (1.0f - u), 0.6f, e * br * p->amp, col);
        gk_dot(&g582, p->x, p->y, col, p->size, p->size * 3.5f, 0.3f);
    }
    gk_present(&g582, fb, w, h);
}
