/* 488 Lichtenberg Tree — a discharge figure growing upward from the ground
 * over the whole segment: a root at the bottom sends up a trunk whose tips
 * wander, thin, and fork, each new twig fading in over a dozen frames so
 * the figure fills the frame like frost or a burn in wood.  Older wood
 * cools to a deeper palette hue; the live tips stay hot.  Repaint pattern
 * with an sl-clocked growth list; black outside the tree. */
#include "_hue469.h"

#define MAXS488 4500
#define MAXT488 70

typedef struct { float x0, y0, x1, y1, wd; int birth; } seg488;
typedef struct { float x, y, a, wd; int alive, nf; } tip488;

static gk g488;
static seg488 segs488[MAXS488];
static tip488 tips488[MAXT488];
static int ns488, nt488, last488 = -1;
static uint32_t bs488 = 0xFFFFFFFFu;

static void reset488(uint32_t seed)
{
    float cw = (float)g488.cw, ch = (float)g488.ch;
    gk_seed(&g488, seed * 7u + 3u);
    ns488 = 0; nt488 = 0;
    memset(tips488, 0, sizeof tips488);
    int roots = 1 + (int)(gk_rf(&g488) * 2.0f);
    for (int i = 0; i < roots; i++) {
        tip488 *t = &tips488[nt488++];
        t->x = cw * (0.3f + 0.4f * gk_rf(&g488)); t->y = ch * 1.02f;
        t->a = -GK_TAU * 0.25f + gk_rs(&g488) * 0.2f; t->wd = 1.0f; t->alive = 1; t->nf = 8 + (int)(gk_rf(&g488) * 8.0f);
    }
    bs488 = seed;
}

static void grow488(int sl)
{
    float cw = (float)g488.cw, ch = (float)g488.ch, sc = g488.sc;
    for (int i = 0; i < nt488; i++) {
        tip488 *t = &tips488[i];
        if (!t->alive) continue;
        if (gk_rf(&g488) > 0.22f) continue;             /* ~every fourth frame */
        if (ns488 >= MAXS488) { t->alive = 0; continue; }
        float step = (2.6f + 2.0f * gk_rf(&g488)) * sc;
        /* wander, pulled back toward straight up, and away from the walls */
        t->a += gk_rs(&g488) * 0.3f;
        float up = -GK_TAU * 0.25f;
        t->a += (up - t->a) * 0.05f;
        if (t->x < cw * 0.15f) t->a += 0.05f; if (t->x > cw * 0.85f) t->a -= 0.05f;
        float nx = t->x + cosf(t->a) * step, ny = t->y + sinf(t->a) * step;
        seg488 *s = &segs488[ns488++];
        s->x0 = t->x; s->y0 = t->y; s->x1 = nx; s->y1 = ny; s->wd = t->wd; s->birth = sl;
        t->x = nx; t->y = ny;
        t->wd *= 0.997f;
        if (ny < ch * 0.04f || nx < 0.0f || nx > cw || t->wd < 0.12f) t->alive = 0;
        /* fork on a schedule: every 10..20 steps a tip splits */
        if (--t->nf <= 0 && nt488 < MAXT488) {
            t->nf = 10 + (int)(gk_rf(&g488) * 10.0f);
            tip488 *n = &tips488[nt488++];
            *n = *t;
            float d = (gk_rf(&g488) < 0.5f ? -1.0f : 1.0f) * (0.5f + 0.5f * gk_rf(&g488));
            n->a += d; t->a -= d * 0.35f;
            n->wd = t->wd * 0.8f; t->wd *= 0.96f;
            n->nf = 8 + (int)(gk_rf(&g488) * 10.0f);
        }
    }
}

void pattern_488(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g488, w, h);
    gk_clear(&g488);
    if (seed != bs488 || sl < 2 || sl < last488) reset488(seed);
    if (sl != last488 && sl < 1750) grow488(sl);
    last488 = sl;
    float sc = g488.sc, t = (float)frame, ch = (float)g488.ch;
    int base = (int)(seed & 8191u) + (int)(t * 0.5f);
    float fade = gk_smooth((float)sl / 30.0f);
    float breathe = 0.85f + 0.15f * sinf(t * 0.015f);
    for (int i = 0; i < ns488; i++) {
        const seg488 *s = &segs488[i];
        float age = (float)(sl - s->birth);
        float env = gk_smooth(age / 14.0f) * fade;
        if (env <= 0.0f) continue;
        float heat = expf(-age / 90.0f);                     /* fresh wood is hot */
        float c[3], hc[3];
        /* hue: cools with age, climbs with height, steps with twig width */
        int pi = base + (int)((1.0f - heat) * 3000.0f) + (int)((1.0f - s->y1 / ch) * 6000.0f) + (int)(s->wd * 1200.0f);
        float wa = (0.35f + 0.65f * s->wd) * env;
        hk_col(pal, pi + 600, 0.05f, 0.5f * wa * breathe, hc);
        hk_col(pal, pi, 0.30f + 0.35f * heat, (0.5f + 0.7f * heat) * wa, c);
        float ws = 0.5f + 0.5f * s->wd;
        gk_seg(&g488, s->x0, s->y0, s->x1, s->y1, hc, 1.8f * sc * ws, 6.0f * sc * ws, 0.5f);
        gk_seg(&g488, s->x0, s->y0, s->x1, s->y1, c, 0.8f * sc * ws, 2.0f * sc * ws, 0.25f);
    }
    for (int i = 0; i < nt488; i++) {
        if (!tips488[i].alive || sl >= 1750) continue;
        float c[3];
        hk_col(pal, base + (int)((1.0f - tips488[i].y / ch) * 6000.0f), 0.5f, 0.9f * fade * tips488[i].wd, c);
        gk_dot(&g488, tips488[i].x, tips488[i].y, c, 1.5f * sc, 5.0f * sc, 0.5f);
    }
    gk_present(&g488, fb, w, h);
}
