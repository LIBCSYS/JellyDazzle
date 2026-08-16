/* 574 Matrix Heavy Edge — the single-edge classic, dense: every column has a
 * stream, long tails, heads bright, all the way through and off the far
 * side.  Seed picks which edge it pours from (top most often).  Hues drift
 * per stream, tails cool off; a rare mono wink.  ACCUMULATOR. */
#include "_spark572.h"

#define NS574 96

static gk g574;
static sk_stream st574[NS574];
static sk_look lk574;
static uint32_t bs574 = 0xFFFFFFFFu;
static int edge574 = 0;
static int nlane574 = 40;

static void spawn574(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st574[i];
    uint32_t r = seed ^ (uint32_t)(i * 4099 + frame * 71 + 5);
    float cs = 16.0f * g574.sc; if (cs < 8.0f) cs = 8.0f;
    float gs = floorf(cs / 8.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    float lane = ((float)(i % nlane574) + 0.5f) / (float)nlane574;
    float spd = 0.07f + 0.13f * gk_hash(r + 2u);
    int len = 12 + (int)(gk_hash(r + 3u) * 20.0f);
    float hue = gk_hash(r + 4u);
    float amp = 1.0f + 0.9f * gk_hash(r + 5u);
    sk_spawn_edge(&g574, s, edge574, lane, cs, gs, spd, len, 1.05f, hue, amp, r);
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0008f;
    s->mut = 0.03f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_574(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g574, w, h);
    sk_gly_init();
    if (seed != bs574 || sl < 2) {
        gk_clear(&g574);
        sk_look_default(&lk574, seed);
        lk574.k = 0.91f; lk574.tail_gain = 0.5f; lk574.tail_tau = 9.0f;
        edge574 = (seed % 5u) < 3u ? 0 : (int)(seed % 4u);
        nlane574 = (edge574 < 2) ? (int)((float)g574.cw / (16.0f * g574.sc)) : (int)((float)g574.ch / (16.0f * g574.sc));
        if (nlane574 < 8) nlane574 = 8;
        for (i = 0; i < NS574; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.6f) spawn574(i, seed, frame, 1);
            else { st574[i].alive = 0; st574[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 160.0f); }
        }
        bs574 = seed;
    }
    sk_look_wink(&lk574, seed, frame);
    gk_decay_snap(&g574, lk574.k);
    for (i = 0; i < NS574; i++) {
        sk_stream *s = &st574[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn574(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 10 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 120.0f); continue; }
        sk_draw(&g574, s, &lk574, pal, 0.0f);
    }
    gk_present(&g574, fb, w, h);
}
