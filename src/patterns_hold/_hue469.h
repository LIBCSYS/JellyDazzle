/* _hue469.h — hue kit for the lightning family (469..508).
 *
 * The base kit (_glow469.h) draws a bolt in ONE colour, which is how you get
 * the white stick nobody wants.  This layer styles a bolt so the hue WALKS
 * along the channel from root to tip, DRIFTS as the strike ages, and JITTERS
 * per segment — one bolt carrying several morphing hues at once, which is the
 * whole point of lightning in this engine.
 *
 * Everything is a thin wrapper over gk_seg: a wide desaturated halo pass laid
 * down first, then a tighter, more saturated body pass over it.  Radii are
 * already in sc-relative pixels when the caller builds the style, so the
 * transform's scale is applied to GEOMETRY only, never to stroke width.
 */
#ifndef JD_HUE469_H
#define JD_HUE469_H
#include "_glow469.h"

typedef struct {
    int   hspan;        /* palette steps swept from root to tip          */
    int   hdrift;       /* palette steps added as the strike ages        */
    int   hjit;         /* per-segment palette jitter                    */
    float wt;           /* body whiteness 0..1 (0 = full hue)            */
    float cr, gr, gi;   /* body core radius, glow radius, glow gain      */
    float hwt;          /* halo whiteness                                */
    float hamp;         /* halo amplitude, relative to the body          */
    float hcr, hgr, hgi;/* halo core radius, glow radius, glow gain      */
} hk_style;

static inline void hk_style_set(hk_style *s, int hspan, int hdrift, int hjit,
                                float wt, float cr, float gr, float gi,
                                float hwt, float hamp, float hcr, float hgr, float hgi)
{
    s->hspan = hspan; s->hdrift = hdrift; s->hjit = hjit;
    /* House rule: lightning may have a white-hot CORE, but it must never be
     * only white — the hue always has to read.  Patterns ask for up to 0.9
     * whiteness; cap it so at least a quarter of the body stays palette
     * colour, and keep the halo well under the body so the hue wraps it. */
    /* The canvas is additive with an exp tone map, so ANY wide bright pass
     * clips all three channels and reads white no matter what hue went in.
     * The first group is the wide glow, the second the tight core — so the
     * hue has to live in the WIDE pass (kept well below clipping) and the
     * white is allowed only in the small hot core. */
    if (wt > 0.72f) wt = 0.72f;
    if (hwt > 0.80f) hwt = 0.80f;
    s->wt = wt; s->cr = cr; s->gr = gr; s->gi = gi;
    s->hwt = hwt; s->hamp = hamp; s->hcr = hcr; s->hgr = hgr; s->hgi = hgi;
}

/* palette sample with the same contract as gk_col — kept as its own name so
 * lightning patterns read consistently and can be retuned independently. */
static inline void hk_col(const uint32_t *pal, int idx, float wt, float amp, float *out)
{
    gk_col(pal, idx, wt, amp, out);
}

/* one styled stroke.  u = 0..1 position along the channel (drives hue),
 * wgt = thickness weight (callers may pass a signed ramp), amp = brightness. */
static void hk_seg(gk *g, float x0, float y0, float x1, float y1,
                   float u, float wgt, float amp,
                   const uint32_t *pal, int pi, const hk_style *st)
{
    if (amp <= 0.0f) return;
    if (wgt < 0.0f) wgt = 0.0f; else if (wgt > 1.0f) wgt = 1.0f;
    float w = 0.35f + 0.65f * wgt;
    int idx = pi + (int)(u * (float)st->hspan);
    float c[3], hc[3];
    gk_col(pal, idx + st->hspan / 3, st->hwt, amp * st->hamp, hc);
    gk_seg(g, x0, y0, x1, y1, hc, st->hcr * w, st->hgr * w, st->hgi);
    gk_col(pal, idx, st->wt, amp, c);
    gk_seg(g, x0, y0, x1, y1, c, st->cr * w, st->gr * w, st->gi);
}

/* full bolt, transformed and styled.  mir = +1 or -1 mirrors about local x.
 * prog 0..1 grows the channel tip-first; segments past prog are not drawn. */
static void hk_bolt_xfm(gk *g, const gk_bolt *b, float ox, float oy, float rot,
                        float scl, float mir, float prog, float amp,
                        const uint32_t *pal, int pi, const hk_style *st)
{
    if (amp <= 0.0f || b->n <= 0) return;
    float cs = cosf(rot), sn = sinf(rot);
    int drift = (int)(prog * (float)st->hdrift);
    int i;
    for (i = 0; i < b->n; i++) {
        const gk_bseg *s = &b->s[i];
        if (s->t0 > prog) continue;                    /* not grown yet */
        float born = prog - s->t0;
        float a = amp * (born < 0.05f ? born * 20.0f : 1.0f);
        if (a <= 0.0f) continue;
        float ax = s->x0 * mir, ay = s->y0;
        float bx = s->x1 * mir, by = s->y1;
        float p0x = ox + (ax * cs - ay * sn) * scl;
        float p0y = oy + (ax * sn + ay * cs) * scl;
        float p1x = ox + (bx * cs - by * sn) * scl;
        float p1y = oy + (bx * sn + by * cs) * scl;
        int jit = st->hjit ? (int)(gk_hash((uint32_t)(i + 1) * 2654435761u)
                                   * (float)st->hjit) : 0;
        int idx = pi + (int)(s->t0 * (float)st->hspan) + drift + jit;
        float w = 0.35f + 0.65f * s->wgt;
        float c[3], hc[3];
        gk_col(pal, idx + st->hspan / 3, st->hwt, a * st->hamp, hc);
        gk_seg(g, p0x, p0y, p1x, p1y, hc, st->hcr * w, st->hgr * w, st->hgi);
        gk_col(pal, idx, st->wt, a, c);
        gk_seg(g, p0x, p0y, p1x, p1y, c, st->cr * w, st->gr * w, st->gi);
    }
}

static void hk_bolt_xf(gk *g, const gk_bolt *b, float ox, float oy, float rot,
                       float scl, float prog, float amp,
                       const uint32_t *pal, int pi, const hk_style *st)
{
    hk_bolt_xfm(g, b, ox, oy, rot, scl, 1.0f, prog, amp, pal, pi, st);
}

static void hk_bolt(gk *g, const gk_bolt *b, float prog, float amp,
                    const uint32_t *pal, int pi, const hk_style *st)
{
    hk_bolt_xfm(g, b, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, prog, amp, pal, pi, st);
}

/* n-fold rosette of the same bolt; mirror adds the reflected copy so the
 * figure closes symmetrically.  Hue is shared across folds on purpose — the
 * symmetry is what makes it read as one object. */
static void hk_kaleido(gk *g, const gk_bolt *b, float ox, float oy, float rot0,
                       float scl, int n, int mirror, float prog, float amp,
                       const uint32_t *pal, int pi, const hk_style *st)
{
    if (n < 1) n = 1;
    float step = GK_TAU / (float)n;
    int k;
    for (k = 0; k < n; k++) {
        float r = rot0 + step * (float)k;
        hk_bolt_xfm(g, b, ox, oy, r, scl, 1.0f, prog, amp, pal, pi, st);
        if (mirror) hk_bolt_xfm(g, b, ox, oy, r, scl, -1.0f, prog, amp, pal, pi, st);
    }
}

#endif /* JD_HUE469_H */
