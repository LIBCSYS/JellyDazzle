/* 494 Upward Streamers — the moment before a strike, stretched out.  A
 * stepped leader descends slowly from the top while three or four short
 * streamers reach up from points along the ground line to meet it; when
 * the leader connects with one, the whole channel brightens over ~15
 * frames into a return stroke and then cools away over ~80.  The unlucky
 * streamers fade back into the ground.  Repaint pattern; sparse overlay. */
#include "_hue469.h"

#define P494 320
#define NST494 4

static gk g494;
static gk_bolt lead494, str494[NST494];
static int bi494 = -1;
static uint32_t bs494 = 0xFFFFFFFFu;
static int win494;
static float hue494;

void pattern_494(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g494, w, h);
    gk_clear(&g494);
    if (seed != bs494) { bi494 = -1; bs494 = seed; }
    float cw = (float)g494.cw, ch = (float)g494.ch, sc = g494.sc, t = (float)frame;
    int idx = frame / P494;
    float age = (float)(frame - idx * P494);
    if (idx != bi494) {
        gk_seed(&g494, seed ^ (uint32_t)(idx * 4093));
        float gy = ch * 0.9f;
        win494 = (int)(gk_rf(&g494) * NST494) % NST494;
        float sx[NST494];
        for (int i = 0; i < NST494; i++) sx[i] = cw * ((float)i + 0.2f + 0.6f * gk_rf(&g494)) / (float)NST494;
        float x0 = sx[win494] + cw * 0.15f * gk_rs(&g494);
        /* leader ends a little above the ground at the winning streamer's tip */
        float tipy = gy - ch * (0.12f + 0.08f * gk_rf(&g494));
        gk_bolt_gen(&g494, &lead494, x0, -ch * 0.02f, sx[win494], tipy, 0.2f, 7, 5, 0.4f);
        for (int i = 0; i < NST494; i++) {
            float len = ch * (0.08f + 0.08f * gk_rf(&g494));
            if (i == win494) len = gy - tipy;
            gk_bolt_gen(&g494, &str494[i], sx[i], gy, sx[i] + len * 0.25f * gk_rs(&g494), gy - len, 0.2f, 4, 1, 0.3f);
        }
        hue494 = gk_rf(&g494);
        bi494 = idx;
    }
    int base = (int)(t * 1.5f) + (int)(seed & 8191u) + (int)(hue494 * 6000.0f);
    /* timeline: leader descends 0..120, streamers rise 40..120, contact at
     * 120, return stroke 120..135 bright, cool 135..260, dark to 320 */
    float lp = age / 120.0f;
    float lenv = gk_smooth(age / 12.0f) * (1.0f - gk_smooth((age - 200.0f) / 60.0f));
    float ret = gk_env(age - 118.0f, 15.0f, 20.0f, 100.0f);
    if (lenv > 0.0f) {
        float amp = 0.35f + 1.2f * ret;
        hk_style st;                      /* hue runs cloud -> tip; return stroke whitens a touch */
        hk_style_set(&st, 5000, 1800, 900,
                     0.5f * lenv * amp, (1.6f + 1.2f * ret) * sc, (5.0f + 4.0f * ret) * sc, 0.5f,
                     0.35f + 0.15f * ret, 0.55f * lenv * amp, (0.7f + 0.5f * ret) * sc, (2.0f + 1.0f * ret) * sc, 0.25f);
        hk_bolt(&g494, &lead494, lp, 1.0f, pal, base + (int)(age * 6.0f), &st);
        /* leader tip glow while descending */
        if (lp < 1.0f) {
            for (int i = 0; i < lead494.n; i++) {
                const gk_bseg *s = &lead494.s[i];
                if (s->t0 < lp && s->t1 >= lp && s->wgt > 0.9f) {
                    float f = (lp - s->t0) / (s->t1 - s->t0);
                    float tc[3];
                    gk_col(pal, base, 0.7f, 1.0f * lenv, tc);
                    gk_dot(&g494, s->x0 + (s->x1 - s->x0) * f, s->y0 + (s->y1 - s->y0) * f, tc, 1.5f * sc, 6.0f * sc, 0.6f);
                    break;
                }
            }
        }
    }
    for (int i = 0; i < NST494; i++) {
        float sp = (age - 40.0f) / 80.0f;
        float senv = gk_smooth((age - 40.0f) / 15.0f);
        if (i == win494) senv *= (1.0f - gk_smooth((age - 200.0f) / 60.0f));
        else senv *= (1.0f - gk_smooth((age - 130.0f) / 40.0f));
        if (senv <= 0.0f) continue;
        float amp = i == win494 ? 0.35f + 1.2f * ret : 0.35f;
        hk_style st;                      /* streamers: ground hue, each its own shade */
        hk_style_set(&st, -2500, 1000, 900,
                     0.5f * senv * amp, 1.4f * sc, 4.5f * sc, 0.5f,
                     0.35f + 0.15f * ret, 0.55f * senv * amp, 0.6f * sc, 1.8f * sc, 0.25f);
        /* runs backward so the winner's tip meets the leader's tip hue */
        hk_bolt(&g494, &str494[i], sp, 1.0f, pal, base + 7500 + (i == win494 ? 0 : i * 700) + (int)(age * 6.0f), &st);
    }
    /* ground glow */
    float gc[3];
    gk_col(pal, base + 2000, 0.1f, 0.10f + 0.5f * ret, gc);
    gk_seg(&g494, 0.0f, ch * 0.9f, cw, ch * 0.9f, gc, 1.0f * sc, 6.0f * sc, 0.6f);
    gk_present(&g494, fb, w, h);
}
