/* 588 Pollen Drift — tiny motes carried sideways on a slowly turning wind
 * field, faint and many, with the occasional mote catching the light and
 * blooming for a couple of seconds.  Hues from a warm-ish palette window
 * per segment, drifting.  Repaint pattern. */
#include "_spark572.h"

#define NP588 200

static gk g588;
static sk_part p588[NP588];
static uint32_t bs588 = 0xFFFFFFFFu;
static int base588;
static float hue0588;

void pattern_588(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g588, w, h);
    gk_clear(&g588);
    float cw = (float)g588.cw, ch = (float)g588.ch, sc = g588.sc;
    if (seed != bs588) {
        base588 = (int)(seed & 0x7FFFu);
        hue0588 = gk_hash(seed ^ 0x588u);
        for (i = 0; i < NP588; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 2971);
            sk_part *p = &p588[i];
            p->x = gk_hash(r + 1u) * cw; p->y = gk_hash(r + 2u) * ch;
            p->vx = p->vy = 0.0f;
            p->size = (0.7f + 1.1f * gk_hash(r + 3u)) * sc;
            p->hue = hue0588 + gk_hash(r + 4u) * 0.16f;
            p->ph = gk_hash(r + 5u) * 100.0f;
            p->amp = 0.4f + 0.45f * gk_hash(r + 6u);
            p->life = 240.0f + 400.0f * gk_hash(r + 7u);   /* glint period */
        }
        bs588 = seed;
    }
    float t = (float)frame;
    float wdir = gk_noise1(t * 0.0012f, 9u) * 6.2831f;
    float wspd = (0.15f + 0.25f * gk_noise1(t * 0.002f, 10u)) * sc;
    float wx = cosf(wdir) * wspd, wy = sinf(wdir) * wspd * 0.4f;
    float col[3];
    for (i = 0; i < NP588; i++) {
        sk_part *p = &p588[i];
        float nx = (gk_noise2(p->x * 0.01f / sc, p->y * 0.01f / sc + t * 0.002f, 21u) - 0.5f) * 0.4f * sc;
        float ny = (gk_noise2(p->x * 0.01f / sc + 40.0f, p->y * 0.01f / sc + t * 0.002f, 22u) - 0.5f) * 0.4f * sc;
        p->vx += (wx + nx - p->vx) * 0.04f; p->vy += (wy + ny - p->vy) * 0.04f;
        p->x += p->vx; p->y += p->vy;
        if (p->x < -6.0f) p->x += cw + 12.0f; else if (p->x > cw + 6.0f) p->x -= cw + 12.0f;
        if (p->y < -6.0f) p->y += ch + 12.0f; else if (p->y > ch + 6.0f) p->y -= ch + 12.0f;
        float ph = (t + p->ph * 20.0f) / p->life; ph -= floorf(ph);
        float glint = ph < 0.18f ? sinf(ph / 0.18f * 3.14159f) : 0.0f;
        glint *= glint;
        float b = p->amp * (1.0f + 3.0f * glint);
        sk_col(pal, sk_hidx(base588, p->hue + t * 0.00004f), 0.3f * glint, 0.5f, b, col);
        gk_dot(&g588, p->x, p->y, col, p->size * (1.0f + glint), p->size * (2.5f + 3.0f * glint), 0.35f);
    }
    gk_present(&g588, fb, w, h);
}
