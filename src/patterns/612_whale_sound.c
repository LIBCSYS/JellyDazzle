/* 612 Whale Sound (creature) — one great whale crosses the frame at a pace
 * that has nothing to prove.  The whole thing hangs off the fluke beat: the
 * tail sweeps, the body flexes behind the head, and forward speed peaks in
 * the middle of a sweep rather than at the turnaround, which is what makes it
 * read as an animal pushing water instead of a sprite on a conveyor.  The
 * flex amplitude grows as u^2 down the body, so the head is almost steady and
 * the tail does all the travelling — get that ratio wrong and it looks like a
 * worm.
 *
 * Mass is drawn as a chain of soft discs rather than an outline, because an
 * outlined whale at this scale reads as a cartoon; the dorsal rim is the only
 * hard edge, and it is what the eye uses to find the shape.  A thin stream of
 * bubbles leaves the blowhole on a slow cadence and is the only fast-moving
 * thing on screen, which gives the slow body something to be slow against. */
#include "_spark572.h"

#define NS612 22           /* spine samples, nose to tail                   */
#define NB612 40           /* bubble pool                                   */

static gk g612;
static uint32_t bs612 = 0xFFFFFFFFu;
static int   base612;
static float wx612, wy612, wdir612, wsc612, whue612;
static float bx612[NB612], by612[NB612], bvy612[NB612], br612[NB612];
static float bage612[NB612], bph612[NB612];
static int   bnext612;

void pattern_612(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int i, k;
    float sx[NS612 + 1], sy[NS612 + 1], sr[NS612 + 1];
    float col[3];

    gk_setup(&g612, w, h);
    gk_decay_snap(&g612, 0.87f);

    float cw = (float)g612.cw, ch = (float)g612.ch, sc = g612.sc;

    if (seed != bs612) {
        base612 = (int)(seed & 0x7FFFu);
        wdir612 = (gk_hash(seed ^ 0x101u) < 0.5f) ? 1.0f : -1.0f;
        wsc612  = (0.85f + 0.40f * gk_hash(seed ^ 0x102u)) * sc;
        wx612   = (wdir612 > 0.0f) ? -140.0f * wsc612 : cw + 140.0f * wsc612;
        wy612   = (0.34f + 0.30f * gk_hash(seed ^ 0x103u)) * ch;
        whue612 = gk_hash(seed ^ 0x104u);
        for (i = 0; i < NB612; i++) bage612[i] = -1.0f;
        bnext612 = 0;
        bs612 = seed;
    }

    float t  = (float)frame;
    float ph = t * 0.050f;                      /* one fluke beat per ~125 f */

    /* Speed follows the sweep rate, not the sweep position.  The -2.1 offset
     * is the same lag the body carries, so thrust arrives when the fluke is
     * actually moving through water. */
    float thrust = fabsf(cosf(ph - 2.1f));
    wx612 += wdir612 * (0.30f + 0.85f * thrust) * wsc612;
    wy612 += (gk_noise1(t * 0.0035f, 61u) - 0.5f) * 0.9f * wsc612;
    if (wy612 < 0.20f * ch) wy612 = 0.20f * ch;
    if (wy612 > 0.82f * ch) wy612 = 0.82f * ch;

    float L = 155.0f * wsc612;
    if (wdir612 > 0.0f && wx612 > cw + L) {
        wx612 = -L; whue612 = gk_hash((uint32_t)t ^ 0x55u);
        wy612 = (0.30f + 0.36f * gk_hash((uint32_t)t ^ 0x57u)) * ch;
    }
    if (wdir612 < 0.0f && wx612 < -L) {
        wx612 = cw + L; whue612 = gk_hash((uint32_t)t ^ 0x56u);
        wy612 = (0.30f + 0.36f * gk_hash((uint32_t)t ^ 0x58u)) * ch;
    }

    /* SPINE: u runs 0 at the snout to 1 at the tail stock. */
    for (k = 0; k <= NS612; k++) {
        float u    = (float)k / (float)NS612;
        float flex = sinf(ph - u * 2.1f) * (1.5f + 15.0f * u * u) * wsc612 * 0.55f;
        float tap  = 1.0f - u;
        tap = tap * (0.55f + 0.45f * tap);           /* full amidships        */
        sx[k] = wx612 + wdir612 * (0.45f - u) * L;
        sy[k] = wy612 + flex;
        sr[k] = 16.0f * wsc612 * gk_smooth(u * 7.0f) * tap + 0.6f * wsc612;
    }

    /* MASS: stacked discs, deliberately dim — they overlap about five deep,
     * so a low amplitude per disc is what keeps the body off the clip ceiling. */
    sk_col(pal, sk_hidx(base612, whue612), 0.08f, 0.70f, 0.15f, col);
    for (k = 0; k <= NS612; k++) gk_disc(&g612, sx[k], sy[k], sr[k], col);

    /* DORSAL RIM: the only crisp edge on the animal. */
    sk_col(pal, sk_hidx(base612, whue612 + 0.03f), 0.34f, 0.50f, 0.95f, col);
    for (k = 1; k <= NS612; k++)
        gk_seg(&g612, sx[k - 1], sy[k - 1] - sr[k - 1] * 0.80f,
                      sx[k],     sy[k]     - sr[k]     * 0.80f,
               col, 0.8f * wsc612, 2.6f * wsc612, 0.18f);

    /* THROAT PLEATS: short ventral strokes over the first third.  Cheap, but
     * they are what says rorqual rather than submarine. */
    sk_col(pal, sk_hidx(base612, whue612 + 0.07f), 0.18f, 0.55f, 0.55f, col);
    for (k = 2; k <= 7; k++)
        gk_seg(&g612, sx[k], sy[k] + sr[k] * 0.25f,
                      sx[k], sy[k] + sr[k] * 0.92f,
               col, 0.5f * wsc612, 1.6f * wsc612, 0.10f);

    /* FLUKES: hung off the tail tangent and lagging the beat by a further
     * quarter cycle, so the lobes feather instead of snapping flat. */
    float tgx = sx[NS612] - sx[NS612 - 1], tgy = sy[NS612] - sy[NS612 - 1];
    float m = sqrtf(tgx * tgx + tgy * tgy); if (m < 1e-4f) m = 1.0f;
    tgx /= m; tgy /= m;
    float nx = -tgy, ny = tgx;
    float fl = 25.0f * wsc612;
    float swp = sinf(ph - 3.3f);
    sk_col(pal, sk_hidx(base612, whue612 + 0.05f), 0.26f, 0.50f, 0.85f, col);
    for (i = -1; i <= 1; i += 2) {
        float fi = (float)i;
        float ex = sx[NS612] + nx * fl * fi * 0.95f + tgx * fl * 0.30f;
        float ey = sy[NS612] + ny * fl * fi * 0.95f + tgy * fl * 0.30f
                 + swp * 4.0f * wsc612 * fi;
        gk_seg(&g612, sx[NS612], sy[NS612], ex, ey,
               col, 1.1f * wsc612, 3.4f * wsc612, 0.20f);
        /* trailing edge, swept back toward the tail stock */
        gk_seg(&g612, ex, ey,
               sx[NS612] + nx * fl * fi * 0.25f - tgx * fl * 0.55f,
               sy[NS612] + ny * fl * fi * 0.25f - tgy * fl * 0.55f,
               col, 0.8f * wsc612, 2.6f * wsc612, 0.16f);
    }

    /* PECTORAL FIN: one, far side only — two symmetric flippers from a side
     * view would read as a plan drawing. */
    {
        int kp = 6;
        float pw = 20.0f * wsc612;
        float pa = 0.55f + 0.30f * sinf(ph - 1.2f);
        sk_col(pal, sk_hidx(base612, whue612 + 0.09f), 0.22f, 0.50f, 0.70f, col);
        gk_seg(&g612, sx[kp], sy[kp] + sr[kp] * 0.4f,
               sx[kp] - wdir612 * pw * cosf(pa), sy[kp] + sr[kp] * 0.4f + pw * sinf(pa),
               col, 1.0f * wsc612, 3.0f * wsc612, 0.16f);
    }

    /* EYE: one dot, just behind the jawline.  Any bigger and it goes comic. */
    sk_col(pal, sk_hidx(base612, whue612 + 0.45f), 0.70f, 0.25f, 1.00f, col);
    gk_dot(&g612, sx[3] + wdir612 * sr[3] * 0.35f, sy[3] + sr[3] * 0.10f,
           col, 0.9f * wsc612, 2.4f * wsc612, 0.30f);

    /* BUBBLES: emitted on a cadence, not every frame — a continuous jet looks
     * like a leak.  They are the only quick thing in the frame. */
    if (((int)t % 7) == 0) {
        int b = bnext612 % NB612;
        uint32_t r = (uint32_t)frame * 2654435761u;
        bx612[b]   = sx[4];
        by612[b]   = sy[4] - sr[4] * 0.95f;
        bvy612[b]  = -(0.45f + 0.55f * gk_hash(r + 1u)) * wsc612;
        br612[b]   = (0.7f + 1.3f * gk_hash(r + 2u)) * wsc612;
        bph612[b]  = gk_hash(r + 3u) * 6.2831853f;
        bage612[b] = 0.0f;
        bnext612   = (bnext612 + 1) % NB612;
    }
    sk_col(pal, sk_hidx(base612, whue612 + 0.52f), 0.55f, 0.35f, 0.80f, col);
    for (i = 0; i < NB612; i++) {
        if (bage612[i] < 0.0f) continue;
        bage612[i] += 1.0f;
        by612[i]   += bvy612[i];
        bx612[i]   += sinf(bph612[i] + bage612[i] * 0.09f) * 0.35f * wsc612;
        bvy612[i]  *= 1.004f;                       /* bubbles accelerate up  */
        if (by612[i] < -8.0f * sc || bage612[i] > 260.0f) { bage612[i] = -1.0f; continue; }
        float fade = 1.0f - bage612[i] / 260.0f;
        float c2[3];
        c2[0] = col[0] * fade; c2[1] = col[1] * fade; c2[2] = col[2] * fade;
        gk_dot(&g612, bx612[i], by612[i], c2,
               br612[i] * 0.8f, br612[i] * 2.4f, 0.22f);
    }

    gk_present(&g612, fb, w, h);
}
