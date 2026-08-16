/* 603 Emblem Ghost — the JellyDazzle mark, but never pasted on.
 *
 * J: "incorporate this image somehow, but not as is — do an AI thing on it."
 *
 * So the logo is not drawn. It is used as a MASK through which the engine's own
 * light is allowed to pass, and the light is generated here by the same kind of
 * maths every other routine uses: interfering rings and a slow rotational
 * shear. The mark therefore has no colour of its own. It is made of whatever
 * the palette is doing at that moment, which means it belongs to the frame
 * instead of sitting on top of it, and it recolours with everything else.
 *
 * It also does not simply appear. It surfaces: the field runs on its own for a
 * while, the mark swims up out of it over a couple of seconds, holds only
 * briefly, and sinks back. Most of the time you would not know it was there —
 * which is the point of an easter egg.
 *
 * Three things keep it from reading as a sticker:
 *   - the mask is sampled through a slow rotation and breathing zoom, so it is
 *     never in the same place at the same size twice;
 *   - it is modulated by the underlying field rather than replacing it, so the
 *     rings show THROUGH the mark;
 *   - the edges are eaten by the field's own noise, so it dissolves rather
 *     than being cut out.
 */
#include "../engine/jellydazzle.h"
#include "_emblem.h"
#include <math.h>
#include <stddef.h>

#define P603_CYCLE 2600         /* ~43 s: surface, hold, sink, then rest */

static float e603_smooth(float x)
{
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;
    return x * x * (3.0f - 2.0f * x);
}

/* the mask, sampled through a rotation and zoom, with soft edges */
static float e603_mask(float u, float v, float rot, float zoom)
{
    float c = cosf(rot), s = sinf(rot);
    float x = (u * c - v * s) / zoom;
    float y = (u * s + v * c) / zoom;
    x = x * 0.5f + 0.5f; y = y * 0.5f + 0.5f;
    if (x < 0.0f || x >= 1.0f || y < 0.0f || y >= 1.0f) return 0.0f;
    int ix = (int)(x * (JD_EMB_N - 1)), iy = (int)(y * (JD_EMB_N - 1));
    return (float)jd_emblem[iy * JD_EMB_N + ix] * (1.0f / 255.0f);
}

void pattern_603(uint32_t *fb, int w, int h, int frame, int sl,
                 uint32_t seed, const uint32_t *pal)
{
    (void)sl;
    float t = (float)frame;
    /* where we are in the surface / hold / sink cycle */
    float ph = fmodf(t + (float)(seed & 2047), (float)P603_CYCLE) / (float)P603_CYCLE;
    float present;
    if      (ph < 0.06f) present = e603_smooth(ph / 0.06f);          /* rise  */
    else if (ph < 0.20f) present = 1.0f;                             /* hold  */
    else if (ph < 0.30f) present = e603_smooth((0.30f - ph) / 0.10f);/* sink  */
    else                 present = 0.0f;                             /* absent */

    float rot  = t * 0.0021f + (float)(seed & 63) * 0.1f;
    float zoom = 0.78f + 0.16f * sinf(t * 0.0032f);
    float cx = (float)w * 0.5f, cy = (float)h * 0.5f;
    float inv = 2.0f / (float)(w < h ? w : h);
    int base = (int)(seed & 8191u) + (int)(t * 1.7f);

    for (int y = 0; y < h; y++) {
        float v = ((float)y - cy) * inv;
        for (int x = 0; x < w; x++) {
            float u = ((float)x - cx) * inv;
            /* the field: interfering rings with a rotational shear — this is
             * what the mark is made OF, and it runs whether the mark is here
             * or not, so the pattern is never idle */
            float r = sqrtf(u * u + v * v);
            float a = atan2f(v, u);
            float f = sinf(r * 9.0f - t * 0.021f)
                    + sinf(r * 5.0f + a * 3.0f + t * 0.013f)
                    + sinf(a * 6.0f - t * 0.009f) * 0.6f;
            float lit = 0.5f + 0.22f * f;

            if (present > 0.0f) {
                float m = e603_mask(u, v, rot, zoom);
                /* the field eats the edges: where the field is dark the mark
                 * dissolves, so it never has a cut-out silhouette */
                float edge = 0.35f + 0.65f * (0.5f + 0.25f * f);
                lit += present * m * edge * 0.85f;
            }
            if (lit < 0.0f) lit = 0.0f;
            int idx = base + (int)(lit * 5200.0f);
            fb[(size_t)y * w + x] = pal[idx & JD_PAL_MASK];
        }
    }
}
