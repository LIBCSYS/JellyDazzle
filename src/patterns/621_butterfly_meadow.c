/* 621 Butterfly Meadow (creature) — butterflies over a thin stand of grass.
 *
 * The hard part is not the wings, it is the path.  A butterfly does not steer;
 * it falls sideways and recovers.  So heading here is a noise random walk with
 * an occasional hard kick, and forward speed is not constant — it is tied to
 * the wingbeat, so the animal lurches on the downstroke and coasts on the up.
 * That coupling plus a vertical bob on the same phase is what reads as
 * "butterfly" long before anyone looks at the wing shape.
 *
 * Wings are four soft fans hinged on the body, two a side.  Opening is a
 * single scalar from the beat: at full open the fan spreads across the heading,
 * at the closed end it folds down to a sliver and the whole insect nearly
 * vanishes, which is exactly the flicker you see in a field.  Fore and hind
 * wings share the beat but the hind lags slightly, so the pair never looks
 * like one stamped shape.
 *
 * Grass is seeded once and only sways — it is there to give the fliers a
 * ground plane and a sense of scale, nothing more.  Repaint clean; a smear
 * would bury the wing flicker. */
#include "_spark572.h"

#define NB621 9            /* butterflies                                   */
#define NG621 34           /* grass stems                                   */

static gk g621;
static uint32_t bs621 = 0xFFFFFFFFu;
static int   base621;
static float bx621[NB621], by621[NB621];      /* position                   */
static float bhd621[NB621];                   /* heading, radians           */
static float bsp621[NB621];                   /* cruise speed               */
static float bsz621[NB621];                   /* body scale                 */
static float bwp621[NB621], bwr621[NB621];    /* wing phase and rate        */
static float bhu621[NB621];                   /* hue                        */
static float gx621[NG621], gh621[NG621], ghu621[NG621];

/* One wing: a fan of strokes from the hinge out to a rim, the rim pushed
 * across the body by `open`.  `side` is +1/-1, `lean` tilts fore vs hind. */
static void wing621(gk *g, float hx, float hy, float ux, float uy,
                    float len, float wid, float open, float side, float lean,
                    const float *col, float sc)
{
    float nx = -uy * side, ny = ux * side;     /* across the body, this side */
    int k;
    float px = hx, py = hy;
    for (k = 1; k <= 4; k++) {
        float f  = (float)k / 4.0f;
        /* the rim runs from near the hinge out and back: f along, bulge across */
        float along = len * (lean * (1.0f - f) + f * 1.0f);
        float across = wid * sinf(3.14159265f * f * 0.85f) * open;
        float nx2 = hx + ux * along + nx * across;
        float ny2 = hy + uy * along + ny * across;
        gk_seg(g, px, py, nx2, ny2, col, 0.6f * sc, 2.6f * sc, 0.18f);
        /* a second stroke straight from the hinge fills the wing interior */
        gk_seg(g, hx, hy, nx2, ny2, col, 0.5f * sc, 2.0f * sc, 0.10f);
        px = nx2; py = ny2;
    }
}

void pattern_621(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    gk_setup(&g621, w, h);
    gk_clear(&g621);

    float cw = (float)g621.cw, ch = (float)g621.ch, sc = g621.sc;
    float t  = (float)frame;
    float col[3];

    if (seed != bs621) {
        base621 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NB621; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 6151);
            bx621[i]  = gk_hash(r + 1u) * cw;
            by621[i]  = ch * (0.08f + 0.70f * gk_hash(r + 2u));
            bhd621[i] = gk_hash(r + 3u) * GK_TAU;
            bsp621[i] = (0.9f + 0.9f * gk_hash(r + 4u)) * sc;
            bsz621[i] = (0.65f + 0.70f * gk_hash(r + 5u)) * sc;
            bwp621[i] = gk_hash(r + 6u) * GK_TAU;
            bwr621[i] = 0.26f + 0.16f * gk_hash(r + 7u);
            bhu621[i] = gk_hash(r + 8u);
        }
        for (i = 0; i < NG621; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 1543 + 7);
            gx621[i]  = gk_hash(r + 1u) * cw;
            gh621[i]  = (0.10f + 0.16f * gk_hash(r + 2u)) * ch;
            ghu621[i] = 0.34f + 0.08f * gk_hash(r + 3u);   /* a tight green band */
        }
        bs621 = seed;
    }

    /* GRASS: stems lean on a shared slow breeze with a per-stem offset, so the
     * stand moves as one without marching in step. */
    for (i = 0; i < NG621; i++) {
        float lean = (gk_noise1(t * 0.006f + gx621[i] * 0.02f, 61u) - 0.5f) * 14.0f * sc;
        sk_col(pal, sk_hidx(base621, ghu621[i]), 0.06f, 0.70f, 0.28f, col);
        float px = gx621[i], py = ch + 2.0f * sc;
        for (k = 1; k <= 4; k++) {
            float f = (float)k / 4.0f;
            float nx = gx621[i] + lean * f * f;       /* bend accumulates upward */
            float ny = ch - gh621[i] * f;
            gk_seg(&g621, px, py, nx, ny, col, 0.55f * sc, 1.8f * sc, 0.10f);
            px = nx; py = ny;
        }
    }

    for (i = 0; i < NB621; i++) {
        float S  = bsz621[i];

        /* Heading: slow noise drift plus a sparse hard kick.  The kick is what
         * turns a smooth curve into the erratic jink of real flight. */
        float drift = (gk_noise1(t * 0.018f + (float)i * 11.3f, 310u) - 0.5f) * 0.26f;
        float kick  = gk_hash((uint32_t)(frame / 14) ^ (uint32_t)(i * 9173));
        if (kick > 0.86f)
            drift += (gk_hash((uint32_t)(frame / 14) + (uint32_t)i * 31u) - 0.5f) * 1.5f;
        bhd621[i] += drift;

        float ph    = bwp621[i] + t * bwr621[i];
        float beat  = sinf(ph);
        float open  = 0.14f + 0.86f * (0.5f + 0.5f * beat);   /* 0.14 .. 1 */
        float thrust = (beat > 0.0f) ? beat * beat : 0.0f;    /* downstroke only */

        float ux = cosf(bhd621[i]), uy = sinf(bhd621[i]);
        bx621[i] += ux * bsp621[i] * (0.35f + 1.3f * thrust);
        by621[i] += uy * bsp621[i] * (0.35f + 1.3f * thrust);
        /* the bob: the body rides up on the downstroke and sags on the up */
        float bob = -cosf(ph) * 2.6f * S;

        if (bx621[i] < -30.0f * sc) bx621[i] += cw + 60.0f * sc;
        if (bx621[i] > cw + 30.0f * sc) bx621[i] -= cw + 60.0f * sc;
        /* the meadow is a floor and the sky is a soft ceiling: bounce, don't wrap */
        if (by621[i] > ch * 0.92f) { by621[i] = ch * 0.92f; bhd621[i] = -1.0f - 1.0f * gk_hash((uint32_t)frame + (uint32_t)i); }
        if (by621[i] < ch * 0.04f) { by621[i] = ch * 0.04f; bhd621[i] =  1.0f + 1.0f * gk_hash((uint32_t)frame + (uint32_t)i * 7u); }

        float x = bx621[i], y = by621[i] + bob;

        /* BODY: a short dark-cored stroke, plus antennae.  Kept thin so the
         * wings own the silhouette. */
        sk_col(pal, sk_hidx(base621, bhu621[i] + 0.5f), 0.08f, 0.35f, 0.55f, col);
        gk_seg(&g621, x - ux * 4.0f * S, y - uy * 4.0f * S,
                      x + ux * 4.0f * S, y + uy * 4.0f * S,
               col, 0.8f * S, 2.4f * S, 0.16f);
        float axx = -uy, axy = ux;
        gk_seg(&g621, x + ux * 4.0f * S, y + uy * 4.0f * S,
                      x + ux * 8.0f * S + axx * 2.5f * S, y + uy * 8.0f * S + axy * 2.5f * S,
               col, 0.35f * S, 1.0f * S, 0.08f);
        gk_seg(&g621, x + ux * 4.0f * S, y + uy * 4.0f * S,
                      x + ux * 8.0f * S - axx * 2.5f * S, y + uy * 8.0f * S - axy * 2.5f * S,
               col, 0.35f * S, 1.0f * S, 0.08f);

        /* FOREWINGS: hinged just behind the head, wide and bright. */
        float fhx = x + ux * 1.6f * S, fhy = y + uy * 1.6f * S;
        sk_col(pal, sk_hidx(base621, bhu621[i]), 0.22f, 0.55f, 0.95f, col);
        wing621(&g621, fhx, fhy, ux, uy, 10.0f * S, 9.0f * S, open,  1.0f, -0.55f, col, S);
        wing621(&g621, fhx, fhy, ux, uy, 10.0f * S, 9.0f * S, open, -1.0f, -0.55f, col, S);

        /* HINDWINGS: hinged further back, smaller, and a touch behind the beat
         * so the pair never stamps out as one shape. */
        float hhx = x - ux * 2.2f * S, hhy = y - uy * 2.2f * S;
        float open2 = 0.14f + 0.86f * (0.5f + 0.5f * sinf(ph - 0.45f));
        sk_col(pal, sk_hidx(base621, bhu621[i] + 0.09f), 0.12f, 0.60f, 0.72f, col);
        wing621(&g621, hhx, hhy, -ux, -uy, 7.5f * S, 7.0f * S, open2, -1.0f, -0.40f, col, S);
        wing621(&g621, hhx, hhy, -ux, -uy, 7.5f * S, 7.0f * S, open2,  1.0f, -0.40f, col, S);
    }

    gk_present(&g621, fb, w, h);
}
