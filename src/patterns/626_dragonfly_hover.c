/* 626 Dragonfly Hover (creature) — dragonflies holding station, then gone.
 *
 * The behaviour is a two-state machine and that is the whole point.  A
 * dragonfly does not cruise: it parks in mid air, trembling on the spot for a
 * second or two, then crosses several body lengths in a few frames and parks
 * again.  So each insect has a hover timer and a dart timer, and velocity is
 * eased toward a target rather than set, which gives the burst a real
 * accelerate-and-arrest shape instead of a teleport.
 *
 * The wings are four, not two, and they are counterstroking: forewings and
 * hindwings run half a cycle apart, which is what dragonflies actually do at
 * low speed and is also the only way the silhouette stays busy — a synchronous
 * four-wing beat looks like a bird.  Each wing is drawn with two dimmer ghost
 * copies at earlier stroke phases, which is cheaper and more convincing than
 * trying to paint a blur: the eye reads the smear and fills in the beat.
 *
 * During a dart the body is drawn with trailing ghosts along the velocity
 * vector for the same reason.  Canvas repaints clean, so a hovering insect
 * cannot bloom by accumulating its own glow in one spot. */
#include "_spark572.h"

#define ND626 5            /* dragonflies                                   */

static gk g626;
static uint32_t bs626 = 0xFFFFFFFFu;
static int   base626;
static float dx626[ND626], dy626[ND626];
static float dvx626[ND626], dvy626[ND626];
static float dtx626[ND626], dty626[ND626];    /* target velocity             */
static float dhd626[ND626];                   /* body heading                */
static float dsz626[ND626], dhu626[ND626];
static float dwp626[ND626], dwr626[ND626];    /* wing phase and rate         */
static int   dmode626[ND626], dtim626[ND626]; /* 0 hover 1 dart, frames left */

/* One wing: hinge, base sweep, stroke angle, length.  Drawn as a tapering
 * two-stroke blade so the tip stays thin at any scale. */
static void wing626(gk *g, float hx, float hy, float ang, float len,
                    const float *col, float sc, float amp)
{
    float c2[3];
    c2[0] = col[0] * amp; c2[1] = col[1] * amp; c2[2] = col[2] * amp;
    float mx = hx + cosf(ang) * len * 0.55f;
    float my = hy + sinf(ang) * len * 0.55f;
    float tx = hx + cosf(ang) * len;
    float ty = hy + sinf(ang) * len;
    gk_seg(g, hx, hy, mx, my, c2, 0.85f * sc, 3.0f * sc, 0.20f);
    gk_seg(g, mx, my, tx, ty, c2, 0.50f * sc, 2.0f * sc, 0.14f);
}

void pattern_626(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    gk_setup(&g626, w, h);
    gk_clear(&g626);

    float cw = (float)g626.cw, ch = (float)g626.ch, sc = g626.sc;
    float t  = (float)frame;
    float col[3];

    if (seed != bs626) {
        base626 = (int)(seed & 0x7FFFu);
        for (i = 0; i < ND626; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 9941);
            dx626[i]   = cw * (0.12f + 0.76f * gk_hash(r + 1u));
            dy626[i]   = ch * (0.12f + 0.76f * gk_hash(r + 2u));
            dvx626[i]  = 0.0f; dvy626[i] = 0.0f;
            dtx626[i]  = 0.0f; dty626[i] = 0.0f;
            dhd626[i]  = gk_hash(r + 3u) * GK_TAU;
            dsz626[i]  = (0.80f + 0.60f * gk_hash(r + 4u)) * sc;
            dhu626[i]  = gk_hash(r + 5u);
            dwp626[i]  = gk_hash(r + 6u) * GK_TAU;
            dwr626[i]  = 0.85f + 0.30f * gk_hash(r + 7u);   /* fast — it is a blur */
            dmode626[i] = 0;
            dtim626[i]  = 20 + (int)(gk_hash(r + 8u) * 90.0f);
        }
        bs626 = seed;
    }

    for (i = 0; i < ND626; i++) {
        float S = dsz626[i];

        if (--dtim626[i] <= 0) {
            if (dmode626[i] == 0) {
                /* HOVER -> DART: commit to a direction and a high target speed */
                uint32_t r = (uint32_t)frame * 2654435761u ^ (uint32_t)(i * 6151);
                float a = gk_hash(r + 1u) * GK_TAU;
                float v = (5.0f + 6.0f * gk_hash(r + 2u)) * sc;
                /* bias the dart back toward the middle if it is near an edge */
                float bx = (cw * 0.5f - dx626[i]) / cw;
                float by = (ch * 0.5f - dy626[i]) / ch;
                dtx626[i] = cosf(a) * v + bx * 7.0f * sc;
                dty626[i] = sinf(a) * v + by * 7.0f * sc;
                dmode626[i] = 1;
                dtim626[i]  = 8 + (int)(gk_hash(r + 3u) * 14.0f);
            } else {
                /* DART -> HOVER: target velocity zero, the easing arrests it */
                dtx626[i] = 0.0f; dty626[i] = 0.0f;
                dmode626[i] = 0;
                dtim626[i]  = 30 + (int)(gk_hash((uint32_t)frame ^ (uint32_t)(i * 787)) * 110.0f);
            }
        }

        /* easing is asymmetric: accelerating into a dart is quicker than the
         * stop, which is what gives the move its snap */
        float ease = (dmode626[i] == 1) ? 0.30f : 0.14f;
        dvx626[i] += (dtx626[i] - dvx626[i]) * ease;
        dvy626[i] += (dty626[i] - dvy626[i]) * ease;

        /* hover tremble: small, fast, uncorrelated between the two axes */
        if (dmode626[i] == 0) {
            dvx626[i] += (gk_noise1(t * 0.22f + (float)i * 4.0f, 811u) - 0.5f) * 0.55f * sc;
            dvy626[i] += (gk_noise1(t * 0.22f + (float)i * 4.0f, 822u) - 0.5f) * 0.55f * sc;
        }

        dx626[i] += dvx626[i];
        dy626[i] += dvy626[i];

        float m = 30.0f * S;
        if (dx626[i] < -m) dx626[i] += cw + 2.0f * m;
        if (dx626[i] > cw + m) dx626[i] -= cw + 2.0f * m;
        if (dy626[i] < -m) dy626[i] += ch + 2.0f * m;
        if (dy626[i] > ch + m) dy626[i] -= ch + 2.0f * m;

        /* heading follows the velocity when there is any, and holds otherwise;
         * a hovering dragonfly keeps pointing where it was going */
        float sp = sqrtf(dvx626[i] * dvx626[i] + dvy626[i] * dvy626[i]);
        if (sp > 0.6f * sc) {
            float want = atan2f(dvy626[i], dvx626[i]);
            float dd = want - dhd626[i];
            while (dd >  3.14159265f) dd -= GK_TAU;
            while (dd < -3.14159265f) dd += GK_TAU;
            dhd626[i] += dd * 0.25f;
        }
        float ux = cosf(dhd626[i]), uy = sinf(dhd626[i]);
        float nx = -uy, ny = ux;

        float x = dx626[i], y = dy626[i];
        float ph = dwp626[i] + t * dwr626[i];

        /* WINGS: fore pair and hind pair half a cycle apart.  The stroke
         * rocks about the body's across-axis, so the blade sweeps up and down
         * rather than fanning forward. */
        sk_col(pal, sk_hidx(base626, dhu626[i] + 0.20f), 0.40f, 0.30f, 0.34f, col);
        for (k = 0; k < 4; k++) {
            int   pair = k >> 1;                      /* 0 fore, 1 hind      */
            float side = (k & 1) ? 1.0f : -1.0f;
            float hingex = x + ux * (pair ? -2.2f : 3.0f) * S;
            float hingey = y + uy * (pair ? -2.2f : 3.0f) * S;
            /* the blade's rest direction is across the body, canted by pair */
            float base = atan2f(ny * side, nx * side) + (pair ? 0.30f : -0.30f) * side;
            float len  = (pair ? 15.0f : 17.0f) * S;
            /* two ghosts at earlier phases do the motion blur */
            for (int gh = 2; gh >= 0; gh--) {
                float st = sinf(ph + (pair ? 3.14159265f : 0.0f) - 0.42f * (float)gh);
                wing626(&g626, hingex, hingey, base + st * 0.80f, len,
                        col, S, gh == 0 ? 1.0f : (gh == 1 ? 0.45f : 0.22f));
            }
        }

        /* BODY ghosts during a dart: trailing copies along the velocity */
        if (dmode626[i] == 1) {
            sk_col(pal, sk_hidx(base626, dhu626[i]), 0.10f, 0.60f, 0.22f, col);
            for (k = 1; k <= 3; k++) {
                float f = (float)k * 0.42f;
                gk_seg(&g626, x - dvx626[i] * f - ux * 10.0f * S,
                              y - dvy626[i] * f - uy * 10.0f * S,
                              x - dvx626[i] * f + ux * 4.0f * S,
                              y - dvy626[i] * f + uy * 4.0f * S,
                       col, 1.1f * S, 3.0f * S, 0.12f / (float)k);
            }
        }

        /* ABDOMEN: long, thin, slightly flexed; this is the shape people
         * recognise even when the wings are invisible */
        sk_col(pal, sk_hidx(base626, dhu626[i]), 0.08f, 0.68f, 0.70f, col);
        float px = x + ux * 2.0f * S, py = y + uy * 2.0f * S;
        for (k = 1; k <= 5; k++) {
            float f = (float)k / 5.0f;
            float bend = sinf(f * 2.2f + ph * 0.12f) * 1.1f * S;
            float nx2 = x - ux * f * 22.0f * S + nx * bend;
            float ny2 = y - uy * f * 22.0f * S + ny * bend;
            gk_seg(&g626, px, py, nx2, ny2, col,
                   (1.6f - 0.9f * f) * S, (4.0f - 2.0f * f) * S, 0.14f);
            px = nx2; py = ny2;
        }

        /* THORAX and the two big eyes */
        gk_disc(&g626, x, y, 3.4f * S, col);
        sk_col(pal, sk_hidx(base626, dhu626[i] + 0.12f), 0.52f, 0.25f, 0.95f, col);
        gk_dot(&g626, x + ux * 5.4f * S + nx * 1.8f * S,
                      y + uy * 5.4f * S + ny * 1.8f * S, col, 1.5f * S, 3.6f * S, 0.26f);
        gk_dot(&g626, x + ux * 5.4f * S - nx * 1.8f * S,
                      y + uy * 5.4f * S - ny * 1.8f * S, col, 1.5f * S, 3.6f * S, 0.26f);
    }

    gk_present(&g626, fb, w, h);
}
