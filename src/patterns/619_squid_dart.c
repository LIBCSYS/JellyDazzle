/* 619 Squid Dart (creature) — squid crossing in bursts, tail-first, which is
 * the way they actually travel at speed.
 *
 * The mantle is a pump and everything else follows from it.  A refill phase
 * swells the body; a contraction phase empties it over a handful of frames,
 * and the impulse is the rate of emptying, not the amount emptied.  Velocity
 * is then pure impulse-and-damping, so the animal goes still-fast-coast-still
 * without any of it being keyframed.  Width and length trade against each
 * other during the contraction, because a squid conserves volume: get that
 * backwards and it looks like a balloon rather than a muscle.
 *
 * Direction: the pointed end with the fins leads, the arms trail.  The two
 * tail fins only have work to do on the coast — under thrust they lie flat
 * along the body, and on the glide they open out and steer, which is the
 * clearest single cue that the burst has ended.
 *
 * The ten arms are the whip.  Each one has a per-segment phase lag, so when a
 * burst fires the straightening runs from the mantle outward over several
 * frames and the tips are still catching up after the body has stopped
 * accelerating.  Two of the ten are the long feeding tentacles and are drawn
 * longer and thinner; without them the animal reads as a cuttlefish. */
#include "_spark572.h"

#define NQ619 3            /* squid                                          */
#define NAR619 10          /* arms, two of which are the long pair           */
#define QS619 8            /* segments per arm                               */

static gk g619;
static uint32_t bs619 = 0xFFFFFFFFu;
static int   base619;
static float qx619[NQ619], qy619[NQ619], qvx619[NQ619], qvy619[NQ619];
static float qsc619[NQ619], qhue619[NQ619], qhd619[NQ619];
static float qph619[NQ619], qper619[NQ619];

void pattern_619(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, a, s;
    float col[3];

    gk_setup(&g619, w, h);
    gk_decay_snap(&g619, 0.81f);

    float cw = (float)g619.cw, ch = (float)g619.ch, sc = g619.sc;

    if (seed != bs619) {
        base619 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NQ619; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 4093);
            float a0 = gk_hash(r + 1u) * 6.2831853f;
            qx619[i]   = gk_hash(r + 2u) * cw;
            qy619[i]   = gk_hash(r + 3u) * ch;
            qsc619[i]  = (0.60f + 0.55f * gk_hash(r + 4u)) * sc;
            qhue619[i] = gk_hash(r + 5u);
            qhd619[i]  = a0;
            qvx619[i]  = cosf(a0) * 0.3f * qsc619[i];
            qvy619[i]  = sinf(a0) * 0.3f * qsc619[i];
            qph619[i]  = gk_hash(r + 6u) * 70.0f;
            qper619[i] = 58.0f + 40.0f * gk_hash(r + 7u);
        }
        bs619 = seed;
    }

    float t = (float)frame;

    for (i = 0; i < NQ619; i++) {
        float S = qsc619[i];

        /* PUMP: 0.82 of the cycle refilling, the rest emptying hard. */
        float cyc = fmodf(t + qph619[i], qper619[i]) / qper619[i];
        float fill, drate;
        if (cyc < 0.82f) {
            fill  = gk_smooth(cyc / 0.82f);
            drate = 0.0f;
        } else {
            float u = (cyc - 0.82f) / 0.18f;
            fill  = 1.0f - gk_smooth(u);
            drate = 1.0f - fabsf(2.0f * u - 1.0f);
        }

        /* Steering happens with the jet; a coasting squid holds its line. */
        qhd619[i] += (gk_noise1(t * 0.0045f + (float)i * 29.0f, 191u + (uint32_t)i) - 0.5f)
                   * 0.075f * (0.15f + drate);

        qvx619[i] += cosf(qhd619[i]) * drate * 0.85f * S;
        qvy619[i] += sinf(qhd619[i]) * drate * 0.85f * S;
        qvx619[i] *= 0.955f;
        qvy619[i] *= 0.955f;
        qx619[i]  += qvx619[i];
        qy619[i]  += qvy619[i];

        float spd = sqrtf(qvx619[i] * qvx619[i] + qvy619[i] * qvy619[i]);
        float tight = spd / (2.2f * S);
        if (tight > 1.0f) tight = 1.0f;

        float mg = 80.0f * S;
        if (qx619[i] < -mg) qx619[i] += cw + 2.0f * mg;
        if (qx619[i] > cw + mg) qx619[i] -= cw + 2.0f * mg;
        if (qy619[i] < -mg) qy619[i] += ch + 2.0f * mg;
        if (qy619[i] > ch + mg) qy619[i] -= ch + 2.0f * mg;

        float hx = cosf(qhd619[i]), hy = sinf(qhd619[i]);
        float nx = -hy, ny = hx;
        float cx = qx619[i], cy = qy619[i];

        /* MANTLE: volume held roughly constant, so emptying makes it longer
         * and thinner.  Length is measured from the fin tip (ahead) back to
         * the collar where the arms attach. */
        float ml = (34.0f + 9.0f * (1.0f - fill)) * S;
        float mr = (5.0f + 6.0f * fill) * S;

        sk_col(pal, sk_hidx(base619, qhue619[i]), 0.12f, 0.62f, 0.24f, col);
        for (s = 0; s <= 8; s++) {
            float u = (float)s / 8.0f;           /* 0 at the fin tip          */
            /* pointed at the front, blunt at the collar */
            float r = mr * (0.12f + 0.95f * sinf(3.14159265f * (0.10f + 0.80f * u)));
            gk_disc(&g619, cx + hx * (0.70f - u) * ml,
                           cy + hy * (0.70f - u) * ml, r, col);
        }

        /* mantle edge, both sides — the readable silhouette */
        sk_col(pal, sk_hidx(base619, qhue619[i] + 0.03f), 0.32f, 0.50f, 0.88f, col);
        {
            int k2;
            float tipx = cx + hx * 0.74f * ml, tipy = cy + hy * 0.74f * ml;
            for (k2 = -1; k2 <= 1; k2 += 2) {
                float fk = (float)k2;
                float midx = cx + hx * 0.10f * ml + nx * fk * mr;
                float midy = cy + hy * 0.10f * ml + ny * fk * mr;
                float colx = cx - hx * 0.30f * ml + nx * fk * mr * 0.92f;
                float coly = cy - hy * 0.30f * ml + ny * fk * mr * 0.92f;
                gk_seg(&g619, tipx, tipy, midx, midy, col, 0.85f * S, 2.6f * S, 0.18f);
                gk_seg(&g619, midx, midy, colx, coly, col, 0.85f * S, 2.6f * S, 0.18f);
            }
        }

        /* TAIL FINS: flat against the body under thrust, fanned out on the
         * coast.  This is the cue that the burst is over. */
        {
            float open = 1.0f - tight;
            int k2;
            sk_col(pal, sk_hidx(base619, qhue619[i] + 0.07f),
                   0.20f, 0.48f, 0.55f + 0.30f * open, col);
            for (k2 = -1; k2 <= 1; k2 += 2) {
                float fk = (float)k2;
                float rx = cx + hx * 0.50f * ml, ry = cy + hy * 0.50f * ml;
                float ox = rx + hx * 11.0f * S + nx * fk * (2.5f + 11.0f * open) * S;
                float oy = ry + hy * 11.0f * S + ny * fk * (2.5f + 11.0f * open) * S;
                gk_seg(&g619, rx, ry, ox, oy, col, 0.9f * S, 2.8f * S, 0.16f);
                gk_seg(&g619, ox, oy,
                       rx - hx * 9.0f * S + nx * fk * 1.5f * S,
                       ry - hy * 9.0f * S + ny * fk * 1.5f * S,
                       col, 0.7f * S, 2.4f * S, 0.14f);
            }
        }

        /* EYES: on the head, at the collar end. */
        sk_col(pal, sk_hidx(base619, qhue619[i] + 0.45f), 0.70f, 0.26f, 1.00f, col);
        {
            int k2;
            for (k2 = -1; k2 <= 1; k2 += 2)
                gk_dot(&g619, cx - hx * 0.34f * ml + nx * (float)k2 * mr * 0.85f,
                              cy - hy * 0.34f * ml + ny * (float)k2 * mr * 0.85f,
                       col, 0.95f * S, 2.5f * S, 0.30f);
        }

        /* JET WASH: thrown forward from the funnel, which sits under the
         * head and points the way the animal is going. */
        if (drate > 0.05f) {
            sk_col(pal, sk_hidx(base619, qhue619[i] + 0.52f),
                   0.52f, 0.34f, 0.65f * drate, col);
            for (s = 1; s <= 4; s++) {
                uint32_t r = (uint32_t)frame * 2246822519u + (uint32_t)(i * 71 + s);
                float d = (float)s * 8.0f * S;
                gk_dot(&g619, cx + hx * (0.80f * ml + d) + nx * (gk_hash(r + 1u) - 0.5f) * 6.0f * S,
                              cy + hy * (0.80f * ml + d) + ny * (gk_hash(r + 2u) - 0.5f) * 6.0f * S,
                       col, 1.2f * S, 4.2f * S, 0.20f);
            }
        }

        /* ARMS: ten, trailing off the collar.  Fan collapses with speed; the
         * per-segment lag is what makes the straightening travel outward. */
        for (a = 0; a < NAR619; a++) {
            float fan  = ((float)a / (float)(NAR619 - 1) - 0.5f) * 2.0f;   /* -1..1 */
            int   longp = (a == 2 || a == NAR619 - 3);   /* the feeding pair  */
            float spread = fan * (0.95f - 0.72f * tight);
            float curl   = (1.0f - tight) * (0.5f + 0.7f * gk_hash((uint32_t)(i * 17 + a)));

            sk_col(pal, sk_hidx(base619, qhue619[i] + 0.09f + 0.015f * (float)a),
                   longp ? 0.20f : 0.13f, 0.46f, longp ? 0.80f : 0.68f, col);

            float bx = cx - hx * 0.36f * ml + nx * fan * mr * 0.70f;
            float by = cy - hy * 0.36f * ml + ny * fan * mr * 0.70f;
            float qx = bx, qy = by;
            float ang = atan2f(-hy, -hx) + spread * 0.75f;
            float segl = (longp ? 11.0f : 7.5f) * S;

            for (s = 1; s <= QS619; s++) {
                float d   = (float)s / (float)QS619;
                float lag = t * 0.11f - d * 2.6f + (float)a * 0.65f;
                ang += curl * 0.30f * d + sinf(lag) * 0.24f * (1.0f - 0.55f * tight);
                float len = segl * (1.0f - 0.35f * d) * (0.70f + 0.50f * tight);
                float nx2 = qx + cosf(ang) * len;
                float ny2 = qy + sinf(ang) * len;
                gk_seg(&g619, qx, qy, nx2, ny2, col,
                       ((longp ? 0.55f : 0.78f) - 0.42f * d) * S,
                       2.0f * S, 0.12f * (1.0f - 0.5f * d));
                qx = nx2; qy = ny2;
            }
        }
    }

    gk_present(&g619, fb, w, h);
}
