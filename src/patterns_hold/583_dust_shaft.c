/* 583 Dust Shaft — one soft diagonal shaft of light, seed-angled, and slow
 * motes drifting through it: bright where they cross the beam, nearly gone
 * outside it.  The shaft breathes very slowly; motes and shaft take their
 * hues from the palette.  Repaint pattern. */
#include "_spark572.h"

#define NP583 220

static gk g583;
static sk_part p583[NP583];
static uint32_t bs583 = 0xFFFFFFFFu;
static int base583;
static float hue0583, ang583;

void pattern_583(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g583, w, h);
    gk_clear(&g583);
    float cw = (float)g583.cw, ch = (float)g583.ch, sc = g583.sc;
    if (seed != bs583) {
        base583 = (int)(seed & 0x7FFFu);
        hue0583 = gk_hash(seed ^ 0x583u);
        ang583 = 0.9f + (gk_hash(seed ^ 0x77u) - 0.5f) * 0.9f;   /* from upper-left-ish */
        for (i = 0; i < NP583; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 6151);
            sk_part *p = &p583[i];
            p->x = gk_hash(r + 1u) * cw; p->y = gk_hash(r + 2u) * ch;
            p->vx = p->vy = 0.0f;
            p->size = (0.9f + 1.5f * gk_hash(r + 3u)) * sc;
            p->hue = hue0583 + (gk_hash(r + 4u) - 0.5f) * 0.1f;
            p->ph = gk_hash(r + 5u) * 100.0f;
            p->amp = 0.8f + 0.7f * gk_hash(r + 6u);
        }
        bs583 = seed;
    }
    float t = (float)frame;
    /* shaft axis through a point that slides slowly */
    float ca = cosf(ang583), sa = sinf(ang583);
    float px = cw * (0.35f + 0.1f * gk_noise1(t * 0.002f, 5u)), py = ch * 0.5f;
    float L = cw + ch;
    float wd = ch * (0.10f + 0.03f * gk_noise1(t * 0.003f, 6u));
    float breath = 0.75f + 0.25f * gk_noise1(t * 0.004f, 8u);
    float col[3];
    sk_col(pal, sk_hidx(base583, hue0583 + 0.03f + t * 0.00003f), 0.15f, 0.4f, 0.17f * breath, col);
    gk_seg(&g583, px - ca * L, py - sa * L, px + ca * L, py + sa * L, col, wd, wd * 1.8f, 0.5f);
    float iw2 = 1.0f / (wd * wd);
    for (i = 0; i < NP583; i++) {
        sk_part *p = &p583[i];
        float ax = (gk_noise2(p->x * 0.006f / sc, t * 0.003f + p->ph, 11u) - 0.5f) * 0.02f * sc;
        float ay = (gk_noise2(p->y * 0.006f / sc, t * 0.003f + p->ph + 50.0f, 12u) - 0.5f) * 0.02f * sc;
        p->vx = p->vx * 0.97f + ax; p->vy = p->vy * 0.97f + ay + 0.002f * sc;
        p->x += p->vx; p->y += p->vy;
        if (p->x < -4.0f) p->x += cw + 8.0f; else if (p->x > cw + 4.0f) p->x -= cw + 8.0f;
        if (p->y < -4.0f) p->y += ch + 8.0f; else if (p->y > ch + 4.0f) p->y -= ch + 8.0f;
        /* distance to the shaft axis */
        float dx = p->x - px, dy = p->y - py;
        float d = dx * sa - dy * ca;
        float in = expf(-d * d * iw2 * 0.9f);
        float br = 0.08f + 0.92f * in;
        float tw = 0.7f + 0.3f * gk_noise1(t * 0.02f + p->ph, (uint32_t)i);
        sk_col(pal, sk_hidx(base583, p->hue + t * 0.00003f), 0.3f * in, 0.5f, br * tw * p->amp * breath, col);
        gk_dot(&g583, p->x, p->y, col, p->size, p->size * 3.0f, 0.3f);
    }
    gk_present(&g583, fb, w, h);
}
