/* 529 Bolt Fountain — a source at the bottom centre throws discharges up
 * and out along parabolic arcs like a firework fountain in slow motion:
 * eight jet slots, each a jagged streak (stable jag along its parabola)
 * that climbs from the source, arcs over and falls back toward the ground
 * line over ~120 frames, its head bright and its trailing length dying
 * away behind it.  Every jet has its own hue and the hue slides along the
 * streak; the source glows with the drifting base hue.  Figure overlay in
 * the lower two thirds.  Repaint. */
#include "_trace509.h"

#define NJ529 12
#define P529 150
#define NP529 90

static gk g529;

void pattern_529(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g529, w, h);
    gk_clear(&g529);
    float cw = (float)g529.cw, ch = (float)g529.ch, sc = g529.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    float sx = cw * 0.5f, sy = ch * 0.9f;
    /* ground line and source */
    float gc[3];
    gk_col(pal, base + 4200, 0.1f, 0.07f, gc);
    gk_seg(&g529, 0.0f, sy, cw, sy, gc, 1.0f * sc, 3.0f * sc, 0.3f);
    for (int j = 0; j < NJ529; j++) {
        int ph = frame + j * (P529 / NJ529) + j * 9;
        int idx = ph / P529;
        float age = (float)(ph - idx * P529);
        uint32_t hs = (uint32_t)idx * 4001u + (uint32_t)j * 7717u + seed;
        float ang = GK_TAU * 0.25f + (gk_hash(hs + 1u) - 0.5f) * 1.3f;     /* from vertical */
        float v = (0.6f + 0.45f * gk_hash(hs + 2u)) * ch * 1.15f;         /* launch "speed" (path length scale) */
        float vx = cosf(ang) * v, vy = -sinf(ang) * v;
        float grav = ch * 1.15f;
        float head = age / 110.0f;                                        /* parameter of the head along the path */
        if (head > 1.25f) continue;
        float tailL = 0.7f;                                               /* trailing length in path parameter */
        int pi = base + (int)(gk_hash(hs + 3u) * 8000.0f);
        float lx = 0.0f, ly = 0.0f;
        int started = 0;
        for (int i = 0; i < NP529; i++) {
            float u = (float)i / (float)(NP529 - 1) * 1.25f;
            if (u > head) break;
            float d = head - u;
            if (d > tailL) continue;
            float px = sx + vx * u, py = sy + vy * u + 0.5f * grav * u * u;
            /* stable jag normal to the path */
            float tx = vx, ty = vy + grav * u, tl = sqrtf(tx * tx + ty * ty); if (tl < 1e-4f) tl = 1.0f;
            float jj = (gk_noise1((float)i * 0.45f, hs) - 0.5f) * 16.0f * sc + (gk_hash((uint32_t)i * 31u + hs) - 0.5f) * 3.0f * sc;
            px += -ty / tl * jj; py += tx / tl * jj;
            if (py > sy + 4.0f * sc) { started = 0; continue; }         /* below ground: gone */
            if (started) {
                float fade = 1.0f - d / tailL; fade *= fade;
                float amp = fade * gk_smooth(head / 0.08f) * (1.0f - gk_smooth((head - 0.9f) / 0.3f));
                int pj = pi + (int)(u * 2500.0f);
                float hc[3], c[3];
                gk_col(pal, pj + 700, 0.05f, 0.35f * amp, hc);
                gk_col(pal, pj, 0.5f, 0.6f * amp, c);
                gk_seg(&g529, lx, ly, px, py, hc, 2.2f * sc, 7.0f * sc, 0.5f);
                gk_seg(&g529, lx, ly, px, py, c, 1.0f * sc, 2.5f * sc, 0.25f);
            }
            lx = px; ly = py; started = 1;
        }
        if (started && head < 1.2f) {
            float tc[3];
            gk_col(pal, pi + 500, 0.55f, 0.9f * (1.0f - gk_smooth((head - 0.9f) / 0.3f)), tc);
            gk_dot(&g529, lx, ly, tc, 1.8f * sc, 7.0f * sc, 0.6f);
        }
    }
    float s0[3], s1[3];
    gk_col(pal, base, 0.5f, 0.9f, s0);
    gk_col(pal, base + 900, 0.1f, 0.35f, s1);
    gk_dot(&g529, sx, sy, s1, 8.0f * sc, 30.0f * sc, 0.6f);
    gk_dot(&g529, sx, sy, s0, 3.0f * sc, 10.0f * sc, 0.6f);
    gk_present(&g529, fb, w, h);
}
