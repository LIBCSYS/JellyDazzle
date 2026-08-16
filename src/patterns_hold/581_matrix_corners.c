/* 581 Matrix Corners — glyph streams pour in diagonally from the four
 * corners along parallel 45-degree lanes, glyphs turned to their diagonal,
 * meeting in an X at the centre and dissolving.  Each corner has its own
 * hue family, drifting.  ACCUMULATOR. */
#include "_spark572.h"

#define NS581 76

static gk g581;
static sk_stream st581[NS581];
static sk_look lk581;
static uint32_t bs581 = 0xFFFFFFFFu;

static void spawn581(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st581[i];
    uint32_t r = seed ^ (uint32_t)(i * 6947 + frame * 59 + 47);
    float cs = 17.0f * g581.sc; if (cs < 9.0f) cs = 9.0f;
    float gs = floorf(cs / 9.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    int corner = i & 3;
    float cw = (float)g581.cw, ch = (float)g581.ch;
    float sx = (corner & 1) ? 1.0f : -1.0f, sy = (corner & 2) ? 1.0f : -1.0f;
    float ux = -sx * 0.70710678f, uy = -sy * 0.70710678f;   /* toward centre */
    float ox = cw * 0.5f + sx * (cw * 0.5f + cs), oy = ch * 0.5f + sy * (ch * 0.5f + cs);
    /* lane offset along the perpendicular (uy,-ux) */
    float lane = (floorf(gk_hash(r + 1u) * 22.0f) - 10.5f) * cs * 1.2f;
    ox += uy * lane; oy += -ux * lane;
    float dist = sqrtf(cw * cw + ch * ch) * 0.5f;
    float spd = 0.05f + 0.10f * gk_hash(r + 2u);
    int len = 8 + (int)(gk_hash(r + 3u) * 12.0f);
    float hue = gk_hash(seed ^ (uint32_t)(corner * 419)) + (gk_hash(r + 4u) - 0.5f) * 0.18f;
    float amp = 1.5f + 0.9f * gk_hash(r + 5u);
    /* streams near the diagonal run further (past the centre); outer lanes end sooner */
    float travel = 1.05f - fabsf(lane) / dist * 0.9f;
    sk_spawn(s, ox, oy, ux, uy, cs, gs, spd, len, (int)(dist * travel / cs), hue, amp, r);
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0006f;
    s->mut = 0.025f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc * 0.9f; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_581(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g581, w, h);
    sk_gly_init();
    if (seed != bs581 || sl < 2) {
        gk_clear(&g581);
        sk_look_default(&lk581, seed);
        for (i = 0; i < NS581; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.55f) spawn581(i, seed, frame, 1);
            else { st581[i].alive = 0; st581[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 200.0f); }
        }
        bs581 = seed;
    }
    sk_look_wink(&lk581, seed, frame);
    gk_decay_snap(&g581, lk581.k);
    for (i = 0; i < NS581; i++) {
        sk_stream *s = &st581[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn581(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 30 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 200.0f); continue; }
        sk_draw(&g581, s, &lk581, pal, 5.0f);
    }
    gk_present(&g581, fb, w, h);
}
