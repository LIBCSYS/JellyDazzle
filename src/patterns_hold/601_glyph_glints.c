/* 601 Glyph Glints — the rain's constellation: no streams, just glyphs
 * appearing here and there on a loose lattice, easing in, sitting a few
 * seconds while they slowly mutate, easing out.  Each glyph owns a drifting
 * palette hue; a soft halo makes each a coloured glint.  ACCUMULATOR
 * (decaying canvas, so mutations crossfade instead of flicking). */
#include "_spark572.h"

#define NG601 44

typedef struct { float x, y, hue, age, life, amp; int gid; int alive; int wait; float gs; } gl601;
static gk g601;
static gl601 gl[NG601];
static uint32_t bs601 = 0xFFFFFFFFu;
static int base601;
static float k601 = 0.90f;

static void born601(gl601 *g, uint32_t r, float cw, float ch, float sc)
{
    float cs = 22.0f * sc;
    g->gs = floorf(cs / 11.0f + 0.5f); if (g->gs < 1.0f) g->gs = 1.0f;
    int nx = (int)(cw / cs), ny = (int)(ch / cs);
    g->x = floorf(((float)(int)(gk_hash(r + 1u) * (float)nx) + 0.5f) * cs) + 0.5f * (float)((int)g->gs & 1);
    g->y = floorf(((float)(int)(gk_hash(r + 2u) * (float)ny) + 0.5f) * cs) + 0.5f * (float)((int)g->gs & 1);
    g->hue = gk_hash(r + 3u);
    g->age = 0.0f; g->life = 200.0f + 300.0f * gk_hash(r + 4u);
    g->amp = 0.9f + 0.7f * gk_hash(r + 5u);
    g->gid = (int)(gk_hash(r + 6u) * (SK_NG - 0.01f));
    g->alive = 1; g->wait = 0;
}

void pattern_601(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g601, w, h);
    sk_gly_init();
    float cw = (float)g601.cw, ch = (float)g601.ch, sc = g601.sc;
    if (seed != bs601 || sl < 2) {
        gk_clear(&g601);
        base601 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NG601; i++) {
            born601(&gl[i], seed ^ (uint32_t)(i * 7333), cw, ch, sc);
            gl[i].age = gk_hash(seed ^ (uint32_t)(i * 31)) * gl[i].life;
        }
        bs601 = seed;
    }
    gk_decay_snap(&g601, k601);
    float t = (float)frame, k1 = 1.0f - k601;
    float col[3];
    for (i = 0; i < NG601; i++) {
        gl601 *g = &gl[i];
        if (!g->alive) {
            if (g->wait > 0) { g->wait--; continue; }
            born601(g, seed ^ (uint32_t)(i * 7333 + frame * 149), cw, ch, sc);
        }
        g->age += 1.0f;
        if (g->age >= g->life) { g->alive = 0; g->wait = 20 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 200.0f); continue; }
        /* slow mutation: crossfades in the accumulator */
        if (gk_hash((uint32_t)frame * 2654435761u ^ (uint32_t)i * 40503u ^ seed) < 0.012f)
            g->gid = (int)(gk_hash((uint32_t)frame * 7u + (uint32_t)i * 13u + seed) * (SK_NG - 0.01f));
        float e = sk_bump(g->age, g->life, 50.0f, 80.0f);
        float hue = g->hue + t * 0.00006f;
        sk_col(pal, sk_hidx(base601, hue), 0.35f, 0.65f, e * g->amp * k1, col);
        sk_glyph(&g601, g->gid, g->x, g->y, 0.0f, 1.0f, g->gs, col);
        float hc[3] = { col[0] * 0.5f, col[1] * 0.5f, col[2] * 0.5f };
        gk_dot(&g601, g->x, g->y, hc, g->gs * 3.0f, g->gs * 8.0f, 0.5f);
    }
    gk_present(&g601, fb, w, h);
}
