/* 506 Ground Strike — a bolt hits a ground plane seen in perspective, and
 * from the strike point rings of light roll outward across the plane as
 * flattened ellipses, slow and soft, while the bolt itself cools.  A faint
 * grid on the plane gives the perspective.  Field-ish overlay, dark sky.
 * Repaint pattern. */
#include "_hue469.h"

#define P506 300

static gk g506;
static gk_bolt b506;
static int bi506 = -1;
static uint32_t bs506 = 0xFFFFFFFFu;
static float sx506, sz506;   /* strike point on the plane: x (-1..1), depth z (0 near..1 far) */

/* plane point -> canvas: horizon at 45%, near edge at bottom */
static void plane506(float x, float z, float cw, float ch, float *ox, float *oy)
{
    /* Gate finding F2: rolling rings reach z < 0 (behind the camera), the
     * projection blows up and ring segments became full-frame lines sweeping
     * the sky (delta 9.5, 25 strobe steps in 2000 frames).  Clamp depth. */
    if (z < 0.03f) z = 0.03f;
    float p = 1.0f / (0.35f + z * 1.4f);
    *ox = cw * 0.5f + x * cw * 0.9f * p * 0.5f;
    *oy = ch * 0.45f + ch * 0.55f * p * 0.5f * 0.7f;
}

void pattern_506(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g506, w, h);
    gk_clear(&g506);
    if (seed != bs506) { bi506 = -1; bs506 = seed; }
    float cw = (float)g506.cw, ch = (float)g506.ch, sc = g506.sc, t = (float)frame;
    int idx = frame / P506;
    float age = (float)(frame - idx * P506);
    if (idx != bi506) {
        gk_seed(&g506, seed ^ (uint32_t)(idx * 4177));
        sx506 = gk_rs(&g506) * 0.5f; sz506 = 0.15f + 0.5f * gk_rf(&g506);
        float gx, gy;
        plane506(sx506, sz506, cw, ch, &gx, &gy);
        gk_bolt_gen(&g506, &b506, gx + cw * 0.15f * gk_rs(&g506), -ch * 0.03f, gx, gy, 0.2f, 7, 5, 0.4f);
        bi506 = idx;
    }
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    /* the plane grid, faint, lit near the strike */
    float ret = gk_env(age - 22.0f, 22.0f, 30.0f, 120.0f) * 0.7f;
    float gc[3];
    gk_col(pal, base + 4000, 0.1f, 0.10f + 0.10f * ret, gc);
    for (int i = -4; i <= 4; i++) {
        float x0, y0, x1, y1;
        plane506((float)i * 0.25f, 0.0f, cw, ch, &x0, &y0);
        plane506((float)i * 0.25f, 3.0f, cw, ch, &x1, &y1);
        gk_seg(&g506, x0, y0, x1, y1, gc, 0.8f * sc, 2.5f * sc, 0.3f);
    }
    for (int j = 0; j < 8; j++) {
        float z = 0.05f * powf(1.55f, (float)j) - 0.05f, x0, y0, x1, y1;
        plane506(-1.0f, z, cw, ch, &x0, &y0);
        plane506(1.0f, z, cw, ch, &x1, &y1);
        gk_seg(&g506, x0, y0, x1, y1, gc, 0.8f * sc, 2.5f * sc, 0.3f);
    }
    /* bolt: grows 0..24, return stroke, cools */
    float benv = gk_smooth(age / 8.0f) * (1.0f - gk_smooth((age - 150.0f) / 90.0f));
    if (benv > 0.0f) {
        float amp = 0.5f + 1.0f * ret;
        hk_style st;                      /* hue sky -> ground, whitens a little on the return stroke */
        hk_style_set(&st, 5500, 1800, 800,
                     0.5f * benv * amp, (1.8f + 1.0f * ret) * sc, (6.0f + 3.0f * ret) * sc, 0.5f,
                     0.35f + 0.15f * ret, 0.65f * benv * amp, (0.8f + 0.4f * ret) * sc, (2.2f + 0.8f * ret) * sc, 0.25f);
        hk_bolt(&g506, &b506, age / 24.0f, 1.0f, pal, base + (int)(age * 8.0f), &st);
    }
    /* rings rolling out on the plane: 3 rings launched at 24, 44, 64 */
    for (int r = 0; r < 3; r++) {
        float ra = age - 24.0f - (float)r * 20.0f;
        if (ra < 0.0f) continue;
        float rad = ra * 0.012f;                       /* plane units */
        float renv = (1.0f - gk_smooth((ra - 60.0f) / 120.0f)) * gk_smooth(ra / 10.0f);
        if (renv <= 0.0f) continue;
        float rc[3];
        gk_col(pal, base + 1500 + r * 500, 0.3f, 0.6f * renv, rc);
        float lx = 0.0f, ly = 0.0f;
        for (int k = 0; k <= 48; k++) {
            float a = GK_TAU * (float)k / 48.0f;
            float x, y;
            plane506(sx506 + cosf(a) * rad, sz506 + sinf(a) * rad * 0.9f, cw, ch, &x, &y);
            if (k > 0) gk_seg(&g506, lx, ly, x, y, rc, 1.5f * sc, 5.0f * sc, 0.6f);
            lx = x; ly = y;
        }
    }
    float sg[3];
    { float gx, gy; plane506(sx506, sz506, cw, ch, &gx, &gy);
      gk_col(pal, base, 0.5f, 1.2f * ret, sg);
      gk_dot(&g506, gx, gy, sg, 3.0f * sc, 16.0f * sc, 0.6f); }
    gk_present(&g506, fb, w, h);
}
