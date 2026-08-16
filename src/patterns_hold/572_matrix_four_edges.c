/* 572 Matrix Four Edges — glyph streams fall from ALL FOUR edges toward the
 * centre, cross in the middle and dissolve a little past it.  Every stream
 * owns a drifting palette hue (head hot, tail cooler); a rare mono "wink"
 * pulls them to one hue for a few seconds and lets go.  ACCUMULATOR: a
 * decaying canvas the streams are painted into every frame, so cells ease
 * toward their level and nothing pops. */
#include "_spark572.h"

#define NS572 88

static gk g572;
static sk_stream st572[NS572];
static sk_look lk572;
static uint32_t bs572 = 0xFFFFFFFFu;

static void spawn572(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st572[i];
    uint32_t r = seed ^ (uint32_t)(i * 7919 + frame * 131 + 17);
    float cs = 18.0f * g572.sc; if (cs < 9.0f) cs = 9.0f;
    float gs = floorf(cs / 9.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    int edge = i & 3;
    float lane = gk_hash(r + 1u);
    /* snap lanes to a lattice so parallel streams line up like columns */
    lane = (floorf(lane * 34.0f) + 0.5f) / 34.0f;
    float spd = 0.05f + 0.10f * gk_hash(r + 2u);
    int len = 8 + (int)(gk_hash(r + 3u) * 14.0f);
    float hue = gk_hash(r + 4u);
    float amp = 1.1f + 0.9f * gk_hash(r + 5u);
    sk_spawn_edge(&g572, s, edge, lane, cs, gs, spd, len, 0.62f, hue, amp, r);
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0006f;
    s->mut = 0.02f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc * 0.8f; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_572(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g572, w, h);
    sk_gly_init();
    if (seed != bs572 || sl < 2) {
        gk_clear(&g572);
        sk_look_default(&lk572, seed);
        for (i = 0; i < NS572; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.5f) spawn572(i, seed, frame, 1);
            else { st572[i].alive = 0; st572[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 200.0f); }
        }
        bs572 = seed;
    }
    sk_look_wink(&lk572, seed, frame);
    gk_decay_snap(&g572, lk572.k);
    for (i = 0; i < NS572; i++) {
        sk_stream *s = &st572[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn572(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 30 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 150.0f); continue; }
        sk_draw(&g572, s, &lk572, pal, 5.0f);
    }
    gk_present(&g572, fb, w, h);
}
