/* 622 Beetle Parade (creature) — a procession of beetles crossing the frame.
 *
 * Two things had to be right.  First the gait: insects do not shuffle, they
 * walk an alternating tripod — front and rear of one side plus the middle leg
 * of the other swing together while the opposite three hold the ground.  That
 * is one phase variable and a table of three-and-three offsets, and it is the
 * whole reason the thing reads as a beetle rather than a bead on a wire.  The
 * feet are placed in body space with a stance/swing split, so during stance a
 * planted foot slides backward while the body drives forward, and the legs
 * push instead of paddling.
 *
 * Second the shell.  A beetle's elytra are hard and lacquered, and what sells
 * that is one small specular highlight that stays put on the carapace while
 * the body rocks under it, plus the seam down the middle.  The body is built
 * from a short row of discs so the oval stays smooth at any canvas size, with
 * the highlight laid over it at high whiteness and a tight radius.
 *
 * Bodies rock on the gait phase and yaw slightly off the line of march, which
 * keeps the parade from looking like it is on rails.  Repaint clean. */
#include "_spark572.h"

#define NB622 7            /* beetles                                       */

static gk g622;
static uint32_t bs622 = 0xFFFFFFFFu;
static int   base622;
static float bx622[NB622], by622[NB622];      /* position                   */
static float bsp622[NB622];                   /* march speed                */
static float bsz622[NB622];                   /* body scale                 */
static float bph622[NB622];                   /* gait phase                 */
static float bhu622[NB622];
static int   bdir622[NB622];                  /* +1 marching right, -1 left */

/* Leg phase offsets, front/mid/rear x left/right.  Tripod A is front-left,
 * mid-right, rear-left; tripod B is the complement, half a cycle later. */
static const float trip622[6] = { 0.0f, 0.5f,      /* front  L, R */
                                  0.5f, 0.0f,      /* middle L, R */
                                  0.0f, 0.5f };    /* rear   L, R */

/* Foot position in body space for gait phase p.  Stance slides the planted
 * foot backward; swing lifts it and carries it forward again. */
static void gait622(float p, float duty, float stride, float lift,
                    float *ox, float *oy)
{
    p -= floorf(p);
    if (p < duty) {
        float u = p / duty;
        *ox = stride * (0.5f - u);
        *oy = 0.0f;
    } else {
        float u = (p - duty) / (1.0f - duty);
        *ox = stride * (u - 0.5f);
        *oy = -lift * sinf(3.14159265f * u);
    }
}

/* Two-bone leg from hip to foot; `bend` picks which way the knee breaks. */
static void ik622(gk *g, float hx, float hy, float fx, float fy,
                  float l1, float l2, float bend, const float *c,
                  float cr, float gr)
{
    float dx = fx - hx, dy = fy - hy;
    float d  = sqrtf(dx * dx + dy * dy);
    float L  = l1 + l2;
    if (d < 1e-4f) { dx = 0.0f; dy = 1.0f; d = 1e-4f; }
    if (d > L * 0.995f) {                     /* out of reach: straighten it */
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
    gk_seg(g, hx, hy, kx, ky, c, cr, gr, 0.14f);
    gk_seg(g, kx, ky, fx, fy, c, cr * 0.85f, gr * 0.90f, 0.11f);
}

void pattern_622(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    gk_setup(&g622, w, h);
    gk_clear(&g622);

    float cw = (float)g622.cw, ch = (float)g622.ch, sc = g622.sc;
    float t  = (float)frame;
    float col[3];

    if (seed != bs622) {
        base622 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NB622; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 4421);
            bsz622[i]  = (0.75f + 0.75f * gk_hash(r + 1u)) * sc;
            bdir622[i] = gk_hash(r + 2u) > 0.25f ? 1 : -1;   /* mostly one way */
            bx622[i]   = gk_hash(r + 3u) * cw;
            by622[i]   = ch * (0.18f + 0.66f * gk_hash(r + 4u));
            bsp622[i]  = (0.55f + 0.55f * gk_hash(r + 5u)) * sc;
            bph622[i]  = gk_hash(r + 6u);
            bhu622[i]  = gk_hash(r + 7u);
        }
        bs622 = seed;
    }

    for (i = 0; i < NB622; i++) {
        float S = bsz622[i];
        float stride = 11.0f * S;

        /* Cycle rate tied to speed over stride, so feet do not skate: one
         * full gait cycle covers two stride lengths of ground. */
        float rate = bsp622[i] / (stride * 1.6f);
        bph622[i] += rate;

        /* lane meander — a beetle holds a heading badly */
        float yaw = (gk_noise1(t * 0.005f + (float)i * 5.7f, 404u) - 0.5f) * 0.45f;
        bx622[i] += (float)bdir622[i] * bsp622[i] * cosf(yaw);
        by622[i] += bsp622[i] * sinf(yaw) * 0.6f;

        if (by622[i] < ch * 0.10f) by622[i] = ch * 0.10f;
        if (by622[i] > ch * 0.90f) by622[i] = ch * 0.90f;
        float m = 46.0f * S;
        if (bx622[i] > cw + m) { bx622[i] = -m; by622[i] = ch * (0.15f + 0.70f * gk_hash((uint32_t)frame ^ (uint32_t)(i * 97))); }
        if (bx622[i] < -m)     { bx622[i] = cw + m; by622[i] = ch * (0.15f + 0.70f * gk_hash((uint32_t)frame ^ (uint32_t)(i * 131))); }

        float hd = (bdir622[i] > 0) ? yaw : (3.14159265f - yaw);
        float ux = cosf(hd), uy = sinf(hd);          /* along the body */
        float nx = -uy, ny = ux;                     /* across it      */

        /* the body rocks twice per cycle — once per tripod touchdown */
        float rock = sinf(bph622[i] * 2.0f * GK_TAU) * 0.9f * S;
        float x = bx622[i], y = by622[i] + rock;

        /* LEGS first, so the shell sits over the hips. */
        sk_col(pal, sk_hidx(base622, bhu622[i] + 0.52f), 0.08f, 0.40f, 0.50f, col);
        for (k = 0; k < 6; k++) {
            int   row  = k >> 1;                           /* 0 front, 2 rear */
            float side = (k & 1) ? 1.0f : -1.0f;
            float along = (4.5f - (float)row * 4.5f) * S;  /* +4.5 .. -4.5 */
            float hipx = x + ux * along + nx * side * 3.2f * S;
            float hipy = y + uy * along + ny * side * 3.2f * S;

            float ox, oy;
            gait622(bph622[i] + trip622[k], 0.58f, stride, 4.2f * S, &ox, &oy);
            /* the foot sits well out to the side — insect legs sprawl */
            float ftx = x + ux * (along + ox) + nx * side * 11.0f * S;
            float fty = y + uy * (along + ox) + ny * side * 11.0f * S + oy;

            ik622(&g622, hipx, hipy, ftx, fty,
                  7.5f * S, 7.5f * S, side, col, 0.50f * S, 1.7f * S);
        }

        /* ELYTRA: a row of discs makes a clean oval at any scale. */
        for (k = 0; k < 5; k++) {
            float f = (float)k / 4.0f;                   /* 0 rear .. 1 front */
            float along = (-7.0f + 12.0f * f) * S;
            float r = (6.6f - 3.2f * (f - 0.45f) * (f - 0.45f) * 4.0f) * S;
            if (r < 1.0f * S) r = 1.0f * S;
            sk_col(pal, sk_hidx(base622, bhu622[i] + 0.03f * f),
                   0.05f, 0.72f, 0.34f, col);
            gk_disc(&g622, x + ux * along, y + uy * along, r, col);
        }

        /* the seam: a dark-cored line down the midline reads as the elytra
         * split.  Drawn at low amp so it darkens by contrast, not by subtraction. */
        sk_col(pal, sk_hidx(base622, bhu622[i] + 0.40f), 0.02f, 0.85f, 0.16f, col);
        gk_seg(&g622, x - ux * 6.0f * S, y - uy * 6.0f * S,
                      x + ux * 4.0f * S, y + uy * 4.0f * S,
               col, 0.5f * S, 1.2f * S, 0.05f);

        /* HIGHLIGHT: fixed on the carapace, slightly forward and to one side.
         * One tight pale dot is the entire difference between chitin and mud. */
        sk_col(pal, sk_hidx(base622, bhu622[i] + 0.02f), 0.86f, 0.10f, 0.95f, col);
        gk_dot(&g622, x + ux * 1.6f * S + nx * 2.1f * S,
                      y + uy * 1.6f * S + ny * 2.1f * S,
               col, 1.15f * S, 3.4f * S, 0.30f);

        /* PRONOTUM and HEAD */
        sk_col(pal, sk_hidx(base622, bhu622[i] + 0.06f), 0.10f, 0.65f, 0.38f, col);
        gk_disc(&g622, x + ux * 7.2f * S, y + uy * 7.2f * S, 4.2f * S, col);
        gk_disc(&g622, x + ux * 10.8f * S, y + uy * 10.8f * S, 2.7f * S, col);

        /* antennae: swept forward, sweeping slowly as it tests the ground */
        float sw = sinf(t * 0.09f + (float)i) * 0.35f;
        sk_col(pal, sk_hidx(base622, bhu622[i] + 0.50f), 0.06f, 0.45f, 0.40f, col);
        for (k = 0; k < 2; k++) {
            float side = k ? 1.0f : -1.0f;
            float a = hd + side * (0.55f + sw * side);
            gk_seg(&g622, x + ux * 11.5f * S, y + uy * 11.5f * S,
                          x + ux * 11.5f * S + cosf(a) * 8.0f * S,
                          y + uy * 11.5f * S + sinf(a) * 8.0f * S,
                   col, 0.40f * S, 1.3f * S, 0.08f);
        }
    }

    gk_present(&g622, fb, w, h);
}
