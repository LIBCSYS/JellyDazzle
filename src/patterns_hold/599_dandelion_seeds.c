/* 599 Dandelion Seeds — a scatter of seed tufts drifting on a slow wind,
 * each a tiny radial fan of filaments turning gently as it goes, with a
 * bright seed at the stem.  Tufts take palette hues in a soft window; the
 * seed sits a shade warmer than its fan.  Repaint pattern. */
#include "_spark572.h"

#define NP599 30

static gk g599;
static sk_part p599[NP599];
static uint32_t bs599 = 0xFFFFFFFFu;
static int base599;
static float hue0599;

static void born599(sk_part *p, uint32_t r, float cw, float ch, float sc, int pre, float wdir)
{
    float depth = gk_hash(r + 1u);
    if (pre) { p->x = gk_hash(r + 2u) * cw; p->y = gk_hash(r + 3u) * ch; }
    else if (wdir > 0.0f) { p->x = -20.0f * sc; p->y = gk_hash(r + 3u) * ch; }
    else { p->x = cw + 20.0f * sc; p->y = gk_hash(r + 3u) * ch; }
    p->vx = p->vy = 0.0f;
    p->size = (8.0f + 8.0f * depth) * sc;
    p->hue = hue0599 + gk_hash(r + 4u) * 0.12f;
    p->ph = gk_hash(r + 5u) * 100.0f;
    p->amp = 0.4f + 0.6f * depth;
    p->life = depth;
    p->age = pre ? 100.0f : 0.0f;
    p->alive = 1;
}

void pattern_599(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    gk_setup(&g599, w, h);
    gk_clear(&g599);
    float cw = (float)g599.cw, ch = (float)g599.ch, sc = g599.sc;
    float t = (float)frame;
    float wdir = (seed & 16u) ? 1.0f : -1.0f;
    if (seed != bs599) {
        base599 = (int)(seed & 0x7FFFu);
        hue0599 = gk_hash(seed ^ 0x599u);
        for (i = 0; i < NP599; i++) born599(&p599[i], seed ^ (uint32_t)(i * 6553), cw, ch, sc, 1, wdir);
        bs599 = seed;
    }
    float wspd = (0.25f + 0.25f * gk_noise1(t * 0.002f, 17u)) * sc * wdir;
    float col[3], c2[3];
    for (i = 0; i < NP599; i++) {
        sk_part *p = &p599[i];
        p->age += 1.0f;
        float lift = (gk_noise1(t * 0.006f + p->ph, (uint32_t)i) - 0.5f) * 0.3f * sc;
        p->vx += (wspd * (0.5f + 0.5f * p->life) - p->vx) * 0.03f;
        p->vy += (lift + 0.02f * sc - p->vy) * 0.03f;
        p->x += p->vx; p->y += p->vy;
        if ((wdir > 0.0f && p->x > cw + 22.0f * sc) || (wdir < 0.0f && p->x < -22.0f * sc) || p->y > ch + 22.0f * sc || p->y < -22.0f * sc)
            born599(p, seed ^ (uint32_t)(i * 6553 + frame * 89), cw, ch, sc, 0, wdir);
        float fin = p->age < 90.0f ? p->age / 90.0f : 1.0f;
        float rot = t * 0.004f * (p->life + 0.5f) + p->ph;
        sk_col(pal, sk_hidx(base599, p->hue + t * 0.00003f), 0.25f, 0.5f, 0.55f * p->amp * fin, col);
        for (k = 0; k < 11; k++) {
            float a = rot + (float)k * (GK_TAU / 11.0f);
            float len = p->size * (0.85f + 0.15f * gk_noise1(t * 0.02f + (float)k, (uint32_t)i));
            sk_line(&g599, p->x, p->y, p->x + cosf(a) * len, p->y + sinf(a) * len, 1.0f * sc, col);
            c2[0] = col[0] * 1.5f; c2[1] = col[1] * 1.5f; c2[2] = col[2] * 1.5f;
            gk_dot(&g599, p->x + cosf(a) * len, p->y + sinf(a) * len, c2, 0.7f * sc, 1.6f * sc, 0.3f);
        }
        sk_col(pal, sk_hidx(base599, p->hue + 0.04f), 0.45f, 0.5f, 0.9f * p->amp * fin, col);
        gk_dot(&g599, p->x, p->y, col, 1.2f * sc, 4.0f * sc, 0.4f);
    }
    gk_present(&g599, fb, w, h);
}
