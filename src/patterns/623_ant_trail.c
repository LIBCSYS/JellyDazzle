/* 623 Ant Trail (creature) — a column of ants running a pheromone line.
 *
 * Built around the trail, not the ants.  The route is a parametric curve: one
 * trunk that meanders across the frame on value noise, plus a handful of
 * branches hung off it at fixed split points, each with its own drift.  Ants
 * are nothing but a parameter on a curve with a direction, which is why they
 * stay in file the way real ones do — no steering, no flocking, the line
 * itself is the instruction.  Traffic runs both ways, because a trail with
 * only outbound ants looks like a conveyor.
 *
 * The branching is the interesting part.  An ant crossing a split has a small
 * per-ant chance of taking it; most carry straight on.  So the branches stay
 * sparse and busy at the junction and thin out toward their ends, which is
 * what a real recruitment trail looks like.  An ant that reaches the end of a
 * branch turns around and heads home.
 *
 * Bodies are three discs — head, thorax, gaster — on the curve tangent, with
 * six short legs on an alternating tripod.  At this size the gait only has to
 * flicker; it reads as scurry.  The pheromone line is drawn faint underneath
 * so the geometry of the route is visible even where no ant is standing. */
#include "_spark572.h"

#define NA623  54          /* ants                                          */
#define NBR623 4           /* branches off the trunk                        */

static gk g623;
static uint32_t bs623 = 0xFFFFFFFFu;
static int   base623;
static uint32_t key623;                        /* noise stream for the route */
static float pw623, ph623;                     /* canvas size, for path623   */
static float bu623[NBR623], ba623[NBR623], bl623[NBR623], bsg623[NBR623];
static int   alane623[NA623];                  /* -1 trunk, else branch id   */
static float au623[NA623], ad623[NA623], asp623[NA623];
static float asz623[NA623], aph623[NA623], ahu623[NA623];

/* Point on the route.  lane -1 is the trunk; 0..NBR-1 are branches, which
 * leave the trunk at bu623[] along the local tangent, rotated by bsg623[]. */
static void path623(int lane, float u, float *ox, float *oy)
{
    if (u < 0.0f) u = 0.0f; else if (u > 1.0f) u = 1.0f;
    if (lane < 0) {
        *ox = -0.06f * pw623 + u * 1.12f * pw623;
        *oy = 0.50f * ph623
            + (gk_noise1(u * 3.4f, key623) - 0.5f) * 0.46f * ph623;
        return;
    }
    /* trunk anchor and its tangent, by finite difference */
    float u0 = bu623[lane], ax, ay, bx, by;
    path623(-1, u0, &ax, &ay);
    path623(-1, u0 + 0.02f, &bx, &by);
    float ang = atan2f(by - ay, bx - ax) + bsg623[lane] * ba623[lane];
    float cx = cosf(ang), cy = sinf(ang);
    float L = bl623[lane];
    /* branches wander too, otherwise they read as drawn rays */
    float off = (gk_noise1(u * 4.2f + (float)lane * 9.0f, key623 + 17u + (uint32_t)lane) - 0.5f)
              * 0.30f * L;
    *ox = ax + cx * u * L - cy * off;
    *oy = ay + cy * u * L + cx * off;
}

void pattern_623(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k, b;
    gk_setup(&g623, w, h);
    gk_clear(&g623);

    float sc = g623.sc;
    pw623 = (float)g623.cw;
    ph623 = (float)g623.ch;
    float t = (float)frame;
    float col[3];

    if (seed != bs623) {
        base623 = (int)(seed & 0x7FFFu);
        key623  = (seed * 2654435761u) | 1u;
        for (b = 0; b < NBR623; b++) {
            uint32_t r = seed ^ (uint32_t)(b * 8191 + 3);
            bu623[b]  = 0.16f + 0.68f * ((float)b + gk_hash(r + 1u)) / (float)NBR623;
            ba623[b]  = 0.55f + 0.55f * gk_hash(r + 2u);
            bl623[b]  = (0.16f + 0.18f * gk_hash(r + 3u)) * (pw623 + ph623) * 0.5f;
            bsg623[b] = gk_hash(r + 4u) > 0.5f ? 1.0f : -1.0f;
        }
        for (i = 0; i < NA623; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 3301 + 11);
            /* most ants start on the trunk; a few already on branches */
            alane623[i] = (gk_hash(r + 1u) > 0.80f)
                        ? (int)(gk_hash(r + 2u) * (float)NBR623) % NBR623 : -1;
            au623[i]  = gk_hash(r + 3u);
            ad623[i]  = gk_hash(r + 4u) > 0.5f ? 1.0f : -1.0f;
            asp623[i] = 0.0022f + 0.0020f * gk_hash(r + 5u);
            asz623[i] = (0.55f + 0.45f * gk_hash(r + 6u)) * sc;
            aph623[i] = gk_hash(r + 7u) * GK_TAU;
            ahu623[i] = gk_hash(r + 8u) * 0.08f;
        }
        bs623 = seed;
    }

    /* THE TRAIL itself: faint, wide-glow strokes.  The trunk is stronger than
     * the branches, which is how a trail actually looks once it is established. */
    for (b = -1; b < NBR623; b++) {
        float amp = (b < 0) ? 0.22f : 0.13f;
        sk_col(pal, sk_hidx(base623, 0.52f), 0.04f, 0.80f, amp, col);
        float px, py, nx, ny;
        path623(b, 0.0f, &px, &py);
        for (k = 1; k <= 28; k++) {
            path623(b, (float)k / 28.0f, &nx, &ny);
            gk_seg(&g623, px, py, nx, ny, col, 0.8f * sc, 4.0f * sc, 0.22f);
            px = nx; py = ny;
        }
    }

    for (i = 0; i < NA623; i++) {
        float S = asz623[i];
        int lane = alane623[i];

        /* lane length drives how fast u advances, so every ant walks at the
         * same ground speed whether it is on the trunk or a short branch */
        float lanelen = (lane < 0) ? pw623 * 1.12f : bl623[lane];
        float du = asp623[i] * (pw623 * 1.12f / (lanelen + 1.0f));
        float uprev = au623[i];
        au623[i] += ad623[i] * du;

        if (lane < 0) {
            /* OFF-RAMP: a crossing ant may take a branch.  The chance is low
             * and per-ant, so the junction stays busy but the branch stays thin. */
            for (b = 0; b < NBR623; b++) {
                float sp = bu623[b];
                int crossed = (uprev < sp && au623[i] >= sp) || (uprev > sp && au623[i] <= sp);
                if (!crossed) continue;
                if (gk_hash((uint32_t)frame * 2654435761u ^ (uint32_t)(i * 4099 + b)) < 0.22f) {
                    alane623[i] = b;
                    au623[i] = 0.02f;
                    ad623[i] = 1.0f;
                }
            }
            if (au623[i] > 1.0f) { au623[i] = 1.0f; ad623[i] = -1.0f; }
            if (au623[i] < 0.0f) { au623[i] = 0.0f; ad623[i] =  1.0f; }
        } else {
            /* end of a branch: turn around and run home.  Back at the junction,
             * rejoin the trunk heading whichever way it was already pointing. */
            if (au623[i] > 1.0f) { au623[i] = 1.0f; ad623[i] = -1.0f; }
            if (au623[i] < 0.0f) {
                alane623[i] = -1;
                au623[i] = bu623[lane];
                ad623[i] = gk_hash((uint32_t)frame ^ (uint32_t)(i * 7717)) > 0.5f ? 1.0f : -1.0f;
                lane = -1;
            }
        }

        float x, y, x2, y2;
        path623(lane, au623[i], &x, &y);
        path623(lane, au623[i] + ad623[i] * 0.01f, &x2, &y2);
        float dx = x2 - x, dy = y2 - y;
        float d = sqrtf(dx * dx + dy * dy) + 1e-5f;
        float ux = dx / d, uy = dy / d;
        float nx = -uy, ny = ux;

        /* lateral jitter: ants do not run the centreline exactly */
        float jit = (gk_noise1(t * 0.05f + (float)i * 3.3f, 707u) - 0.5f) * 2.4f * sc;
        x += nx * jit; y += ny * jit;

        /* LEGS: two tripods, flickering.  At this size the only job is to make
         * the body look driven rather than slid. */
        float gp = aph623[i] + t * 0.55f;
        sk_col(pal, sk_hidx(base623, ahu623[i] + 0.48f), 0.06f, 0.45f, 0.34f, col);
        for (k = 0; k < 6; k++) {
            int   row  = k >> 1;
            float side = (k & 1) ? 1.0f : -1.0f;
            /* tripod: front-left, mid-right, rear-left together */
            float off  = (((row & 1) == 0) == ((k & 1) == 0)) ? 0.0f : 3.14159265f;
            float sw   = sinf(gp + off) * 1.8f * S;
            float along = (1.6f - (float)row * 1.6f) * S;
            gk_seg(&g623,
                   x + ux * along, y + uy * along,
                   x + ux * (along + sw) + nx * side * 3.6f * S,
                   y + uy * (along + sw) + ny * side * 3.6f * S,
                   col, 0.32f * S, 1.1f * S, 0.07f);
        }

        /* BODY: gaster, thorax, head — the three-lobe silhouette does the work */
        sk_col(pal, sk_hidx(base623, ahu623[i]), 0.08f, 0.68f, 0.52f, col);
        gk_disc(&g623, x - ux * 3.0f * S, y - uy * 3.0f * S, 2.3f * S, col);
        gk_disc(&g623, x,                  y,                 1.5f * S, col);
        gk_disc(&g623, x + ux * 2.6f * S, y + uy * 2.6f * S, 1.6f * S, col);

        /* antennae, sweeping on the same clock as the legs */
        float sw2 = sinf(gp * 0.7f) * 0.4f;
        gk_seg(&g623, x + ux * 3.2f * S, y + uy * 3.2f * S,
                      x + ux * 6.0f * S + nx * (2.2f + sw2) * S,
                      y + uy * 6.0f * S + ny * (2.2f + sw2) * S,
               col, 0.28f * S, 0.9f * S, 0.06f);
        gk_seg(&g623, x + ux * 3.2f * S, y + uy * 3.2f * S,
                      x + ux * 6.0f * S - nx * (2.2f - sw2) * S,
                      y + uy * 6.0f * S - ny * (2.2f - sw2) * S,
               col, 0.28f * S, 0.9f * S, 0.06f);
    }

    gk_present(&g623, fb, w, h);
}
