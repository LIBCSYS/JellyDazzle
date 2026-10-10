/* 625 Dog Bound (creature) — a dog at full gallop, side on, in silhouette.
 *
 * A gallop is not a fast walk.  It is a four-beat asymmetric gait with a
 * flight phase: the two hind feet land close together, then the two fore feet,
 * then nothing is on the ground at all.  Here that is four phase offsets
 * bunched into the first half of the cycle (LH .00, RH .12, LF .45, RF .57)
 * and a stance duty of under a third, which leaves the suspension gap for
 * free.  Nothing else about the drawing matters as much as those six numbers.
 *
 * The second half of the illusion is the spine.  A galloping dog gathers and
 * extends — hip and shoulder pull together at the moment the hind feet reach
 * under the body, then fling apart during flight.  So the body length is
 * modulated on the same cycle as the bound, and the whole mass rises once per
 * stride rather than twice.  A rigid trunk at these speeds reads as a toy.
 *
 * Ears and tail are loose on purpose: both are lag chains that ease toward a
 * target a fraction per frame, with the ear target pushed by the head's own
 * vertical velocity, so they flap a beat behind the body and never settle into
 * the pose.  Near legs bright, far legs dim and offset.  Repaint clean. */
#include "_spark572.h"

#define ND625  3           /* dogs — three, staggered, so the frame is never empty */
#define NT625  7           /* tail joints                                   */
#define NE625  3           /* ear joints                                    */

static gk g625;
static uint32_t bs625 = 0xFFFFFFFFu;
static int   base625;
static float dx625[ND625], dgy625[ND625];
static float dsz625[ND625], dsp625[ND625];
static float dph625[ND625], dhu625[ND625];
static int   ddir625[ND625];
static float tang625[ND625][NT625];
static float eang625[ND625][2][NE625];        /* two ears, each a chain      */
static float hprev625[ND625];                 /* last head y, for ear inertia */

/* Transverse gallop: hinds together, then fores, then air. */
static const float gph625[4] = { 0.00f, 0.12f, 0.45f, 0.57f };

static void gait625(float p, float duty, float stride, float lift,
                    float *ox, float *oy)
{
    p -= floorf(p);
    if (p < duty) {                            /* brief, hard stance          */
        float u = p / duty;
        *ox = stride * (0.5f - u);
        *oy = 0.0f;
    } else {                                   /* long reach through the air   */
        float u = (p - duty) / (1.0f - duty);
        *ox = stride * (u - 0.5f);
        *oy = -lift * sinf(3.14159265f * u);
    }
}

static void ik625(gk *g, float hx, float hy, float fx, float fy,
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

void pattern_625(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k, e;
    gk_setup(&g625, w, h);
    gk_clear(&g625);

    float cw = (float)g625.cw, ch = (float)g625.ch, sc = g625.sc;
    float t  = (float)frame;
    float col[3];

    if (seed != bs625) {
        base625 = (int)(seed & 0x7FFFu);
        for (i = 0; i < ND625; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 7013);
            dsz625[i]  = (0.90f + 0.50f * gk_hash(r + 1u)) * sc;
            ddir625[i] = gk_hash(r + 2u) > 0.5f ? 1 : -1;
            /* staggered starts — a gallop crosses fast, and two dogs that
             * happen to spawn together leave the frame empty together */
            dx625[i]   = cw * (((float)i + 0.75f * gk_hash(r + 3u)) / (float)ND625);
            dgy625[i]  = ch * (0.50f + 0.36f * ((float)i + gk_hash(r + 4u)) / (float)ND625);
            dsp625[i]  = (2.1f + 1.1f * gk_hash(r + 5u)) * sc;
            dph625[i]  = gk_hash(r + 6u);
            dhu625[i]  = gk_hash(r + 7u);
            for (k = 0; k < NT625; k++) tang625[i][k] = 0.0f;
            for (e = 0; e < 2; e++) for (k = 0; k < NE625; k++) eang625[i][e][k] = 1.2f;
            hprev625[i] = dgy625[i];
        }
        bs625 = seed;
    }

    for (i = 0; i < ND625; i++) {
        float S   = dsz625[i];
        float dir = (float)ddir625[i];
        float stride = 46.0f * S;               /* long — this is a bound      */
        float duty   = 0.28f;                   /* short stance leaves the air  */

        /* Ground speed breathes a little on slow noise — a running dog is
         * not a metronome.  Phase and translation share the same value, so
         * however the speed wanders the stance feet stay stuck to the floor. */
        float spd = dsp625[i] * (0.90f + 0.20f * gk_noise1(t * 0.006f + (float)i * 3.0f, 616u));
        dph625[i] += spd / (stride * 1.05f);
        dx625[i]  += dir * spd;

        float m = 70.0f * S;        /* tight wrap: less time with no dog up */
        if (dx625[i] > cw + m) dx625[i] = -m;
        if (dx625[i] < -m)     dx625[i] = cw + m;

        float ph = dph625[i];
        float cyc = ph - floorf(ph);
        float gy  = dgy625[i];

        /* One rise per stride, peaking in the suspension window, not twice
         * like a trot.  sin over the back half of the cycle does that. */
        float air = sinf(3.14159265f * gk_smooth((cyc - 0.55f) / 0.45f));
        if (cyc < 0.55f) air = 0.0f;
        float bob = -air * 13.0f * S;

        /* Gather and extend: the trunk shortens as the hinds come under the
         * body and lengthens through the flight phase. */
        float ext  = 0.86f + 0.26f * air - 0.10f * sinf(cyc * GK_TAU);
        float half = 26.0f * S * ext;

        float back = 34.0f * S;
        float hipx = dx625[i] - dir * half;
        float hipy = gy - back + bob + sinf(cyc * GK_TAU) * 2.0f * S;
        float shx  = dx625[i] + dir * half;
        float shy  = gy - back + bob - 2.0f * S;

        /* ---- FAR pair: dim, offset away from the viewer ---- */
        float fx0 = -dir * 3.0f * S, fy0 = -2.0f * S;
        sk_col(pal, sk_hidx(base625, dhu625[i] + 0.04f), 0.04f, 0.78f, 0.34f, col);
        for (k = 0; k < 2; k++) {               /* far hind, far fore */
            float bx = (k == 0) ? hipx : shx;
            float by = (k == 0) ? hipy : shy;
            float ox, oy;
            gait625(ph + gph625[k * 2], duty, stride, 16.0f * S, &ox, &oy);
            ik625(&g625, bx + fx0, by + fy0,
                  bx + fx0 + dir * ox, gy + fy0 + oy,
                  16.0f * S, 16.0f * S, (k == 0) ? -dir : dir,
                  col, 1.7f * S, 3.8f * S);
        }

        /* ---- TRUNK: discs hip to shoulder, deeper at the chest ---- */
        sk_col(pal, sk_hidx(base625, dhu625[i]), 0.05f, 0.80f, 0.44f, col);
        for (k = 0; k <= 8; k++) {
            float f = (float)k / 8.0f;
            float bx = hipx + (shx - hipx) * f;
            float by = hipy + (shy - hipy) * f - sinf(f * 3.14159265f) * (1.0f - air) * 2.4f * S;
            float r  = (10.0f - 3.0f * (f - 0.62f) * (f - 0.62f) * 4.0f) * S;
            gk_disc(&g625, bx, by, r, col);
        }
        sk_col(pal, sk_hidx(base625, dhu625[i] + 0.02f), 0.32f, 0.42f, 0.44f, col);
        gk_seg(&g625, hipx, hipy - 8.4f * S, shx, shy - 8.8f * S,
               col, 1.0f * S, 3.0f * S, 0.14f);

        /* ---- HEAD and NECK: the head reaches forward on the extension ---- */
        float hdx = shx + dir * (20.0f + 5.0f * air) * S;
        float hdy = shy - (6.0f - 5.0f * air) * S;
        sk_col(pal, sk_hidx(base625, dhu625[i]), 0.05f, 0.80f, 0.44f, col);
        gk_seg(&g625, shx, shy - 2.0f * S, hdx, hdy, col, 5.2f * S, 8.0f * S, 0.12f);
        gk_disc(&g625, hdx, hdy, 7.4f * S, col);
        gk_disc(&g625, hdx + dir * 7.0f * S, hdy + 2.2f * S, 4.2f * S, col);  /* muzzle */

        /* ears: the target leans on the head's vertical velocity, so a drop
         * throws them up and a lift lets them fall.  Then three joints of lag. */
        float hvel = hdy - hprev625[i];
        hprev625[i] = hdy;
        sk_col(pal, sk_hidx(base625, dhu625[i] + 0.03f), 0.08f, 0.72f, 0.40f, col);
        for (e = 0; e < 2; e++) {
            float lean = (e ? 0.22f : -0.22f);
            float tgt  = 1.15f - hvel * 0.09f + lean;    /* radians, mostly down */
            float px = hdx - dir * 2.0f * S, py = hdy - 4.0f * S;
            float prev = tgt;
            for (k = 0; k < NE625; k++) {
                eang625[i][e][k] += (prev - eang625[i][e][k]) * (0.34f - 0.06f * (float)k);
                float a = eang625[i][e][k];
                float seg = (6.0f - 1.0f * (float)k) * S;
                float nx = px - dir * cosf(a) * seg * 0.45f;
                float ny = py + sinf(a) * seg;
                float r  = (2.6f - 0.5f * (float)k) * S * (e ? 1.0f : 0.8f);
                gk_seg(&g625, px, py, nx, ny, col, r, r * 2.2f, 0.12f);
                px = nx; py = ny;
                prev = eang625[i][e][k];
            }
        }

        /* ---- TAIL: streams out behind, lagging the hip ---- */
        float tgt2 = -0.25f + 0.40f * air + 0.25f * sinf(cyc * GK_TAU);
        sk_col(pal, sk_hidx(base625, dhu625[i] + 0.03f), 0.08f, 0.70f, 0.40f, col);
        float px2 = hipx - dir * 8.0f * S, py2 = hipy - 4.0f * S, prev2 = tgt2;
        for (k = 0; k < NT625; k++) {
            tang625[i][k] += (prev2 - tang625[i][k]) * (0.26f - 0.02f * (float)k);
            float a = tang625[i][k];
            float seg = (9.0f - 0.5f * (float)k) * S;
            float nx = px2 - dir * cosf(a) * seg;
            float ny = py2 - sinf(a) * seg;
            float r  = (3.0f - 0.26f * (float)k) * S;
            gk_seg(&g625, px2, py2, nx, ny, col, r, r * 2.4f, 0.12f);
            px2 = nx; py2 = ny;
            prev2 = tang625[i][k] + 0.10f;
        }

        /* ---- NEAR pair last, bright ---- */
        sk_col(pal, sk_hidx(base625, dhu625[i] + 0.01f), 0.10f, 0.72f, 0.58f, col);
        for (k = 0; k < 2; k++) {
            float bx = (k == 0) ? hipx : shx;
            float by = (k == 0) ? hipy : shy;
            float ox, oy;
            gait625(ph + gph625[k * 2 + 1], duty, stride, 16.0f * S, &ox, &oy);
            ik625(&g625, bx, by, bx + dir * ox, gy + oy,
                  16.0f * S, 16.0f * S, (k == 0) ? -dir : dir,
                  col, 1.9f * S, 4.2f * S);
        }
    }

    gk_present(&g625, fb, w, h);
}
