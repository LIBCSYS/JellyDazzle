/* 589 Grinder Sparks — a slow fan of sparks thrown from a point low on one
 * side, arcing under gravity, hot at birth and cooling along the palette
 * as they fly, leaving short trails in a decaying canvas.  Slowed to a
 * dreamlike pace: nothing here moves fast enough to strobe.  ACCUMULATOR
 * (fast decay, cleared at segment start). */
#include "_spark572.h"

#define NP589 220

static gk g589;
static sk_part p589[NP589];
static uint32_t bs589 = 0xFFFFFFFFu;
static int base589;
static float hue0589, sx589, sy589, dir589;
static float acc589;

void pattern_589(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g589, w, h);
    float cw = (float)g589.cw, ch = (float)g589.ch, sc = g589.sc;
    if (seed != bs589 || sl < 2) {
        gk_clear(&g589);
        base589 = (int)(seed & 0x7FFFu);
        hue0589 = gk_hash(seed ^ 0x589u);
        int left = (seed & 8u) != 0u;
        sx589 = left ? cw * 0.12f : cw * 0.88f; sy589 = ch * (0.6f + 0.25f * gk_hash(seed ^ 3u));
        dir589 = left ? -0.9f : -3.14159f + 0.9f;   /* up and inward */
        for (i = 0; i < NP589; i++) p589[i].alive = 0;
        acc589 = 0.0f;
        bs589 = seed;
    }
    gk_decay_snap(&g589, 0.86f);
    float t = (float)frame;
    /* spawn rate breathes slowly */
    acc589 += 0.9f + 1.4f * gk_noise1(t * 0.01f, 4u);
    while (acc589 >= 1.0f) {
        acc589 -= 1.0f;
        for (i = 0; i < NP589; i++) if (!p589[i].alive) break;
        if (i == NP589) break;
        uint32_t r = seed ^ (uint32_t)(frame * 7919 + i * 131);
        sk_part *p = &p589[i];
        float a = dir589 + (gk_hash(r + 1u) - 0.5f) * 0.9f;
        float v = (1.4f + 1.8f * gk_hash(r + 2u)) * sc;
        p->x = sx589; p->y = sy589; p->vx = cosf(a) * v; p->vy = sinf(a) * v;
        p->age = 0.0f; p->life = 90.0f + 110.0f * gk_hash(r + 3u);
        p->hue = hue0589 + gk_hash(r + 4u) * 0.15f;
        p->size = (0.7f + 0.8f * gk_hash(r + 5u)) * sc;
        p->amp = 0.8f + 0.6f * gk_hash(r + 6u);
        p->alive = 1;
    }
    float col[3];
    for (i = 0; i < NP589; i++) {
        sk_part *p = &p589[i];
        if (!p->alive) continue;
        p->age += 1.0f;
        if (p->age >= p->life || p->y > ch + 10.0f) { p->alive = 0; continue; }
        p->vy += 0.016f * sc; p->vx *= 0.996f;
        p->x += p->vx; p->y += p->vy;
        float u = p->age / p->life;
        float e = sk_bump(p->age, p->life, 6.0f, 40.0f);
        sk_col(pal, sk_hidx(base589, p->hue + u * 0.16f + t * 0.00003f), 0.5f * (1.0f - u), 0.6f, e * p->amp * 0.5f, col);
        gk_dot(&g589, p->x, p->y, col, p->size, p->size * 2.5f, 0.3f);
    }
    /* the source: a soft hot knot */
    sk_col(pal, sk_hidx(base589, hue0589 + t * 0.00003f), 0.5f, 0.6f, 0.06f, col);
    gk_dot(&g589, sx589, sy589, col, 2.5f * sc, 9.0f * sc, 0.5f);
    gk_present(&g589, fb, w, h);
}
