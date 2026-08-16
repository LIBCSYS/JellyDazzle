/* 593 Lantern Drift — a couple of dozen paper lanterns rising slowly through
 * the dark, each a soft warm body with a wide halo, breathing gently as its
 * flame moves; near ones larger and brighter.  Bodies take palette hues in
 * a per-segment window; the flame accents brighten toward white but the hue
 * stays.  Repaint pattern. */
#include "_spark572.h"

#define NP593 22

static gk g593;
static sk_part p593[NP593];
static uint32_t bs593 = 0xFFFFFFFFu;
static int base593;
static float hue0593;

static void born593(sk_part *p, uint32_t r, float cw, float ch, float sc, int pre)
{
    float depth = gk_hash(r + 1u);
    p->x = gk_hash(r + 2u) * cw;
    p->y = pre ? gk_hash(r + 3u) * ch : ch + 14.0f * sc;
    p->vy = -(0.08f + 0.22f * depth) * sc;
    p->vx = 0.0f;
    p->size = (3.5f + 7.5f * depth) * sc;
    p->hue = hue0593 + gk_hash(r + 4u) * 0.10f;
    p->ph = gk_hash(r + 5u) * 100.0f;
    p->amp = 0.35f + 0.65f * depth;
    p->life = depth;
    p->age = pre ? 200.0f : 0.0f;
    p->alive = 1;
}

void pattern_593(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g593, w, h);
    gk_clear(&g593);
    float cw = (float)g593.cw, ch = (float)g593.ch, sc = g593.sc;
    if (seed != bs593) {
        base593 = (int)(seed & 0x7FFFu);
        hue0593 = gk_hash(seed ^ 0x593u);
        for (i = 0; i < NP593; i++) born593(&p593[i], seed ^ (uint32_t)(i * 8191), cw, ch, sc, 1);
        bs593 = seed;
    }
    float t = (float)frame;
    float wind = (gk_noise1(t * 0.002f, 12u) - 0.5f) * 0.25f * sc;
    float col[3], c2[3];
    for (i = 0; i < NP593; i++) {
        sk_part *p = &p593[i];
        p->age += 1.0f;
        float sway = (gk_noise1(t * 0.008f + p->ph, (uint32_t)i) - 0.5f) * 0.2f * sc;
        p->vx += (wind * (0.4f + 0.6f * p->life) + sway - p->vx) * 0.03f;
        p->x += p->vx; p->y += p->vy;
        if (p->y < -16.0f * sc || p->x < -20.0f || p->x > cw + 20.0f)
            born593(p, seed ^ (uint32_t)(i * 8191 + frame * 61), cw, ch, sc, 0);
        float fin = p->age < 120.0f ? p->age / 120.0f : 1.0f;
        float top = p->y < 40.0f * sc ? p->y / (40.0f * sc) : 1.0f; if (top < 0.0f) top = 0.0f;
        float flame = 0.8f + 0.2f * gk_noise1(t * 0.03f + p->ph, (uint32_t)i + 50u);
        float a = p->amp * fin * top * flame;
        sk_col(pal, sk_hidx(base593, p->hue + t * 0.00003f), 0.1f, 0.75f, a * 1.0f, col);
        gk_disc(&g593, p->x, p->y, p->size, col);
        c2[0] = col[0] * 0.35f; c2[1] = col[1] * 0.35f; c2[2] = col[2] * 0.35f;
        gk_dot(&g593, p->x, p->y, c2, p->size * 1.5f, p->size * 3.5f, 0.6f);
        /* flame accent, low in the body */
        sk_col(pal, sk_hidx(base593, p->hue + 0.03f), 0.3f, 0.6f, a * 0.5f, col);
        gk_dot(&g593, p->x, p->y + p->size * 0.3f, col, p->size * 0.35f, p->size * 0.9f, 0.4f);
    }
    gk_present(&g593, fb, w, h);
}
