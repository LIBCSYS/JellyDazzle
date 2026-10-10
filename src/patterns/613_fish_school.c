/* 613 Fish School (creature) — a few dozen small fish behaving as one body.
 *
 * Straight boids (separation, alignment, cohesion) gives you a clump that
 * mills about; it does not give you the thing that makes a school worth
 * watching, which is the whole shoal committing to a turn at once.  So there
 * is a fourth force: a shared heading driven by slow noise, weighted hard
 * enough to dominate the local rules.  Individuals still jostle, but the body
 * turns as a body.
 *
 * The shimmer is the point of the pattern.  Real fish flash when they bank,
 * because a flank catches the light; here each fish carries its heading from
 * last frame, and the size of this frame's turn drives a whiteness and
 * brightness bump.  So a turn ripples through the shoal as a flash, strongest
 * where the turn is sharpest, and the school is brightest exactly when it
 * changes its mind.  Neighbour search is O(n^2) — forty-odd fish, nobody
 * cares, and a grid would cost more in code than it saves in cycles. */
#include "_spark572.h"

#define NF613 46

static gk g613;
static uint32_t bs613 = 0xFFFFFFFFu;
static int   base613;
static float fx613[NF613], fy613[NF613], fvx613[NF613], fvy613[NF613];
static float fang613[NF613], fsh613[NF613], fsz613[NF613], fhue613[NF613];

void pattern_613(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, j;
    float col[3];

    gk_setup(&g613, w, h);
    gk_decay_snap(&g613, 0.72f);               /* short wake, fish are quick */

    float cw = (float)g613.cw, ch = (float)g613.ch, sc = g613.sc;

    if (seed != bs613) {
        base613 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NF613; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 6151);
            float a = gk_hash(r + 1u) * 6.2831853f;
            fx613[i]   = (0.35f + 0.30f * gk_hash(r + 2u)) * cw;
            fy613[i]   = (0.35f + 0.30f * gk_hash(r + 3u)) * ch;
            fvx613[i]  = cosf(a) * 1.4f * sc;
            fvy613[i]  = sinf(a) * 1.4f * sc;
            fang613[i] = a;
            fsh613[i]  = 0.0f;
            fsz613[i]  = (0.75f + 0.55f * gk_hash(r + 4u)) * sc;
            fhue613[i] = gk_hash(r + 5u) * 0.14f;   /* one species, not a reef */
        }
        bs613 = seed;
    }

    float t = (float)frame;

    /* The shared intent.  Two noise streams, slow, so the shoal's heading
     * wanders over seconds rather than frames. */
    float lead = gk_noise1(t * 0.0045f, 311u) * 6.2831853f
               + gk_noise1(t * 0.0013f, 312u) * 3.0f;
    float lx = cosf(lead), ly = sinf(lead);

    /* centroid and mean heading, computed once for the whole shoal */
    float ccx = 0.0f, ccy = 0.0f, cvx = 0.0f, cvy = 0.0f;
    for (i = 0; i < NF613; i++) {
        ccx += fx613[i]; ccy += fy613[i];
        cvx += fvx613[i]; cvy += fvy613[i];
    }
    ccx /= (float)NF613; ccy /= (float)NF613;
    cvx /= (float)NF613; cvy /= (float)NF613;

    float sep_r = 11.0f * sc;

    for (i = 0; i < NF613; i++) {
        float ax = 0.0f, ay = 0.0f;

        /* SEPARATION — the only short-range rule, and it has to be strong or
         * the shoal collapses into one bright blob. */
        for (j = 0; j < NF613; j++) {
            if (j == i) continue;
            float dx = fx613[i] - fx613[j], dy = fy613[i] - fy613[j];
            float d2 = dx * dx + dy * dy;
            if (d2 > sep_r * sep_r || d2 < 1e-4f) continue;
            float d = sqrtf(d2);
            ax += (dx / d) * (1.0f - d / sep_r) * 0.42f * sc;
            ay += (dy / d) * (1.0f - d / sep_r) * 0.42f * sc;
        }

        /* COHESION — pull to the centroid, deliberately weak; the shared
         * heading is what holds them together, not this. */
        ax += (ccx - fx613[i]) * 0.0016f;
        ay += (ccy - fy613[i]) * 0.0016f;

        /* ALIGNMENT to the shoal's mean heading */
        ax += (cvx - fvx613[i]) * 0.055f;
        ay += (cvy - fvy613[i]) * 0.055f;

        /* SHARED INTENT — dominant, and the reason the turns are collective */
        ax += lx * 0.085f * sc;
        ay += ly * 0.085f * sc;

        /* a little private restlessness, per-fish noise stream */
        ax += (gk_noise1(t * 0.03f + (float)i * 7.1f, 400u + (uint32_t)i) - 0.5f) * 0.09f * sc;
        ay += (gk_noise1(t * 0.03f + (float)i * 7.1f, 500u + (uint32_t)i) - 0.5f) * 0.09f * sc;

        fvx613[i] += ax;
        fvy613[i] += ay;

        /* speed clamp: fish cruise in a narrow band, which is what keeps the
         * school coherent after a hard turn */
        float sp = sqrtf(fvx613[i] * fvx613[i] + fvy613[i] * fvy613[i]);
        float want = 1.75f * sc;
        if (sp > 1e-4f) {
            float f = (want + (sp - want) * 0.30f) / sp;
            fvx613[i] *= f; fvy613[i] *= f;
        }

        fx613[i] += fvx613[i];
        fy613[i] += fvy613[i];

        /* wrap with a margin, so nothing pops in at the very edge */
        float mg = 24.0f * sc;
        if (fx613[i] < -mg) fx613[i] += cw + 2.0f * mg;
        if (fx613[i] > cw + mg) fx613[i] -= cw + 2.0f * mg;
        if (fy613[i] < -mg) fy613[i] += ch + 2.0f * mg;
        if (fy613[i] > ch + mg) fy613[i] -= ch + 2.0f * mg;

        /* THE SHIMMER: turn rate this frame, wrapped to -pi..pi, drives a
         * flash that decays over about a dozen frames. */
        float na = atan2f(fvy613[i], fvx613[i]);
        float da = na - fang613[i];
        while (da >  3.14159265f) da -= 6.2831853f;
        while (da < -3.14159265f) da += 6.2831853f;
        fang613[i] = na;
        float flash = fabsf(da) * 7.0f;
        if (flash > 1.0f) flash = 1.0f;
        if (flash > fsh613[i]) fsh613[i] = flash;
        fsh613[i] *= 0.90f;

        /* BODY: a short dash along the heading plus a forked tail that lags
         * the heading, so the fish looks like it is beating rather than
         * pointing. */
        float S  = fsz613[i];
        float ca = cosf(na), sa = sinf(na);
        float nxp = -sa, nyp = ca;
        float beat = sinf(t * 0.42f + (float)i * 1.7f);

        float hx = fx613[i] + ca * 4.2f * S, hy = fy613[i] + sa * 4.2f * S;
        float bx = fx613[i] - ca * 3.4f * S, by = fy613[i] - sa * 3.4f * S;
        float tailoff = beat * 2.1f * S;
        float tx = bx - ca * 2.2f * S + nxp * tailoff;
        float ty = by - sa * 2.2f * S + nyp * tailoff;

        sk_col(pal, sk_hidx(base613, fhue613[i] + 0.02f * fsh613[i]),
               0.18f + 0.62f * fsh613[i], 0.45f,
               0.55f + 0.75f * fsh613[i], col);
        gk_seg(&g613, bx, by, hx, hy, col, 1.05f * S, 3.0f * S, 0.22f);

        /* tail fork, dimmer — it is a hint, not a feature */
        sk_col(pal, sk_hidx(base613, fhue613[i] + 0.05f),
               0.12f + 0.40f * fsh613[i], 0.45f,
               0.32f + 0.45f * fsh613[i], col);
        gk_seg(&g613, bx, by, tx + nxp * 1.6f * S, ty + nyp * 1.6f * S,
               col, 0.55f * S, 1.8f * S, 0.12f);
        gk_seg(&g613, bx, by, tx - nxp * 1.6f * S, ty - nyp * 1.6f * S,
               col, 0.55f * S, 1.8f * S, 0.12f);
    }

    gk_present(&g613, fb, w, h);
}
