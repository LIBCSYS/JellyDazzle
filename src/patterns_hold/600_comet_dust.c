/* 600 Comet Dust — three comet heads glide slow noise paths across the
 * dark, each trailing a long soft tail in a slowly forgetting canvas and
 * shedding motes of dust that drift off and fade.  Each comet owns a
 * palette hue that drifts through its flight; dust cools behind it.
 * ACCUMULATOR (cleared at segment start). */
#include "_spark572.h"

#define NC600 3
#define NP600 240

static gk g600;
static float ch600[NC600], cph600[NC600];
static sk_part p600[NP600];
static uint32_t bs600 = 0xFFFFFFFFu;
static int base600;
static float acc600;

void pattern_600(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i, k;
    gk_setup(&g600, w, h);
    float cw = (float)g600.cw, ch = (float)g600.ch, sc = g600.sc;
    if (seed != bs600 || sl < 2) {
        gk_clear(&g600);
        base600 = (int)(seed & 0x7FFFu);
        for (k = 0; k < NC600; k++) { ch600[k] = gk_hash(seed ^ (uint32_t)(k * 613 + 9)); cph600[k] = gk_hash(seed ^ (uint32_t)(k * 719)) * 100.0f; }
        for (i = 0; i < NP600; i++) p600[i].alive = 0;
        acc600 = 0.0f;
        bs600 = seed;
    }
    gk_decay_snap(&g600, 0.965f);
    float t = (float)frame;
    float col[3];
    for (k = 0; k < NC600; k++) {
        float x = cw * (-0.05f + 1.1f * gk_noise1(t * 0.0022f + cph600[k], (uint32_t)k * 2u + 500u));
        float y = ch * (-0.05f + 1.1f * gk_noise1(t * 0.0022f + cph600[k] * 1.3f, (uint32_t)k * 2u + 501u));
        float hue = ch600[k] + t * 0.00005f;
        sk_col(pal, sk_hidx(base600, hue), 0.45f, 0.55f, 0.12f, col);
        gk_dot(&g600, x, y, col, 2.0f * sc, 9.0f * sc, 0.5f);
        acc600 += 0.7f;
        while (acc600 >= 1.0f) {
            acc600 -= 1.0f;
            for (i = 0; i < NP600; i++) if (!p600[i].alive) break;
            if (i == NP600) break;
            uint32_t r = seed ^ (uint32_t)(frame * 977 + k * 131 + i * 7);
            sk_part *p = &p600[i];
            float a = gk_hash(r + 1u) * GK_TAU, v = (0.15f + 0.35f * gk_hash(r + 2u)) * sc;
            p->x = x + (gk_hash(r + 5u) - 0.5f) * 3.0f * sc; p->y = y + (gk_hash(r + 6u) - 0.5f) * 3.0f * sc;
            p->vx = cosf(a) * v; p->vy = sinf(a) * v;
            p->age = 0.0f; p->life = 80.0f + 120.0f * gk_hash(r + 3u);
            p->hue = hue; p->size = (0.5f + 0.7f * gk_hash(r + 4u)) * sc;
            p->amp = 0.035f + 0.03f * gk_hash(r + 7u); p->alive = 1;
        }
    }
    for (i = 0; i < NP600; i++) {
        sk_part *p = &p600[i];
        if (!p->alive) continue;
        p->age += 1.0f;
        if (p->age >= p->life) { p->alive = 0; continue; }
        p->vx *= 0.99f; p->vy *= 0.99f;
        p->x += p->vx; p->y += p->vy;
        float u = p->age / p->life;
        float e = sk_bump(p->age, p->life, 10.0f, 50.0f);
        sk_col(pal, sk_hidx(base600, p->hue + u * 0.08f), 0.2f, 0.55f, e * p->amp, col);
        gk_dot(&g600, p->x, p->y, col, p->size, p->size * 2.5f, 0.3f);
    }
    gk_present(&g600, fb, w, h);
}
