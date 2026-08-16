/* 523 Strike Cascade — chain lightning as a family tree: a root bolt
 * drops from the top to a node, and from that node two or three hops
 * leap on (staggered by a couple of dozen frames) to lower nodes, each of
 * which throws one or two more, so a cascade of a dozen hops fills the
 * frame top-to-bottom over ~150 frames, every hop growing in over ~22
 * frames and cooling over ~80, nodes glowing as the charge arrives.  Hue
 * shifts with each generation and along each hop; the whole tree fades
 * while the next cascade (new layout, new hue) starts.  Repaint. */
#include "_trace509.h"

#define P523 340
#define MH523 15

typedef struct { float x0, y0, x1, y1, t0; int gen; float hue; } hop523;
typedef struct { hop523 hp[MH523]; int n; gk_bolt b[MH523]; } casc523;

static gk g523;
static casc523 c523[2];
static int ci523[2] = { -1, -1 };
static uint32_t bs523 = 0xFFFFFFFFu;

static void grow523(casc523 *c, float x, float y, float t0, int gen, float hue, float cw, float ch)
{
    int nk = gen == 0 ? 1 : gen == 1 ? 2 + (gk_rf(&g523) < 0.5f ? 1 : 0) : gen == 2 ? 1 + (gk_rf(&g523) < 0.6f ? 1 : 0) : 0;
    for (int k = 0; k < nk && c->n < MH523; k++) {
        hop523 *hh = &c->hp[c->n];
        float dy = ch * (0.16f + 0.14f * gk_rf(&g523));
        float dx = cw * (gen == 0 ? 0.08f : 0.22f) * gk_rs(&g523);
        hh->x0 = x; hh->y0 = y; hh->x1 = x + dx; hh->y1 = y + dy;
        if (hh->x1 < cw * 0.05f) hh->x1 = cw * 0.05f;
        if (hh->x1 > cw * 0.95f) hh->x1 = cw * 0.95f;
        hh->t0 = t0; hh->gen = gen; hh->hue = hue + 0.09f * (float)gen + 0.04f * gk_rs(&g523);
        gk_bolt_gen(&g523, &c->b[c->n], hh->x0, hh->y0, hh->x1, hh->y1, 0.2f, 6, 3, 0.35f);
        c->n++;
        if (hh->y1 < ch * 0.9f)
            grow523(c, hh->x1, hh->y1, t0 + 24.0f + 14.0f * gk_rf(&g523), gen + 1, hue, cw, ch);
    }
}

void pattern_523(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g523, w, h);
    gk_clear(&g523);
    if (seed != bs523) { ci523[0] = ci523[1] = -1; bs523 = seed; }
    float cw = (float)g523.cw, ch = (float)g523.ch, sc = g523.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    for (int k = 0; k < 2; k++) {
        int idx = frame / P523 - k;
        if (idx < 0) continue;
        float age = (float)(frame - idx * P523);
        casc523 *c = &c523[idx & 1];
        if (ci523[idx & 1] != idx) {
            gk_seed(&g523, seed ^ (uint32_t)(idx * 6863 + 17));
            c->n = 0;
            grow523(c, cw * (0.3f + 0.4f * gk_rf(&g523)), -ch * 0.02f, 0.0f, 0, gk_rf(&g523), cw, ch);
            ci523[idx & 1] = idx;
        }
        float fade = 1.0f - gk_smooth((age - 250.0f) / 120.0f);
        if (fade <= 0.0f) continue;
        for (int i = 0; i < c->n; i++) {
            const hop523 *hh = &c->hp[i];
            float a = age - hh->t0;
            /* hot burn, then a quarter-strength afterglow keeps the tree connected */
            float env = (0.35f * gk_smooth(a / 6.0f) + 0.65f * gk_env(a, 6.0f, 60.0f, 90.0f)) * fade;
            if (env <= 0.0f) continue;
            float prog = a / 22.0f;
            int pi = base + (int)(hh->hue * 8000.0f);
            float c0[3], c1[3], h0[3], h1[3];
            gk_col(pal, pi, 0.05f, 0.42f * env, h0);
            gk_col(pal, pi + 900, 0.05f, 0.40f * env, h1);
            gk_col(pal, pi + 250, 0.6f, 0.7f * env, c0);
            gk_col(pal, pi + 1100, 0.45f, 0.65f * env, c1);
            float th = hh->gen == 0 ? 1.1f : 0.85f;
            bx_draw_grad(&g523, &c->b[i], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, prog, 1.0f, h0, h1, 0.0f, 1.8f * sc * th, 6.5f * sc * th, 0.5f);
            bx_draw_grad(&g523, &c->b[i], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, prog, 1.0f, c0, c1, 0.0f, 0.8f * sc * th, 2.2f * sc * th, 0.25f);
            /* node glow when the charge arrives */
            float nb = gk_smooth(prog - 0.85f) * env;
            if (nb > 0.0f) {
                float nc[3], nh[3];
                gk_col(pal, pi + 1300, 0.5f, 0.9f * nb, nc);
                gk_col(pal, pi + 2000, 0.1f, 0.4f * nb, nh);
                gk_dot(&g523, hh->x1, hh->y1, nh, 5.0f * sc, 20.0f * sc, 0.6f);
                gk_dot(&g523, hh->x1, hh->y1, nc, 2.2f * sc, 7.0f * sc, 0.6f);
            }
        }
    }
    gk_present(&g523, fb, w, h);
}
