/* 575 Matrix Slow Mutation — a handful of very slow streams with long, bright
 * tails whose glyphs keep changing in place: the classic flicker, but each
 * change is a crossfade over a dozen frames in the decaying canvas, so the
 * field murmurs rather than strobes.  Hues drift slowly along and across
 * streams.  ACCUMULATOR. */
#include "_spark572.h"

#define NS575 30

static gk g575;
static sk_stream st575[NS575];
static sk_look lk575;
static uint32_t bs575 = 0xFFFFFFFFu;

static void spawn575(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st575[i];
    uint32_t r = seed ^ (uint32_t)(i * 2731 + frame * 53 + 9);
    float cs = 20.0f * g575.sc; if (cs < 10.0f) cs = 10.0f;
    float gs = floorf(cs / 10.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    int edge = (seed & 1u) ? (i & 1) : 0;                /* top, or top+bottom */
    float lane = (floorf(gk_hash(r + 1u) * 30.0f) + 0.5f) / 30.0f;
    float spd = 0.018f + 0.03f * gk_hash(r + 2u);
    int len = 16 + (int)(gk_hash(r + 3u) * 14.0f);
    float hue = gk_hash(r + 4u);
    float amp = 1.0f + 0.7f * gk_hash(r + 5u);
    sk_spawn_edge(&g575, s, edge, lane, cs, gs, spd, len, 1.05f, hue, amp, r);
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0007f;
    s->mut = 0.16f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_575(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g575, w, h);
    sk_gly_init();
    if (seed != bs575 || sl < 2) {
        gk_clear(&g575);
        sk_look_default(&lk575, seed);
        lk575.k = 0.93f; lk575.tail_gain = 0.62f; lk575.tail_tau = 16.0f; lk575.tail_shift = 0.008f;
        for (i = 0; i < NS575; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.7f) spawn575(i, seed, frame, 1);
            else { st575[i].alive = 0; st575[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 300.0f); }
        }
        bs575 = seed;
    }
    sk_look_wink(&lk575, seed, frame);
    gk_decay_snap(&g575, lk575.k);
    for (i = 0; i < NS575; i++) {
        sk_stream *s = &st575[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn575(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 60 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 300.0f); continue; }
        sk_draw(&g575, s, &lk575, pal, 0.0f);
    }
    gk_present(&g575, fb, w, h);
}
