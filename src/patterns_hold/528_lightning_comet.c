/* 528 Lightning Comet — two glowing heads sail slowly about the frame on
 * Lissajous-ish paths, each dragging a long jagged tail of discharge that
 * follows the path it has just travelled (a history of positions, jag
 * stable per history slot), brightest at the head and dissolving toward
 * the tail's end, with a few small sparks branching off it.  Head and tail
 * carry different hues per comet, tail hue shifting along its length,
 * all drifting with time.  Sparse overlay.  Repaint. */
#include "_trace509.h"

#define NH528 90
#define STEP528 3

static gk g528;
static float hx528[2][NH528], hy528[2][NH528];
static int hn528 = -1;
static uint32_t bs528 = 0xFFFFFFFFu;

static void head528(int c, float t, float cw, float ch, float *x, float *y)
{
    float sp = 0.0045f + 0.0012f * (float)c;
    *x = cw * (0.5f + 0.38f * sinf(t * sp * 1.0f + (float)c * 2.1f) * cosf(t * sp * 0.13f));
    *y = ch * (0.5f + 0.36f * sinf(t * sp * 1.31f + (float)c * 0.7f + 1.0f));
}

void pattern_528(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g528, w, h);
    gk_clear(&g528);
    float cw = (float)g528.cw, ch = (float)g528.ch, sc = g528.sc, t = (float)frame;
    int base = (int)(t * 1.4f) + (int)(seed & 8191u);
    /* history is rebuilt from the closed-form path, so any frame is exact */
    for (int c = 0; c < 2; c++) {
        head528(c, t, cw, ch, &hx528[c][0], &hy528[c][0]);
        float tq = floorf(t / (float)STEP528) * (float)STEP528;     /* samples sit still; a new one appears every STEP frames */
        for (int i = 1; i < NH528; i++)
            head528(c, tq - (float)(i * STEP528), cw, ch, &hx528[c][i], &hy528[c][i]);
    }
    hn528 = frame; bs528 = seed;
    for (int c = 0; c < 2; c++) {
        int pi = base + c * 4000;
        float lx = hx528[c][0], ly = hy528[c][0];
        for (int i = 1; i < NH528; i++) {
            float u = (float)i / (float)NH528;
            float fade = (1.0f - u); fade *= fade;
            /* stable jag keyed to absolute time of the slot */
            uint32_t key = (uint32_t)((int)floorf(t / (float)STEP528) - i) * 71u + (uint32_t)c * 917u + seed;
            float nx = -(hy528[c][i] - hy528[c][i - 1]), ny = hx528[c][i] - hx528[c][i - 1];
            float nl = sqrtf(nx * nx + ny * ny); if (nl < 1e-4f) nl = 1.0f;
            float j = (gk_hash(key) - 0.5f) * 18.0f * sc * (0.3f + u);
            float x = hx528[c][i] + nx / nl * j, y = hy528[c][i] + ny / nl * j;
            int pj = pi + (int)(u * 3000.0f);
            float hc[3], cc[3];
            gk_col(pal, pj + 700, 0.05f, 0.35f * fade, hc);
            gk_col(pal, pj, 0.45f * (1.0f - u), 0.6f * fade, cc);
            gk_seg(&g528, lx, ly, x, y, hc, 2.4f * sc * (0.5f + fade), 7.5f * sc * (0.5f + fade), 0.5f);
            gk_seg(&g528, lx, ly, x, y, cc, 1.1f * sc * (0.5f + fade), 2.6f * sc * (0.5f + fade), 0.25f);
            /* sparks off the tail */
            if (gk_hash(key + 5u) < 0.12f && u < 0.7f) {
                float a = atan2f(ny, nx) + (gk_hash(key + 6u) - 0.5f) * 1.2f;
                float len = (8.0f + 18.0f * gk_hash(key + 7u)) * sc * (1.0f - u);
                float side = gk_hash(key + 8u) < 0.5f ? -1.0f : 1.0f;
                float sx = x + cosf(a) * len * side, sy = y + sinf(a) * len * side;
                float sc0[3];
                gk_col(pal, pj + 1500, 0.3f, 0.45f * fade, sc0);
                gk_seg(&g528, x, y, sx, sy, sc0, 0.7f * sc, 2.5f * sc, 0.4f);
            }
            lx = x; ly = y;
        }
        float k0[3], k1[3];
        gk_col(pal, pi, 0.55f, 1.0f, k0);
        gk_col(pal, pi + 600, 0.15f, 0.4f, k1);
        gk_dot(&g528, hx528[c][0], hy528[c][0], k1, 8.0f * sc, 24.0f * sc, 0.6f);
        gk_dot(&g528, hx528[c][0], hy528[c][0], k0, 3.5f * sc, 10.0f * sc, 0.6f);
    }
    gk_present(&g528, fb, w, h);
}
