/* 617 Krill Cloud (creature) — a swarm dense enough that the swarm, not the
 * animal, is the subject.
 *
 * The trick with a cloud of a couple of hundred things is that per-individual
 * randomness averages out to mush.  So the motion comes from one shared
 * velocity field — two-dimensional value noise, sampled at each krill's own
 * position and advanced slowly in time — which means neighbours get nearly
 * the same push and the whole cloud develops sheets and curls.  Private
 * jitter is layered on top, small, and exists only to stop the field looking
 * like a texture scroll.
 *
 * On top of that each krill carries its own flick: krill do not swim, they
 * dart, and the dart is a short impulse on a per-animal cadence.  Flicks fire
 * independently, so the cloud crackles while drifting coherently.
 *
 * Containment is the other half of the design.  Letting the cloud disperse
 * is physically right and visually dead, so there is a soft centre that
 * drifts across the frame and anything that gets too far from it is
 * re-injected on the near side rather than teleported to the far edge — a
 * wrap would tear the cloud in two. */
#include "_spark572.h"

#define NK617 230          /* krill                                          */

static gk g617;
static uint32_t bs617 = 0xFFFFFFFFu;
static int   base617;
static float kx617[NK617], ky617[NK617], kvx617[NK617], kvy617[NK617];
static float khue617[NK617], ksz617[NK617], kflick617[NK617], kcad617[NK617];
static float ccx617, ccy617, chue617;

void pattern_617(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i;
    float col[3];

    gk_setup(&g617, w, h);
    gk_decay_snap(&g617, 0.76f);

    float cw = (float)g617.cw, ch = (float)g617.ch, sc = g617.sc;
    float R = 0.34f * (cw < ch ? cw : ch);       /* nominal cloud radius      */

    if (seed != bs617) {
        base617 = (int)(seed & 0x7FFFu);
        ccx617  = cw * 0.5f;
        ccy617  = ch * 0.5f;
        chue617 = gk_hash(seed ^ 0x301u);
        for (i = 0; i < NK617; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 2693);
            float a = gk_hash(r + 1u) * 6.2831853f;
            float d = sqrtf(gk_hash(r + 2u)) * R;      /* sqrt = even density */
            kx617[i]     = ccx617 + cosf(a) * d;
            ky617[i]     = ccy617 + sinf(a) * d;
            kvx617[i]    = 0.0f;
            kvy617[i]    = 0.0f;
            khue617[i]   = chue617 + (gk_hash(r + 3u) - 0.5f) * 0.06f;
            ksz617[i]    = (0.55f + 0.50f * gk_hash(r + 4u)) * sc;
            kcad617[i]   = 14.0f + 30.0f * gk_hash(r + 5u);
            kflick617[i] = gk_hash(r + 6u) * kcad617[i];
        }
        bs617 = seed;
    }

    float t = (float)frame;

    /* The cloud's centre takes its own slow walk, bounded well inside the
     * frame so the swarm never fully leaves. */
    ccx617 = cw * (0.28f + 0.44f * gk_noise1(t * 0.0022f, 601u));
    ccy617 = ch * (0.28f + 0.44f * gk_noise1(t * 0.0019f, 602u));

    /* Field scale: one noise cell is about a fifth of the cloud, which is
     * what gives sheets rather than either a single sweep or static. */
    float fscale = 3.2f / (R > 1.0f ? R : 1.0f);
    float ft = t * 0.006f;

    for (i = 0; i < NK617; i++) {
        /* SHARED FIELD: two noise reads, offset streams, mapped to -1..1 */
        float nx = gk_noise2(kx617[i] * fscale + ft, ky617[i] * fscale, 711u) - 0.5f;
        float ny = gk_noise2(kx617[i] * fscale, ky617[i] * fscale + ft, 712u) - 0.5f;
        kvx617[i] += nx * 0.30f * sc;
        kvy617[i] += ny * 0.30f * sc;

        /* private jitter, small on purpose */
        uint32_t jr = (uint32_t)frame * 2654435761u + (uint32_t)(i * 97);
        kvx617[i] += (gk_hash(jr + 1u) - 0.5f) * 0.10f * sc;
        kvy617[i] += (gk_hash(jr + 2u) - 0.5f) * 0.10f * sc;

        /* THE FLICK: an impulse on the animal's own cadence, aimed along its
         * current heading so a dart continues the line it was already on. */
        float flick = 0.0f;
        if (fmodf(t + kflick617[i], kcad617[i]) < 1.0f) {
            float sp = sqrtf(kvx617[i] * kvx617[i] + kvy617[i] * kvy617[i]);
            if (sp > 1e-4f) {
                kvx617[i] += (kvx617[i] / sp) * 1.5f * sc;
                kvy617[i] += (kvy617[i] / sp) * 1.5f * sc;
            }
        }
        /* how recently it flicked, for the brightness bump */
        {
            float ph = fmodf(t + kflick617[i], kcad617[i]);
            flick = (ph < 7.0f) ? (1.0f - ph / 7.0f) : 0.0f;
        }

        /* gentle pull home — this is the containment, not a hard boundary */
        float dx = ccx617 - kx617[i], dy = ccy617 - ky617[i];
        float d2 = dx * dx + dy * dy;
        float d  = sqrtf(d2);
        if (d > R * 0.55f) {
            float k = (d - R * 0.55f) / R * 0.045f;
            kvx617[i] += dx / (d + 1e-3f) * k * sc * 10.0f;
            kvy617[i] += dy / (d + 1e-3f) * k * sc * 10.0f;
        }

        kvx617[i] *= 0.90f;                      /* water is thick at this size */
        kvy617[i] *= 0.90f;
        kx617[i]  += kvx617[i];
        ky617[i]  += kvy617[i];

        /* Re-inject strays on the near side of the cloud.  Wrapping to the
         * opposite edge would split the swarm, which is exactly the thing
         * this pattern is about not doing. */
        if (d > R * 1.9f) {
            uint32_t r = (uint32_t)frame * 40503u + (uint32_t)(i * 131);
            float a = gk_hash(r + 1u) * 6.2831853f;
            float rr = sqrtf(gk_hash(r + 2u)) * R * 0.7f;
            kx617[i] = ccx617 + cosf(a) * rr;
            ky617[i] = ccy617 + sinf(a) * rr;
            kvx617[i] = 0.0f; kvy617[i] = 0.0f;
        }

        /* BODY: a dash along the heading, two or three pixels at base scale.
         * Anything more detailed is invisible and just costs fill rate. */
        float sp = sqrtf(kvx617[i] * kvx617[i] + kvy617[i] * kvy617[i]);
        float ux, uy;
        if (sp > 1e-4f) { ux = kvx617[i] / sp; uy = kvy617[i] / sp; }
        else            { ux = 1.0f; uy = 0.0f; }

        float S   = ksz617[i];
        float len = (2.4f + 2.6f * flick) * S;

        sk_col(pal, sk_hidx(base617, khue617[i] + 0.03f * flick),
               0.16f + 0.52f * flick, 0.45f,
               0.42f + 0.70f * flick, col);
        gk_seg(&g617, kx617[i] - ux * len, ky617[i] - uy * len,
                      kx617[i] + ux * len * 0.55f, ky617[i] + uy * len * 0.55f,
               col, 0.52f * S, 1.9f * S, 0.16f);

        /* the one bright speck: krill eyes are the only thing that catches a
         * light at this range, and they are what makes the cloud sparkle */
        sk_col(pal, sk_hidx(base617, khue617[i] + 0.40f),
               0.62f, 0.28f, 0.40f + 0.55f * flick, col);
        gk_dot(&g617, kx617[i] + ux * len * 0.7f, ky617[i] + uy * len * 0.7f,
               col, 0.40f * S, 1.3f * S, 0.20f);
    }

    gk_present(&g617, fb, w, h);
}
