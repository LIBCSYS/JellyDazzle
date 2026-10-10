/* 620 Starling Murmuration (creature) — a few hundred birds held together by
 * the three classic flocking rules and nothing else.  Separation, alignment
 * and cohesion are cheap; what sells a murmuration is the second layer: a
 * slowly wandering roost that the flock orbits, with a tangential term so the
 * whole mass wheels instead of collapsing into a ball.  Without the swirl the
 * boids settle into a lump within a few seconds and the illusion dies.
 *
 * The other half of the look is density.  Real murmurations are read by the
 * eye as shifting bands of dark and light — a bird is invisible on its own and
 * the flock is only legible where bodies stack up.  So every bird counts its
 * near neighbours and spends that count on brightness and whiteness.  Thin
 * edges go dim and coloured, the packed core goes bright and pale, and the
 * banding appears for free as the flock folds through itself.
 *
 * Each bird is a chevron: two short strokes swept back from the nose, their
 * dihedral driven by its own flap phase, so the silhouette flickers between
 * a wide V and a near-dash.  Repaint clean — a murmuration with trails turns
 * into smoke. */
#include "_spark572.h"

#define NB620 170          /* birds; the neighbour pass is O(n^2)            */

static gk g620;
static uint32_t bs620 = 0xFFFFFFFFu;
static int   base620;
static float bx620[NB620], by620[NB620];       /* position, canvas px        */
static float bvx620[NB620], bvy620[NB620];     /* velocity, px/frame         */
static float bfl620[NB620], bfp620[NB620];     /* flap rate and phase        */
static float bhu620[NB620];                    /* small per-bird hue offset  */
static float bdn620[NB620];                    /* neighbour count, 0..1      */

void pattern_620(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, j;
    gk_setup(&g620, w, h);
    gk_clear(&g620);

    float cw = (float)g620.cw, ch = (float)g620.ch, sc = g620.sc;
    float t  = (float)frame;

    if (seed != bs620) {
        base620 = (int)(seed & 0x7FFFu);
        for (i = 0; i < NB620; i++) {
            uint32_t r = seed ^ (uint32_t)(i * 2749);
            bx620[i]  = gk_hash(r + 1u) * cw;
            by620[i]  = gk_hash(r + 2u) * ch;
            float a   = gk_hash(r + 3u) * GK_TAU;
            float s   = (1.4f + 0.8f * gk_hash(r + 4u)) * sc;
            bvx620[i] = cosf(a) * s;
            bvy620[i] = sinf(a) * s;
            bfl620[i] = 0.32f + 0.22f * gk_hash(r + 5u);
            bfp620[i] = gk_hash(r + 6u) * GK_TAU;
            bhu620[i] = gk_hash(r + 7u) * 0.10f;
            bdn620[i] = 0.0f;
        }
        bs620 = seed;
    }

    /* The roost drifts on noise rather than a circle, so the flock never
     * repeats a lap.  Everything is pulled toward it and pushed sideways
     * around it; the sideways term is what makes the mass wheel. */
    float rcx = cw * (0.22f + 0.56f * gk_noise1(t * 0.0022f, 11u));
    float rcy = ch * (0.24f + 0.52f * gk_noise1(t * 0.0022f, 23u));
    float spin = (gk_noise1(t * 0.0014f, 37u) - 0.5f) * 2.0f;   /* -1..1 */

    float rsep = 11.0f * sc, rsep2 = rsep * rsep;
    float rali = 30.0f * sc, rali2 = rali * rali;
    float rcoh = 52.0f * sc, rcoh2 = rcoh * rcoh;

    for (i = 0; i < NB620; i++) {
        float sx = 0.0f, sy = 0.0f;      /* separation accumulator */
        float ax = 0.0f, ay = 0.0f;      /* alignment              */
        float cx = 0.0f, cy = 0.0f;      /* cohesion               */
        int na = 0, nc = 0, nd = 0;

        for (j = 0; j < NB620; j++) {
            if (j == i) continue;
            float dx = bx620[j] - bx620[i], dy = by620[j] - by620[i];
            float d2 = dx * dx + dy * dy;
            if (d2 > rcoh2) continue;
            if (d2 < rsep2) {
                float inv = 1.0f / (d2 + 1.0f);   /* 1/r^2 shove, no sqrt  */
                sx -= dx * inv * 90.0f * sc;
                sy -= dy * inv * 90.0f * sc;
                nd++;
            }
            if (d2 < rali2) { ax += bvx620[j]; ay += bvy620[j]; na++; }
            cx += bx620[j]; cy += by620[j]; nc++;
        }

        /* density: close company counts double, the wider ring counts once */
        float dn = ((float)nd * 2.0f + (float)nc * 0.35f) / 9.0f;
        if (dn > 1.0f) dn = 1.0f;
        bdn620[i] += (dn - bdn620[i]) * 0.18f;   /* smoothed, so it doesn't strobe */

        float fx = sx, fy = sy;
        if (na) {
            ax /= (float)na; ay /= (float)na;
            fx += (ax - bvx620[i]) * 0.09f;
            fy += (ay - bvy620[i]) * 0.09f;
        }
        if (nc) {
            cx /= (float)nc; cy /= (float)nc;
            fx += (cx - bx620[i]) * 0.0022f;
            fy += (cy - by620[i]) * 0.0022f;
        }

        /* roost pull plus the tangential swirl that keeps the flock wheeling */
        float px = rcx - bx620[i], py = rcy - by620[i];
        float pd = sqrtf(px * px + py * py) + 1e-3f;
        fx += (px / pd) * 0.075f * sc * (0.4f + pd / (cw * 0.5f));
        fy += (py / pd) * 0.075f * sc * (0.4f + pd / (ch * 0.5f));
        fx += (-py / pd) * 0.13f * sc * spin;
        fy += ( px / pd) * 0.13f * sc * spin;

        /* a little private wander keeps identical neighbours from locking up */
        fx += (gk_noise1(t * 0.03f + (float)i * 7.1f, 101u) - 0.5f) * 0.10f * sc;
        fy += (gk_noise1(t * 0.03f + (float)i * 7.1f, 202u) - 0.5f) * 0.10f * sc;

        bvx620[i] += fx;
        bvy620[i] += fy;

        /* birds cruise: clamp into a narrow speed band, never to zero */
        float sp = sqrtf(bvx620[i] * bvx620[i] + bvy620[i] * bvy620[i]);
        float lo = 1.5f * sc, hi = 3.4f * sc;
        if (sp < 1e-4f) { bvx620[i] = lo; sp = lo; }
        if (sp < lo) { bvx620[i] *= lo / sp; bvy620[i] *= lo / sp; }
        else if (sp > hi) { bvx620[i] *= hi / sp; bvy620[i] *= hi / sp; }

        bx620[i] += bvx620[i];
        by620[i] += bvy620[i];

        /* torus wrap with a margin, so a bird that slips out slides back in */
        float m = 24.0f * sc;
        if (bx620[i] < -m) bx620[i] += cw + 2.0f * m;
        if (bx620[i] > cw + m) bx620[i] -= cw + 2.0f * m;
        if (by620[i] < -m) by620[i] += ch + 2.0f * m;
        if (by620[i] > ch + m) by620[i] -= ch + 2.0f * m;
    }

    float col[3];
    for (i = 0; i < NB620; i++) {
        float vx = bvx620[i], vy = bvy620[i];
        float sp = sqrtf(vx * vx + vy * vy) + 1e-5f;
        float ux = vx / sp, uy = vy / sp;          /* heading */
        float nx = -uy, ny = ux;                   /* wing axis */

        /* Flap: the dihedral collapses as the wings pass through the body
         * line, so at the bottom of the stroke the bird is nearly a dash. */
        float flap = sinf(bfp620[i] + t * bfl620[i]);
        float span = (3.2f + 3.6f * fabsf(flap)) * sc;
        float sweep = (1.9f + 0.9f * (1.0f - fabsf(flap))) * sc;

        float d = bdn620[i];
        sk_col(pal, sk_hidx(base620, bhu620[i] + 0.06f * d),
               0.10f + 0.45f * d,            /* packed birds go pale        */
               0.55f - 0.25f * d,
               0.38f + 0.72f * d, col);      /* and carry the brightness    */

        float hx = bx620[i] + ux * 2.2f * sc;      /* nose */
        float hy = by620[i] + uy * 2.2f * sc;
        float tx = bx620[i] - ux * sweep;          /* wing root, swept back */
        float ty = by620[i] - uy * sweep;
        float cr = 0.52f * sc, gr = 1.9f * sc;

        gk_seg(&g620, hx, hy, tx + nx * span, ty + ny * span, col, cr, gr, 0.20f);
        gk_seg(&g620, hx, hy, tx - nx * span, ty - ny * span, col, cr, gr, 0.20f);
    }

    gk_present(&g620, fb, w, h);
}
