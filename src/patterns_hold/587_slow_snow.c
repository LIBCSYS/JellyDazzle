/* 587 Slow Snow — three depths of soft flakes drifting down through black,
 * near ones larger and slower-swaying, far ones tiny; a gentle side wind
 * that changes over a minute.  Flakes take cool-to-warm palette hues in a
 * narrow per-segment window so the fall reads as one weather.  Repaint. */
#include "_spark572.h"

#define NP587 240

static gk g587;
static sk_part p587[NP587];
static uint32_t bs587 = 0xFFFFFFFFu;
static int base587;
static float hue0587;

static void born587(sk_part *p, uint32_t r, float cw, float ch, float sc, int pre)
{
    float depth = gk_hash(r + 1u);                    /* 0 far .. 1 near */
    p->x = gk_hash(r + 2u) * (cw + 40.0f) - 20.0f;
    p->y = pre ? gk_hash(r + 3u) * ch : -6.0f * sc;
    p->vy = (0.15f + 0.45f * depth) * sc;
    p->vx = 0.0f;
    p->size = (0.8f + 2.6f * depth) * sc;
    p->hue = hue0587 + gk_hash(r + 4u) * 0.12f;
    p->ph = gk_hash(r + 5u) * 100.0f;
    p->amp = (0.35f + 0.65f * depth) * (0.7f + 0.3f * gk_hash(r + 6u));
    p->life = depth;
    p->alive = 1;
}

void pattern_587(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g587, w, h);
    gk_clear(&g587);
    float cw = (float)g587.cw, ch = (float)g587.ch, sc = g587.sc;
    if (seed != bs587) {
        base587 = (int)(seed & 0x7FFFu);
        hue0587 = gk_hash(seed ^ 0x587u);
        for (i = 0; i < NP587; i++) born587(&p587[i], seed ^ (uint32_t)(i * 3557), cw, ch, sc, 1);
        bs587 = seed;
    }
    float t = (float)frame;
    float wind = (gk_noise1(t * 0.0015f, 3u) - 0.5f) * 0.5f * sc;
    float col[3];
    for (i = 0; i < NP587; i++) {
        sk_part *p = &p587[i];
        float sway = sinf(t * 0.02f * (0.6f + 0.6f * p->life) + p->ph) * 0.15f * sc * (0.5f + p->life);
        p->vx += (wind * (0.4f + 0.6f * p->life) + sway - p->vx) * 0.05f;
        p->x += p->vx; p->y += p->vy;
        if (p->y > ch + 6.0f * sc || p->x < -30.0f || p->x > cw + 30.0f)
            born587(p, seed ^ (uint32_t)(i * 3557 + frame * 97), cw, ch, sc, 0);
        float fade = p->y < 20.0f * sc ? p->y / (20.0f * sc) : 1.0f;
        if (fade < 0.0f) fade = 0.0f;
        float tw = 0.8f + 0.2f * gk_noise1(t * 0.02f + p->ph, (uint32_t)i);
        sk_col(pal, sk_hidx(base587, p->hue + t * 0.00003f), 0.35f, 0.4f, p->amp * fade * tw, col);
        gk_dot(&g587, p->x, p->y, col, p->size, p->size * 3.0f, 0.3f);
    }
    gk_present(&g587, fb, w, h);
}
