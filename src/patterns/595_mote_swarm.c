/* 595 Mote Swarm — a hundred and fifty glints loosely following a wandering
 * attractor, each with its own lag and jitter so the swarm stretches, folds
 * and regathers like a murmuration seen from far off.  Hues by lag: the
 * leaders warmer, the stragglers cooler, all drifting.  Repaint pattern. */
#include "_spark572.h"

#define NP595 150

static gk g595;
static sk_part p595[NP595];
static uint32_t bs595 = 0xFFFFFFFFu;
static int base595;
static float hue0595;

void pattern_595(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    gk_setup(&g595, w, h);
    gk_clear(&g595);
    float cw = (float)g595.cw, ch = (float)g595.ch, sc = g595.sc;
    if (seed != bs595) {
        base595 = (int)(seed & 0x7FFFu);
        hue0595 = gk_hash(seed ^ 0x595u);
        for (i = 0; i < NP595; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 4877);
            sk_part *p = &p595[i];
            p->x = gk_hash(r + 1u) * cw; p->y = gk_hash(r + 2u) * ch;
            p->vx = p->vy = 0.0f;
            p->life = 0.002f + 0.006f * gk_hash(r + 3u);   /* pull strength = eagerness */
            p->hue = hue0595 + (p->life - 0.002f) / 0.006f * 0.22f;
            p->size = (0.7f + 1.0f * gk_hash(r + 4u)) * sc;
            p->ph = gk_hash(r + 5u) * 100.0f;
            p->amp = 0.5f + 0.5f * gk_hash(r + 6u);
        }
        bs595 = seed;
    }
    float t = (float)frame;
    float axp = cw * (0.15f + 0.7f * gk_noise1(t * 0.003f, 41u));
    float ayp = ch * (0.15f + 0.7f * gk_noise1(t * 0.003f + 77.0f, 42u));
    float col[3];
    for (i = 0; i < NP595; i++) {
        sk_part *p = &p595[i];
        /* each mote holds its own slowly wheeling offset from the attractor */
        float oa = p->ph + t * 0.004f * (0.5f + p->life * 100.0f) + gk_noise1(t * 0.005f + p->ph, (uint32_t)i + 77u) * 2.0f;
        float orad = ch * (0.04f + 0.30f * gk_noise1(t * 0.003f + p->ph * 3.0f, (uint32_t)i + 88u));
        float tx = axp + cosf(oa) * orad, ty = ayp + sinf(oa) * orad * 0.7f;
        float jx = (gk_noise1(t * 0.02f + p->ph, (uint32_t)i * 2u + 1u) - 0.5f) * 0.10f * sc;
        float jy = (gk_noise1(t * 0.02f + p->ph + 5.0f, (uint32_t)i * 2u + 2u) - 0.5f) * 0.10f * sc;
        p->vx = (p->vx + (tx - p->x) * p->life * 0.5f + jx) * 0.95f;
        p->vy = (p->vy + (ty - p->y) * p->life * 0.5f + jy) * 0.95f;
        p->x += p->vx; p->y += p->vy;
        float sp = sqrtf(p->vx * p->vx + p->vy * p->vy) / sc;
        float tw = 0.7f + 0.3f * gk_noise1(t * 0.03f + p->ph, (uint32_t)i + 9u);
        sk_col(pal, sk_hidx(base595, p->hue + t * 0.00004f + sp * 0.02f), 0.25f, 0.55f, p->amp * tw * 1.2f, col);
        gk_dot(&g595, p->x, p->y, col, p->size, p->size * 3.0f, 0.3f);
    }
    gk_present(&g595, fb, w, h);
}
