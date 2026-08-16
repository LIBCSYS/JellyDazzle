/* 518 Sheet Wash — sheet lightning reduced to pure colour: no cloud, no
 * bolt, only broad soft washes of light that swell across the frame and
 * fade, each a wide band at its own angle and offset, its own palette hue,
 * rising over ~90 frames and dying over ~150, three or four overlapping
 * at any time so the frame is a slow, breathing gradient of morphing
 * hues, gently textured by a drifting fbm veil.  Full-frame field.
 * Repaint. */
#include "_trace509.h"

#define GW518 48
#define GH518 36
#define NW518 4
#define P518 320

static gk g518;
static float grid518[GW518 * GH518 * 3];
static float fbm518[GW518 * GH518];

static void fill518(gk *g, const float *grid)
{
    int cw = g->cw, ch = g->ch, x, y;
    float sx = (float)(GW518 - 1) / (float)cw, sy = (float)(GH518 - 1) / (float)ch;
    for (y = 0; y < ch; y++) {
        float fy = ((float)y + 0.5f) * sy; int iy = (int)fy; if (iy >= GH518 - 1) iy = GH518 - 2;
        float uy = fy - (float)iy;
        const float *r0 = grid + iy * GW518 * 3, *r1 = r0 + GW518 * 3;
        float *row = g->acc + ((size_t)y * (size_t)cw) * 3;
        for (x = 0; x < cw; x++) {
            float fx = ((float)x + 0.5f) * sx; int ix = (int)fx; if (ix >= GW518 - 1) ix = GW518 - 2;
            float ux = fx - (float)ix;
            for (int c = 0; c < 3; c++) {
                float a = r0[ix * 3 + c] + (r0[ix * 3 + 3 + c] - r0[ix * 3 + c]) * ux;
                float b = r1[ix * 3 + c] + (r1[ix * 3 + 3 + c] - r1[ix * 3 + c]) * ux;
                row[x * 3 + c] += a + (b - a) * uy;
            }
        }
    }
}

void pattern_518(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    gk_setup(&g518, w, h);
    gk_clear(&g518);
    float t = (float)frame;
    int base = (int)(t * 1.1f) + (int)(seed & 8191u);
    gk_grid_fbm(fbm518, GW518, GH518, 0.09f, t * 0.0011f + (float)(seed & 255u), t * 0.0007f, 77u);
    memset(grid518, 0, sizeof grid518);
    for (int s = 0; s < NW518; s++) {
        int ph = frame + s * (P518 / NW518) + s * 23;
        int idx = ph / P518;
        float age = (float)(ph - idx * P518);
        float env = gk_env(age, 90.0f, 40.0f, 150.0f);
        if (env <= 0.0f) continue;
        uint32_t hs = (uint32_t)idx * 3571u + (uint32_t)s * 8887u + seed;
        float ang = gk_hash(hs + 1u) * GK_TAU;
        float off = gk_hash(hs + 2u) * 1.4f - 0.7f;                 /* band centre along the normal */
        float wid = 0.14f + 0.18f * gk_hash(hs + 3u);              /* band half-width            */
        float drift = (gk_hash(hs + 4u) - 0.5f) * 0.0006f * age;    /* the band slides very slowly */
        float ca = cosf(ang), sa = sinf(ang);
        int pi = base + (int)(gk_hash(hs + 5u) * 8000.0f);
        float c[3];
        gk_col(pal, pi, 0.10f, 0.34f * env, c);
        float c2[3];
        gk_col(pal, pi + 1200, 0.05f, 0.18f * env, c2);
        for (int y = 0; y < GH518; y++)
            for (int x = 0; x < GW518; x++) {
                float xx = ((float)x + 0.5f) / (float)GW518 - 0.5f, yy = ((float)y + 0.5f) / (float)GH518 - 0.5f;
                float d = (xx * ca + yy * sa - off - drift) / wid;
                float along = (-xx * sa + yy * ca);
                float v = expf(-d * d);
                float v2 = expf(-(d + 0.8f) * (d + 0.8f) * 0.7f) * (0.6f + 0.4f * sinf(along * 5.0f + t * 0.01f));
                float tex = 0.75f + 0.5f * fbm518[y * GW518 + x];
                float *o = grid518 + (y * GW518 + x) * 3;
                o[0] += (c[0] * v + c2[0] * v2) * tex;
                o[1] += (c[1] * v + c2[1] * v2) * tex;
                o[2] += (c[2] * v + c2[2] * v2) * tex;
            }
    }
    fill518(&g518, grid518);
    gk_present(&g518, fb, w, h);
}
