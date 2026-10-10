/* 628 Crab Scuttle (creature) — crabs working across the frame sideways.
 *
 * Seen from above, which is the only view where a crab is instantly a crab:
 * a wide flat carapace, four walking legs a side, claws held up and forward.
 * Travel is along the body's wide axis, because that is how crabs actually
 * move — the legs on the leading side pull and the trailing side pushes, and
 * the body never turns to face where it is going.
 *
 * The gait is metachronal, not a tripod.  Each leg on a side lags the one
 * ahead of it by a small constant, and the two sides run half a cycle apart,
 * so a wave runs along each flank and the two waves interleave.  That is four
 * phase offsets and a side flip, and it is what stops eight legs from looking
 * like two combs flapping.  From this angle a lifted foot cannot be drawn as
 * height, so swing is drawn as reach instead: the foot pulls in toward the
 * body during the swing and plants back out at the end of it.
 *
 * Claws are two-segment arms held above the shell with the pincers opening on
 * a slow clock, independent of the gait — a crab waves its chelae while it
 * walks.  Direction flips at random intervals, which is the behaviour: a few
 * fast steps one way, a pause, then away the other way.  Repaint clean. */
#include "_spark572.h"

#define NC628 5            /* crabs                                         */

static gk g628;
static uint32_t bs628 = 0xFFFFFFFFu;
static int   base628;
static float cx628[NC628], cy628[NC628];
static float csz628[NC628], csp628[NC628];
static float cph628[NC628], chu628[NC628];
static float ccl628[NC628];                   /* claw clock phase            */
static int   cdir628[NC628], cflip628[NC628]; /* travel sign, frames to flip */

/* Lag along one flank: leg 0 leads, leg 3 trails. */
static const float lag628[4] = { 0.00f, 0.14f, 0.28f, 0.42f };

/* Sideways gait in body space.  Returns the along-body slide and a 0..1 swing
 * factor; from overhead the swing shows as the foot pulling in, not lifting. */
static void gait628(float p, float duty, float stride, float *ox, float *swing)
{
    p -= floorf(p);
    if (p < duty) {
        float u = p / duty;
        *ox = stride * (0.5f - u);             /* planted, sliding back       */
        *swing = 0.0f;
    } else {
        float u = (p - duty) / (1.0f - duty);
        *ox = stride * (u - 0.5f);
        *swing = sinf(3.14159265f * u);
    }
}

static void ik628(gk *g, float hx, float hy, float fx, float fy,
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
    gk_seg(g, hx, hy, kx, ky, c, cr, gr, 0.15f);
    gk_seg(g, kx, ky, fx, fy, c, cr * 0.78f, gr * 0.85f, 0.12f);
}

void pattern_628(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    gk_setup(&g628, w, h);
    gk_clear(&g628);

    float cw = (float)g628.cw, ch = (float)g628.ch, sc = g628.sc;
    float t  = (float)frame;
    float col[3];

    if (seed != bs628) {
        base628 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NC628; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 8237);
            csz628[i]   = (0.70f + 0.65f * gk_hash(r + 1u)) * sc;
            cx628[i]    = gk_hash(r + 2u) * cw;
            cy628[i]    = ch * (0.16f + 0.68f * gk_hash(r + 3u));
            csp628[i]   = (0.95f + 0.95f * gk_hash(r + 4u)) * sc;
            cph628[i]   = gk_hash(r + 5u);
            chu628[i]   = gk_hash(r + 6u);
            ccl628[i]   = gk_hash(r + 7u) * GK_TAU;
            cdir628[i]  = gk_hash(r + 8u) > 0.5f ? 1 : -1;
            cflip628[i] = 40 + (int)(gk_hash(r + 9u) * 160.0f);
        }
        bs628 = seed;
    }

    for (i = 0; i < NC628; i++) {
        float S = csz628[i];
        float stride = 13.0f * S;
        float duty = 0.55f;

        if (--cflip628[i] <= 0) {              /* turn and go the other way   */
            cdir628[i] = -cdir628[i];
            cflip628[i] = 50 + (int)(gk_hash((uint32_t)frame ^ (uint32_t)(i * 1721)) * 190.0f);
        }
        float dir = (float)cdir628[i];

        cph628[i] += csp628[i] / (stride * 1.1f);
        cx628[i]  += dir * csp628[i];
        /* a slow vertical wander so the crabs are not on rails */
        cy628[i]  += (gk_noise1(t * 0.004f + (float)i * 8.2f, 515u) - 0.5f) * 0.45f * sc;
        if (cy628[i] < ch * 0.10f) cy628[i] = ch * 0.10f;
        if (cy628[i] > ch * 0.90f) cy628[i] = ch * 0.90f;

        float m = 60.0f * S;
        if (cx628[i] > cw + m) cx628[i] = -m;
        if (cx628[i] < -m)     cx628[i] = cw + m;

        float ph = cph628[i];
        /* the shell rocks twice a cycle, once per flank's wave */
        float x = cx628[i];
        float y = cy628[i] + sinf(ph * 2.0f * GK_TAU) * 0.7f * S;

        /* ---- EIGHT WALKING LEGS: a wave down each flank, flanks opposed --- */
        sk_col(pal, sk_hidx(base628, chu628[i] + 0.50f), 0.07f, 0.45f, 0.48f, col);
        for (k = 0; k < 8; k++) {
            int   leg  = k >> 1;                       /* 0 leading .. 3 trailing */
            float side = (k & 1) ? 1.0f : -1.0f;       /* lower / upper flank     */
            /* leading leg is the one on the side the crab is heading toward */
            float along = dir * (9.0f - (float)leg * 6.5f) * S;
            float hipx = x + along * 0.55f;
            float hipy = y + side * 4.0f * S;

            float ox, sw;
            gait628(ph + lag628[leg] + ((k & 1) ? 0.5f : 0.0f), duty, stride, &ox, &sw);
            float reach = (15.0f - 4.5f * sw) * S;     /* swing pulls the foot in */
            float ftx = x + along + ox * dir;
            float fty = y + side * reach;

            ik628(&g628, hipx, hipy, ftx, fty,
                  9.5f * S, 9.5f * S, side * dir, col,
                  0.55f * S, 1.9f * S);
        }

        /* ---- CARAPACE: a row of discs across the travel axis ------------- */
        for (k = 0; k < 7; k++) {
            float f = (float)k / 6.0f * 2.0f - 1.0f;   /* -1 .. 1 across      */
            float r = (6.2f - 3.4f * f * f) * S;
            if (r < 1.0f * S) r = 1.0f * S;
            sk_col(pal, sk_hidx(base628, chu628[i] + 0.02f * f),
                   0.05f, 0.74f, 0.36f, col);
            gk_disc(&g628, x + f * 12.0f * S, y, r, col);
        }
        /* shell rim along the front edge, and one hard highlight */
        sk_col(pal, sk_hidx(base628, chu628[i] + 0.04f), 0.30f, 0.45f, 0.40f, col);
        gk_seg(&g628, x - 11.0f * S, y - 4.4f * S, x + 11.0f * S, y - 4.4f * S,
               col, 0.7f * S, 2.4f * S, 0.14f);
        sk_col(pal, sk_hidx(base628, chu628[i]), 0.84f, 0.10f, 0.85f, col);
        gk_dot(&g628, x + dir * 4.0f * S, y - 2.0f * S, col, 1.0f * S, 3.0f * S, 0.26f);

        /* ---- EYESTALKS: short, up, twitching on their own clock ---------- */
        sk_col(pal, sk_hidx(base628, chu628[i] + 0.20f), 0.55f, 0.25f, 0.80f, col);
        for (k = 0; k < 2; k++) {
            float sx = x + (k ? 2.6f : -2.6f) * S;
            float tw = sinf(t * 0.11f + (float)i + (float)k * 1.7f) * 0.8f * S;
            gk_seg(&g628, sx, y - 2.0f * S, sx + tw, y - 7.5f * S,
                   col, 0.45f * S, 1.4f * S, 0.10f);
            gk_dot(&g628, sx + tw, y - 8.0f * S, col, 0.95f * S, 2.6f * S, 0.24f);
        }

        /* ---- CLAWS: raised, swung toward the line of travel, pincers on a
         * slow clock of their own so the wave is not tied to the steps ----- */
        float open = 0.35f + 0.65f * (0.5f + 0.5f * sinf(ccl628[i] + t * 0.055f));
        sk_col(pal, sk_hidx(base628, chu628[i] + 0.46f), 0.10f, 0.52f, 0.60f, col);
        for (k = 0; k < 2; k++) {
            float side = k ? 1.0f : -1.0f;
            float shx = x + dir * 10.0f * S;
            float shy = y + side * 2.6f * S;
            /* elbow out and up, wrist above the shell: held, not dragged */
            float elx = shx + dir * 8.0f * S + side * 2.0f * S;
            float ely = shy - 7.0f * S;
            float wrx = elx + dir * 6.5f * S;
            float wry = ely - 7.5f * S - side * 1.5f * S;
            gk_seg(&g628, shx, shy, elx, ely, col, 1.5f * S, 3.4f * S, 0.15f);
            gk_seg(&g628, elx, ely, wrx, wry, col, 1.3f * S, 3.0f * S, 0.14f);
            /* the pincer: two short fingers hinged at the wrist */
            float a0 = atan2f(wry - ely, wrx - elx);
            float sp = open * 0.55f;
            gk_seg(&g628, wrx, wry,
                   wrx + cosf(a0 + sp) * 6.0f * S, wry + sinf(a0 + sp) * 6.0f * S,
                   col, 0.85f * S, 2.2f * S, 0.13f);
            gk_seg(&g628, wrx, wry,
                   wrx + cosf(a0 - sp) * 5.2f * S, wry + sinf(a0 - sp) * 5.2f * S,
                   col, 0.75f * S, 2.0f * S, 0.12f);
        }
    }

    gk_present(&g628, fb, w, h);
}
