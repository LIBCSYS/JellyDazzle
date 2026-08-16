/* 598 Rain Glints — sparse raindrops catching light: short falling streaks
 * at an easy pace, and where each lands, a soft ring spreads and fades.
 * Drops carry cool palette hues, rings the same hue a touch warmer; the
 * fall leans with a slow wind.  Repaint pattern. */
#include "_spark572.h"

#define ND598 56
#define NR598 40

static gk g598;
static sk_part d598[ND598], r598[NR598];
static uint32_t bs598 = 0xFFFFFFFFu;
static int base598;
static float hue0598;

static void born598(sk_part *p, uint32_t r, float cw, float ch, float sc, int pre)
{
    p->x = gk_hash(r + 1u) * (cw + 60.0f) - 30.0f;
    p->y = pre ? gk_hash(r + 2u) * ch : -14.0f * sc;
    p->vy = (1.2f + 1.3f * gk_hash(r + 3u)) * sc;
    p->vx = 0.0f;
    p->life = ch * (0.78f + 0.2f * gk_hash(r + 4u));      /* landing height */
    p->size = (5.0f + 7.0f * gk_hash(r + 5u)) * sc;       /* streak length */
    p->hue = hue0598 + gk_hash(r + 6u) * 0.1f;
    p->amp = 0.5f + 0.5f * gk_hash(r + 7u);
    p->alive = 1;
}

void pattern_598(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g598, w, h);
    gk_clear(&g598);
    float cw = (float)g598.cw, ch = (float)g598.ch, sc = g598.sc;
    if (seed != bs598) {
        base598 = (int)(seed & 0x7FFFu);
        hue0598 = gk_hash(seed ^ 0x598u);
        for (i = 0; i < ND598; i++) born598(&d598[i], seed ^ (uint32_t)(i * 5099), cw, ch, sc, 1);
        for (i = 0; i < NR598; i++) r598[i].alive = 0;
        bs598 = seed;
    }
    float t = (float)frame;
    float wind = (gk_noise1(t * 0.002f, 15u) - 0.5f) * 0.6f * sc;
    float col[3];
    for (i = 0; i < ND598; i++) {
        sk_part *p = &d598[i];
        p->vx += (wind - p->vx) * 0.05f;
        p->x += p->vx; p->y += p->vy;
        if (p->y >= p->life) {
            int k;
            for (k = 0; k < NR598; k++) if (!r598[k].alive) break;
            if (k < NR598) {
                sk_part *q = &r598[k];
                q->x = p->x; q->y = p->y; q->age = 0.0f; q->life = 70.0f;
                q->hue = p->hue + 0.03f; q->amp = p->amp; q->size = (8.0f + 10.0f * gk_hash((uint32_t)frame * 31u + (uint32_t)i)) * sc;
                q->alive = 1;
            }
            born598(p, seed ^ (uint32_t)(i * 5099 + frame * 211), cw, ch, sc, 0);
        }
        float fin = p->y < 30.0f * sc ? (p->y + 14.0f * sc) / (44.0f * sc) : 1.0f;
        if (fin < 0.0f) fin = 0.0f;
        sk_col(pal, sk_hidx(base598, p->hue + t * 0.00003f), 0.35f, 0.5f, 0.9f * p->amp * fin, col);
        float dl = sqrtf(p->vx * p->vx + p->vy * p->vy);
        float ux = p->vx / dl, uy = p->vy / dl;
        sk_line(&g598, p->x - ux * p->size, p->y - uy * p->size, p->x, p->y, 1.2f * sc, col);
        float c2[3] = { col[0] * 1.2f, col[1] * 1.2f, col[2] * 1.2f };
        gk_dot(&g598, p->x, p->y, c2, 1.0f * sc, 3.0f * sc, 0.4f);
    }
    for (i = 0; i < NR598; i++) {
        sk_part *q = &r598[i];
        if (!q->alive) continue;
        q->age += 1.0f;
        if (q->age >= q->life) { q->alive = 0; continue; }
        float u = q->age / q->life;
        float e = (1.0f - u); e = e * e;
        float rr = q->size * (0.15f + 0.85f * (1.0f - (1.0f - u) * (1.0f - u)));
        sk_col(pal, sk_hidx(base598, q->hue + t * 0.00003f), 0.2f, 0.5f, 0.7f * e * q->amp, col);
        gk_ring(&g598, q->x, q->y, rr, 1.6f * sc, col);
    }
    gk_present(&g598, fb, w, h);
}
