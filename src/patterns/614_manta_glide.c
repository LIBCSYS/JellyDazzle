/* 614 Manta Glide (creature) — a manta seen from behind and a little above,
 * climbing the frame, wings working in a travelling wave.
 *
 * The camera choice is the whole design.  A manta drawn in side view is a
 * sliver; drawn in plan view the flap has nowhere to go, because span and
 * flap would share the screen's vertical axis.  Put the animal on a course up
 * the frame and the span gets the horizontal axis to itself, leaving vertical
 * free for the flap — so the tips visibly rise and fall while the span
 * foreshortens by cos(flap), exactly as it does in life.  That foreshortening
 * is what sells it; without it the wings look hinged.
 *
 * The wave runs outward from the spine with a phase lag proportional to span
 * fraction, so a stroke starts at the shoulder and arrives at the tip a beat
 * later.  Surface is drawn as four span-wise curves at increasing chord
 * fractions rather than a filled polygon: the leading edge stays bright, the
 * trailing edge goes soft, and the gap between them reads as thickness. */
#include "_spark572.h"

#define NM614 2            /* animals; more than two and the frame is busy   */
#define NV614 15           /* span samples across the full wingspan          */
#define NC614 4            /* chord-wise curves per wing                     */

static gk g614;
static uint32_t bs614 = 0xFFFFFFFFu;
static int   base614;
static float mx614[NM614], my614[NM614], msc614[NM614], mhue614[NM614];
static float mph614[NM614], mrate614[NM614], mdrift614[NM614], mroll614[NM614];

void pattern_614(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, c, v;
    float col[3];

    gk_setup(&g614, w, h);
    gk_decay_snap(&g614, 0.80f);

    float cw = (float)g614.cw, ch = (float)g614.ch, sc = g614.sc;

    if (seed != bs614) {
        base614 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NM614; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 9173);
            mx614[i]    = (0.22f + 0.56f * gk_hash(r + 1u)) * cw;
            my614[i]    = gk_hash(r + 2u) * ch;
            msc614[i]   = (0.60f + 0.60f * gk_hash(r + 3u)) * sc;
            mhue614[i]  = gk_hash(r + 4u);
            mph614[i]   = gk_hash(r + 5u) * 6.2831853f;
            mrate614[i] = 0.030f + 0.018f * gk_hash(r + 6u);
            mdrift614[i]= (gk_hash(r + 7u) - 0.5f) * 0.5f;
            mroll614[i] = 0.0f;
        }
        bs614 = seed;
    }

    float t = (float)frame;

    for (i = 0; i < NM614; i++) {
        float S  = msc614[i];
        float ph = mph614[i] + t * mrate614[i];

        /* A manta glides most of the time; the downstroke supplies a nudge,
         * not a lurch, so thrust is a shallow function of the stroke rate. */
        float thrust = 0.5f + 0.5f * cosf(ph);
        my614[i] -= (0.55f + 0.65f * thrust) * S;

        /* A slow roll makes the course look flown rather than driven. */
        mroll614[i] = (gk_noise1(t * 0.0035f + (float)i * 11.0f, 71u + (uint32_t)i) - 0.5f) * 0.45f;
        mx614[i] += (mdrift614[i] + mroll614[i] * 1.6f) * S;

        float span  = 52.0f * S;
        float chord = 34.0f * S;

        if (my614[i] < -span * 1.4f) {          /* off the top, back in below */
            my614[i]   = ch + span * 1.1f;
            mx614[i]   = gk_hash((uint32_t)t ^ (uint32_t)(i * 3301)) * cw;
            mhue614[i] = gk_hash((uint32_t)t ^ (uint32_t)(i * 877));
        }
        if (mx614[i] < -span) mx614[i] += cw + 2.0f * span;
        if (mx614[i] > cw + span) mx614[i] -= cw + 2.0f * span;

        float cx = mx614[i], cy = my614[i];
        float rl = mroll614[i];                 /* banked: one tip drops      */

        /* SURFACE: chord-wise curves from leading (c=0) to trailing edge.
         * Each span sample gets its own flap angle, lagged by span fraction. */
        for (c = 0; c < NC614; c++) {
            float cf = (float)c / (float)(NC614 - 1);        /* 0..1 back     */
            float px = 0.0f, py = 0.0f;
            int   started = 0;

            /* leading edge carries the shape; the rest is wash */
            sk_col(pal, sk_hidx(base614, mhue614[i] + 0.05f * cf),
                   0.34f * (1.0f - cf) + 0.10f, 0.50f,
                   1.00f - 0.52f * cf, col);

            for (v = 0; v < NV614; v++) {
                float u  = ((float)v / (float)(NV614 - 1)) * 2.0f - 1.0f;  /* -1..1 */
                float au = fabsf(u);
                /* the travelling wave: lag grows with span fraction */
                float flap = 0.52f * sinf(ph - au * 2.5f) * (0.25f + 0.95f * au * au);
                /* chord shrinks toward the tips, and sweeps back */
                float ch_v  = chord * (1.0f - 0.72f * au * au);
                float sweep = au * au * chord * 0.42f;

                float x = cx + u * span * cosf(flap) + rl * cf * 6.0f * S;
                float y = cy - sinf(flap) * span * (0.30f + 0.60f * au)
                        + (cf - 0.30f) * ch_v + sweep
                        + u * rl * span * 0.30f;          /* the bank         */

                if (started) gk_seg(&g614, px, py, x, y, col,
                                    (0.95f - 0.40f * cf) * S,
                                    (3.0f - 1.0f * cf) * S, 0.20f);
                px = x; py = y; started = 1;
            }
        }

        /* BODY: a short bright ridge down the spine, the densest part of the
         * animal and the thing the eye locks onto. */
        sk_col(pal, sk_hidx(base614, mhue614[i] + 0.02f), 0.30f, 0.55f, 0.70f, col);
        gk_disc(&g614, cx, cy - chord * 0.05f, 7.0f * S, col);
        gk_disc(&g614, cx, cy + chord * 0.22f, 5.6f * S, col);

        /* CEPHALIC FINS: the two little scoops either side of the mouth.
         * They curl in and out with the stroke, a half cycle behind. */
        sk_col(pal, sk_hidx(base614, mhue614[i] + 0.10f), 0.24f, 0.50f, 0.80f, col);
        {
            float curl = 0.45f + 0.35f * sinf(ph - 1.6f);
            int k2;
            for (k2 = -1; k2 <= 1; k2 += 2) {
                float fk = (float)k2;
                gk_seg(&g614, cx + fk * 5.0f * S, cy - chord * 0.26f,
                       cx + fk * (9.0f + 4.0f * curl) * S,
                       cy - chord * (0.26f + 0.22f * curl),
                       col, 0.7f * S, 2.2f * S, 0.16f);
            }
        }

        /* TAIL: a whip, trailing the stroke with a longer lag than the wings
         * because it is the last thing in the water. */
        sk_col(pal, sk_hidx(base614, mhue614[i] + 0.14f), 0.16f, 0.48f, 0.55f, col);
        {
            float px = cx, py = cy + chord * 0.60f;
            int s;
            for (s = 1; s <= 7; s++) {
                float d  = (float)s / 7.0f;
                float wag = sinf(ph - 2.9f - d * 2.2f) * 5.0f * S * d;
                float x = cx + wag + rl * d * 8.0f * S;
                float y = cy + chord * 0.60f + d * 46.0f * S;
                gk_seg(&g614, px, py, x, y, col,
                       0.55f * (1.0f - 0.6f * d) * S, 1.8f * S, 0.11f);
                px = x; py = y;
            }
        }
    }

    gk_present(&g614, fb, w, h);
}
