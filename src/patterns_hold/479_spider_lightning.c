/* 479 Spider Lightning — a cloud-to-cloud web.  From one hub near the top
 * of the frame, eight to twelve thin channels crawl outward and sideways
 * in every direction, each throwing tiny branches, the whole thing filling
 * in over ~50 frames like frost forming, glowing softly for a while, then
 * dying from the hub outward.  Two hubs alternate.  Repaint pattern. */
#include "_hue469.h"

#define NH479 2
#define NL479 11
#define P479 340

static gk g479;
static gk_bolt legs479[NH479][NL479];
static int hi479[NH479] = { -1, -1 };
static uint32_t hs479 = 0xFFFFFFFFu;
static float hx479[NH479], hy479[NH479], hue479[NH479];

void pattern_479(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g479, w, h);
    gk_clear(&g479);
    if (seed != hs479) { hi479[0] = hi479[1] = -1; hs479 = seed; }
    float cw = (float)g479.cw, ch = (float)g479.ch, sc = g479.sc, t = (float)frame;
    int base = (int)(t * 1.8f) + (int)(seed & 8191u);
    for (int hh = 0; hh < NH479; hh++) {
        int ph = frame + hh * (P479 / 2);
        int idx = ph / P479;
        float age = (float)(ph - idx * P479);
        if (idx != hi479[hh]) {
            gk_seed(&g479, seed ^ (uint32_t)(idx * 2333 + hh * 40009));
            hx479[hh] = cw * (0.25f + 0.5f * gk_rf(&g479));
            hy479[hh] = ch * (0.15f + 0.35f * gk_rf(&g479));
            hue479[hh] = gk_rf(&g479);
            for (int l = 0; l < NL479; l++) {
                float a = GK_TAU * ((float)l + 0.5f * gk_rf(&g479)) / (float)NL479;
                float len = cw * (0.15f + 0.3f * gk_rf(&g479));
                /* flatten: crawlers spread sideways more than up/down */
                float ex = hx479[hh] + cosf(a) * len * 1.3f, ey = hy479[hh] + sinf(a) * len * 0.55f;
                gk_bolt_gen(&g479, &legs479[hh][l], hx479[hh], hy479[hh], ex, ey, 0.2f, 5, 3, 0.35f);
            }
            hi479[hh] = idx;
        }
        float env = gk_env(age, 10.0f, 80.0f, 90.0f);
        if (env <= 0.0f) continue;
        /* death from the hub outward: after hold, inner segments dim first */
        float dying = age > 90.0f ? (age - 90.0f) / 90.0f : 0.0f;
        int pi = base + (int)(hue479[hh] * 8000.0f) + (int)(age * 6.0f);
        hk_style st;                      /* hue hub -> leg tips, each leg offset a little */
        hk_style_set(&st, 3500, 1400, 700,
                     0.45f, 1.8f * sc, 6.0f * sc, 0.5f,
                     0.40f, 0.6f, 0.8f * sc, 2.0f * sc, 0.2f);
        for (int l = 0; l < NL479; l++) {
            gk_bolt *b = &legs479[hh][l];
            float prog = age / 50.0f + (float)(l % 3) * 0.05f;
            for (int i = 0; i < b->n; i++) {
                const gk_bseg *sg = &b->s[i];
                if (sg->t0 >= prog) continue;
                float x1 = sg->x1, y1 = sg->y1;
                if (sg->t1 > prog) { float f = (prog - sg->t0) / (sg->t1 - sg->t0); x1 = sg->x0 + (sg->x1 - sg->x0) * f; y1 = sg->y0 + (sg->y1 - sg->y0) * f; }
                float k = env * (0.35f + 0.65f * sg->wgt);
                if (dying > 0.0f) { float m = gk_smooth((sg->t0 - dying * 1.2f) * 4.0f + 0.5f); k *= m; }
                if (k <= 0.001f) continue;
                hk_seg(&g479, sg->x0, sg->y0, x1, y1, sg->t0, sg->wgt, k / (0.35f + 0.65f * sg->wgt),
                       pal, pi + l * 350, &st);
            }
        }
        float hub[3];
        gk_col(pal, pi, 0.5f, 1.2f * env * (1.0f - dying), hub);
        gk_dot(&g479, hx479[hh], hy479[hh], hub, 3.0f * sc, 14.0f * sc, 0.5f);
    }
    gk_present(&g479, fb, w, h);
}
