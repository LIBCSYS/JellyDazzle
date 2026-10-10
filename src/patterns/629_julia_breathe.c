/* 629 Julia Breathe (fractal) — a Julia set that never stops changing shape.
 *
 * z -> z^2 + c is only interesting because the picture is violently sensitive
 * to c: nudge it and a connected blob falls apart into dust, a spiral uncurls
 * into a dendrite.  Here c walks a CLOSED loop through a short list of
 * hand-picked values, so the shape morphs forever and always comes back.  The
 * loop matters.  An earlier version drifted c at random and spent most of its
 * life either inside the main cardioid (a solid lump, nothing to look at) or
 * far outside it (empty dust).  The band that is worth watching is thin, so
 * the waypoints are named Julias — rabbit, seahorse, san marco, dendrite —
 * and morphing just means easing between neighbours.
 *
 * COST STRATEGY.  No per-pixel escape loop anywhere.  The plane is sampled on
 * a COARSE grid of cells about five canvas pixels across, and only a band of
 * grid rows is recomputed per frame — the sweep wipes top to bottom and starts
 * again.  Everything not touched this frame is still on the canvas, held by a
 * slow decay, so the viewer sees a complete fractal while only a fifteenth of
 * it is being computed.  c advances once per completed sweep rather than once
 * per frame, otherwise the band boundaries would read as tearing instead of a
 * wipe.  Iteration cap is 40 with an early bail; the sampled cell count per
 * frame is about two thousand.
 *
 * On screen: a soft lace of glow blobs, breathing in and out of different
 * shapes, with the palette walking underneath it.
 */
#include "_spark572.h"

#define JB629_CELL 5.0f           /* grid pitch, canvas px before scale   */
#define JB629_BAND 14             /* grid rows recomputed per frame       */
#define JB629_IT   40             /* iteration cap — modest on purpose    */

static gk       g629;
static uint32_t bs629 = 0xFFFFFFFFu;
static int      base629;
static int      row629;           /* sweep position, in grid rows         */
static float    ang629;           /* where we are around the c-loop       */
static float    half629;          /* view half-height                     */
static float    rot629;           /* view rotation                        */
static float    zrate629;         /* breathing rate of the framing        */

/* Named Julia parameters.  These are waypoints, not a formula — see above. */
static const float C629[8][2] = {
    { -0.4f,      0.6f    },      /* the classic swirl                    */
    { -0.70176f, -0.3842f },      /* seahorse, delicate arms              */
    { -0.835f,   -0.2321f },      /* dense filigree                       */
    { -0.7269f,   0.1889f },      /* dendrite lace                        */
    { -0.123f,    0.745f  },      /* Douady rabbit, three-fold            */
    { -0.75f,     0.11f   },      /* san marco, budding                   */
    { -0.391f,   -0.587f  },      /* siegel disk, smooth whorls           */
    {  0.285f,    0.01f   }       /* near-parabolic, fine spirals         */
};
#define JB629_NC 8

/* Ease between neighbouring waypoints so the morph never snaps. */
static void c629_at(float t, float *cr, float *ci)
{
    while (t < 0.0f)               t += (float)JB629_NC;
    while (t >= (float)JB629_NC)    t -= (float)JB629_NC;
    int a = (int)t, b = (a + 1) % JB629_NC;
    float f = t - (float)a;
    f = f * f * (3.0f - 2.0f * f);
    *cr = C629[a][0] + (C629[b][0] - C629[a][0]) * f;
    *ci = C629[a][1] + (C629[b][1] - C629[a][1]) * f;
}

void pattern_629(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    int gx, gy, i;
    gk_setup(&g629, w, h);
    gk_decay_snap(&g629, 0.90f);          /* holds the untouched rows      */

    float cw = (float)g629.cw, ch = (float)g629.ch, sc = g629.sc;

    if (seed != bs629) {
        base629  = (int)(seed & 0x7FFFu);
        ang629   = gk_hash(seed + 11u) * (float)JB629_NC;
        half629  = 1.25f + 0.55f * gk_hash(seed + 12u);
        rot629   = gk_hash(seed + 13u) * GK_TAU;
        zrate629 = 0.0017f + 0.0022f * gk_hash(seed + 14u);
        row629   = 0;
        gk_clear(&g629);
        bs629 = seed;
    }

    float cell  = JB629_CELL * sc;
    int   gcols = (int)(cw / cell) + 1;
    int   grows = (int)(ch / cell) + 1;

    /* Framing breathes slowly.  The plane is rewritten every sweep anyway, so
     * a drifting zoom costs nothing and means the set is never twice the same
     * size in the same place. */
    float t    = (float)frame;
    float hh   = half629 * (1.0f + 0.22f * sinf(t * zrate629));
    float offr = 0.10f * sinf(t * zrate629 * 0.61f);
    float offi = 0.10f * cosf(t * zrate629 * 0.43f);
    float cs   = cosf(rot629 + t * 0.00035f);
    float sn   = sinf(rot629 + t * 0.00035f);
    float asp  = cw / ch;

    float cr, ci;
    c629_at(ang629, &cr, &ci);

    float col[3];
    int   ylast = row629 + JB629_BAND;
    if (ylast > grows) ylast = grows;

    for (gy = row629; gy < ylast; gy++) {
        float py = ((float)gy + 0.5f) * cell;
        float v  = (py / ch - 0.5f) * 2.0f * hh;
        for (gx = 0; gx < gcols; gx++) {
            float px = ((float)gx + 0.5f) * cell;
            float u  = (px / cw - 0.5f) * 2.0f * hh * asp;

            float zr = u * cs - v * sn + offr;
            float zi = u * sn + v * cs + offi;
            float zr2 = zr * zr, zi2 = zi * zi;
            float trap = 1e18f;               /* closest approach to origin */
            i = 0;
            while (i < JB629_IT && zr2 + zi2 <= 16.0f) {
                zi = 2.0f * zr * zi + ci;
                zr = zr2 - zi2 + cr;
                zr2 = zr * zr; zi2 = zi * zi;
                float d = zr2 + zi2;
                if (d < trap) trap = d;
                i++;
            }

            float hue, amp, core, glow;
            if (i >= JB629_IT) {
                /* Inside the set there is no escape count, and painting the
                 * interior black would put a hole in the middle of the frame.
                 * The orbit trap — how close the orbit ever came to zero —
                 * varies smoothly in there and carries the internal bulbs, so
                 * the inside gets its own sweep of hue instead. */
                float lt = logf(trap + 1e-12f);           /* about -28..0   */
                hue  = 0.58f + lt * -0.012f;
                amp  = 0.62f;
                core = cell * 0.52f;
                glow = cell * 0.70f;
            } else {
                /* Smooth escape count: the integer count alone bands into
                 * visible contour rings at this grid pitch. */
                float mag = sqrtf(zr2 + zi2);
                float nu  = (float)i + 1.0f
                          - logf(logf(mag) * 1.442695f) * 1.442695f;
                hue  = nu * 0.035f;
                amp  = 0.35f + 0.55f * (1.0f - (float)i / (float)JB629_IT);
                core = cell * 0.42f;
                glow = cell * 0.62f;
            }
            sk_col(pal, sk_hidx(base629, hue + t * 0.00022f),
                   0.22f, 0.50f, amp, col);
            gk_dot(&g629, px, py, col, core, glow, 0.30f);
        }
    }

    row629 = ylast;
    if (row629 >= grows) {                 /* sweep done: advance c, repeat */
        row629 = 0;
        ang629 += 0.0215f;                 /* one waypoint hop about 6 s    */
        if (ang629 >= (float)JB629_NC) ang629 -= (float)JB629_NC;
    }

    gk_present(&g629, fb, w, h);
}
