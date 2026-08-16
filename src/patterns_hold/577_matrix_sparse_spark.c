/* 577 Matrix Sparse Spark — the SPARK-role take: a few short, quick streams
 * from every edge, three or four glyphs long with a big soft head halo, so
 * each is a coloured spark with a glyph comet-tail; most of the frame stays
 * black for the layers beneath.  ACCUMULATOR. */
#include "_spark572.h"

#define NS577 26

static gk g577;
static sk_stream st577[NS577];
static sk_look lk577;
static uint32_t bs577 = 0xFFFFFFFFu;

static void spawn577(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st577[i];
    uint32_t r = seed ^ (uint32_t)(i * 5003 + frame * 113 + 13);
    float cs = 16.0f * g577.sc; if (cs < 8.0f) cs = 8.0f;
    float gs = floorf(cs / 8.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    int edge = (int)(gk_hash(r + 9u) * 3.99f);
    float lane = (floorf(gk_hash(r + 1u) * 40.0f) + 0.5f) / 40.0f;
    float spd = 0.10f + 0.14f * gk_hash(r + 2u);
    int len = 3 + (int)(gk_hash(r + 3u) * 3.0f);
    float hue = gk_hash(r + 4u);
    float amp = 1.4f + 0.8f * gk_hash(r + 5u);
    sk_spawn_edge(&g577, s, edge, lane, cs, gs, spd, len, 0.35f + 0.6f * gk_hash(r + 8u), hue, amp, r);
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0012f;
    s->mut = 0.05f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_577(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g577, w, h);
    sk_gly_init();
    if (seed != bs577 || sl < 2) {
        gk_clear(&g577);
        sk_look_default(&lk577, seed);
        lk577.k = 0.90f; lk577.tail_gain = 0.55f; lk577.tail_tau = 3.0f; lk577.head_wt = 0.4f; lk577.halo = 1.8f;
        for (i = 0; i < NS577; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.5f) spawn577(i, seed, frame, 1);
            else { st577[i].alive = 0; st577[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 250.0f); }
        }
        bs577 = seed;
    }
    sk_look_wink(&lk577, seed, frame);
    gk_decay_snap(&g577, lk577.k);
    for (i = 0; i < NS577; i++) {
        sk_stream *s = &st577[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn577(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 40 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 260.0f); continue; }
        sk_draw(&g577, s, &lk577, pal, 6.0f);
    }
    gk_present(&g577, fb, w, h);
}
