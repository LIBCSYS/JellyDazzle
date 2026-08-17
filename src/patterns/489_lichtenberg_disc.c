/* 489 Lichtenberg Disc — a radial discharge figure spreading outward from
 * the centre over the whole segment: nine primary channels crawl toward
 * the rim, wandering and forking, every twig fading in over a dozen frames.
 * The figure cools from a hot centre-outward gradient to a settled hue,
 * then rests, breathing.  Repaint pattern with sl-clocked growth; black
 * outside the figure. */
#include "_hue469.h"

#define MAXS489 1600
#define MAXT489 72

typedef struct { float x0, y0, x1, y1, wd; int birth; } seg489;
typedef struct { float x, y, a, wd; int alive; } tip489;

static gk g489;
static seg489 segs489[MAXS489];
static tip489 tips489[MAXT489];
static int ns489, nt489, last489 = -1;
static uint32_t bs489 = 0xFFFFFFFFu;

static void reset489(uint32_t seed)
{
    float cw = (float)g489.cw, ch = (float)g489.ch;
    gk_seed(&g489, seed * 11u + 5u);
    ns489 = 0; nt489 = 0;
    memset(tips489, 0, sizeof tips489);
    int arms = 7 + (int)(gk_rf(&g489) * 5.0f);
    float a0 = gk_rf(&g489) * GK_TAU;
    for (int i = 0; i < arms; i++) {
        tip489 *t = &tips489[nt489++];
        t->x = cw * 0.5f; t->y = ch * 0.5f;
        t->a = a0 + GK_TAU * (float)i / (float)arms + gk_rs(&g489) * 0.2f; t->wd = 1.0f; t->alive = 1;
    }
    bs489 = seed;
}

static void grow489(int sl)
{
    float cw = (float)g489.cw, ch = (float)g489.ch, sc = g489.sc;
    float cx = cw * 0.5f, cy = ch * 0.5f, rmax = (cw < ch ? cw : ch) * 0.47f;
    for (int i = 0; i < nt489; i++) {
        tip489 *t = &tips489[i];
        if (!t->alive) continue;
        if (gk_rf(&g489) > 0.4f) continue;
        if (ns489 >= MAXS489) { t->alive = 0; continue; }
        float step = (3.5f + 3.0f * gk_rf(&g489)) * sc;
        float radial = atan2f(t->y - cy, t->x - cx);
        t->a += gk_rs(&g489) * 0.55f;
        float d = radial - t->a;
        while (d > 3.14159f) d -= GK_TAU; while (d < -3.14159f) d += GK_TAU;
        t->a += d * 0.15f;
        float nx = t->x + cosf(t->a) * step, ny = t->y + sinf(t->a) * step;
        seg489 *s = &segs489[ns489++];
        s->x0 = t->x; s->y0 = t->y; s->x1 = nx; s->y1 = ny; s->wd = t->wd; s->birth = sl;
        t->x = nx; t->y = ny;
        t->wd *= 0.994f;
        float rr = sqrtf((nx - cx) * (nx - cx) + (ny - cy) * (ny - cy));
        if (rr > rmax || t->wd < 0.2f) t->alive = 0;
        if (nt489 < MAXT489 && gk_rf(&g489) < 0.05f) {
            tip489 *n = &tips489[nt489++];
            *n = *t;
            float dd = (gk_rf(&g489) < 0.5f ? -1.0f : 1.0f) * (0.45f + 0.6f * gk_rf(&g489));
            n->a += dd; t->a -= dd * 0.25f;
            n->wd = t->wd * 0.7f; t->wd *= 0.92f;
        }
    }
}

void pattern_489(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    gk_setup(&g489, w, h);
    gk_clear(&g489);
    if (seed != bs489 || sl < 2 || sl < last489) reset489(seed);
    if (sl != last489 && sl < 1750) grow489(sl);
    last489 = sl;
    float sc = g489.sc, t = (float)frame;
    float cw = (float)g489.cw, ch = (float)g489.ch;
    int base = (int)(seed & 8191u) + (int)(t * 0.5f);
    float fade = gk_smooth((float)sl / 30.0f);
    float breathe = 0.85f + 0.15f * sinf(t * 0.013f);
    for (int i = 0; i < ns489; i++) {
        const seg489 *s = &segs489[i];
        float age = (float)(sl - s->birth);
        float env = gk_smooth(age / 14.0f) * fade;
        if (env <= 0.0f) continue;
        float heat = expf(-age / 110.0f);
        float c[3], hc[3];
        /* hue: slides with radius (centre -> rim), steps with twig width,
         * and cools from the hot birth colour as the segment ages */
        float rr = sqrtf((s->x1 - cw * 0.5f) * (s->x1 - cw * 0.5f) + (s->y1 - ch * 0.5f) * (s->y1 - ch * 0.5f))
                 / ((cw < ch ? cw : ch) * 0.47f);
        int pi = base + (int)((1.0f - heat) * 2500.0f) + (int)(s->wd * 1500.0f) + (int)(rr * 6000.0f);
        float wa = (0.35f + 0.65f * s->wd) * env;
        hk_col(pal, pi + 600, 0.05f, 0.5f * wa * breathe, hc);
        hk_col(pal, pi, 0.30f + 0.35f * heat, (0.5f + 0.7f * heat) * wa, c);
        float ws = 0.5f + 0.5f * s->wd;
        gk_seg(&g489, s->x0, s->y0, s->x1, s->y1, hc, 1.8f * sc * ws, 6.0f * sc * ws, 0.5f);
        gk_seg(&g489, s->x0, s->y0, s->x1, s->y1, c, 0.8f * sc * ws, 2.0f * sc * ws, 0.25f);
    }
    float hub[3];
    gk_col(pal, base, 0.5f, 1.2f * fade * breathe, hub);
    gk_dot(&g489, cw * 0.5f, ch * 0.5f, hub, 3.0f * sc, 14.0f * sc, 0.5f);
    for (int i = 0; i < nt489; i++) {
        if (!tips489[i].alive || sl >= 1750) continue;
        float c[3];
        hk_col(pal, base + 6000, 0.5f, 0.8f * fade * tips489[i].wd, c);
        gk_dot(&g489, tips489[i].x, tips489[i].y, c, 1.5f * sc, 5.0f * sc, 0.5f);
    }
    gk_present(&g489, fb, w, h);
}
