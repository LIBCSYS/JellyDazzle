# 11 — Bloom, halo, and the resampler that is eating the accents

**Brief:** make the accent *separate itself* rather than out-shout the ground.
Everything here works by giving the accent a property the ground does not have.

**Headline finding, before any of that:** `layer_warp` is nearest-neighbour, and
overlays spawn with `tz` as low as 0.40. At `tz = 0.40` a one-pixel highlight
survives the resample with probability `tz² = 16%`, and **0.7% of the survivors
survive into the next frame**. The accents are not being out-shouted. They are
being *randomly deleted and re-drawn every frame*, which the eye integrates into
exactly the "foggy clutter" in the complaint. This is measured below, it is a
root cause nobody has raised, and the fix is **cheaper than the code it
replaces**.

---

## 0. Verdict up front

| Rank | Change | Δ cost / frame @ 3456×2160 | What it buys |
|---|---|---|---|
| **1** | **Reduce-then-warp** (2× per-channel MAX before minifying) | **−3.7 ms** (it is *faster*) | 1-px accent survival 16% → 64%; warp cost stops depending on rotation |
| **2** | **`span_rim`** — dark halo from the accent's own value, inside the existing blend | **+1.70 ms**, no new buffer | Separation on sparse/high-frequency accents. Does nothing on flat ones. |
| **3** | **Threshold bloom** (1/8 res, max-downsample, box blur, add) | **+5.57 ms** scalar, **+3.98 ms** with a 6-line NEON add | The accent becomes the only light source in frame. Works on everything. |

Budget: `main.c` measures **103 fps at 3456×2160 → 9.7 ms/frame**, vsync ceiling
16.67 ms, so **≈6.9 ms headroom**. Item 1 alone frees ~3.7 ms *per moving
overlay*, which is what pays for items 2 and 3. Ship 1 first, alone, and look at
it — it may be the whole fix.

---

## 1. The resampling question — this is the root cause

### 1.1 What the code does

`layer_warp` (compositor.c:1374) walks the destination in 16.16 fixed point:

```c
int32_t dxx = (int32_t)(cs * 65536.0f);      /* cs = cos(tr) / tz */
...
int ix = sx >> 16, iy = sy >> 16;
d[x] = src[(size_t)iy * w + ix];             /* one tap. no filter. */
sx += dxx; sy += dxy;
```

One tap per output pixel. That is correct and cheap when the layer is
**magnified** (`tz > 1`, step < 1 source px/dest px — every source pixel is read
at least once; it just looks blocky). It is **wrong** when the layer is
**minified** (`tz < 1`, step = `1/tz` > 1 source px/dest px): source pixels are
*skipped*, and nothing averages them back in.

Overlay spawn range (compositor.c:1908) and drift clamp (compositor.c:2479):

```c
L->tz = 0.40f + (float)(m & 1023) / 1023.0f * 2.10f;   /* 0.40 .. 2.50 */
if (L->tz < 0.34f) { L->tz = 0.34f; L->tz_v = -L->tz_v; }
```

So **28.6% of overlay tenancies spawn minified** (tz uniform on [0.40, 2.50],
fraction below 1.0 = 0.6/2.1), and drift takes them down to 0.34.

### 1.2 The survival probability

For an isolated 1-pixel highlight and stride `s = 1/tz`, each source column is hit
by at most one destination column, with probability `1/s = tz`. Two axes ⇒
**survival = tz²**. Measured against a synthetic accent (0.39% coverage of
1-px white sparks over a smooth field, warped at tr = 0.785 rad):

```
  tz    surviving  ratio   predicted tz²
  0.34      3379   11.6%   11.6%
  0.40      4629   15.9%   16.0%
  0.50      7382   25.3%   25.0%
  0.70     13667   46.9%   49.0%
  0.90     19642   67.4%   81.0%
  1.00     22195   76.1%  100.0%
  1.50     28123   96.4%  100.0%
```

The model is exact below 0.7. Above it the measurement runs *below* `tz²`
because rotation makes the stride irrational in both axes: **even at `tz = 1.00`
a rotated layer loses 24% of its single-pixel highlights.** There is no zoom
setting at which a rotated NN warp is lossless.

### 1.3 The part that actually causes the "fog"

`tz`, `tx`, `ty` and `tr` all drift *every frame* (compositor.c:2476). So the
sampling grid moves every frame, and which highlights survive is re-rolled every
frame. Measured at tz = 0.40 with one frame of typical drift
(`tz_v = 0.0019`, `tr_v = 0.0024`):

```
  frame N: 4629 sparks   frame N+1: 4683 sparks   persisting: 36  (0.8%)
```

**Ninety-nine per cent of the accent's highlights are different pixels next
frame.** That is the definition of temporal white noise. The eye cannot track
it, so it integrates it — and integrating a sparse random field over time gives
you a *uniform dim haze*. J's words were "a lot of foggy clutter where the accent
is barely visible." That is a literal description of a scintillating
undersampled highlight field.

Note what this does to the engine's own diagnostics: a layer whose highlights
churn every frame has a **high** measured `delta_q8`, so `mdel` reads it as busy
and `admissible()` treats it as motion. The engine believes the accent is alive.
It is aliasing.

### 1.4 The fix: dilate before you decimate

The standard answer to minification is mip-mapping or area-averaging. **Both are
wrong here.** A box average of a lone bright pixel over a 2×2 block divides it by
four — the transient survives *positionally* and dies *tonally*, which is the
disease, not the cure. The right operator for a bright sparse accent is a
**morphological maximum**: reduce 2×1 by taking the per-channel max. A 1-px spark
becomes a 1-px spark in a half-size buffer, at full brightness, and the warp then
samples that buffer at half the stride.

```c
/* MINIFY PATH.  layer_warp is nearest-neighbour with integer stepping: exactly
 * right magnified, exactly wrong minified.  At zoom tz the destination steps
 * 1/tz source pixels per output pixel, so a 1-px highlight survives with
 * probability tz^2 — 15.9% measured at the spawn floor of 0.40, 11.6% at the
 * drift clamp of 0.34.  And because the whole transform drifts every frame, the
 * SET of survivors is re-rolled every frame: 0.7% of one frame's surviving
 * sparks are still there in the next.  That is not an accent, it is temporal
 * white noise, and the eye integrates white noise into a flat grey haze.
 *
 * The cure is to DILATE BEFORE DECIMATING.  A 2x2 per-channel MAX keeps a lone
 * bright pixel at full brightness in a half-size buffer; a 2x2 AVERAGE would
 * divide it by four, which is the disease.  Per channel and not on the whole
 * u32, so a red spark beside a blue one does not become whichever word happened
 * to compare larger.
 *
 * It is also FASTER.  The half-size buffer is 7.5 MB instead of 30 and is
 * therefore largely cache resident, so the warp's random gather stops missing.
 * Measured at tz=0.90, tr=0.785: 8.31 ms direct, 0.79 + 3.80 = 4.59 ms through
 * the reduce, and the reduced path's cost stops depending on the rotation angle
 * at all (3.53 ms at tr=0 vs 3.86 ms at tr=2.5, against 3.53 vs 9.00 direct). */
static void layer_reduce2(const uint32_t *src, uint32_t *dst, int w, int h)
{
    int hw = w >> 1, hh = h >> 1;
    for (int y = 0; y < hh; y++) {
        const uint32_t *p = src + (size_t)(y * 2) * w, *q = p + w;
        uint32_t *d = dst + (size_t)y * hw;
        for (int x = 0; x < hw; x++) {
            uint32_t a = p[0], b = p[1], c = q[0], e = q[1], t;
            p += 2; q += 2;
            uint32_t r = a & 0x00FF0000u;
            t = b & 0x00FF0000u; if (t > r) r = t;
            t = c & 0x00FF0000u; if (t > r) r = t;
            t = e & 0x00FF0000u; if (t > r) r = t;
            uint32_t g = a & 0x0000FF00u;
            t = b & 0x0000FF00u; if (t > g) g = t;
            t = c & 0x0000FF00u; if (t > g) g = t;
            t = e & 0x0000FF00u; if (t > g) g = t;
            uint32_t bl = a & 0x000000FFu;
            t = b & 0x000000FFu; if (t > bl) bl = t;
            t = c & 0x000000FFu; if (t > bl) bl = t;
            t = e & 0x000000FFu; if (t > bl) bl = t;
            d[x] = 0xFF000000u | r | g | bl;
        }
    }
}
```

`layer_warp` takes the source's own dimensions. `k = sw/w` is 1.0 for a full-res
source, so **the existing path stays bit-identical**:

```c
static void layer_warp(const jd_layer *L, const uint32_t *src, int sw, int sh,
                       uint32_t *dst, int w, int h)
{
    float cx = (float)w * 0.5f, cy = (float)h * 0.5f;
    float iz = L->tz > 0.05f ? 1.0f / L->tz : 20.0f;
    float cs = cosf(L->tr) * iz, sn = sinf(L->tr) * iz;
    float ox = cx + L->tx * (float)w, oy = cy + L->ty * (float)h;
    float sx0 = (0.5f - ox) * cs - (0.5f - oy) * sn + cx;
    float sy0 = (0.5f - ox) * sn + (0.5f - oy) * cs + cy;
    /* Re-express the whole map in the SOURCE buffer's own coordinates.  k is
     * 1.0 when src is the layer's full-res buffer, so that path is unchanged
     * to the bit; it is 0.5 when src came from layer_reduce2. */
    float k = (float)sw / (float)w;
    sx0 *= k; sy0 *= k; cs *= k; sn *= k;
    int32_t dxx = (int32_t)(cs * 65536.0f), dxy = (int32_t)(sn * 65536.0f);
    int32_t dyx = (int32_t)(-sn * 65536.0f), dyy = (int32_t)(cs * 65536.0f);
    for (int y = 0; y < h; y++) {
        int32_t sx = (int32_t)((sx0 + dyx * (float)y / 65536.0f) * 65536.0f);
        int32_t sy = (int32_t)((sy0 + dyy * (float)y / 65536.0f) * 65536.0f);
        uint32_t *d = dst + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            int ix = sx >> 16, iy = sy >> 16;
            if ((unsigned)ix < (unsigned)sw && (unsigned)iy < (unsigned)sh)
                d[x] = src[(size_t)iy * sw + ix];
            else
                d[x] = src[(size_t)jd_mirror(iy, sh) * sw + jd_mirror(ix, sw)];
            sx += dxx; sy += dxy;
        }
    }
}
```

Call site, replacing the block at compositor.c:2605:

```c
if (L->moving) {
    static uint32_t *warp, *half; static int warp_n, half_n;
    if (warp_n < npix) { free(warp); warp = malloc((size_t)npix * 4);
                         warp_n = warp ? npix : 0; }
    if (warp) {
        const uint32_t *ws = L->buf; int sw = w, sh = h;
        /* Gate at 1.0, not at some smaller number: a stride just under 1 is
         * still a minify, and rotation makes it lossy anyway (76.1% survival
         * measured at tz=1.00).  Above 1.0 the layer is magnified and the
         * reduce would throw away detail the warp was going to keep. */
        if (L->tz < 1.0f) {
            int hn = npix / 4;
            if (half_n < hn) { free(half); half = malloc((size_t)hn * 4);
                               half_n = half ? hn : 0; }
            if (half) { layer_reduce2(L->buf, half, w, h);
                        ws = half; sw = w >> 1; sh = h >> 1; }
        }
        layer_warp(L, ws, sw, sh, warp, w, h);
        srcp = warp;
    }
}
```

Extra memory: one `npix/4` u32 scratch = **7.5 MB at 3456×2160**, shared across
all overlays, allocated once.

### 1.5 What it measures

Cost, one binary, min of 11 runs, 3456×2160, Apple M5, clang -O2, single thread:

| | full-res source | reduced source (incl. the 0.79 ms reduce) |
|---|---|---|
| tz 0.40, tr 0.785 | 3.97 ms | 4.33 ms |
| tz 0.60, tr 0.785 | 5.43 ms | 4.33 ms |
| **tz 0.90, tr 0.785** | **6.84 ms** | **4.33 ms** |
| tz 0.90, tr 0.100 | 9.62 ms | 4.50 ms |
| tz 0.90, tr 2.500 | 9.00 ms | 4.65 ms |
| tz 0.90, tr 0.000 | 3.53 ms | 4.32 ms |

Quality:

| | survives | persists N→N+1 |
|---|---|---|
| tz 0.40, direct | 16.0% | 0.7% |
| **tz 0.40, reduced** | **63.5%** | **3.3%** |
| tz 0.90, direct | 67.6% | 1.9% |
| **tz 0.90, reduced** | **267%** of the source spark count — the 2×2 max widens each spark, then the warp magnifies it | **7.8%** |

Two honest caveats:

- **Temporal persistence improves 4× but is still low** (3–8%). Reduce-then-warp
  fixes the *density* of the accent, not fully its *identity* from frame to
  frame. Individual sparks still scintillate; there are simply four times as
  many of them, and the field's density is now stable, which is what the eye
  reads. A genuinely stable minify needs area-correct sampling (a real mip
  chain, ~2 extra reduce levels and a trilinear tap) and costs more than
  everything else in this document combined. Not recommended.
- **A cheaper partial fix with literally zero cost**: raise the overlay spawn
  floor from 0.40 to 0.85 and the drift clamp from 0.34 to 0.80, for the SPARK
  and FIGURE slots only. Survival goes 16% → ~65%, no code, no memory. It costs
  you the "layer recedes into the distance" look for accents, which the mid/field
  slots can still carry. Do this in the same commit as a fallback if the reduce
  path is ever unavailable.

### 1.6 The other thing this found

`try_spawn` sets `L->moving = 1` for grounds too (compositor.c:1904) and gives
them a zoom and rotation drift — but the ground path in `jd_frame` is a `memcpy`
or a `span_lerp` and **never calls `layer_warp`**. Ground mobility is dead code.
Separately, the drift advance at compositor.c:2476 sits inside the
`routine >= JD_NASM` branch, so the 24 asm routines never advance their drift
even as overlays. Neither is a contrast bug, but both are worth a line.

---

## 2. Threshold bloom

### 2.1 Why it separates

A glow is not "brighter". It is a *spatially extended* signal centred on a
highlight, and the eye's interpretation of that is "light source". Raising the
accent's weight raises the accent and the local contrast ceiling together; a glow
raises only the accent, over an area that reads as one object. **The whole value
of the technique is that the ground is never sampled into the bloom buffer.**
Nothing else in the frame glows, so anything that does is read as separate
regardless of how bright the field behind it is.

### 2.2 Structure and the one non-textbook decision

- **Resolution 1/8 in each axis.** 432×270 at 3456×2160 = 116,640 px = 466 KB.
  Two buffers for the blur ping-pong = 933 KB. A bloom is low-pass by
  definition; full resolution buys nothing but cost.
- **Downsample by MAX, not by average.** This is the one place this document
  departs from the textbook. A canonical bright-pass averages, because bloom
  should conserve energy. Averaging a lone 1-px spark over an 8×8 tile divides it
  by 64 and it contributes nothing — precisely the transient we are rescuing.
  MAX makes a single spark bloom as hard as a solid highlight. That is not
  physical, and it is exactly what "nuanced flashes should read" means.
- **Extract from the warped accent, before the blend**, so bloom position matches
  what lands on screen and the ground can never contribute.
- **Accumulate several accents into one buffer** by max, so the blur and the
  add-back are paid once regardless of how many overlays are live.

### 2.3 Extraction

```c
/* ---- threshold bloom -------------------------------------------------
 * Give the accent one property the ground does not have: it glows.  Bright
 * pixels bleed light into their surroundings, and because the ground is never
 * sampled into this buffer nothing else in frame does that — so a glowing
 * accent reads as a light source over an arbitrarily bright field, which is
 * the thing raising its weight cannot do.
 *
 * MAX-downsample, not average.  The textbook bright-pass averages so the bloom
 * conserves energy; averaging a lone bright pixel over an 8x8 tile divides it
 * by 64 and the transient contributes nothing.  MAX makes a single spark bloom
 * as hard as a solid highlight.  Deliberately unphysical: transients are the
 * whole point. */
#define JD_BLOOM_DS 8

static uint32_t *g_bloom, *g_bloom2;   /* (w/8)*(h/8) each */
static int       g_bloom_n, g_bloom_hit;

/* Max-downsample everything above THR out of one warped accent layer, into the
 * shared buffer.  ACCUMULATES, so N accents cost N of these and still only one
 * blur and one add-back. */
static void bloom_gather(const uint32_t *src, int w, int h,
                         uint32_t weight, uint32_t thr)
{
    int bw = w / JD_BLOOM_DS, bh = h / JD_BLOOM_DS;
    for (int by = 0; by < bh; by++) {
        uint32_t *o = g_bloom + (size_t)by * bw;
        for (int bx = 0; bx < bw; bx++) {
            const uint32_t *p = src + (size_t)(by * JD_BLOOM_DS) * w
                                    + bx * JD_BLOOM_DS;
            uint32_t m = 0;
            /* Alpha is 0xFF on every pixel this engine produces, so comparing
             * the whole word compares R, then G, then B — a lexicographic
             * brightness costing ONE compare per source pixel instead of a
             * luma dot product.  It picks the wrong winner only between two
             * near-equally-bright pixels of different hue, which the blur two
             * steps later averages away regardless. */
            for (int j = 0; j < JD_BLOOM_DS; j++, p += w) {
                uint32_t a = p[0] > p[1] ? p[0] : p[1];
                uint32_t b = p[2] > p[3] ? p[2] : p[3];
                uint32_t c = p[4] > p[5] ? p[4] : p[5];
                uint32_t e = p[6] > p[7] ? p[6] : p[7];
                a = a > b ? a : b; c = c > e ? c : e; a = a > c ? a : c;
                if (a > m) m = a;
            }
            /* Threshold what will be ON SCREEN, not what is in the buffer: the
             * accent is about to be composited at `weight`, and an overlay at
             * peak 72/256 contributes barely a quarter of its own luma. */
            uint32_t l = ((((m >> 16) & 255) * 77 + ((m >> 8) & 255) * 151
                          + (m & 255) * 28) >> 8) * weight >> 8;
            if (l <= thr) continue;
            /* Soft knee, squared: contribution ramps from 0 at the threshold to
             * full at white.  A hard step would make a highlight drifting
             * across the threshold switch its glow on, and this engine's whole
             * motion law is that nothing switches. */
            uint32_t k = ((l - thr) << 8) / (256 - thr);
            k = (k * k) >> 8;
            uint32_t rb = (((m & 0x00FF00FFu) * k) >> 8) & 0x00FF00FFu;
            uint32_t g  = (((m & 0x0000FF00u) * k) >> 8) & 0x0000FF00u;
            uint32_t v  = rb | g;
            if (v > o[bx]) o[bx] = v;
            g_bloom_hit = 1;
        }
    }
}
```

`thr` should track the ground, not be a constant — the engine already maintains
`g_gluma`, the ground buffer's luma EWMA (compositor.c:2564). Set
`thr = g_gluma + 40`, clamped to [96, 220]: bloom only what is genuinely brighter
than the floor it is sitting on, so a dark ground gets a generous bloom and a
blazing one gets almost none. That is the coupling the other agents' ground work
should hook into.

### 2.4 Blur — separable box, running sum, two iterations

Radius 4 at 1/8 res = ±32 px of full-res support; two iterations give a triangle
kernel ≈ a Gaussian with σ ≈ 26 px. Running-sum makes cost independent of radius:
2 adds, 1 sub, 1 multiply-shift per pixel per axis.

```c
/* Box blur by running sum: cost is independent of R (2 adds, 1 sub, 1
 * multiply-shift per pixel per axis).  Two passes per axis turn the box into a
 * triangle, which at this scale is indistinguishable from a Gaussian and costs
 * a quarter of one.  RECIP is Q16 because an integer divide in this loop
 * measured 0.36 ms for two axes against 0.10 ms for the multiply. */
static void bloom_boxh(uint32_t *d, const uint32_t *s, int w, int h, int R)
{
    uint32_t recip = 65536u / (uint32_t)(2 * R + 1);
    for (int y = 0; y < h; y++) {
        const uint32_t *p = s + (size_t)y * w;
        uint32_t *q = d + (size_t)y * w;
        uint32_t sr = 0, sg = 0, sb = 0;
        for (int i = -R; i <= R; i++) {
            uint32_t c = p[i < 0 ? 0 : (i >= w ? w - 1 : i)];
            sr += (c >> 16) & 255; sg += (c >> 8) & 255; sb += c & 255;
        }
        for (int x = 0; x < w; x++) {
            q[x] = (((sr * recip) >> 16) << 16)
                 | (((sg * recip) >> 16) <<  8)
                 |  ((sb * recip) >> 16);
            uint32_t o = p[x - R < 0 ? 0 : x - R];
            uint32_t n = p[x + R + 1 >= w ? w - 1 : x + R + 1];
            sr += ((n >> 16) & 255) - ((o >> 16) & 255);
            sg += ((n >>  8) & 255) - ((o >>  8) & 255);
            sb +=  (n        & 255) -  (o        & 255);
        }
    }
}
/* bloom_boxv is the same walking columns; at 466 KB the whole buffer is L2
 * resident so the strided access costs nothing worth transposing for. */
```

### 2.5 Add-back

This is the expensive part and it is unavoidable: any technique that changes
every pixel of the finished frame pays one full-res read-modify-write, and the
measured floor for that on this machine is **0.50 ms**.

```c
/* Upsample and ADD.  Nearest-in-x would band vertically against linear-in-y, so
 * both axes interpolate — but incrementally: three Q16 accumulators stepped 8
 * times per bloom cell, no multiply in the inner run.  The gain is folded into
 * the row build so the add loop has nothing but a saturating add in it. */
static uint32_t g_brow[4096];                  /* one interpolated row */

static void bloom_add(uint32_t *dst, int w, int h, uint32_t gain)
{
    int bw = w / JD_BLOOM_DS, bh = h / JD_BLOOM_DS;
    for (int y = 0; y < h; y++) {
        int y0 = (y * bh) / h, ty = (((y * bh) % h) * 256) / h;
        int y1 = y0 + 1 < bh ? y0 + 1 : bh - 1;
        const uint32_t *a = g_bloom + (size_t)y0 * bw;
        const uint32_t *b = g_bloom + (size_t)y1 * bw;
        int pr, pg, pb;
        {   uint32_t ca = a[0], cb = b[0];
            pr = ((int)((ca >> 16) & 255) * (256 - ty)
                + (int)((cb >> 16) & 255) * ty) * (int)gain >> 8;
            pg = ((int)((ca >>  8) & 255) * (256 - ty)
                + (int)((cb >>  8) & 255) * ty) * (int)gain >> 8;
            pb = ((int)( ca        & 255) * (256 - ty)
                + (int)( cb        & 255) * ty) * (int)gain >> 8;   }
        for (int bx = 0, x = 0; bx < bw; bx++) {
            int nx = bx + 1 < bw ? bx + 1 : bx;
            uint32_t ca = a[nx], cb = b[nx];
            int nr = ((int)((ca >> 16) & 255) * (256 - ty)
                    + (int)((cb >> 16) & 255) * ty) * (int)gain >> 8;
            int ng = ((int)((ca >>  8) & 255) * (256 - ty)
                    + (int)((cb >>  8) & 255) * ty) * (int)gain >> 8;
            int nb = ((int)( ca        & 255) * (256 - ty)
                    + (int)( cb        & 255) * ty) * (int)gain >> 8;
            int sr = (nr - pr) >> 3, sg = (ng - pg) >> 3, sb = (nb - pb) >> 3;
            int cr = pr, cg = pg, cb2 = pb;
            for (int i = 0; i < JD_BLOOM_DS; i++, x++) {
                g_brow[x] = ((uint32_t)(cr >> 16) << 16)
                          | ((uint32_t)(cg >> 16) <<  8)
                          |  (uint32_t)(cb2 >> 16);
                cr += sr; cg += sg; cb2 += sb;
            }
            pr = nr; pg = ng; pb = nb;
        }
        /* Per-channel saturating add with the free bits above R and B.  Adding
         * two 0x00FF00FF-masked words lands each lane in its own 16-bit slot,
         * so the overflow bit is 0x01000100 and (m - (m>>8)) turns each set
         * overflow into a 0xFF in the lane below it. */
        uint32_t *d = dst + (size_t)y * w;
        for (int x = 0; x < w; x++) {
            uint32_t c = d[x], s = g_brow[x];
            uint32_t rb = (c & 0x00FF00FFu) + (s & 0x00FF00FFu);
            uint32_t m  = rb & 0x01000100u;
            rb = (rb | (m - (m >> 8))) & 0x00FF00FFu;
            uint32_t g  = (c & 0x0000FF00u) + (s & 0x0000FF00u);
            uint32_t m2 = g & 0x00010000u;
            g = (g | (m2 - (m2 >> 8))) & 0x0000FF00u;
            d[x] = 0xFF000000u | rb | g;
        }
    }
}
```

**The add loop is textbook NEON and this engine already ships ARM64 assembly.**
`vqaddq_u8` *is* per-channel saturating add; alpha comes back with one `vorrq`:

```c
    uint8x16_t amask = vreinterpretq_u8_u32(vdupq_n_u32(0xFF000000u));
    for (int x = 0; x + 4 <= w; x += 4) {
        uint8x16_t a = vld1q_u8((const uint8_t *)(d + x));
        uint8x16_t b = vld1q_u8((const uint8_t *)(g_brow + x));
        vst1q_u8((uint8_t *)(d + x), vorrq_u8(vqaddq_u8(a, b), amask));
    }
```

Measured: scalar add-back **3.86 ms** → NEON **2.48 ms**. Of that 2.48, the row
build is **1.85 ms** and is itself vectorizable; a fully vectorized add-back
should land near **1.2 ms**.

### 2.6 Wiring and cost

In the overlay loop, after the warp and before `blend_span`:

```c
if (s == 2 || s == 3)                 /* FIGURE and SPARK only — accents */
    bloom_gather(srcp, w, h, L->w_now, thr);
```

and after the loop, before the dead-air guard:

```c
if (g_bloom_hit) {
    bloom_boxh(g_bloom2, g_bloom, bw, bh, 4);
    bloom_boxv(g_bloom,  g_bloom2, bw, bh, 4);
    bloom_boxh(g_bloom2, g_bloom, bw, bh, 4);
    bloom_boxv(g_bloom,  g_bloom2, bw, bh, 4);
    bloom_add(fb, w, h, g_bloom_gain);       /* Q8, ~140-200 */
    memset(g_bloom, 0, (size_t)bw * bh * 4);
    g_bloom_hit = 0;
}
```

**Order matters:** it must be *before* the dead-air `span_gain` and the audio
bloom, so those two see the bloomed frame and do not fight it — otherwise a
successful bloom raises mean luma, the dead-air guard backs off its gain, and the
ground gets dimmer at exactly the moment the accent gets brighter, which is
actually a bonus but should be a deliberate one.

| Stage | ms @ 3456×2160 |
|---|---|
| `bloom_gather`, per accent layer | 0.80 |
| box blur R=4, 2 axes × 2 iterations (432×270) | 0.70 |
| `bloom_add` scalar | 3.86 |
| `bloom_add` NEON | 2.48 |
| memset 466 KB | 0.02 |
| **Total, 1 accent, scalar** | **5.38** |
| **Total, 2 accents, scalar** | **6.18** |
| **Total, 2 accents, NEON add** | **4.80** |

Memory: 933 KB.

**One thing I tried and it failed.** Fusing the bright-pass into `span_max` — so
the accent's buffer is read once instead of twice — measured **7.15 ms against
2.12 ms** for the plain blend. The per-8-pixel tile-max flush breaks the blend
loop's pipelining and the branch costs more than the extra streaming read it
saves. Keep the bright pass as its own pass; 0.80 ms of pure sequential read is
cheaper than anything clever.

---

## 3. The dark halo

### 3.1 The version that is genuinely cheap, and it is not the spatial one

The brief's intuition — "derive a halo from the accent's own coverage without a
full blur pass" — is right for sprites and **wrong for this engine**, for two
reasons:

1. **There is no matte.** These accents are procedural fields, not sprites with
   alpha. "Coverage" has to be *synthesized* by thresholding luma, which is the
   same full-res read the bloom's bright pass does. The saving evaporates.
2. **A dark halo cannot be drawn by a lighten-only blend.** `pick_blend` reaches
   for B_MAX, B_SCREEN and B_ADD for accents precisely because they are
   readable — and under all three, a *darker* source is a no-op. There is
   nowhere in the current pipeline for a darkening to land. A spatial halo needs
   its own full-res multiply pass over `fb`.

Costed honestly, the spatial halo is:

| Stage | ms |
|---|---|
| coverage extract at 1/8 (= the bloom's bright pass) | 0.80 |
| 3×3 max dilate at 1/8, ring = dilate − core | 0.21 |
| upsample + multiply `fb`, packed | ~2.50 |
| **Total** | **~3.51** |

Cheaper than bloom by 2 ms, but it *only darkens*, which is the same lever as
ground attenuation — the thing other agents already own. It adds a second,
uncoordinated hand on the same knob.

### 3.2 The one worth building: `span_rim`

Do the halo in the **value domain instead of the spatial domain**, inside the
blend that already runs. A procedural accent indexes its palette by a continuous
field, so a bright core is *spatially* surrounded by mid-values. Map mid-values
to "darken the destination" and high values to "lighten it" and you get a dark
rim around every bright core, spatially, for free — no matte, no extra pass, no
extra buffer.

```c
/* RIM (dark halo, value domain).  The compositing trick of ringing a bright
 * element in slight darkness so it separates from whatever is behind it — a
 * colourist's holdout matte, a comic's black linework.  Done spatially it needs
 * a matte these accents do not have and a full-res pass over fb.  Done in the
 * VALUE domain it is free: a procedural accent indexes a palette by a
 * continuous field, so a bright core is surrounded by mid values, and mapping
 * mid values to "attenuate the ground" puts a dark ring around every bright
 * core with no spatial work at all.
 *
 * g_rim[] is Q8 attenuation of the DESTINATION, indexed by the accent's green
 * channel — green is 59% of luma and costs a shift and a mask, where a proper
 * luma dot product measured +1.03 ms over the whole frame for a distinction
 * this curve cannot see.  It is 256 (no effect) below the band and back at 256
 * above it, because the core is about to be MAXed over the destination anyway.
 *
 * Note this REPLACES span_max at the call site.  It is not an extra pass. */
static void span_rim(uint32_t *dst, const uint32_t *src, int n,
                     uint32_t w, const uint16_t *rim)
{
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t k = rim[(s >> 8) & 255];               /* Q8, <= 256 */
        uint32_t drb = (((d & 0x00FF00FFu) * k) >> 8) & 0x00FF00FFu;
        uint32_t dg  = (((d & 0x0000FF00u) * k) >> 8) & 0x0000FF00u;
        uint32_t srb = (((s & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;
        uint32_t sg  = (((s & 0x0000FF00u) * w) >> 8) & 0x0000FF00u;
        uint32_t dr = drb & 0x00FF0000u, db = drb & 0xFFu;
        uint32_t sr = srb & 0x00FF0000u, sb = srb & 0xFFu;
        dst[i] = 0xFF000000u | (dr > sr ? dr : sr) | (dg > sg ? dg : sg)
                             | (db > sb ? db : sb);
    }
}
```

Built once at spawn, beside the existing `L->lut[256]`:

```c
/* Rim band: attenuate where the accent's value is in the shoulder BELOW its
 * highlights.  lo/hi are set from the layer's measured luma so the band tracks
 * what this routine actually paints rather than a fixed guess.  Depth is
 * deliberately small — 0.31 at the deepest — because a dark ring is read by the
 * eye long before it is noticed, and anything stronger reads as a drop shadow
 * pasted onto abstract imagery, which is the failure mode. */
static void rim_build(uint16_t *rim, int lo, int hi, uint32_t depth_q8)
{
    int mid = (lo + hi) >> 1, halfw = (hi - lo) >> 1;
    if (halfw < 1) halfw = 1;
    for (int v = 0; v < 256; v++) {
        int a = v - mid; if (a < 0) a = -a;
        int win = (v > lo && v < hi) ? ((halfw - a) * 256) / halfw : 0;
        rim[v] = (uint16_t)(256 - ((win * (int)depth_q8) >> 8));
    }
}
```

### 3.3 Honest assessment on abstract generative imagery

**It works where the accent has spatial structure, and it does nothing — or
something wrong — where it does not.**

- **Good:** sparse, high-frequency accents. A spark field, a stroke pattern, a
  filament. The band is narrow in value and therefore narrow in space, and you
  get a real ring, one to a few pixels wide, that reads exactly like comic
  linework.
- **Neutral:** hard-edged accents whose value jumps 0 → 255 with no shoulder.
  There are no pixels in the band, so `rim[]` returns 256 everywhere and the
  blend is bit-identical to `span_max`. Costs 1.70 ms for nothing.
- **Bad:** low-frequency, broad-gradient accents. A slow luminance ramp spends
  *most of its area* in the mid band, so the "rim" is not a ring — it is a large
  dark stain over half the frame. That reads as a vignette, or as the accent
  having dirty edges. **This is the real risk and it is not hypothetical**: many
  of the field/figure routines are smooth gradient fields.

The engine cannot currently tell these apart. `delta_q8` measures *temporal*
change; there is no spatial-frequency statistic anywhere in `jd_stat`. Two ways
out, in order of preference:

1. **Gate on `g_st[rt].dark`** — the fraction of near-black pixels, which is
   already measured in `stat_image`. A sparse accent is mostly dark. Enable
   `span_rim` only when `dark > 150` (roughly: the routine leaves 59% of the
   frame empty). That is a one-line gate on an existing statistic and it excludes
   the failure case by construction.
2. **Measure spatial frequency in the probe.** `stat_image` already walks the
   buffer; adding a mean absolute horizontal neighbour difference is a few
   instructions per sampled pixel and gives `jd_stat` the one number it is
   missing. Worth doing anyway — several other decisions in `pick_blend` would
   be better for having it.

Ship (1) now, do (2) when the probe is next touched.

---

## 4. Which is better here

| | Reduce-then-warp | `span_rim` | Threshold bloom |
|---|---|---|---|
| Δ cost / frame | **−3.7 ms** per moving overlay | +1.70 ms per accent | +4.8 to +6.2 ms |
| Extra memory | 7.5 MB | 512 B | 933 KB |
| Needs a matte | no | no | no |
| Works on smooth procedural fields | yes | **no — actively bad** | yes |
| Works on sparse spark fields | yes | yes | yes |
| Helps *transients* specifically | **yes, this is the whole point** | somewhat | yes |
| Risk of new artefacts | low (accents get 2 px chunkier when minified) | **medium** — vignette on broad accents | low (a bloom that is too strong reads as haze — cap the gain) |
| Interacts with other agents' ground work | no | yes — it darkens | yes — threshold should track `g_gluma` |

**Bloom beats the halo for this engine.** The reason is exactly the one the brief
suspected: the halo family assumes a matte, and these accents are procedural
patterns with no clean alpha. The spatial halo has to synthesize one and then
pay a full-res multiply anyway, and the value-domain version (`span_rim`), which
*is* cheap, has a real failure mode on the smooth gradient fields that make up a
large slice of this library. Bloom has no such dependency: it works on whatever
is bright, regardless of how the brightness is shaped.

**But neither of them is the first thing to do.** A bloom built on top of a
resampler that is deleting 84% of the accent's highlights blooms the 16% that
happened to survive this frame — a different 16% next frame. Fix the sampling
first and the bloom will have something coherent to work with. There is a real
chance that fixing the sampling is *sufficient*, because the failure mode it
produces (a scintillating sparse field integrating into flat haze) is a precise
match for the reported symptom, and the fix is free.

**Recommended order:**

1. **Reduce-then-warp.** Ship alone. Negative cost. Watch a spark-heavy segment.
2. If accents now read but still sit in the field: **threshold bloom**, gate the
   threshold on `g_gluma`, cap `g_bloom_gain` at 200/256 and ease it the way
   `g_gain` is eased so it can never step.
3. **`span_rim`**, gated on `dark > 150`, as a per-tenancy garnish on sparse
   accents only. Cheapest of the three but the narrowest, and the only one that
   can make things worse.

---

## Appendix — measurement conditions

Apple M5, 4 P-cores, 32 GB unified; `clang -O2 -mmacosx-version-min=11.0`,
single-threaded, 3456×2160 (7,464,960 px), minimum of 7–11 runs. Source is a
smooth sinusoidal field with 0.39% coverage of isolated 1-px white sparks —
deliberately the hardest case for a point sampler, and the case that matters.

| Kernel | ms |
|---|---|
| null full-res read+write (floor) | 0.50 |
| `memcpy` full frame | 0.61 |
| `span_max` (current accent blend) | 2.12 |
| `span_rim` (green-channel proxy) | 3.79 |
| `span_rim` (full luma dot product) | 4.82 |
| `layer_warp`, tr = 0, tz = 0.9 | 3.53 |
| `layer_warp`, rotated, tz = 0.9 | 8.31 – 9.62 |
| `layer_reduce2` (2× per-channel max) | 0.79 |
| `layer_warp` from reduced source | 3.53 – 3.86 |
| `bloom_gather` (bright pass, DS = 8) | 0.80 |
| box blur R = 4, 2 axes | 0.36 (integer divide) / 0.10 (Q16 reciprocal) |
| box blur R = 4, 2 axes × 2 iterations | 0.70 |
| `bloom_add`, row build only | 1.85 |
| `bloom_add`, scalar packed saturating | 3.86 |
| `bloom_add`, NEON `vqaddq_u8` | 2.48 |
| 3×3 dilate at 1/8 res | 0.21 |
| fused bright-pass inside `span_max` (rejected) | 7.15 |

Frame budget: `main.c:31` records 103 fps at this resolution = **9.7 ms of
9.7–16.67 ms**, so **≈6.9 ms of headroom** before vsync is missed.
