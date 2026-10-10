/* 624 Cat Prowl (creature) — a cat crossing in silhouette, carrying itself low.
 *
 * This is a gait study, not an illustration.  The shape is a spine of discs
 * between a hip and a shoulder, a head hung forward and low, four two-bone
 * legs and a tail — suggestive mass, nothing drawn in detail.  What makes the
 * eye call it a cat is the timing.  A walking cat uses a lateral sequence:
 * left hind, left fore, right hind, right fore, at quarter-cycle spacing, with
 * a long stance phase so three feet are usually down.  Get those four numbers
 * right and the animal appears; get them wrong and it is a trotting dog.
 *
 * The prowl is the walk with three edits: stance duty pushed up, stride and
 * foot lift cut down, and the whole body dropped closer to the ground with the
 * head below the line of the shoulders.  Slow, deliberate, nothing bouncing.
 *
 * The tail is a lag chain that counterbalances.  Body sway is one scalar off
 * the gait cycle; the tail base is driven by its negative and each joint
 * inherits the previous joint's angle a frame late, so the tip keeps writing
 * after the body has already moved on.  Near-side legs are drawn bright and
 * far-side legs dim and slightly offset, which is all the depth a silhouette
 * needs.  Repaint clean. */
#include "_spark572.h"

#define NC624  2           /* cats on screen                                */
#define NT624  9           /* tail joints                                   */

static gk g624;
static uint32_t bs624 = 0xFFFFFFFFu;
static int   base624;
static float cx624[NC624], cgy624[NC624];     /* position, ground line      */
static float csz624[NC624], csp624[NC624];    /* scale, speed               */
static float cph624[NC624], chu624[NC624];    /* gait phase, hue            */
static int   cdir624[NC624];
static float tang624[NC624][NT624];           /* tail joint angles, persistent */

/* Footfall order for a lateral-sequence walk: LH, LF, RH, RF. */
static const float gph624[4] = { 0.00f, 0.25f, 0.50f, 0.75f };

static void gait624(float p, float duty, float stride, float lift,
                    float *ox, float *oy)
{
    p -= floorf(p);
    if (p < duty) {                            /* stance: planted, sliding back */
        float u = p / duty;
        *ox = stride * (0.5f - u);
        *oy = 0.0f;
    } else {                                   /* swing: lifted, carried forward */
        float u = (p - duty) / (1.0f - duty);
        *ox = stride * (u - 0.5f);
        *oy = -lift * sinf(3.14159265f * u);
    }
}

/* two-bone limb; bend picks the joint side (knee forward, hock back) */
static void ik624(gk *g, float hx, float hy, float fx, float fy,
                  float l1, float l2, float bend, const float *c,
                  float cr, float gr)
{
    float dx = fx - hx, dy = fy - hy;
    float d  = sqrtf(dx * dx + dy * dy);
    float L  = l1 + l2;
    if (d < 1e-4f) { dx = 0.0f; dy = 1.0f; d = 1e-4f; }
    if (d > L * 0.995f) {
        float s = L * 0.995f / d;
        dx *= s; dy *= s; d = L * 0.995f;
        fx = hx + dx; fy = hy + dy;
    }
    float ux = dx / d, uy = dy / d;
    float a  = (l1 * l1 - l2 * l2 + d * d) / (2.0f * d);
    float hh = l1 * l1 - a * a; if (hh < 0.0f) hh = 0.0f;
    float kh = sqrtf(hh) * bend;
    float kx = hx + ux * a - uy * kh;
    float ky = hy + uy * a + ux * kh;
    gk_seg(g, hx, hy, kx, ky, c, cr, gr, 0.16f);
    gk_seg(g, kx, ky, fx, fy, c, cr * 0.80f, gr * 0.85f, 0.13f);
}

void pattern_624(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    gk_setup(&g624, w, h);
    gk_clear(&g624);

    float cw = (float)g624.cw, ch = (float)g624.ch, sc = g624.sc;
    float t  = (float)frame;
    float col[3];

    if (seed != bs624) {
        base624 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NC624; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 5531);
            csz624[i]  = (0.85f + 0.55f * gk_hash(r + 1u)) * sc;
            cdir624[i] = gk_hash(r + 2u) > 0.5f ? 1 : -1;
            /* spaced around the frame, not thrown at random: with only a
             * couple of slow animals, two bunched starts means long stretches
             * of empty screen while both are off the side */
            cx624[i]   = cw * (((float)i + 0.75f * gk_hash(r + 3u)) / (float)NC624);
            cgy624[i]  = ch * (0.52f + 0.34f * ((float)i + gk_hash(r + 4u)) / (float)NC624);
            csp624[i]  = (0.42f + 0.26f * gk_hash(r + 5u)) * sc;
            cph624[i]  = gk_hash(r + 6u);
            chu624[i]  = gk_hash(r + 7u);
            for (k = 0; k < NT624; k++) tang624[i][k] = 0.0f;
        }
        bs624 = seed;
    }

    for (i = 0; i < NC624; i++) {
        float S  = csz624[i];
        float dir = (float)cdir624[i];
        float stride = 16.0f * S;               /* short — this is a prowl    */
        float duty   = 0.72f;                   /* three feet down most of the time */

        /* Cycle rate from ground speed over stride: one cycle per stride, so
         * stance feet stay stuck to the floor instead of skating. */
        cph624[i] += csp624[i] / (stride * 1.05f);
        cx624[i]  += dir * csp624[i];

        float m = 46.0f * S;        /* just clears the body; no dead time */
        if (cx624[i] > cw + m) cx624[i] = -m;
        if (cx624[i] < -m)     cx624[i] = cw + m;

        float ph = cph624[i];
        float gy = cgy624[i];
        /* breathing-slow vertical settle; a prowl barely bobs */
        float bob = sinf(ph * 2.0f * GK_TAU) * 0.8f * S;
        /* body sway, the scalar the tail answers to */
        float sway = sinf(ph * GK_TAU);

        float back = 20.0f * S;                 /* ride height — low          */
        float hipx = cx624[i] - dir * 17.0f * S;
        float hipy = gy - back - bob + 0.6f * S;
        float shx  = cx624[i] + dir * 17.0f * S;
        float shy  = gy - back - bob - 1.4f * S;  /* shoulders slightly higher */

        /* ---- FAR legs first, dim, nudged away from the viewer ---- */
        float farx = -dir * 2.5f * S, fary = -1.5f * S;
        sk_col(pal, sk_hidx(base624, chu624[i] + 0.04f), 0.04f, 0.78f, 0.34f, col);
        for (k = 0; k < 2; k++) {               /* k 0 hind, 1 fore — left side */
            float bx = (k == 0) ? hipx : shx;
            float by = (k == 0) ? hipy : shy;
            float ox, oy;
            gait624(ph + gph624[k], duty, stride, 5.0f * S, &ox, &oy);
            ik624(&g624, bx + farx, by + fary,
                  bx + farx + dir * ox, gy + fary + oy,
                  12.0f * S, 12.0f * S, (k == 0) ? -dir : dir,
                  col, 1.5f * S, 3.4f * S);
        }

        /* ---- BODY: spine of discs, hip to shoulder, slight dip in the middle */
        sk_col(pal, sk_hidx(base624, chu624[i]), 0.05f, 0.80f, 0.42f, col);
        for (k = 0; k <= 7; k++) {
            float f = (float)k / 7.0f;
            float bx = hipx + (shx - hipx) * f;
            float by = hipy + (shy - hipy) * f + sinf(f * 3.14159265f) * 1.6f * S;
            float r  = (7.4f - 2.2f * (f - 0.45f) * (f - 0.45f) * 4.0f) * S;
            gk_disc(&g624, bx, by, r, col);
        }
        /* a brighter line along the top of the back: the rim that makes a
         * silhouette read as a body and not a blob */
        sk_col(pal, sk_hidx(base624, chu624[i] + 0.02f), 0.34f, 0.40f, 0.42f, col);
        gk_seg(&g624, hipx, hipy - 5.6f * S, shx, shy - 6.0f * S,
               col, 0.8f * S, 2.6f * S, 0.14f);

        /* ---- HEAD: carried forward and below the shoulder line ---- */
        float hdx = shx + dir * 15.0f * S;
        float hdy = shy + 5.0f * S + sway * 0.8f * S;
        sk_col(pal, sk_hidx(base624, chu624[i]), 0.05f, 0.80f, 0.42f, col);
        gk_seg(&g624, shx, shy, hdx, hdy, col, 4.2f * S, 7.0f * S, 0.12f);  /* neck */
        gk_disc(&g624, hdx, hdy, 6.0f * S, col);
        gk_disc(&g624, hdx + dir * 4.6f * S, hdy + 1.2f * S, 3.4f * S, col); /* muzzle */
        /* ears: two short strokes off the crown, flicking on a slow clock */
        float flick = sinf(t * 0.07f + (float)i * 2.1f) * 0.18f;
        for (k = 0; k < 2; k++) {
            float lean = (k ? 0.45f : -0.10f) + flick * (k ? 1.0f : -1.0f);
            gk_seg(&g624, hdx + dir * (k ? 2.6f : -1.0f) * S, hdy - 4.4f * S,
                          hdx + dir * ((k ? 2.6f : -1.0f) + lean * 4.0f) * S,
                          hdy - 10.0f * S,
                   col, 1.2f * S, 2.6f * S, 0.12f);
        }

        /* ---- TAIL: lag chain driven by the negative of body sway ---- */
        float tbx = hipx - dir * 7.0f * S, tby = hipy - 2.0f * S;
        float tgt = -sway * 0.55f - 0.55f;      /* base angle, held high-ish  */
        sk_col(pal, sk_hidx(base624, chu624[i] + 0.03f), 0.08f, 0.70f, 0.38f, col);
        float px = tbx, py = tby, prev = tgt;
        for (k = 0; k < NT624; k++) {
            /* each joint eases toward the one ahead of it, so the whip travels
             * down the tail over several frames instead of arriving at once */
            tang624[i][k] += (prev - tang624[i][k]) * (0.30f - 0.016f * (float)k);
            float a = tang624[i][k];
            float seg = (6.2f - 0.25f * (float)k) * S;
            float nx = px - dir * cosf(a) * seg;
            float ny = py + sinf(a) * seg;
            float r  = (2.6f - 0.18f * (float)k) * S;
            gk_seg(&g624, px, py, nx, ny, col, r, r * 2.4f, 0.12f);
            px = nx; py = ny;
            prev = tang624[i][k] - 0.16f;       /* the tail curls as it goes   */
        }

        /* ---- NEAR legs last, bright, over everything ---- */
        sk_col(pal, sk_hidx(base624, chu624[i] + 0.01f), 0.10f, 0.72f, 0.55f, col);
        for (k = 0; k < 2; k++) {
            float bx = (k == 0) ? hipx : shx;
            float by = (k == 0) ? hipy : shy;
            float ox, oy;
            gait624(ph + gph624[k + 2], duty, stride, 5.0f * S, &ox, &oy);
            ik624(&g624, bx, by, bx + dir * ox, gy + oy,
                  12.0f * S, 12.0f * S, (k == 0) ? -dir : dir,
                  col, 1.7f * S, 3.8f * S);
        }
    }

    gk_present(&g624, fb, w, h);
}
