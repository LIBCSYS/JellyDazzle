/* 615 Octopus Jet (creature) — octopuses crossing open water, which they do
 * badly and intermittently: a hard squirt from the siphon, a long coast, then
 * another squirt when the first has nearly died.
 *
 * Everything readable about the animal comes out of that duty cycle, so the
 * motion is modelled as impulse plus damping rather than a velocity curve.
 * The mantle fills over the coast and empties in a few frames, and the
 * impulse is taken from the rate of emptying — so the body visibly deflates
 * at the same instant it accelerates, and the two are the same event rather
 * than two animations that happen to agree.
 *
 * The arms are the tell.  Under thrust they stream out dead straight behind,
 * because water is pulling them; on the coast they relax and curl, each one
 * at its own rate.  A per-segment phase lag means a straightening travels
 * down an arm from the base outward over several frames.  Nothing about the
 * arms is driven directly by the clock: they follow speed, and speed follows
 * the mantle.  Head-first is wrong for a jetting octopus — it goes arms-
 * trailing, mantle leading, which is why the eyes sit at the back of travel. */
#include "_spark572.h"

#define NO615 3            /* animals                                        */
#define NA615 8            /* arms, obviously                                */
#define AS615 9            /* segments per arm                               */

static gk g615;
static uint32_t bs615 = 0xFFFFFFFFu;
static int   base615;
static float ox615[NO615], oy615[NO615], ovx615[NO615], ovy615[NO615];
static float osc615[NO615], ohue615[NO615], oph615[NO615], operiod615[NO615];
static float ohead615[NO615], ocurl615[NO615][NA615];

void pattern_615(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, a, s;
    float col[3];

    gk_setup(&g615, w, h);
    gk_decay_snap(&g615, 0.84f);

    float cw = (float)g615.cw, ch = (float)g615.ch, sc = g615.sc;

    if (seed != bs615) {
        base615 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NO615; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 5443);
            float a0 = gk_hash(r + 1u) * 6.2831853f;
            ox615[i]      = gk_hash(r + 2u) * cw;
            oy615[i]      = gk_hash(r + 3u) * ch;
            osc615[i]     = (0.55f + 0.60f * gk_hash(r + 4u)) * sc;
            ohue615[i]    = gk_hash(r + 5u);
            oph615[i]     = gk_hash(r + 6u) * 90.0f;
            operiod615[i] = 78.0f + 46.0f * gk_hash(r + 7u);
            ohead615[i]   = a0;
            ovx615[i]     = cosf(a0) * 0.4f * osc615[i];
            ovy615[i]     = sinf(a0) * 0.4f * osc615[i];
            for (a = 0; a < NA615; a++)
                ocurl615[i][a] = 0.6f + 0.8f * gk_hash(r + 20u + (uint32_t)a);
        }
        bs615 = seed;
    }

    float t = (float)frame;

    for (i = 0; i < NO615; i++) {
        float S = osc615[i];

        /* MANTLE CYCLE: fill slowly, empty fast.  `fill` is 1 when fully
         * inflated.  The contraction is the last fifth of the period, shaped
         * so the collapse is sharp and the refill is lazy. */
        float cyc = fmodf(t + oph615[i], operiod615[i]) / operiod615[i];
        float fill, drate;
        if (cyc < 0.80f) {
            float u = cyc / 0.80f;
            fill  = gk_smooth(u);
            drate = 0.0f;                       /* refilling, no thrust      */
        } else {
            float u = (cyc - 0.80f) / 0.20f;
            fill  = 1.0f - gk_smooth(u);
            drate = 1.0f - fabsf(2.0f * u - 1.0f);   /* peaks mid-squirt     */
        }

        /* Heading wanders only between squirts; an octopus cannot steer
         * while coasting, so the turn happens with the siphon. */
        ohead615[i] += (gk_noise1(t * 0.004f + (float)i * 17.0f, 141u + (uint32_t)i) - 0.5f)
                     * 0.055f * (1.0f - drate);

        /* impulse + damping — this is the entire locomotion model */
        float imp = drate * 0.55f * S;
        ovx615[i] += cosf(ohead615[i]) * imp;
        ovy615[i] += sinf(ohead615[i]) * imp;
        ovx615[i] *= 0.962f;
        ovy615[i] *= 0.962f;
        ox615[i]  += ovx615[i];
        oy615[i]  += ovy615[i];

        float spd  = sqrtf(ovx615[i] * ovx615[i] + ovy615[i] * ovy615[i]);
        float tight = spd / (1.4f * S);              /* 0 relaxed, 1 streamed */
        if (tight > 1.0f) tight = 1.0f;

        float mg = 70.0f * S;
        if (ox615[i] < -mg) ox615[i] += cw + 2.0f * mg;
        if (ox615[i] > cw + mg) ox615[i] -= cw + 2.0f * mg;
        if (oy615[i] < -mg) oy615[i] += ch + 2.0f * mg;
        if (oy615[i] > ch + mg) oy615[i] -= ch + 2.0f * mg;

        /* Travel axis.  The mantle tip leads, the eyes sit behind it, and the
         * arms go out the back. */
        float hx = cosf(ohead615[i]), hy = sinf(ohead615[i]);
        float px = -hy, py = hx;
        float cx = ox615[i], cy = oy615[i];

        /* MANTLE: discs along the travel axis, length and girth traded off
         * against `fill` so it reads as a bag of water being squeezed. */
        float ml = (30.0f + 10.0f * fill) * S;
        float mr = (7.0f + 7.0f * fill) * S;
        sk_col(pal, sk_hidx(base615, ohue615[i]), 0.12f, 0.62f, 0.26f, col);
        for (s = 0; s <= 7; s++) {
            float u = (float)s / 7.0f;
            float r = mr * (0.35f + 0.92f * sinf(3.14159265f * (0.18f + 0.78f * u)));
            gk_disc(&g615, cx + hx * (0.62f - u) * ml,
                           cy + hy * (0.62f - u) * ml, r, col);
        }

        /* mantle rim, the readable edge */
        sk_col(pal, sk_hidx(base615, ohue615[i] + 0.03f), 0.32f, 0.50f, 0.85f, col);
        {
            float qx = cx + hx * 0.62f * ml, qy = cy + hy * 0.62f * ml;
            int k2;
            for (k2 = -1; k2 <= 1; k2 += 2) {
                float fk = (float)k2;
                gk_seg(&g615, qx, qy,
                       cx - hx * 0.12f * ml + px * fk * mr * 1.02f,
                       cy - hy * 0.12f * ml + py * fk * mr * 1.02f,
                       col, 0.85f * S, 2.8f * S, 0.18f);
            }
        }

        /* EYES: two, on the head, which is at the back of travel. */
        sk_col(pal, sk_hidx(base615, ohue615[i] + 0.44f), 0.68f, 0.28f, 1.00f, col);
        {
            int k2;
            for (k2 = -1; k2 <= 1; k2 += 2)
                gk_dot(&g615, cx - hx * 0.22f * ml + px * (float)k2 * mr * 0.72f,
                              cy - hy * 0.22f * ml + py * (float)k2 * mr * 0.72f,
                       col, 1.0f * S, 2.6f * S, 0.30f);
        }

        /* SIPHON PUFF: the water actually leaving, visible only during the
         * squirt and thrown forward past the mantle tip. */
        if (drate > 0.05f) {
            sk_col(pal, sk_hidx(base615, ohue615[i] + 0.50f),
                   0.55f, 0.35f, 0.70f * drate, col);
            for (s = 1; s <= 4; s++) {
                float d = (float)s;
                gk_dot(&g615, cx + hx * (0.70f * ml + d * 7.0f * S)
                            + px * (gk_hash((uint32_t)(i * 31 + s) ^ (uint32_t)frame) - 0.5f) * 5.0f * S,
                              cy + hy * (0.70f * ml + d * 7.0f * S)
                            + py * (gk_hash((uint32_t)(i * 37 + s) ^ (uint32_t)frame) - 0.5f) * 5.0f * S,
                       col, 1.3f * S, 4.5f * S, 0.22f);
            }
        }

        /* ARMS: rooted at the head end and sent out the back.  Base angle
         * fans them across a half-turn; `tight` collapses that fan toward
         * straight-behind, and the curl only shows up on the coast.  The
         * per-segment phase lag is what makes a straightening look like it
         * travels outward rather than toggling. */
        for (a = 0; a < NA615; a++) {
            float fan = ((float)a / (float)(NA615 - 1) - 0.5f) * 2.0f;   /* -1..1 */
            float spread = fan * (1.15f - 0.85f * tight);
            float curl   = ocurl615[i][a] * (1.0f - tight);

            sk_col(pal, sk_hidx(base615, ohue615[i] + 0.08f + 0.02f * (float)a),
                   0.14f, 0.48f, 0.72f, col);

            float bx = cx - hx * 0.30f * ml + px * fan * mr * 0.55f;
            float by = cy - hy * 0.30f * ml + py * fan * mr * 0.55f;
            float qx = bx, qy = by;
            float ang = atan2f(-hy, -hx) + spread * 0.80f;

            for (s = 1; s <= AS615; s++) {
                float d   = (float)s / (float)AS615;
                float lag = t * 0.085f - d * 2.4f + (float)a * 0.8f;
                /* curl grows down the arm; the sine is the loose wander that
                 * stops eight arms looking like eight wires */
                ang += curl * 0.34f * d + sinf(lag) * 0.26f * (1.0f - 0.5f * tight);
                float len = (8.5f - 3.0f * d) * S * (0.75f + 0.45f * tight);
                float nx2 = qx + cosf(ang) * len;
                float ny2 = qy + sinf(ang) * len;
                gk_seg(&g615, qx, qy, nx2, ny2, col,
                       (0.80f - 0.52f * d) * S, 2.2f * S,
                       0.13f * (1.0f - 0.5f * d));
                qx = nx2; qy = ny2;
            }
        }
    }

    gk_present(&g615, fb, w, h);
}
