/* 578 Matrix Side Rain — the rain turned on its side: glyph rows pour in
 * from the left and right edges, cross in the middle and carry on through
 * to the far side, glyphs turned to read along their row.  Left and right
 * families start on different hues and drift toward each other.
 * ACCUMULATOR. */
#include "_spark572.h"

#define NS578 48

static gk g578;
static sk_stream st578[NS578];
static sk_look lk578;
static uint32_t bs578 = 0xFFFFFFFFu;

static void spawn578(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st578[i];
    uint32_t r = seed ^ (uint32_t)(i * 7013 + frame * 61 + 21);
    float cs = 17.0f * g578.sc; if (cs < 9.0f) cs = 9.0f;
    float gs = floorf(cs / 9.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    int edge = 2 + (i & 1);
    float lane = (floorf(gk_hash(r + 1u) * 27.0f) + 0.5f) / 27.0f;
    float spd = 0.05f + 0.10f * gk_hash(r + 2u);
    int len = 9 + (int)(gk_hash(r + 3u) * 15.0f);
    float hue = gk_hash(seed ^ (uint32_t)(edge * 313)) + (gk_hash(r + 4u) - 0.5f) * 0.25f;
    float amp = 1.1f + 0.8f * gk_hash(r + 5u);
    sk_spawn_edge(&g578, s, edge, lane, cs, gs, spd, len, 1.05f, hue, amp, r);
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0006f + (edge == 2 ? 0.00015f : -0.00015f);
    s->mut = 0.025f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_578(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g578, w, h);
    sk_gly_init();
    if (seed != bs578 || sl < 2) {
        gk_clear(&g578);
        sk_look_default(&lk578, seed);
        for (i = 0; i < NS578; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.55f) spawn578(i, seed, frame, 1);
            else { st578[i].alive = 0; st578[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 200.0f); }
        }
        bs578 = seed;
    }
    sk_look_wink(&lk578, seed, frame);
    gk_decay_snap(&g578, lk578.k);
    for (i = 0; i < NS578; i++) {
        sk_stream *s = &st578[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn578(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 20 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 160.0f); continue; }
        sk_draw(&g578, s, &lk578, pal, 0.0f);
    }
    gk_present(&g578, fb, w, h);
}
