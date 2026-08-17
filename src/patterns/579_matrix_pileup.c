/* 579 Matrix Pile-Up — streams from all four edges run to a soft disc at
 * the centre and STOP there: the head parks, the tail catches up and the
 * parked glyphs sit and glow, mutating slowly, before dissolving over a few
 * seconds — a slow heap of glyphs forming and melting at the hub.  Hues
 * drift per stream.  ACCUMULATOR. */
#include "_spark572.h"

#define NS579 60

static gk g579;
static sk_stream st579[NS579];
static sk_look lk579;
static uint32_t bs579 = 0xFFFFFFFFu;
static float linger579[NS579];
static int park579[NS579];

static void spawn579(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st579[i];
    uint32_t r = seed ^ (uint32_t)(i * 4423 + frame * 103 + 29);
    float cs = 17.0f * g579.sc; if (cs < 9.0f) cs = 9.0f;
    float gs = floorf(cs / 9.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    int edge = i & 3;
    float lane = 0.5f + (floorf(gk_hash(r + 1u) * 24.0f) - 11.5f) / 36.0f;
    float spd = 0.05f + 0.10f * gk_hash(r + 2u);
    int len = 6 + (int)(gk_hash(r + 3u) * 7.0f);
    float hue = gk_hash(r + 4u);
    float amp = 1.1f + 0.8f * gk_hash(r + 5u);
    sk_spawn_edge(&g579, s, edge, lane, cs, gs, spd, len, 1.05f, hue, amp, r);
    /* end at the central disc: solve for the cell where the stream enters r < R */
    float cx = (float)g579.cw * 0.5f, cy = (float)g579.ch * 0.5f;
    float R = (float)g579.ch * (0.10f + 0.10f * gk_hash(r + 8u));
    int c;
    for (c = 0; c < s->maxc; c++) {
        float x = s->ox + s->ux * cs * (float)c, y = s->oy + s->uy * cs * (float)c;
        float dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < R * R) break;
    }
    park579[i] = c < s->maxc;
    s->maxc = c;
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0006f;
    s->mut = 0.03f;
    linger579[i] = 0.0f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc * 0.9f; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_579(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g579, w, h);
    sk_gly_init();
    if (seed != bs579 || sl < 2) {
        gk_clear(&g579);
        sk_look_default(&lk579, seed);
        lk579.tail_gain = 0.55f; lk579.tail_tau = 9.0f;
        for (i = 0; i < NS579; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.5f) spawn579(i, seed, frame, 1);
            else { st579[i].alive = 0; st579[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 200.0f); }
        }
        bs579 = seed;
    }
    sk_look_wink(&lk579, seed, frame);
    gk_decay_snap(&g579, lk579.k);
    for (i = 0; i < NS579; i++) {
        sk_stream *s = &st579[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn579(i, seed, frame, 0);
        }
        /* park at the disc: hold the head, then dissolve */
        if (park579[i] && s->pos + s->spd >= (float)s->maxc + 0.95f) {
            s->pos = (float)s->maxc + 0.95f; s->spd = 0.0f;
            linger579[i] += 1.0f;
            float L = linger579[i];
            if (L > 120.0f) { s->amp *= 0.975f; if (s->amp < 0.02f) { s->alive = 0; s->wait = 30 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 200.0f); continue; } }
        }
        if (!sk_step(s)) { s->wait = 30 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 200.0f); continue; }
        sk_draw(&g579, s, &lk579, pal, 0.0f);
    }
    gk_present(&g579, fb, w, h);
}
