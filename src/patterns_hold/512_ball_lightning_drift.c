/* 512 Ball Lightning Drift — one large luminous globe rolls slowly about
 * the frame on a noise path above a faint ground line, leaving a soft
 * fading trail behind it on a persistence canvas; every so often a
 * tendril of lightning reaches from the ball to the ground (or up into
 * the air) and withdraws, each on its own slow clock.  The ball's colour
 * turns through the palette; tendrils carry the ball's hue at the root
 * and shift toward another hue at the tip.  Sparse overlay.  Repaint with
 * memory (decay). */
#include "_trace509.h"

#define NT512 4
#define P512 150

static gk g512;      /* persistence: ball + trail */
static gk g512b;     /* per-frame: tendrils on top of a copy    */
static gk_bolt b512[NT512];
static int bi512[NT512] = { -1, -1, -1, -1 };
static uint32_t bs512 = 0xFFFFFFFFu;
static float tx512[NT512], ty512[NT512], th512[NT512];   /* tendril target (canvas), hue */

void pattern_512(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g512, w, h);
    if (seed != bs512) { for (int s = 0; s < NT512; s++) bi512[s] = -1; bs512 = seed; gk_clear(&g512); }
    gk_decay_snap(&g512, 0.94f);
    float cw = (float)g512.cw, ch = (float)g512.ch, sc = g512.sc, t = (float)frame;
    int base = (int)(t * 1.5f) + (int)(seed & 8191u);
    float gy = ch * 0.84f;
    /* ball position: slow noise wander */
    float bx = cw * (0.15f + 0.7f * gk_noise1(t * 0.003f, 11u + seed));
    float by = ch * (0.25f + 0.45f * gk_noise1(t * 0.0024f + 5.0f, 23u + seed));
    /* ground line, faint, lit near the ball */
    float gc[3];
    gk_col(pal, base + 4200, 0.1f, 0.06f, gc);
    gk_seg(&g512, 0.0f, gy, cw, gy, gc, 1.0f * sc, 3.0f * sc, 0.3f);
    float lit = expf(-((by - gy) * (by - gy)) / (ch * ch * 0.06f));
    gk_col(pal, base + 4200, 0.2f, 0.25f * lit, gc);
    gk_dot(&g512, bx, gy, gc, 30.0f * sc, 90.0f * sc, 0.5f);
    /* the ball: layered soft glow, hue morphing (persistent -> trail) */
    float k0[3], k1[3], k2[3];
    gk_col(pal, base, 0.5f, 0.9f, k0);
    gk_col(pal, base + 900, 0.2f, 0.35f, k1);
    gk_col(pal, base + 1800, 0.05f, 0.10f, k2);
    gk_dot(&g512, bx, by, k0, 5.0f * sc, 14.0f * sc, 0.6f);
    gk_dot(&g512, bx, by, k1, 12.0f * sc, 30.0f * sc, 0.6f);
    gk_dot(&g512, bx, by, k2, 30.0f * sc, 60.0f * sc, 0.4f);
    /* tendrils go on a fresh copy so they do not smear */
    gk_setup(&g512b, w, h);
    memcpy(g512b.acc, g512.acc, sizeof(float) * (size_t)(g512.cw * g512.ch) * 3);
    for (int s = 0; s < NT512; s++) {
        int ph = frame + s * (P512 / NT512) + s * 17;
        int idx = ph / P512;
        float age = (float)(ph - idx * P512);
        if (idx != bi512[s]) {
            gk_seed(&g512, seed ^ (uint32_t)(idx * 4423 + s * 7013));
            if (gk_rf(&g512) < 0.6f) { tx512[s] = bx + cw * 0.25f * gk_rs(&g512); ty512[s] = gy; }
            else { tx512[s] = bx + cw * 0.3f * gk_rs(&g512); ty512[s] = by - ch * (0.15f + 0.3f * gk_rf(&g512)); }
            th512[s] = gk_rf(&g512);
            gk_bolt_gen(&g512, &b512[s], 0.0f, 0.0f, 1.0f, 0.0f, 0.15f, 6, 3, 0.3f);
            bi512[s] = idx;
        }
        float env = gk_env(age, 14.0f, 30.0f, 45.0f);
        if (env <= 0.0f) continue;
        float dx = tx512[s] - bx, dy = ty512[s] - by;
        float len = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
        if (len > cw * 0.5f) len = cw * 0.5f;                     /* the ball may have wandered */
        float prog = age / 20.0f;
        int pi = base + (int)(th512[s] * 3000.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, base, 0.05f, 0.35f * env, h0);
        gk_col(pal, pi + 2500, 0.05f, 0.35f * env, h1);
        gk_col(pal, base + 300, 0.4f, 0.6f * env, c0);
        gk_col(pal, pi + 2800, 0.3f, 0.6f * env, c1);
        bx_draw_grad(&g512b, &b512[s], bx, by, ang, len, 1.0f, prog, 1.0f, h0, h1, 0.2f, 1.6f * sc, 5.5f * sc, 0.5f);
        bx_draw_grad(&g512b, &b512[s], bx, by, ang, len, 1.0f, prog, 1.0f, c0, c1, 0.2f, 0.7f * sc, 1.8f * sc, 0.25f);
    }
    gk_present(&g512b, fb, w, h);
}
