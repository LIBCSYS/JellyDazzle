/* 594 Ember Column — a dense plume of embers rising from a hearth just
 * below the frame's bottom centre, spreading and tumbling in a slow
 * turbulence, cooling from hot palette accents to dim tail hues as they
 * climb and thin out.  Repaint pattern. */
#include "_spark572.h"

#define NP594 300

static gk g594;
static sk_part p594[NP594];
static uint32_t bs594 = 0xFFFFFFFFu;
static int base594;
static float hue0594, srcx594;

static void born594(sk_part *p, uint32_t r, float cw, float ch, float sc, int pre)
{
    p->x = srcx594 + (gk_hash(r + 1u) - 0.5f) * cw * 0.12f;
    p->y = ch + 4.0f * sc;
    p->vx = (gk_hash(r + 2u) - 0.5f) * 0.3f * sc;
    p->vy = -(0.35f + 0.6f * gk_hash(r + 3u)) * sc;
    p->life = 320.0f + 380.0f * gk_hash(r + 4u);
    p->age = 0.0f;
    if (pre) { p->age = gk_hash(r + 5u) * p->life; p->y = ch + p->age * p->vy * 0.9f; p->x += (gk_hash(r + 6u) - 0.5f) * p->age * 0.4f * sc; }
    p->hue = hue0594 + gk_hash(r + 7u) * 0.08f;
    p->size = (0.9f + 1.5f * gk_hash(r + 8u)) * sc;
    p->amp = 0.6f + 0.6f * gk_hash(r + 9u);
    p->ph = gk_hash(r + 10u) * 100.0f;
    p->alive = 1;
}

void pattern_594(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g594, w, h);
    gk_clear(&g594);
    float cw = (float)g594.cw, ch = (float)g594.ch, sc = g594.sc;
    if (seed != bs594) {
        base594 = (int)(seed & 0x7FFFu);
        hue0594 = gk_hash(seed ^ 0x594u);
        srcx594 = cw * (0.35f + 0.3f * gk_hash(seed ^ 5u));
        for (i = 0; i < NP594; i++) born594(&p594[i], seed ^ (uint32_t)(i * 3001), cw, ch, sc, 1);
        bs594 = seed;
    }
    float t = (float)frame;
    float col[3];
    for (i = 0; i < NP594; i++) {
        sk_part *p = &p594[i];
        p->age += 1.0f;
        if (p->age >= p->life || p->y < -8.0f) born594(p, seed ^ (uint32_t)(i * 3001 + frame * 173), cw, ch, sc, 0);
        float u = p->age / p->life;
        float turb = 0.15f + 0.6f * u;
        float ax = (gk_noise2(p->x * 0.02f / sc, p->y * 0.02f / sc + t * 0.01f, 31u) - 0.5f) * 0.09f * sc * turb;
        float ay = (gk_noise2(p->x * 0.02f / sc + 30.0f, p->y * 0.02f / sc + t * 0.01f, 32u) - 0.5f) * 0.05f * sc * turb;
        p->vx = (p->vx + ax) * 0.985f; p->vy = (p->vy + ay) * 0.995f - 0.001f * sc;
        p->x += p->vx; p->y += p->vy;
        float e = sk_bump(p->age, p->life, 20.0f, 100.0f);
        float fl = 0.75f + 0.25f * gk_noise1(t * 0.04f + p->ph, (uint32_t)i);
        sk_col(pal, sk_hidx(base594, p->hue + u * 0.14f + t * 0.00003f), 0.45f * (1.0f - u) * (1.0f - u), 0.6f, e * fl * p->amp * (1.0f - 0.5f * u), col);
        gk_dot(&g594, p->x, p->y, col, p->size * (1.0f - 0.4f * u), p->size * 3.0f, 0.3f);
    }
    gk_present(&g594, fb, w, h);
}
