/* 618 Turtle Paddle (creature) — green turtles in plan view, rowing.
 *
 * The gait is the only thing worth getting right.  A turtle's forelimbs do
 * the work and the hindlimbs mostly steer, and the four limbs run on a
 * diagonal couple: front-left with rear-right, front-right with rear-left.
 * Put all four in phase and it looks like a wind-up toy.
 *
 * A plain sine on the stroke angle is also wrong, because a paddle stroke is
 * not symmetric — the power phase is quick and the recovery is slow, since on
 * recovery the limb is feathered and has no water to fight.  So the phase is
 * warped (p + k*sin p) before the sine is taken, which snaps the backstroke
 * and loiters on the return.  Forward thrust is read off the same warped
 * phase, so the animal surges on the power stroke and coasts on the recovery
 * rather than travelling at a constant rate while its legs wave about.
 *
 * Flipper blades carry a tip lag so they trail and feather instead of
 * staying rigid, which is what makes them look like skin and bone rather
 * than oars.  The shell is drawn as a disc plus a few scute lines: at this
 * size a proper carapace pattern is noise, but three ring arcs and a spine
 * line read unmistakably as a turtle's back. */
#include "_spark572.h"

#define NT618 2            /* turtles                                        */

static gk g618;
static uint32_t bs618 = 0xFFFFFFFFu;
static int   base618;
static float tx618[NT618], ty618[NT618], thd618[NT618], tsc618[NT618];
static float thue618[NT618], tph618[NT618], trate618[NT618], tspd618[NT618];

/* one flipper: rooted at (rx,ry), swinging about `rest`, stroke phase `p`.
 * `big` scales the blade.  The tip lag is applied as a second, later phase so
 * the blade trails the shoulder. */
static void t618_flipper(gk *g, const uint32_t *pal, int base, float hue,
                         float rx, float ry, float rest, float p,
                         float sweep, float big, float S)
{
    float col[3];
    /* warped phase: quick back, slow return */
    float wp = p + 0.55f * sinf(p);
    float ang = rest + sweep * sinf(wp);
    float tipang = rest + sweep * sinf(wp - 0.75f);     /* the lag            */

    float ex = rx + cosf(ang) * 11.0f * big * S;
    float ey = ry + sinf(ang) * 11.0f * big * S;

    sk_col(pal, sk_hidx(base, hue + 0.06f), 0.24f, 0.50f, 0.80f, col);
    gk_seg(g, rx, ry, ex, ey, col, 1.1f * big * S, 3.0f * big * S, 0.18f);

    /* blade: two struts fanning off the limb toward the trailing tip */
    sk_col(pal, sk_hidx(base, hue + 0.10f), 0.18f, 0.48f, 0.62f, col);
    {
        float bx = ex + cosf(tipang) * 13.0f * big * S;
        float by = ey + sinf(tipang) * 13.0f * big * S;
        float nx = -sinf(tipang), ny = cosf(tipang);
        gk_seg(g, ex, ey, bx + nx * 3.4f * big * S, by + ny * 3.4f * big * S,
               col, 0.85f * big * S, 2.6f * big * S, 0.15f);
        gk_seg(g, ex, ey, bx - nx * 2.2f * big * S, by - ny * 2.2f * big * S,
               col, 0.70f * big * S, 2.2f * big * S, 0.13f);
        gk_seg(g, bx + nx * 3.4f * big * S, by + ny * 3.4f * big * S,
                  bx - nx * 2.2f * big * S, by - ny * 2.2f * big * S,
               col, 0.55f * big * S, 1.8f * big * S, 0.11f);
    }
}

void pattern_618(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    float col[3];

    gk_setup(&g618, w, h);
    gk_decay_snap(&g618, 0.83f);

    float cw = (float)g618.cw, ch = (float)g618.ch, sc = g618.sc;

    if (seed != bs618) {
        base618 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NT618; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 8419);
            tx618[i]    = gk_hash(r + 1u) * cw;
            ty618[i]    = gk_hash(r + 2u) * ch;
            thd618[i]   = gk_hash(r + 3u) * 6.2831853f;
            tsc618[i]   = (0.80f + 0.55f * gk_hash(r + 4u)) * sc;
            thue618[i]  = gk_hash(r + 5u);
            tph618[i]   = gk_hash(r + 6u) * 6.2831853f;
            trate618[i] = 0.052f + 0.022f * gk_hash(r + 7u);
            tspd618[i]  = 0.0f;
        }
        bs618 = seed;
    }

    float t = (float)frame;

    for (i = 0; i < NT618; i++) {
        float S = tsc618[i];
        float p = tph618[i] + t * trate618[i];
        float wp = p + 0.55f * sinf(p);

        /* Thrust from the power half of the warped stroke only.  cos(wp) is
         * positive while the limb is sweeping back; the other half is
         * recovery and contributes nothing. */
        float pw = cosf(wp);
        float thrust = pw > 0.0f ? pw * pw : 0.0f;

        thd618[i] += (gk_noise1(t * 0.003f + (float)i * 23.0f, 181u + (uint32_t)i) - 0.5f) * 0.035f;

        tspd618[i] = tspd618[i] * 0.90f + thrust * 0.26f * S;
        float vx = cosf(thd618[i]) * (0.30f * S + tspd618[i]);
        float vy = sinf(thd618[i]) * (0.30f * S + tspd618[i]);
        tx618[i] += vx;
        ty618[i] += vy;

        float mg = 60.0f * S;
        if (tx618[i] < -mg) tx618[i] += cw + 2.0f * mg;
        if (tx618[i] > cw + mg) tx618[i] -= cw + 2.0f * mg;
        if (ty618[i] < -mg) ty618[i] += ch + 2.0f * mg;
        if (ty618[i] > ch + mg) ty618[i] -= ch + 2.0f * mg;

        float hx = cosf(thd618[i]), hy = sinf(thd618[i]);
        float nx = -hy, ny = hx;
        float cx = tx618[i], cy = ty618[i];
        float Rsh = 16.0f * S;                   /* carapace radius           */

        /* HEAD: extends a little on the power stroke, because a swimming
         * turtle cranes forward as it pulls. */
        {
            float reach = (1.0f + 0.22f * thrust);
            float hxx = cx + hx * Rsh * 1.18f * reach;
            float hyy = cy + hy * Rsh * 1.18f * reach;
            sk_col(pal, sk_hidx(base618, thue618[i] + 0.05f), 0.22f, 0.52f, 0.60f, col);
            gk_seg(&g618, cx + hx * Rsh * 0.80f, cy + hy * Rsh * 0.80f, hxx, hyy,
                   col, 2.2f * S, 5.0f * S, 0.18f);
            gk_disc(&g618, hxx, hyy, 4.2f * S, col);
            sk_col(pal, sk_hidx(base618, thue618[i] + 0.44f), 0.68f, 0.26f, 1.00f, col);
            gk_dot(&g618, hxx + nx * 2.4f * S, hyy + ny * 2.4f * S,
                   col, 0.7f * S, 2.0f * S, 0.28f);
            gk_dot(&g618, hxx - nx * 2.4f * S, hyy - ny * 2.4f * S,
                   col, 0.7f * S, 2.0f * S, 0.28f);
        }

        /* FLIPPERS, diagonal couple: front-left pairs with rear-right.
         * Fronts get the big blades and the wide sweep. */
        t618_flipper(&g618, pal, base618, thue618[i],
                     cx + hx * Rsh * 0.62f + nx * Rsh * 0.70f,
                     cy + hy * Rsh * 0.62f + ny * Rsh * 0.70f,
                     atan2f(hy, hx) + 1.05f, p,          1.10f, 1.00f, S);
        t618_flipper(&g618, pal, base618, thue618[i],
                     cx + hx * Rsh * 0.62f - nx * Rsh * 0.70f,
                     cy + hy * Rsh * 0.62f - ny * Rsh * 0.70f,
                     atan2f(hy, hx) - 1.05f, p + 3.14159265f, -1.10f, 1.00f, S);
        t618_flipper(&g618, pal, base618, thue618[i],
                     cx - hx * Rsh * 0.72f + nx * Rsh * 0.58f,
                     cy - hy * Rsh * 0.72f + ny * Rsh * 0.58f,
                     atan2f(hy, hx) + 2.20f, p + 3.14159265f, 0.55f, 0.60f, S);
        t618_flipper(&g618, pal, base618, thue618[i],
                     cx - hx * Rsh * 0.72f - nx * Rsh * 0.58f,
                     cy - hy * Rsh * 0.72f - ny * Rsh * 0.58f,
                     atan2f(hy, hx) - 2.20f, p,          -0.55f, 0.60f, S);

        /* CARAPACE: drawn after the limbs so the shell sits over the
         * shoulders, which is how the real overlap works. */
        sk_col(pal, sk_hidx(base618, thue618[i]), 0.10f, 0.65f, 0.34f, col);
        gk_disc(&g618, cx, cy, Rsh * 1.02f, col);
        gk_disc(&g618, cx - hx * Rsh * 0.30f, cy - hy * Rsh * 0.30f, Rsh * 0.80f, col);

        /* rim */
        sk_col(pal, sk_hidx(base618, thue618[i] + 0.03f), 0.32f, 0.50f, 0.85f, col);
        {
            float px = cx + Rsh * 1.04f, py = cy;
            for (k = 1; k <= 18; k++) {
                float a = 6.2831853f * (float)k / 18.0f;
                /* slightly longer than wide, aligned to travel */
                float lx = cosf(a) * Rsh * 1.04f, ly = sinf(a) * Rsh * 0.88f;
                float qx = cx + hx * lx - hy * ly;
                float qy = cy + hy * lx + hx * ly;
                if (k == 1) { px = cx + hx * Rsh * 1.04f; py = cy + hy * Rsh * 1.04f; }
                gk_seg(&g618, px, py, qx, qy, col, 0.75f * S, 2.4f * S, 0.16f);
                px = qx; py = qy;
            }
        }

        /* SCUTES: a spine line and two ring arcs.  Three strokes is all the
         * detail that survives at this size. */
        sk_col(pal, sk_hidx(base618, thue618[i] + 0.08f), 0.20f, 0.52f, 0.46f, col);
        gk_seg(&g618, cx + hx * Rsh * 0.85f, cy + hy * Rsh * 0.85f,
                      cx - hx * Rsh * 0.85f, cy - hy * Rsh * 0.85f,
               col, 0.50f * S, 1.6f * S, 0.10f);
        for (k = -1; k <= 1; k += 2) {
            float fk = (float)k;
            gk_seg(&g618, cx + hx * Rsh * 0.45f + nx * fk * Rsh * 0.25f,
                          cy + hy * Rsh * 0.45f + ny * fk * Rsh * 0.25f,
                          cx - hx * Rsh * 0.45f + nx * fk * Rsh * 0.42f,
                          cy - hy * Rsh * 0.45f + ny * fk * Rsh * 0.42f,
                   col, 0.45f * S, 1.5f * S, 0.09f);
        }

        /* TAIL: barely there, but it closes the silhouette. */
        sk_col(pal, sk_hidx(base618, thue618[i] + 0.12f), 0.16f, 0.48f, 0.40f, col);
        gk_seg(&g618, cx - hx * Rsh * 0.95f, cy - hy * Rsh * 0.95f,
                      cx - hx * Rsh * 1.35f + nx * sinf(p) * 2.0f * S,
                      cy - hy * Rsh * 1.35f + ny * sinf(p) * 2.0f * S,
               col, 0.6f * S, 1.8f * S, 0.10f);
    }

    gk_present(&g618, fb, w, h);
}
