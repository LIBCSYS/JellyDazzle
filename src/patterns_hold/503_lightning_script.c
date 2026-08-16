/* 503 Lightning Script — a leader tip writes across the frame along a
 * slow Lissajous path, laying down a jagged luminous line behind it that
 * fades over a few seconds, like calligraphy in electricity.  Two pens of
 * different colour and rhythm.  Persistence canvas with decay; the tip
 * moves a few pixels a frame so nothing pops.  Sparse-to-figure overlay. */
#include "_hue469.h"

static gk g503;
static float px503[2], py503[2];
static int have503;
static uint32_t bs503 = 0xFFFFFFFFu;

void pattern_503(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g503, w, h);
    if (seed != bs503) { have503 = 0; bs503 = seed; }
    gk_decay_snap(&g503, 0.975f);
    float cw = (float)g503.cw, ch = (float)g503.ch, sc = g503.sc, t = (float)frame;
    int base = (int)(t * 1.6f) + (int)(seed & 8191u);
    float sd = (float)(seed & 1023u) * 0.01f;
    for (int p = 0; p < 2; p++) {
        float fa = p ? 0.0071f : 0.0053f, fb = p ? 0.0047f : 0.0067f;
        float x = cw * (0.5f + 0.40f * sinf(t * fa + sd + (float)p * 2.0f) * cosf(t * 0.0009f));
        float y = ch * (0.5f + 0.40f * sinf(t * fb + sd * 1.7f + 1.0f + (float)p));
        if (!have503) { px503[p] = x; py503[p] = y; }
        /* jag the stroke: offset the endpoint sideways by a stable hash of the frame */
        float dx = x - px503[p], dy = y - py503[p];
        float len = sqrtf(dx * dx + dy * dy) + 1e-4f;
        float nx = -dy / len, ny = dx / len;
        float j = (gk_hash((uint32_t)frame * 7u + (uint32_t)p * 131u + seed) - 0.5f) * 9.0f * sc;
        float mx = px503[p] + dx * 0.5f + nx * j, my = py503[p] + dy * 0.5f + ny * j;
        int pi = base + p * 4000 + (int)(t * 0.6f);
        float c[3], hc[3];
        hk_col(pal, pi + 700, 0.05f, 0.35f, hc);
        hk_col(pal, pi, 0.40f, 0.55f, c);
        gk_seg(&g503, px503[p], py503[p], mx, my, hc, 1.8f * sc, 6.0f * sc, 0.5f);
        gk_seg(&g503, mx, my, x, y, hc, 1.8f * sc, 6.0f * sc, 0.5f);
        gk_seg(&g503, px503[p], py503[p], mx, my, c, 0.8f * sc, 2.0f * sc, 0.25f);
        gk_seg(&g503, mx, my, x, y, c, 0.8f * sc, 2.0f * sc, 0.25f);
        px503[p] = x; py503[p] = y;
    }
    have503 = 1;
    /* present with the tips glowing on top (not persisted): draw, present,
     * subtract — cheaper: draw the tip into the canvas at a dose the decay
     * removes quickly */
    for (int p = 0; p < 2; p++) {
        float tc[3];
        hk_col(pal, base + p * 4000 + (int)(t * 0.6f), 0.5f, 0.25f, tc);
        gk_dot(&g503, px503[p], py503[p], tc, 1.5f * sc, 5.0f * sc, 0.6f);
    }
    gk_present(&g503, fb, w, h);
}
