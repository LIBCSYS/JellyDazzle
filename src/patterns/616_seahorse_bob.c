/* 616 Seahorse Bob (creature) — seahorses holding station in the water
 * column, which is the whole character of the animal: almost no translation,
 * endless small adjustment.
 *
 * The silhouette is built by integrating a curvature profile along an
 * arc-length chain rather than by placing control points, because a seahorse
 * is one continuous bend that reverses twice — snout down, neck back, trunk
 * forward, tail curled under — and a point-based body loses that the moment
 * you scale it.  Two blended smoothsteps give the two reversals; the third
 * term keeps winding past a half turn so the tail closes into a spiral.
 *
 * The bob is intentionally not a sine.  Two slow noise streams at different
 * rates give drift that never repeats and never looks mechanical, which is
 * what station-keeping looks like.  The dorsal fin is the only fast thing on
 * the animal: it beats at about seven times the bob and is the reason the
 * thing reads as alive while the body barely moves.  Fin segments are drawn
 * perpendicular to the local spine tangent, so the fin stays attached to the
 * back no matter how the body bends. */
#include "_spark572.h"

#define NH616 4            /* seahorses                                      */
#define SP616 26           /* spine samples, snout to tail tip                */

static gk g616;
static uint32_t bs616 = 0xFFFFFFFFu;
static int   base616;
static float hx616[NH616], hy616[NH616], hsc616[NH616], hhue616[NH616];
static float hface616[NH616], hph616[NH616];

void pattern_616(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    float col[3];
    float sx[SP616], sy[SP616], sa[SP616];

    gk_setup(&g616, w, h);
    gk_decay_snap(&g616, 0.80f);

    float cw = (float)g616.cw, ch = (float)g616.ch, sc = g616.sc;

    if (seed != bs616) {
        base616 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NH616; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 7723);
            hx616[i]    = (0.12f + 0.76f * gk_hash(r + 1u)) * cw;
            hy616[i]    = (0.15f + 0.70f * gk_hash(r + 2u)) * ch;
            hsc616[i]   = (0.70f + 0.65f * gk_hash(r + 3u)) * sc;
            hhue616[i]  = gk_hash(r + 4u);
            hface616[i] = (gk_hash(r + 5u) < 0.5f) ? 1.0f : -1.0f;
            hph616[i]   = gk_hash(r + 6u) * 100.0f;
        }
        bs616 = seed;
    }

    float t = (float)frame;

    for (i = 0; i < NH616; i++) {
        float S  = hsc616[i];
        float fc = hface616[i];                 /* which way it looks         */

        /* STATION-KEEPING: two noise rates, no periodicity.  The vertical
         * term is larger because a seahorse rides up and down more than it
         * wanders sideways. */
        float bob  = (gk_noise1(t * 0.013f + hph616[i], 211u + (uint32_t)i) - 0.5f) * 2.0f;
        float bob2 = (gk_noise1(t * 0.042f + hph616[i], 231u + (uint32_t)i) - 0.5f) * 2.0f;
        float swy  = (gk_noise1(t * 0.009f + hph616[i], 251u + (uint32_t)i) - 0.5f) * 2.0f;

        float cx = hx616[i] + swy * 7.0f * S;
        float cy = hy616[i] + bob * 9.0f * S + bob2 * 2.2f * S;

        /* very slow true drift, so the cast does not look pinned */
        hy616[i] -= 0.035f * S;
        hx616[i] += 0.012f * S * fc;
        if (hy616[i] < -60.0f * S) {
            hy616[i]   = ch + 50.0f * S;
            hx616[i]   = gk_hash((uint32_t)t ^ (uint32_t)(i * 1511)) * cw;
            hhue616[i] = gk_hash((uint32_t)t ^ (uint32_t)(i * 613));
        }
        if (hx616[i] < -40.0f * S) hx616[i] += cw + 80.0f * S;
        if (hx616[i] > cw + 40.0f * S) hx616[i] -= cw + 80.0f * S;

        /* SPINE by integrated curvature.  Angle is measured in screen space
         * (y down), and the sign flips with `fc` so a mirrored animal curls
         * the right way.  Step length tapers toward the tail, which is what
         * tightens the spiral without extra maths. */
        float step0 = 6.2f * S;
        float px = cx, py = cy;
        float lean = swy * 0.12f;                /* the whole animal sways    */
        for (k = 0; k < SP616; k++) {
            float u = (float)k / (float)(SP616 - 1);
            /* snout pointing forward-down, neck reversing, tail winding on */
            float ang = 1.05f                                  /* start: down-forward */
                      - 1.45f * gk_smooth(u / 0.26f)           /* head/neck reversal  */
                      + 1.35f * gk_smooth((u - 0.22f) / 0.30f) /* trunk comes back    */
                      + 4.30f * gk_smooth((u - 0.52f) / 0.48f);/* tail spiral         */
            ang = 1.5707963f + (ang - 1.5707963f) * fc + lean;
            sa[k] = ang;
            sx[k] = px; sy[k] = py;
            float st = step0 * (1.0f - 0.55f * u * u);
            px += cosf(ang) * st * fc;
            py += sinf(ang) * st;
        }

        /* BODY: discs along the spine, thick at the trunk, pinched at the
         * neck, thinning to nothing at the tail tip. */
        sk_col(pal, sk_hidx(base616, hhue616[i]), 0.14f, 0.60f, 0.22f, col);
        for (k = 0; k < SP616; k++) {
            float u = (float)k / (float)(SP616 - 1);
            float r = (1.2f + 4.6f * sinf(3.14159265f * (0.10f + 0.72f * u))) * S
                    * (1.0f - 0.72f * u);
            if (u < 0.16f) r *= 0.55f;           /* snout and head are slim   */
            gk_disc(&g616, sx[k], sy[k], r + 0.9f * S, col);
        }

        /* SPINE STROKE: the readable line, brighter at the head end. */
        for (k = 1; k < SP616; k++) {
            float u = (float)k / (float)(SP616 - 1);
            sk_col(pal, sk_hidx(base616, hhue616[i] + 0.04f * u),
                   0.30f - 0.16f * u, 0.50f, 0.95f - 0.35f * u, col);
            gk_seg(&g616, sx[k - 1], sy[k - 1], sx[k], sy[k],
                   col, (1.0f - 0.45f * u) * S, 2.8f * S, 0.18f);
        }

        /* BONY RINGS: short cross-strokes along the trunk.  Seahorses are
         * armoured, and without these the body reads as a plain tube. */
        sk_col(pal, sk_hidx(base616, hhue616[i] + 0.07f), 0.22f, 0.52f, 0.50f, col);
        for (k = 5; k < SP616 - 4; k += 3) {
            float nx2 = -sinf(sa[k]), ny2 = cosf(sa[k]);
            float u = (float)k / (float)(SP616 - 1);
            float r = (3.6f * (1.0f - 0.70f * u)) * S;
            gk_seg(&g616, sx[k] - nx2 * r, sy[k] - ny2 * r,
                          sx[k] + nx2 * r, sy[k] + ny2 * r,
                   col, 0.45f * S, 1.4f * S, 0.09f);
        }

        /* SNOUT: a straight tapered tube off the head, plus the eye. */
        sk_col(pal, sk_hidx(base616, hhue616[i] + 0.02f), 0.28f, 0.50f, 0.85f, col);
        gk_seg(&g616, sx[0], sy[0],
               sx[0] + cosf(sa[0]) * 9.0f * S * fc, sy[0] + sinf(sa[0]) * 9.0f * S,
               col, 0.8f * S, 2.2f * S, 0.15f);
        sk_col(pal, sk_hidx(base616, hhue616[i] + 0.46f), 0.70f, 0.26f, 1.00f, col);
        gk_dot(&g616, sx[2] - sinf(sa[2]) * 2.6f * S * fc,
                      sy[2] + cosf(sa[2]) * 2.6f * S,
               col, 0.85f * S, 2.2f * S, 0.30f);

        /* CORONET: the little crown on the back of the skull. */
        sk_col(pal, sk_hidx(base616, hhue616[i] + 0.11f), 0.26f, 0.50f, 0.65f, col);
        {
            float nx2 = -sinf(sa[3]), ny2 = cosf(sa[3]);
            gk_seg(&g616, sx[3], sy[3],
                   sx[3] - nx2 * 6.5f * S, sy[3] - ny2 * 6.5f * S,
                   col, 0.6f * S, 1.8f * S, 0.12f);
        }

        /* DORSAL FIN: the fast element.  It flutters at roughly seven times
         * the bob rate, with a phase lag down the fin so the beat travels
         * tailward — a fin that flaps in unison looks like a flag. */
        for (k = 8; k < 16; k++) {
            float f  = (float)(k - 8) / 7.0f;
            float env = sinf(3.14159265f * f);           /* fin is lens-shaped */
            float flut = sinf(t * 0.42f + hph616[i] - f * 2.3f);
            float nx2 = -sinf(sa[k]), ny2 = cosf(sa[k]);
            float len = (3.0f + 4.2f * env) * S * (0.75f + 0.35f * flut);
            sk_col(pal, sk_hidx(base616, hhue616[i] + 0.18f),
                   0.18f, 0.45f, 0.40f + 0.35f * fabsf(flut), col);
            gk_seg(&g616, sx[k], sy[k],
                   sx[k] - nx2 * len + cosf(sa[k]) * flut * 1.4f * S,
                   sy[k] - ny2 * len + sinf(sa[k]) * flut * 1.4f * S,
                   col, 0.42f * S, 1.6f * S, 0.10f);
        }
    }

    gk_present(&g616, fb, w, h);
}
