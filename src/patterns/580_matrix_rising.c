/* 580 Matrix Rising — the rain runs backwards: glyph streams climb from the
 * bottom edge like data embers, slow, with wide soft head halos, thinning
 * and dissolving before they reach the top.  Hues drift per stream and
 * along the tail.  ACCUMULATOR. */
#include "_spark572.h"

#define NS580 60

static gk g580;
static sk_stream st580[NS580];
static sk_look lk580;
static uint32_t bs580 = 0xFFFFFFFFu;

static void spawn580(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st580[i];
    uint32_t r = seed ^ (uint32_t)(i * 3301 + frame * 67 + 41);
    float cs = 17.0f * g580.sc; if (cs < 9.0f) cs = 9.0f;
    float gs = floorf(cs / 9.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    float lane = (floorf(gk_hash(r + 1u) * 36.0f) + 0.5f) / 36.0f;
    float spd = 0.035f + 0.08f * gk_hash(r + 2u);
    int len = 8 + (int)(gk_hash(r + 3u) * 14.0f);
    float hue = gk_hash(r + 4u);
    float amp = 1.1f + 0.8f * gk_hash(r + 5u);
    sk_spawn_edge(&g580, s, 1, lane, cs, gs, spd, len, 0.55f + 0.45f * gk_hash(r + 8u), hue, amp, r);
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0008f;
    s->mut = 0.03f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_580(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g580, w, h);
    sk_gly_init();
    if (seed != bs580 || sl < 2) {
        gk_clear(&g580);
        sk_look_default(&lk580, seed);
        lk580.k = 0.92f;
        for (i = 0; i < NS580; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.55f) spawn580(i, seed, frame, 1);
            else { st580[i].alive = 0; st580[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 220.0f); }
        }
        bs580 = seed;
    }
    sk_look_wink(&lk580, seed, frame);
    gk_decay_snap(&g580, lk580.k);
    for (i = 0; i < NS580; i++) {
        sk_stream *s = &st580[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn580(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 30 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 200.0f); continue; }
        sk_draw(&g580, s, &lk580, pal, 8.0f);
    }
    gk_present(&g580, fb, w, h);
}
