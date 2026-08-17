/* _spark572.h — sprite kit for the spark / matrix family (572..601).
 *
 * Two things live here, both drawn through the _glow469.h canvas so they
 * blend the same way everything else does:
 *
 *   sk_stream   a glyph column: it enters from an edge (or a spoke), walks a
 *               straight line one CELL at a time, and leaves a trail whose
 *               head is hot and whose tail cools and drifts in hue.  This is
 *               the Matrix rain, and it can arrive from any of the four sides.
 *   sk_part     a free particle: position, velocity, age, life, hue, amp.
 *               Snow, embers, pollen, fireflies, rain glints.
 *
 * Glyphs are generated, not loaded — 64 deterministic 5x7 marks built from a
 * hash, so there is no font file and every build draws the identical set.
 *
 * Palette discipline is the same as the rest of the engine: nothing carries
 * its own RGB.  A hue in 0..1 becomes a palette index via sk_hidx(), so
 * streams and particles ride the live palette walk like every other layer.
 */
#ifndef JD_SPARK572_H
#define JD_SPARK572_H
#include "_glow469.h"

/* ---------------- colour ------------------------------------------------ */

/* hue 0..1 (wraps) -> palette index, anchored at a per-pattern base */
static inline int sk_hidx(int base, float hue)
{
    float f = hue - floorf(hue);
    return base + (int)(f * 32768.0f);
}

/* Palette sample with a mix control: `mix` blends the vivid normalised read
 * (gk_col, full-strength hue) against the raw palette read (gk_colraw, which
 * keeps the scheme's own darkness).  wt is whiteness, as everywhere else. */
static inline void sk_col(const uint32_t *pal, int idx, float wt, float mix,
                          float amp, float *out)
{
    float a[3], b[3];
    gk_col(pal, idx, wt, amp, a);
    gk_colraw(pal, idx, amp, b);
    if (mix < 0.0f) mix = 0.0f; else if (mix > 1.0f) mix = 1.0f;
    out[0] = a[0] * (1.0f - mix) + b[0] * mix;
    out[1] = a[1] * (1.0f - mix) + b[1] * mix;
    out[2] = a[2] * (1.0f - mix) + b[2] * mix;
}

/* attack/release envelope over a lifetime: rises in `att` frames, holds,
 * falls over the last `rel` frames.  Never steps — everything here eases. */
static inline float sk_bump(float age, float life, float att, float rel)
{
    if (age < 0.0f || age > life) return 0.0f;
    float u = 1.0f;
    if (att > 0.0f && age < att) u = age / att;
    float left = life - age;
    if (rel > 0.0f && left < rel) { float v = left / rel; if (v < u) u = v; }
    return gk_smooth(u);
}

/* ---------------- look: how a stream is coloured ------------------------ */

typedef struct {
    float k;            /* canvas decay per frame (gk_decay_snap)          */
    float head_wt;      /* whiteness of the leading cell                   */
    float tail_tau;     /* trail fade constant, in cells                   */
    float tail_gain;    /* overall trail brightness                        */
    float tail_shift;   /* hue drift from head to tail (0 = one hue)       */
    float halo;         /* glow radius multiplier around each cell         */
} sk_look;

static inline void sk_look_default(sk_look *lk, uint32_t seed)
{
    lk->k          = 0.86f + 0.09f * gk_hash(seed ^ 0x5EEDu);
    lk->head_wt    = 0.45f + 0.25f * gk_hash(seed ^ 0x11u);
    lk->tail_tau   = 4.0f  + 6.0f  * gk_hash(seed ^ 0x22u);
    lk->tail_gain  = 0.75f + 0.45f * gk_hash(seed ^ 0x33u);
    lk->tail_shift = 0.02f + 0.10f * gk_hash(seed ^ 0x44u);
    lk->halo       = 1.6f  + 1.2f  * gk_hash(seed ^ 0x55u);
}

/* Rare "wink": every ~25 s the trails pull toward a single hue for a few
 * seconds and let go again.  Eased in and out — it must never snap. */
static inline void sk_look_wink(sk_look *lk, uint32_t seed, int frame)
{
    const float period = 1500.0f;
    float ph = fmodf((float)frame + gk_hash(seed ^ 0x77u) * period, period);
    float u = 0.0f;
    if (ph < 260.0f) {                       /* 60 in, 140 hold, 60 out */
        if (ph < 60.0f)        u = ph / 60.0f;
        else if (ph < 200.0f)  u = 1.0f;
        else                   u = (260.0f - ph) / 60.0f;
        u = gk_smooth(u);
    }
    float base_shift = 0.02f + 0.10f * gk_hash(seed ^ 0x44u);
    float base_wt    = 0.45f + 0.25f * gk_hash(seed ^ 0x11u);
    lk->tail_shift = base_shift * (1.0f - u);          /* -> one hue      */
    lk->head_wt    = base_wt + (0.85f - base_wt) * u;  /* -> hotter head  */
}

/* ---------------- glyphs ------------------------------------------------ */

#define SK_NGLY 64
#define SK_NG   SK_NGLY          /* 601 uses the short name */
static uint8_t sk_gly[SK_NGLY][7];      /* 5 bits wide, 7 rows */
static int     sk_gly_ready;

/* Deterministic marks: mostly-connected 5x7 bit patterns with a bias toward
 * vertical strokes, which is what reads as a "character" at cell size. */
static void sk_gly_init(void)
{
    if (sk_gly_ready) return;
    for (int g = 0; g < SK_NGLY; g++) {
        uint32_t s = (uint32_t)(g + 1) * 2654435761u;
        int spine = 1 + (int)(gk_hash(s) * 3.0f);          /* 1..3 */
        for (int r = 0; r < 7; r++) {
            uint32_t h = s ^ ((uint32_t)r * 0x9E3779B9u);
            uint8_t row = 0;
            row |= (uint8_t)(1u << spine);                  /* keep it connected */
            if (gk_hash(h + 1u) > 0.45f) row |= (uint8_t)(1u << ((spine + 1) % 5));
            if (gk_hash(h + 2u) > 0.62f) row |= (uint8_t)(1u << ((spine + 4) % 5));
            if (gk_hash(h + 3u) > 0.80f) row |= (uint8_t)(1u << ((spine + 2) % 5));
            if (r == 0 || r == 6) row &= (uint8_t)(row & ~(1u << ((spine + 2) % 5)));
            sk_gly[g][r] = row;
        }
    }
    sk_gly_ready = 1;
}

/* one glyph, centred on (x,y).  gs is the pixel size of a glyph texel. */
static void sk_glyph(gk *g, int gid, float x, float y, float rot, float amp,
                     float gs, const float *col)
{
    if (amp <= 0.0f || gs <= 0.0f) return;
    sk_gly_init();
    const uint8_t *G = sk_gly[((gid % SK_NGLY) + SK_NGLY) % SK_NGLY];
    float cs = cosf(rot), sn = sinf(rot);
    float c[3];
    c[0] = col[0] * amp; c[1] = col[1] * amp; c[2] = col[2] * amp;
    for (int r = 0; r < 7; r++) {
        for (int b = 0; b < 5; b++) {
            if (!(G[r] & (1u << b))) continue;
            float lx = ((float)b - 2.0f) * gs;
            float ly = ((float)r - 3.0f) * gs;
            float px = x + lx * cs - ly * sn;
            float py = y + lx * sn + ly * cs;
            gk_dot(g, px, py, c, gs * 0.62f, gs * 1.05f, 0.35f);
        }
    }
}

/* a straight glowing stroke — used for webs, trails and constellation links */
static void sk_line(gk *g, float x0, float y0, float x1, float y1,
                    float wd, const float *col)
{
    if (wd <= 0.0f) return;
    gk_seg(g, x0, y0, x1, y1, col, wd, wd * 2.6f, 0.4f);
}

/* ---------------- streams ----------------------------------------------- */

typedef struct {
    float ox, oy;       /* origin (cell 0)                                 */
    float ux, uy;       /* unit direction, scaled to one cell on step      */
    float cs;           /* cell size, px                                   */
    float gs;           /* glyph texel size, px                            */
    float spd;          /* cells per frame                                 */
    float pos;          /* head position, in cells                         */
    int   len;          /* trail length, in cells                          */
    int   maxc;         /* cells before the stream dies                    */
    int   lastc;        /* last integer cell, so glyphs only change on step*/
    float hue;          /* base hue 0..1                                   */
    float hdrift;       /* hue drift per frame                             */
    float amp;          /* brightness                                      */
    float mut;          /* per-frame chance a trail glyph mutates          */
    float age;          /* frames alive                                    */
    int   alive;
    int   wait;         /* frames to wait before respawn                   */
    uint32_t rs;        /* own rng, so glyph choice is stable per cell     */
} sk_stream;

/* the general spawn: caller supplies origin and direction */
static void sk_spawn(sk_stream *s, float ox, float oy, float ux, float uy,
                     float cs, float gs, float spd, int len, int maxc,
                     float hue, float amp, uint32_t r)
{
    float m = sqrtf(ux * ux + uy * uy); if (m < 1e-6f) { ux = 0.0f; uy = 1.0f; m = 1.0f; }
    s->ox = ox; s->oy = oy;
    s->ux = ux / m; s->uy = uy / m;
    s->cs = cs; s->gs = gs > 0.0f ? gs : cs / 9.0f;
    s->spd = spd > 0.0f ? spd : 0.08f;
    s->pos = 0.0f; s->lastc = -1;
    s->len = len < 1 ? 1 : len;
    s->maxc = maxc < 2 ? 2 : maxc;
    s->hue = hue; s->hdrift = 0.0f; s->amp = amp;
    s->mut = 0.0f; s->age = 0.0f; s->alive = 1; s->wait = 0;
    s->rs = r ? r : 0x9E3779B9u;
}

/* enter from one of the four edges.  edge 0 top, 1 right, 2 bottom, 3 left;
 * lane 0..1 slides along that edge; travel is how far across it gets. */
static void sk_spawn_edge(gk *g, sk_stream *s, int edge, float lane,
                          float cs, float gs, float spd, int len, float travel,
                          float hue, float amp, uint32_t r)
{
    float cw = (float)g->cw, ch = (float)g->ch;
    float ox, oy, ux, uy, span;
    edge &= 3;
    if (edge == 0)      { ox = lane * cw; oy = -cs;      ux = 0.0f;  uy = 1.0f;  span = ch; }
    else if (edge == 1) { ox = cw + cs;   oy = lane * ch; ux = -1.0f; uy = 0.0f; span = cw; }
    else if (edge == 2) { ox = lane * cw; oy = ch + cs;  ux = 0.0f;  uy = -1.0f; span = ch; }
    else                { ox = -cs;       oy = lane * ch; ux = 1.0f;  uy = 0.0f; span = cw; }
    int maxc = (int)(span * travel / (cs > 0.5f ? cs : 0.5f)) + len;
    sk_spawn(s, ox, oy, ux, uy, cs, gs, spd, len, maxc, hue, amp, r);
}

/* radiate outward from the centre along an angle */
static void sk_spawn_spoke(gk *g, sk_stream *s, float ang, float r0,
                           float cs, float gs, float spd, int len, float travel,
                           float hue, float amp, uint32_t r)
{
    float cw = (float)g->cw, ch = (float)g->ch;
    float cx = cw * 0.5f, cy = ch * 0.5f;
    float ux = cosf(ang), uy = sinf(ang);
    float reach = (cw > ch ? cw : ch) * 0.5f * travel;
    int maxc = (int)(reach / (cs > 0.5f ? cs : 0.5f)) + len;
    sk_spawn(s, cx + ux * r0, cy + uy * r0, ux, uy, cs, gs, spd, len, maxc, hue, amp, r);
}

/* Advance the stream's own rng.
 *
 * This used to call gk_ru(&s->rs), which compiled with a warning and was a
 * genuine out-of-bounds access: gk_ru takes a `gk *` and reads g->rs, but
 * gk::rs sits about thirty bytes into that struct, so passing the address of
 * a bare uint32_t made it read and write past the end of the stream. The
 * arithmetic is four lines; it does not need to borrow another kit's helper. */
static inline uint32_t sk_ru(uint32_t *st)
{
    uint32_t x = *st;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *st = x ? x : 0x9E3779B9u;
    return *st;
}

/* advance one frame.  returns 0 once the stream has run its course. */
static int sk_step(sk_stream *s)
{
    if (!s->alive) return 0;
    s->pos += s->spd;
    s->age += 1.0f;
    s->hue += s->hdrift;
    int c = (int)s->pos;
    if (c != s->lastc) { s->lastc = c; sk_ru(&s->rs); }
    if (s->pos - (float)s->len > (float)s->maxc) { s->alive = 0; return 0; }
    return 1;
}

/* paint the trail: head first (hot, whitest), then back along the tail with
 * an exponential fade and a slow hue shift.  `extra` widens the head glow —
 * patterns pass 0 for a flat look, ~5 for a lantern. */
static void sk_draw(gk *g, const sk_stream *s, const sk_look *lk,
                    const uint32_t *pal, float extra)
{
    if (!s->alive) return;
    int base = (int)(s->hue * 32768.0f);
    int head = (int)s->pos;
    for (int i = 0; i < s->len; i++) {
        int c = head - i;
        if (c < 0) break;
        if (c > s->maxc) continue;
        float fade = expf(-(float)i / (lk->tail_tau > 0.1f ? lk->tail_tau : 0.1f));
        float a = s->amp * lk->tail_gain * fade;
        if (a <= 0.004f) break;
        float x = s->ox + s->ux * ((float)c * s->cs);
        float y = s->oy + s->uy * ((float)c * s->cs);
        if (x < -s->cs * 2.0f || y < -s->cs * 2.0f ||
            x > (float)g->cw + s->cs * 2.0f || y > (float)g->ch + s->cs * 2.0f) continue;
        float wt = (i == 0) ? lk->head_wt : lk->head_wt * 0.35f;
        int idx = sk_hidx(base, (float)i * lk->tail_shift * 0.06f);
        float col[3];
        sk_col(pal, idx, wt, 0.35f, a, col);
        /* glyph identity is stable per cell, with a rare mutation */
        uint32_t gseed = s->rs ^ ((uint32_t)c * 0x85EBCA6Bu);
        int gid = (int)(gk_hash(gseed) * (float)SK_NGLY);
        if (s->mut > 0.0f && gk_hash(gseed ^ (uint32_t)(int)s->age) < s->mut)
            gid = (gid + 1 + (int)(gk_hash(gseed + 9u) * 7.0f)) % SK_NGLY;
        sk_glyph(g, gid, x, y, 0.0f, 1.0f, s->gs, col);
        if (i == 0 && extra > 0.0f)
            gk_dot(g, x, y, col, s->cs * 0.30f, s->cs * 0.30f + extra, 0.30f * lk->halo);
    }
}

/* ---------------- particles --------------------------------------------- */

typedef struct {
    float x, y;         /* position, canvas px                             */
    float vx, vy;       /* velocity, px/frame                              */
    float age, life;    /* frames                                          */
    float size;         /* radius / streak length, px                      */
    float hue;          /* 0..1                                            */
    float amp;          /* brightness                                      */
    float ph;           /* free phase, for twinkle and wander              */
    int   gid;          /* glyph id, when a particle draws as a glyph      */
    float gs;           /* glyph texel size                                */
    int   alive;
    int   wait;
} sk_part;

#endif /* JD_SPARK572_H */
