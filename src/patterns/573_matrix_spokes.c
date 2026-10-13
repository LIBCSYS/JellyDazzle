/* 573 Matrix Spokes — the kaleidoscope fold of the rain: glyph streams
 * fall inward along 5 to 8 spokes (seed picks), three parallel lanes per
 * spoke, glyphs turned to face along their spoke, dissolving as they reach
 * the hub.  Per-stream drifting hues, opposite spokes trade colour slowly.
 * ACCUMULATOR (decaying canvas). */
#include "_spark572.h"

#define NS573 80

static gk g573;
static sk_stream st573[NS573];
static sk_look lk573;
static uint32_t bs573 = 0xFFFFFFFFu;
static int nsp573 = 8;

static void spawn573(int i, uint32_t seed, int frame, int prewarm)
{
    sk_stream *s = &st573[i];
    uint32_t r = seed ^ (uint32_t)(i * 6151 + frame * 97 + 3);
    float cs = 17.0f * g573.sc; if (cs < 9.0f) cs = 9.0f;
    float gs = floorf(cs / 9.0f + 0.5f); if (gs < 1.0f) gs = 1.0f;
    int sp = i % nsp573;
    int lane = (i / nsp573) % 5 - 2;
    float ang = GK_TAU * (float)sp / (float)nsp573
              + gk_hash(seed ^ 0x51u) * GK_TAU;
    float cw = (float)g573.cw, ch = (float)g573.ch;
    float r0 = sqrtf(cw * cw + ch * ch) * 0.5f + cs;
    float spd = 0.05f + 0.09f * gk_hash(r + 2u);
    int len = 8 + (int)(gk_hash(r + 3u) * 12.0f);
    float hue = gk_hash(seed ^ (uint32_t)(sp * 977)) + (gk_hash(r + 4u) - 0.5f) * 0.12f;
    float amp = 1.4f + 0.9f * gk_hash(r + 5u);
    /* 3.6.5: this was sk_spawn_spoke(), which radiates OUTWARD from the
     * centre — and with the origin already past the corner (r0 is half the
     * diagonal) every stream started off-canvas walking further away, so the
     * pattern drew nothing at any size: motion and luma 0.00 in the variety
     * audit.  The header comment says the rain falls inward, so spawn on the
     * rim facing the hub, and stop the head a cell and a half short of it so
     * the trail drains into the centre instead of crossing to the far side. */
    float ux = cosf(ang), uy = sinf(ang);
    int maxc = (int)((r0 - cs * 1.5f) / cs);
    sk_spawn(s, cw * 0.5f + ux * r0, ch * 0.5f + uy * r0, -ux, -uy,
             cs, gs, spd, len, maxc, hue, amp, r);
    /* shift onto a parallel lane */
    float px = -s->uy, py = s->ux;
    s->ox += px * (float)lane * cs * 1.1f; s->oy += py * (float)lane * cs * 1.1f;
    s->hdrift = (gk_hash(r + 6u) - 0.5f) * 0.0005f;
    s->mut = 0.025f;
    if (prewarm) { s->pos = gk_hash(r + 7u) * (float)s->maxc * 0.9f; s->age = 30.0f; s->lastc = (int)s->pos; }
}

void pattern_573(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    int i;
    gk_setup(&g573, w, h);
    sk_gly_init();
    if (seed != bs573 || sl < 2) {
        gk_clear(&g573);
        sk_look_default(&lk573, seed);
        /* 3.6.5: hashed, and 5..8 spokes instead of 6 or 8 off one raw bit —
         * seeds that shared bit 2 always got the same fold (variety audit) */
        nsp573 = 5 + (int)(gk_hash(seed ^ 0x573u) * 4.0f);
        for (i = 0; i < NS573; i++) {
            if (gk_hash(seed + (uint32_t)i * 31u) < 0.55f) spawn573(i, seed, frame, 1);
            else { st573[i].alive = 0; st573[i].wait = (int)(gk_hash(seed + (uint32_t)i * 57u) * 220.0f); }
        }
        bs573 = seed;
    }
    sk_look_wink(&lk573, seed, frame);
    gk_decay_snap(&g573, lk573.k);
    for (i = 0; i < NS573; i++) {
        sk_stream *s = &st573[i];
        if (!s->alive) {
            if (s->wait > 0) { s->wait--; continue; }
            spawn573(i, seed, frame, 0);
        }
        if (!sk_step(s)) { s->wait = 40 + (int)(gk_hash(seed ^ (uint32_t)(frame * 7 + i)) * 200.0f); continue; }
        sk_draw(&g573, s, &lk573, pal, 4.0f);
    }
    gk_present(&g573, fb, w, h);
}
