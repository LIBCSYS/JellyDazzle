/* 525 Anvil Underside — looking up at the flat base of a storm anvil in
 * perspective: the cloud ceiling recedes to a horizon at ~55%, textured
 * with a slow fbm; across it spider lightning crawls in the plane of the
 * cloud — from a hub, eight channels creep outward on the ceiling,
 * foreshortened with distance, over ~60 frames, glow, and die from the hub
 * outward, lighting the cloud base around them.  Two hubs alternate; hue
 * per hub, sliding outward along the channels.  Full-width field above
 * the horizon; below is dark.  Repaint. */
#include "_trace509.h"

#define GW525 72
#define GH525 40
#define NC525 8
#define P525 320

static gk g525;
static float grid525[GW525 * GH525];
static float lut525[256 * 3];
static gk_bolt b525[2][NC525];
static int bi525[2] = { -1, -1 };
static uint32_t bs525 = 0xFFFFFFFFu;
static float hx525[2], hz525[2], hue525[2];

/* ceiling point (x: -1..1 across, z: 0 near .. 1 far) -> canvas */
static void proj525(float x, float z, float cw, float ch, float *ox, float *oy)
{
    if (z < 0.03f) z = 0.03f;
    float p = 1.0f / (0.30f + z * 1.6f);
    *ox = cw * 0.5f + x * cw * 0.55f * p;
    *oy = ch * 0.55f - ch * 0.5f * p * 0.45f;
}
static void drawbolt525(gk *g, const gk_bolt *b, float prog, const float *c0, const float *c1,
                        float cw, float ch, float sc, float cr, float gr, float gi)
{
    for (int i = 0; i < b->n; i++) {
        const gk_bseg *s = &b->s[i];
        if (s->t0 >= prog) continue;
        float x1 = s->x1, y1 = s->y1, t1 = s->t1;
        if (s->t1 > prog) { float f = (prog - s->t0) / (s->t1 - s->t0); x1 = s->x0 + (s->x1 - s->x0) * f; y1 = s->y0 + (s->y1 - s->y0) * f; t1 = prog; }
        float tm = 0.5f * (s->t0 + t1);
        float wa = 0.35f + 0.65f * s->wgt;
        float col[3] = { (c0[0] + (c1[0] - c0[0]) * tm) * wa, (c0[1] + (c1[1] - c0[1]) * tm) * wa, (c0[2] + (c1[2] - c0[2]) * tm) * wa };
        float ax, ay, bx, by;
        proj525(s->x0, s->y0, cw, ch, &ax, &ay);
        proj525(x1, y1, cw, ch, &bx, &by);
        float zm = 0.5f * (s->y0 + y1); if (zm < 0.03f) zm = 0.03f;   /* z may wander behind the eye */
        float p = 1.0f / (0.30f + zm * 1.6f); if (p > 1.6f) p = 1.6f;  /* thinner when far */
        float ws = (0.5f + 0.5f * s->wgt) * (0.4f + 0.6f * p);
        gk_seg(g, ax, ay, bx, by, col, cr * sc * ws, gr * sc * ws, gi);
    }
}

void pattern_525(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g525, w, h);
    gk_clear(&g525);
    if (seed != bs525) { bi525[0] = bi525[1] = -1; bs525 = seed; }
    float cw = (float)g525.cw, ch = (float)g525.ch, sc = g525.sc, t = (float)frame;
    int base = (int)(t * 1.3f) + (int)(seed & 8191u);
    float lit[2] = { 0.0f, 0.0f };
    for (int s = 0; s < 2; s++) {
        int ph = frame + s * (P525 / 2);
        int idx = ph / P525;
        float age = (float)(ph - idx * P525);
        if (idx != bi525[s]) {
            gk_seed(&g525, seed ^ (uint32_t)(idx * 4933 + s * 8231));
            hx525[s] = 0.7f * gk_rs(&g525); hz525[s] = 0.15f + 0.5f * gk_rf(&g525);
            hue525[s] = gk_rf(&g525);
            for (int c = 0; c < NC525; c++) {
                float a = GK_TAU * ((float)c + 0.5f * gk_rf(&g525)) / (float)NC525;
                float len = 0.35f + 0.4f * gk_rf(&g525);
                gk_bolt_gen(&g525, &b525[s][c], hx525[s], hz525[s], hx525[s] + cosf(a) * len, hz525[s] + sinf(a) * len * 0.8f, 0.14f, 6, 3, 0.35f);
            }
            bi525[s] = idx;
        }
        float env = gk_env(age, 10.0f, 70.0f, 90.0f);
        if (env <= 0.0f) continue;
        lit[s] = env;
        float prog = age / 60.0f;
        float die = gk_smooth((age - 90.0f) / 80.0f);       /* hub dies first */
        int pi = base + (int)(hue525[s] * 8000.0f);
        float c0[3], c1[3], h0[3], h1[3];
        gk_col(pal, pi, 0.05f, 0.42f * env * (1.0f - 0.7f * die), h0);
        gk_col(pal, pi + 2000, 0.05f, 0.40f * env, h1);
        gk_col(pal, pi + 300, 0.55f, 0.7f * env * (1.0f - 0.7f * die), c0);
        gk_col(pal, pi + 2300, 0.45f, 0.65f * env, c1);
        for (int c = 0; c < NC525; c++) {
            drawbolt525(&g525, &b525[s][c], prog, h0, h1, cw, ch, sc, 1.8f, 6.5f, 0.5f);
            drawbolt525(&g525, &b525[s][c], prog, c0, c1, cw, ch, sc, 0.8f, 2.2f, 0.25f);
        }
        float hxs, hys, hc[3];
        proj525(hx525[s], hz525[s], cw, ch, &hxs, &hys);
        gk_col(pal, pi + 1000, 0.4f, 0.8f * env * (1.0f - die), hc);
        gk_dot(&g525, hxs, hys, hc, 3.0f * sc, 14.0f * sc, 0.6f);
    }
    /* cloud ceiling: sample fbm in plane coordinates for each grid cell */
    for (int y = 0; y < GH525; y++)
        for (int x = 0; x < GW525; x++) {
            float xx = ((float)x + 0.5f) / (float)GW525, yy = ((float)y + 0.5f) / (float)GH525;
            float v = 0.0f;
            if (yy < 0.55f) {
                float p = (0.55f - yy) / (0.5f * 0.45f);           /* = 1/(0.3+1.6z) */
                if (p < 0.08f) p = 0.08f;
                float z = (1.0f / p - 0.30f) / 1.6f;
                float px = (xx - 0.5f) / (0.55f * p);
                float d = 0.55f * gk_noise2(px * 3.0f + t * 0.0012f + (float)(seed & 255u), z * 3.0f + t * 0.0004f, 21u)
                        + 0.30f * gk_noise2(px * 6.3f + 5.0f, z * 6.3f + 1.0f, 22u)
                        + 0.15f * gk_noise2(px * 12.0f + 9.0f, z * 12.0f + 3.0f, 23u);
                float fadefar = 1.0f - gk_smooth((z - 1.2f) / 1.5f);
                float l = 0.0f;
                for (int s = 0; s < 2; s++) {
                    if (lit[s] <= 0.0f) continue;
                    float dx = (px - hx525[s]) / 0.5f, dz = (z - hz525[s]) / 0.45f;
                    l += lit[s] * expf(-(dx * dx + dz * dz));
                }
                v = fadefar * (0.05f + 0.30f * d * d + 0.30f * l * (0.4f + 0.8f * d));
            }
            grid525[y * GW525 + x] = v;
        }
    gk_lut_ramp(lut525, pal, base + 2600, 4000, 0.3f, 1.5f, 1.2f);
    gk_grid_fill(&g525, grid525, GW525, GH525, lut525);
    gk_present(&g525, fb, w, h);
}
