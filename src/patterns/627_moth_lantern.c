/* 627 Moth Lantern (creature) — moths working a light that is never drawn.
 *
 * The light is a point that drifts slowly and emits nothing.  It exists only
 * in two places: the moths spiral in toward it, and their brightness rises as
 * they close, as if lit from the side.  Drawing the lamp would kill it — the
 * eye infers a source from the geometry and the falloff, and inferring is what
 * makes the frame feel like a porch at night rather than a particle demo.
 *
 * Flight is a decaying spiral by construction: radius multiplied down each
 * frame, angular rate scaled by 1/r so the moth whips around as it closes,
 * exactly the way angular momentum makes a real approach tighten.  At a small
 * inner radius the moth breaks off and is thrown back out to its starting
 * radius on a new bearing, which is the repeated bounce-and-return of a moth
 * that keeps overshooting the thing it is orbiting.  Decay rates differ per
 * moth so the runs never fall into step.
 *
 * Wings beat far faster than the body moves, drawn as a pair of soft fans whose
 * span collapses on the stroke — a moth is mostly a flicker.  The canvas keeps
 * a decay rather than clearing, so each approach leaves its own fading spiral
 * on the air; that trace is half the picture. */
#include "_spark572.h"

#define NM627 16           /* moths                                         */

static gk g627;
static uint32_t bs627 = 0xFFFFFFFFu;
static int   base627;
static float mr627[NM627], mth627[NM627];     /* polar position about the light */
static float mdk627[NM627];                   /* radius decay per frame      */
static float mr0627[NM627];                   /* radius to be thrown back to */
static float msp627[NM627];                   /* angular scale               */
static float msz627[NM627], mhu627[NM627];
static float mwp627[NM627], mwr627[NM627];

void pattern_627(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    gk_setup(&g627, w, h);
    gk_decay_snap(&g627, 0.74f);               /* the spiral traces live here */

    float cw = (float)g627.cw, ch = (float)g627.ch, sc = g627.sc;
    float t  = (float)frame;
    float col[3];
    float reach = (cw < ch ? cw : ch) * 0.5f;

    if (seed != bs627) {
        base627 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NM627; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 3571);
            mr0627[i] = reach * (0.55f + 0.60f * gk_hash(r + 1u));
            mr627[i]  = reach * (0.10f + 0.90f * gk_hash(r + 2u));
            mth627[i] = gk_hash(r + 3u) * GK_TAU;
            mdk627[i] = 0.9900f + 0.0075f * gk_hash(r + 4u);  /* 0.990..0.998 */
            msp627[i] = (gk_hash(r + 5u) > 0.5f ? 1.0f : -1.0f)
                      * (2.2f + 1.8f * gk_hash(r + 6u)) * sc;
            msz627[i] = (0.70f + 0.60f * gk_hash(r + 7u)) * sc;
            mhu627[i] = gk_hash(r + 8u) * 0.14f;
            mwp627[i] = gk_hash(r + 9u) * GK_TAU;
            mwr627[i] = 0.55f + 0.35f * gk_hash(r + 10u);
        }
        bs627 = seed;
    }

    /* The lamp: off-centre and slowly wandering, so the spirals do not sit on
     * the frame's axis of symmetry.  Never drawn. */
    float lx = cw * (0.34f + 0.32f * gk_noise1(t * 0.0018f, 141u));
    float ly = ch * (0.30f + 0.34f * gk_noise1(t * 0.0018f, 242u));
    float rmin = reach * 0.10f;

    for (i = 0; i < NM627; i++) {
        float S = msz627[i];

        /* Angular rate goes as 1/r: the approach tightens into a whip. */
        float rr = mr627[i] > rmin * 0.5f ? mr627[i] : rmin * 0.5f;
        mth627[i] += msp627[i] / rr;
        mr627[i]  *= mdk627[i];
        /* a little radial noise keeps the spiral from being a perfect curve */
        mr627[i]  += (gk_noise1(t * 0.05f + (float)i * 6.1f, 333u) - 0.5f) * 1.1f * sc;

        if (mr627[i] < rmin) {
            /* break off: flung back out on a fresh bearing, a new decay rate */
            uint32_t r = (uint32_t)frame * 2654435761u ^ (uint32_t)(i * 4973);
            mr627[i]  = mr0627[i] * (0.85f + 0.30f * gk_hash(r + 1u));
            mth627[i] += (gk_hash(r + 2u) - 0.5f) * 2.0f;
            mdk627[i] = 0.9900f + 0.0075f * gk_hash(r + 3u);
        }

        float x = lx + cosf(mth627[i]) * mr627[i];
        float y = ly + sinf(mth627[i]) * mr627[i] * 0.80f;   /* a flattened orbit */

        /* tangent: differentiate the spiral one small step back */
        float th2 = mth627[i] - msp627[i] / rr * 0.6f;
        float r2  = mr627[i] / mdk627[i];
        float x2 = lx + cosf(th2) * r2, y2 = ly + sinf(th2) * r2 * 0.80f;
        float dx = x - x2, dy = y - y2;
        float d = sqrtf(dx * dx + dy * dy) + 1e-5f;
        float ux = dx / d, uy = dy / d;
        float nx = -uy, ny = ux;

        /* lit by the lamp: brightness falls off with distance, which is the
         * only evidence on screen that there is a light at all */
        float prox = 1.0f - mr627[i] / (reach * 1.4f);
        if (prox < 0.0f) prox = 0.0f; else if (prox > 1.0f) prox = 1.0f;
        float lit = 0.26f + 0.95f * prox * prox;

        float ph = mwp627[i] + t * mwr627[i];
        float open = 0.22f + 0.78f * fabsf(sinf(ph));

        /* WINGS: two fans, span collapsing on the stroke.  Fast enough that
         * most frames catch them part way, which is the flicker. */
        sk_col(pal, sk_hidx(base627, mhu627[i] + 0.04f), 0.18f + 0.30f * prox,
               0.45f, lit * 0.85f, col);
        for (k = 0; k < 2; k++) {
            float side = k ? 1.0f : -1.0f;
            float px = x, py = y;
            int s;
            for (s = 1; s <= 3; s++) {
                float f = (float)s / 3.0f;
                float along = (2.5f - 7.0f * f) * S;
                float across = side * (9.5f * sinf(3.14159265f * f * 0.8f)) * open * S;
                float nx2 = x + ux * along + nx * across;
                float ny2 = y + uy * along + ny * across;
                gk_seg(&g627, px, py, nx2, ny2, col, 0.7f * S, 2.8f * S, 0.20f);
                gk_seg(&g627, x, y, nx2, ny2, col, 0.5f * S, 2.0f * S, 0.10f);
                px = nx2; py = ny2;
            }
        }

        /* BODY: short, blunt, furry — two discs and a head dot */
        sk_col(pal, sk_hidx(base627, mhu627[i]), 0.12f + 0.36f * prox, 0.55f, lit, col);
        gk_disc(&g627, x - ux * 2.0f * S, y - uy * 2.0f * S, 2.6f * S, col);
        gk_disc(&g627, x + ux * 1.6f * S, y + uy * 1.6f * S, 2.0f * S, col);
        gk_dot(&g627, x + ux * 3.6f * S, y + uy * 3.6f * S, col, 0.9f * S, 2.4f * S, 0.22f);
    }

    gk_present(&g627, fb, w, h);
}
