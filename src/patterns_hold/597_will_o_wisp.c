/* 597 Will-o'-the-Wisp — six pale flames wander the dark on slow noise
 * paths, each a soft glow that leaves a smoky trail in a decaying canvas
 * and sheds the odd small spark that drifts and dies.  Each wisp owns a
 * drifting palette hue; sparks take their parent's hue a little warmer.
 * ACCUMULATOR (cleared at segment start). */
#include "_spark572.h"

#define NW597 6
#define NP597 160

static gk g597;
static float wh597[NW597], wph597[NW597];
static sk_part p597[NP597];
static uint32_t bs597 = 0xFFFFFFFFu;
static int base597;
static float acc597;

void pattern_597(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i, k;
    gk_setup(&g597, w, h);
    float cw = (float)g597.cw, ch = (float)g597.ch, sc = g597.sc;
    if (seed != bs597 || sl < 2) {
        gk_clear(&g597);
        base597 = (int)(seed & 0x7FFFu);
        for (k = 0; k < NW597; k++) { wh597[k] = gk_hash(seed ^ (uint32_t)(k * 613 + 7)); wph597[k] = gk_hash(seed ^ (uint32_t)(k * 919)) * 100.0f; }
        for (i = 0; i < NP597; i++) p597[i].alive = 0;
        acc597 = 0.0f;
        bs597 = seed;
    }
    gk_decay_snap(&g597, 0.95f);
    float t = (float)frame;
    float col[3];
    for (k = 0; k < NW597; k++) {
        float x = cw * (0.08f + 0.84f * gk_noise1(t * 0.003f + wph597[k], (uint32_t)k * 2u + 300u));
        float y = ch * (0.08f + 0.84f * gk_noise1(t * 0.003f + wph597[k] * 1.7f, (uint32_t)k * 2u + 301u));
        float br = 0.7f + 0.3f * gk_noise1(t * 0.02f + wph597[k], (uint32_t)k + 400u);
        float hue = wh597[k] + t * 0.00006f;
        sk_col(pal, sk_hidx(base597, hue), 0.35f, 0.55f, 0.16f * br, col);
        gk_dot(&g597, x, y, col, 3.0f * sc, 13.0f * sc, 0.6f);
        /* shed sparks */
        acc597 += 0.09f;
        if (acc597 >= 1.0f) {
            acc597 -= 1.0f;
            for (i = 0; i < NP597; i++) if (!p597[i].alive) break;
            if (i < NP597) {
                uint32_t r = seed ^ (uint32_t)(frame * 977 + k * 131 + i);
                sk_part *p = &p597[i];
                p->x = x; p->y = y;
                float a = gk_hash(r + 1u) * GK_TAU, v = (0.2f + 0.4f * gk_hash(r + 2u)) * sc;
                p->vx = cosf(a) * v; p->vy = sinf(a) * v - 0.1f * sc;
                p->age = 0.0f; p->life = 60.0f + 80.0f * gk_hash(r + 3u);
                p->hue = hue + 0.04f; p->size = (0.6f + 0.6f * gk_hash(r + 4u)) * sc;
                p->amp = 0.06f; p->alive = 1;
            }
        }
    }
    for (i = 0; i < NP597; i++) {
        sk_part *p = &p597[i];
        if (!p->alive) continue;
        p->age += 1.0f;
        if (p->age >= p->life) { p->alive = 0; continue; }
        p->vx *= 0.98f; p->vy = p->vy * 0.98f - 0.002f * sc;
        p->x += p->vx; p->y += p->vy;
        float e = sk_bump(p->age, p->life, 8.0f, 30.0f);
        sk_col(pal, sk_hidx(base597, p->hue), 0.4f, 0.55f, e * p->amp, col);
        gk_dot(&g597, p->x, p->y, col, p->size, p->size * 2.5f, 0.3f);
    }
    gk_present(&g597, fb, w, h);
}
