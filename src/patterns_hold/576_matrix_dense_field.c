/* 576 Matrix Dense Field — the FIELD-density take: small glyphs, many
 * streams from all four edges passing right through each other, lower
 * amplitude so it reads as a shimmering woven texture rather than figures.
 * Hues drift per stream; tails cool.  ACCUMULATOR. */
#include "_spark572.h"

#define NS576 170

static gk g576;
static sk_stream st576[NS576];
static sk_look lk576;
static uint32_t bs576 = 0xFFFFFFFFu;

static void spawn576(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st576[i];
    uint32_t r = seed ^ (uint32_t)(i * 3557 + frame * 89 + 11);
    float cs = 12.0f * g576.sc; if (cs < 6.0f) cs = 6.0f;
    float gs = floorf(cs / 12.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    int edge = i & 3;
    float lane = (floorf(gk_hash(r + 1u) * 52.0f) + 0.5f) / 52.0f;
    float spd = 0.05f + 0.10f * gk_hash(r + 2u);
    int len = 10 + (int)(gk_hash(r + 3u) * 16.0f);
    float hue = gk_hash(r + 4u);
    float amp = 1.0f + 0.7f * gk_hash(r + 5u);
    sk_spawn_edge(&g576, s, edge, lane, cs, gs, spd, len, 1.05f, hue, amp, r);
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0006f;
    s->mut = 0.03f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_576(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g576, w, h);
    sk_gly_init();
    if (seed != bs576 || sl < 2) {
        gk_clear(&g576);
        sk_look_default(&lk576, seed);
        lk576.k = 0.92f; lk576.tail_gain = 0.5f; lk576.tail_tau = 8.0f; lk576.head_wt = 0.3f;
        for (i = 0; i < NS576; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.7f) spawn576(i, seed, frame, 1);
            else { st576[i].alive = 0; st576[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 150.0f); }
        }
        bs576 = seed;
    }
    sk_look_wink(&lk576, seed, frame);
    gk_decay_snap(&g576, lk576.k);
    for (i = 0; i < NS576; i++) {
        sk_stream *s = &st576[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn576(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 10 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 100.0f); continue; }
        sk_draw(&g576, s, &lk576, pal, 0.0f);
    }
    gk_present(&g576, fb, w, h);
}
