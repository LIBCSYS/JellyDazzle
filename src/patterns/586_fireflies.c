/* 586 Fireflies — four dozen slow wanderers over black, each pulsing on its
 * own long period (a second and a half to four seconds, smooth in and out,
 * never a blink), each with its own palette hue that drifts through its
 * life; a faint body glow stays between pulses so nothing appears from
 * nowhere.  Repaint pattern. */
#include "_spark572.h"

#define NP586 60

static gk g586;
static sk_part p586[NP586];
static uint32_t bs586 = 0xFFFFFFFFu;
static int base586;

void pattern_586(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g586, w, h);
    gk_clear(&g586);
    float cw = (float)g586.cw, ch = (float)g586.ch, sc = g586.sc;
    if (seed != bs586) {
        base586 = (int)(seed & 0x7FFFu);
        float h0 = gk_hash(seed ^ 0x586u);
        for (i = 0; i < NP586; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 5081);
            sk_part *p = &p586[i];
            p->x = gk_hash(r + 1u) * cw; p->y = gk_hash(r + 2u) * ch;
            p->vx = p->vy = 0.0f;
            p->hue = h0 + gk_hash(r + 3u) * 0.5f;
            p->ph = gk_hash(r + 4u) * 100.0f;
            p->life = 90.0f + 150.0f * gk_hash(r + 5u);      /* pulse period */
            p->size = (1.3f + 1.4f * gk_hash(r + 6u)) * sc;
            p->amp = 0.8f + 0.5f * gk_hash(r + 7u);
        }
        bs586 = seed;
    }
    float t = (float)frame;
    float col[3];
    for (i = 0; i < NP586; i++) {
        sk_part *p = &p586[i];
        float ax = (gk_noise1(t * 0.01f + p->ph, (uint32_t)i * 2u + 1u) - 0.5f) * 0.05f * sc;
        float ay = (gk_noise1(t * 0.01f + p->ph + 33.0f, (uint32_t)i * 2u + 2u) - 0.5f) * 0.05f * sc;
        p->vx = (p->vx + ax) * 0.96f; p->vy = (p->vy + ay) * 0.96f;
        p->x += p->vx; p->y += p->vy;
        if (p->x < 4.0f) p->vx += 0.01f * sc; if (p->x > cw - 4.0f) p->vx -= 0.01f * sc;
        if (p->y < 4.0f) p->vy += 0.01f * sc; if (p->y > ch - 4.0f) p->vy -= 0.01f * sc;
        /* pulse: smooth bump repeated every period with a dark gap */
        float ph = (t + p->ph * 10.0f) / p->life; ph -= floorf(ph);
        float pulse = ph < 0.55f ? sinf(ph / 0.55f * 3.14159f) : 0.0f;
        pulse = pulse * pulse;
        float b = 0.2f + 0.9f * pulse;
        float hue = p->hue + t * 0.00006f + 0.03f * pulse;
        sk_col(pal, sk_hidx(base586, hue), 0.18f * pulse, 0.95f, b * p->amp, col);
        gk_dot(&g586, p->x, p->y, col, p->size, p->size * (3.0f + 2.5f * pulse), 0.35f);
    }
    gk_present(&g586, fb, w, h);
}
