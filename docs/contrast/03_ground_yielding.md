# 03 — Ground Yielding

**JellyDazzle 3.0.4 → 3.0.5 · `src/engine/compositor.c`**
Design for the control law that makes the ground make room for the accent.

---

## 0. The problem, stated as a control problem

- **The ground has no mechanism to yield.** Every luma control in the engine pushes one way:
  - `JD_LUMA_FLOOR` (30) + `g_gain` — *lifts* a composite that is too dark.
  - `g_gluma < 20` — *retires* a ground whose own buffer went too dark.
  - `audio_surge` / `g_surge` — *raises* overlay weight; never lowers the ground.
  - The strobe detector — *turns down* a layer that jumps, but only on a fault.
- **Result:** ground draws at Q8 weight 256 (100%), full frame, every frame. Overlays cap at `PEAK_HI` = 180 / 140 / 115 Q8 (70% / 55% / 45%), and land nearer the `PEAK_LO`–`PEAK_HI` midpoints — 154 / 120 / 94 Q8, i.e. the 59% / 43% / 35% measured. A 35% spark composited over a 100% ground is arithmetically a *tint*, not a highlight.
- **The mechanism of the failure, precisely.** For a `B_MIX` accent at weight `w` over ground luma `G` with accent-buffer luma `A`, the on-screen delta is `d = (w/256)·(A − G)`. Ground and accent are indexed from *the same palette ramp* (`layer_pal_build`, shared `jd_palette`), so `A ≈ G` is the common case and `d ≈ 0`. The accent is not dim — it is **the same brightness as the thing behind it**, which is the definition of foggy clutter.
- **The fix is not to make the accent brighter.** Raising `w_peak` pushes overlays toward 256, which the file already calls out as "replaced the ground instead of enriching it". Raising accent palette brightness fights `g_gain` and the strobe guard. The only lever that increases separation without touching either is **the denominator**.

---

## 1. The signal

### 1.1 What it must not be

| Candidate | Why it fails |
|---|---|
| Composite luma | Ground-dominated (100% coverage vs ≤59%). It is already `g_gain`'s input — reusing it guarantees the two controllers fight over one measurement. |
| Accent absolute luma | Says nothing about legibility. A luma-40 spark on a luma-20 ground reads perfectly; a luma-90 spark on a luma-100 ground is invisible. The eye is a ratio detector (Weber), not a photometer. |
| Accent coverage alone | A 40%-coverage accent that already separates needs no help; a 3%-coverage accent that doesn't, does — but not at the cost of the whole frame. |
| Accent layer `w_now` | Weight is intent, not outcome. It ignores blend mode entirely: `B_MAX` at w=115 over a bright ground contributes literally nothing, and `w_now` cannot tell you that. |

### 1.2 What it is

**Signal = coverage-weighted separation deficit, measured on the finished pixels.**

This is exactly the pair of quantities a compositor works with when an element is not reading over a plate:

- **The matte** — *where* the element lives, and how much of the frame that is. Here: the fraction of sample taps where the overlay stack measurably lifts the pixel.
- **The delta** — *how far above its surround* the element sits at those pixels. Here: `composite_luma − ground_luma`, per tap, normalised against the ground under it.

A colourist protecting a highlight does not brighten the highlight; they pull the surround down until the ratio is right, and they do it over the *whole plate* so the pulldown reads as exposure rather than as a garbage matte. That is the law below.

### 1.3 Why it is measured as a difference, not derived

Sampling the **ground before overlays** and the **composite after overlays**, at the same taps, and taking `d = lc − gs`, gives the overlay contribution **as the eye receives it** — through `B_MIX`, `B_MAX`, `B_SCREEN`, `B_ADD`, `B_DIFF`, through `layer_warp`, through `g_surge`, through the envelope, through the per-layer `lut[]` and `gq` saturation gain. One subtraction replaces five blend-mode special cases and every future one. Nothing to keep in sync.

### 1.4 Weber, and why this works at all

- Perceived contrast is `Δ/L`. Attenuating the ground by factor `a` leaves `Δ` unchanged for `B_ADD`/`B_SCREEN`/`B_MAX` and **increases** it for `B_MIX` (`Δ = w(A − aG)/256` grows as `a` falls), while dividing the denominator by `a`. Separation therefore rises by **at least `1/a`**.
- The eye adapts to overall level; it does not adapt to local contrast. A ground held 20% down for ten seconds is not perceived as "darker" — adaptation absorbs it. The same 20% applied in ten frames is perceived as a *dip*.
- **That asymmetry is the entire time-constant argument.** The mechanism only works if the correction is slower than adaptation is fast. It is not a tuning preference; it is the precondition for the law being invisible.

---

## 2. The control law

### 2.1 Structure

```
                        (measured, this frame, upstream of every gain stage)
  g_L[0].buf ──┐
               ├─► 512-tap ground luma  lg[q] ──┐
  g_L[4].buf ──┘        (UNATTENUATED)          │
                                                ├─► deficit D  (Q8, 0..256)
  fb after overlays ─► 512-tap composite lc[q] ─┘
                                                        │
   g_gluma ──► authority gate  (feed-forward limit) ─────┤
   clum    ──► dead-air interlock (live backstop) ───────┤
                                                        ▼
                              want (Q8)  ──►  asymmetric single-pole, Q16 state
                                                        │
                                                        ▼
                                                   g_gyield (Q8)
                                                        │
                                        folded into the ground composite weights
```

### 2.2 Constants and their derivations

| Name | Value | Meaning | Derivation |
|---|---|---|---|
| `JD_GY_MIN` | 179 Q8 | Attenuation floor, 0.699× | §5 |
| `JD_GY_KSEP` | 96 Q8 | Required delta: 0.375 × ground | §2.3 |
| `JD_GY_DON` | 6 | Luma counts below which an overlay is not contributing | 6/255 = 2.4%, ≈2× the Weber floor for complex imagery. Below it the overlay is invisible and may not demand room. |
| `JD_GY_ATK` | 8 | Attack shift → τ = 256 f ≈ **4.3 s** | §2.4 |
| `JD_GY_REL` | 10 | Release shift → τ = 1024 f ≈ **17.1 s** | §2.4 |
| `JD_GY_SAFE` | 4 | Fault release → τ = 16 f ≈ **0.27 s** | §2.5 |
| `JD_GY_GUARD` | 45 | Composite luma interlock (pre-gain) | 1.5 × `JD_LUMA_FLOOR` |
| `JD_GY_GATE_LO` | 40 | `g_gluma` below this: zero authority | 2 × the `g_gluma < 20` handover trip |
| `JD_GY_GATE_HI` | 75 | `g_gluma` above this: full authority | `75 × 179/256 = 52.4 ≥ JD_GY_GUARD` — the level at which full depth is *provably* floor-safe |

### 2.3 `JD_GY_KSEP` = 96: where 0.375 comes from

Not a taste number — it is set by what the stack can actually achieve.

- Accent slot ceiling `PEAK_HI[2]` = 140 Q8 = 0.547.
- Healthy case: accent drawn from a brighter band of the ramp, `A ≈ 1.6G`.
- **Today**, at `a = 1.0`: `d = 0.547 × (1.6G − G) = 0.328G`. Requirement `0.375G`. **Misses.** This is the complaint, in one line of arithmetic.
- **With yield** at `a = 0.70`: requirement falls to `0.375 × 0.70G = 0.263G`; delta rises to `0.547 × (1.6G − 0.70G) = 0.492G`. Clears with 1.9× margin.
- `KSEP` is therefore chosen as the smallest ratio that today's stack *fails* and the yielded stack *passes* — a target the law can actually reach, not an aspiration. `1 + 0.375` = 0.46 stop of separation, which is the low end of the range at which an element reads as sitting in front of a plate rather than in it.

### 2.4 Attack and release: derived from a pump budget, not chosen

**The rule:** a controller pumps when its bandwidth overlaps the content's own rhythm. The fastest recurring accent event here is a beat — 30 frames at 120 BPM, and `g_surge` modulates overlay weight on exactly that period (its own attack is 4 f, release 16 f). So the yield loop must not respond meaningfully inside 30 frames, and must settle well inside a tenancy (`HOLD_LO[3]` = 500 f).

**Budget:** tolerable per-beat ripple in ground luma `r` = **1%**, the conservative end of the 1–2% Weber fraction for complex moving imagery.

**Arithmetic:**
- Full authority `A` = `256 − 179` = 77 Q8 counts.
- Beat-driven demand does not swing the full range: `g_surge` moves overlay weight by roughly ±20% within a bar, so demand `D` ripples ≈ ±0.15 of range → pulse amplitude `A_p ≈ 11.5` counts.
- First-order pulse response for `p ≪ τ`: `ripple ≈ A_p · p / τ`.
- Require `11.5 × 30 / τ ≤ 0.01 × 256 = 2.56` → **τ ≥ 135 frames.**
- Nearest power-of-two shift with margin: **`>> 8`, τ = 256 f.** Realised ripple `11.5 × 30 / 256 = 1.35` Q8 = **0.53% of ground luma** — half the budget.
- Worst case (demand swinging its *entire* range every beat, which requires the accent stack to appear and vanish on the beat): `77 × 30 / 256 = 9.0` counts = 3.5%. Visible, but that content does not exist — the envelope is `FADE_IN[3]` = 120 f minimum. Stated so the bound is known rather than assumed away.

**Release is 4× slower than attack (`>> 10`, τ = 1024 f ≈ 17 s), on purpose:**
- Dimming while the accent is present is the useful action. Restoring while it is briefly absent is cosmetic — and it is the *rise* half of a pump.
- Slow release makes the yield a **per-passage bias**, not a per-event gesture: it rides the programme, not the syllables. Same discipline as a broadcast limiter's release.
- 17 s is comparable to a ground tenancy (`HOLD_LO[0]`/`HOLD_HI[0]` = 1080–1640 f), so the yield carries smoothly *across* a handover rather than snapping at it — which is correct: the room a passage needs does not change because the ground did.
- 17 s is longer than any gap between spark tenancies, so gaps integrate out instead of producing a rise.

**Loop separation check** — this is the test that guarantees no interaction with `g_gain`:

| Loop | Attack τ | Release τ |
|---|---|---|
| `g_gain` (dead-air) | 32 f (`>>5`) | 8 f (`>>3`) |
| `g_gain` (audio bloom) | 8 f (`>>3`) | 32 f (`>>5`) |
| **Ground yield** | **256 f** | **1024 f** |

Yield is 8× slower than the slowest `g_gain` pole and 32× slower than the fastest. Cascaded-loop practice wants ≥5×; we have 8× minimum. `g_gain` settles fully before yield has moved 12% of a step. Necessary, and — with §4's ordering — sufficient.

### 2.5 The one fast path

`JD_GY_SAFE` = 4 (τ = 16 f, full restore in ~48 f = 0.8 s) engages **only when the pre-gain composite luma falls below `JD_GY_GUARD` (45)**, and it **only ever raises** `g_gy16`.

- Rationale: gain *reduction* eases in; gain *restoration on a fault* is immediate. Brightening is far more tolerable to the eye than dimming, and the alternative is dead air — which the engine already treats as its worst outcome.
- Honest cost: if this path fired often it would itself be a pump. It fires only when a ground has gone dark under an active yield, which the feed-forward gate is designed to prevent in the first place. It is the backstop, not the mechanism. Telemetry (§7) counts its firings; if it fires more than once a minute the gate is mistuned, not the release.

### 2.6 Fixed-point: the state must be Q16, not Q8

- With `>>8` on a Q8 error, any error under 256 counts shifts to **zero** — the integrator stalls. Full authority is only 77 counts, so a Q8 controller with these time constants **never moves at all**.
- The file's existing idiom (`+ ((want - g_gain + 31) >> 5)`) avoids the stall by forcing ≥1 count per frame. **Do not copy it at Q8 here:** 1 Q8 count/frame = 0.39%/frame traverses the whole 77-count range in 77 frames (1.3 s) — squarely inside the pump band. The idiom is right; the resolution is wrong.
- **Keep the state in Q16** (`g_gy16`, 65536 = 1.0) and derive the Q8 weight by `>>8`. The round-up idiom then becomes correct, because the forced minimum rate is 1/65536 per frame — below any threshold that exists.
- Max slew at Q16 with `>>8`: `19712 >> 8 = 77` Q16 = **0.30 Q8 counts/frame** = 0.117%/frame. Full range takes ≥256 frames by construction. **The shift is the rate limiter**; no separate slew clamp is needed.

---

## 3. The code

### 3.1 New span kernel — `span_lerp2` (near line 1274, replacing `span_lerp`)

```c
/* Two-weight lerp.  Split out of span_lerp so the ground composite can carry
 * an ATTENUATION in the weights it was already applying: with wa + wb < 256
 * the pair mixes AND scales in the same pass, which is how ground yielding
 * costs nothing per pixel.  Overflow: (c & 0x00FF00FF) * 256 = 0xFF00FF00,
 * and wa + wb <= 256 always, so the sum still fits u32 exactly — the same
 * bound the single-weight version already relied on. */
static void span_lerp2(uint32_t *dst, const uint32_t *a, const uint32_t *b,
                       int n, uint32_t wa, uint32_t wb)
{
    for (int i = 0; i < n; i++) {
        uint32_t ca = a[i], cb = b[i];
        uint32_t rb = (((ca & 0xFF00FFu) * wa + (cb & 0xFF00FFu) * wb) >> 8) & 0xFF00FFu;
        uint32_t g  = (((ca & 0x00FF00u) * wa + (cb & 0x00FF00u) * wb) >> 8) & 0x00FF00u;
        dst[i] = 0xFF000000u | rb | g;
    }
}

static void span_lerp(uint32_t *dst, const uint32_t *a, const uint32_t *b,
                      int n, uint32_t wb)
{
    span_lerp2(dst, a, b, n, 256 - wb, wb);
}
```

### 3.2 State and knobs (near `g_gluma`, line ~1456)

```c
/* ---- GROUND YIELDING (3.0.5) -------------------------------------------
 * Every luma control in this engine pushes one way.  JD_LUMA_FLOOR and
 * g_gain LIFT a dark frame; g_gluma RETIRES a dark ground.  Nothing ever
 * detected a ground that was too BRIGHT for what was drawn on top of it,
 * so a ground ran at 100% of the frame, 100% of the time, while the accent
 * that is meant to carry the picture peaked at 43%.  Ground and accent are
 * indexed from the same palette ramp, so the accent is frequently the SAME
 * luma as the thing behind it: not dim, unseparated.
 *
 * The law: measure how much of the frame carries accent energy that is
 * failing to clear the ground beneath it, and pull the ground down by that
 * much — globally, slowly enough that visual adaptation absorbs it, and
 * only as far as the dead-air machinery can provably tolerate.
 *
 * It is a FEED-FORWARD measurement with respect to every gain stage in the
 * frame: both taps are read upstream of the boot fade, of span_gain and of
 * the emblem, so the controller can never observe its own correction
 * reflected back through g_gain.  That, not the time constants, is what
 * makes it impossible for the two loops to oscillate.  See docs/contrast/
 * 03_ground_yielding.md for the derivations behind every number here. */
#define JD_GY_MIN     179   /* Q8 floor: 0.699x.  Three independent bounds
                             * agree within 4% at this value; see the doc. */
#define JD_GY_KSEP     96   /* Q8: the accent must clear 0.375 x the ground
                             * under it.  Chosen as the smallest ratio the
                             * unyielded stack fails and the yielded one
                             * passes -- reachable, not aspirational.      */
#define JD_GY_DON       6   /* luma counts: under this the overlay is not
                             * contributing anything the eye can resolve,
                             * so it does not get to ask for room.         */
#define JD_GY_ATK       8   /* attack shift: tau = 256 f ~ 4.3 s @60.
                             * Derived from a 1%-of-ground ripple budget
                             * against a 30-frame beat pulse; realised
                             * ripple is 0.53%, half the budget.           */
#define JD_GY_REL      10   /* release shift: tau = 1024 f ~ 17 s.  Four
                             * times the attack ON PURPOSE: dimming for a
                             * present accent is the useful act, restoring
                             * for an absent one is the rise half of a
                             * pump.  17 s makes this a per-PASSAGE bias
                             * that rides across a ground handover instead
                             * of snapping at it.                          */
#define JD_GY_SAFE      4   /* fault release: tau = 16 f, full in ~0.8 s.
                             * Only ever RAISES, and only when the frame is
                             * heading for the dead-air guard.  Gain
                             * reduction eases in; restoration on a fault
                             * does not wait.                              */
#define JD_GY_GUARD    45   /* pre-gain composite luma interlock = 1.5 x
                             * JD_LUMA_FLOOR.  Below it, yield is released
                             * at JD_GY_SAFE.  This is what makes "g_gain
                             * lifting" and "yield dimming" mutually
                             * exclusive by construction rather than by
                             * tuning.                                     */
#define JD_GY_GATE_LO  40   /* g_gluma at or under this: NO authority.  Two
                             * times the g_gluma < 20 handover trip -- a
                             * ground that close to being retired for
                             * darkness has no brightness to give.         */
#define JD_GY_GATE_HI  75   /* full authority.  75 * 179/256 = 52.4, which
                             * is above JD_GY_GUARD: at this luma the full
                             * depth is PROVABLY floor-safe.               */

static uint32_t g_gy16   = 65536;   /* controller state, Q16 (65536 = 1.0).
                                     * Q16 and not Q8 because a >>8 on a Q8
                                     * error under 256 counts shifts to
                                     * zero and the integrator stalls --
                                     * and full authority here is only 77
                                     * Q8 counts.  See the doc, section 2.6. */
static uint32_t g_gyield = 256;     /* Q8, what the composite actually uses */
static uint32_t g_gtap[512];        /* UNATTENUATED ground luma, this frame  */
static int      g_gtapn  = 0;       /* taps valid this frame                 */
static int      g_gy_on  = -1;      /* JD_GY=0 kills the law, for A/B         */
```

### 3.3 Sampler and controller (place just above `jd_frame`)

```c
/* Sample the ground on EXACTLY the lattice and with EXACTLY the luma weights
 * the dark-ground guard and the dead-air guard already use (npix/512 stride,
 * 77/151/28), and take it from the LAYER BUFFERS -- never from fb after the
 * yield has been applied.  The reference has to be the unattenuated ground
 * or the controller measures its own output and the loop closes on itself. */
static void gy_sample_ground(const int *gnd, int ng, uint32_t wb, int npix)
{
    int st = npix / 512; if (st < 1) st = 1;
    const uint32_t *a = g_L[gnd[0]].buf;
    const uint32_t *b = (ng == 2) ? g_L[gnd[1]].buf : NULL;
    for (int q = 0; q < 512; q++) {
        uint32_t c = a[q * st];
        uint32_t l = (((c >> 16) & 255) * 77 + ((c >> 8) & 255) * 151
                    + ( c        & 255) *  28) >> 8;
        if (b) {                       /* mirror the handover mix exactly */
            uint32_t c2 = b[q * st];
            uint32_t l2 = (((c2 >> 16) & 255) * 77 + ((c2 >> 8) & 255) * 151
                         + ( c2        & 255) *  28) >> 8;
            l = (l * (256 - wb) + l2 * wb) >> 8;
        }
        g_gtap[q] = l;
    }
    g_gtapn = 1;
}

/* Measure the finished composite against those taps and advance the
 * controller.  Called immediately after the overlay loop and BEFORE the boot
 * fade, span_gain and the emblem -- every one of which would contaminate the
 * measurement with a correction the controller must not see. */
static void gy_update(const uint32_t *fb, int npix, int frame)
{
    if (!g_gtapn) return;
    g_gtapn = 0;
    int st = npix / 512; if (st < 1) st = 1;

    uint32_t deficit = 0;        /* sum of per-tap Q8 shortfalls           */
    uint32_t clum    = 0;        /* composite luma, for the interlock      */
    uint32_t cov     = 0;        /* taps the overlays actually reach (TR)  */

    for (int q = 0; q < 512; q++) {
        uint32_t c  = fb[q * st];
        uint32_t lc = (((c >> 16) & 255) * 77 + ((c >> 8) & 255) * 151
                     + ( c        & 255) *  28) >> 8;
        clum += lc;

        /* the ground AS COMPOSITED -- what the overlay actually had to beat */
        uint32_t gs = (g_gtap[q] * g_gyield) >> 8;

        /* One subtraction stands in for every blend mode, the warp, the
         * surge, the envelope, the per-layer lut and the saturation gain:
         * this is the overlay's contribution as the EYE receives it, not as
         * the scheduler intended it.  Nothing to keep in sync when a sixth
         * blend mode is added. */
        if (lc <= gs + JD_GY_DON) continue;      /* nothing readable here   */
        cov++;
        uint32_t d = lc - gs;

        /* Required delta is measured against the UNATTENUATED ground: the
         * target is a property of the content, not of how far the controller
         * has already pulled.  Otherwise the goalposts move with the output
         * and the law walks itself to the floor. */
        uint32_t need = (g_gtap[q] * JD_GY_KSEP) >> 8;
        if (!need || d >= need) continue;        /* separates already       */
        deficit += ((need - d) << 8) / need;     /* Q8 shortfall, 0..256    */
    }
    clum /= 512;

    /* COVERAGE ENTERS HERE, and it enters by DIVISION over ALL 512 taps
     * rather than over the failing ones.  One failing tap in 512 yields
     * D < 1 and the ground does not move; 200 failing taps yield real
     * demand.  That single choice is what stops a lone sprite from pulling
     * the whole frame down -- which would be worse than the bug. */
    uint32_t D = deficit >> 9;
    if (D > 256) D = 256;

    /* AUTHORITY GATE (feed-forward).  How much the ground is ALLOWED to give
     * is a function of how much it HAS.  g_gluma is the ground's own luma
     * EWMA -- the same number the dark-ground guard retires a ground on -- so
     * gating on it means the yield can never walk a ground toward the
     * g_gluma < 20 handover trip.  It structurally cannot anyway (g_gluma is
     * computed from g_L[0].buf, upstream of everything here), but a control
     * law that merely happens not to reach a cliff is not the same as one
     * that is forbidden to. */
    uint32_t gate = 0;
    if (g_gluma > JD_GY_GATE_LO) {
        gate = ((g_gluma - JD_GY_GATE_LO) << 8) / (JD_GY_GATE_HI - JD_GY_GATE_LO);
        if (gate > 256) gate = 256;
    }
    uint32_t depth = ((256u - JD_GY_MIN) * gate) >> 8;     /* counts on offer */
    uint32_t want  = 256u - ((depth * D) >> 8);
    if (want < JD_GY_MIN) want = JD_GY_MIN;
    uint32_t w16 = want << 8;

    /* ---- the controller ---- */
    if (clum < JD_GY_GUARD) {
        /* INTERLOCK.  The frame is inside the dead-air guard's guard band.
         * Hand the brightness back at JD_GY_SAFE and do not take any more
         * until it recovers.  This branch only ever RAISES: it is the reason
         * "g_gain > 260" and "g_gyield < 256" cannot both hold, which is the
         * one invariant that keeps the two loops from fighting. */
        g_gy16 += (65536u - g_gy16) >> JD_GY_SAFE;
        g_gy_fault++;
    } else if (w16 < g_gy16) {
        /* ATTACK.  Round-up so the integrator cannot stall.  Safe to force a
         * minimum step HERE and not at Q8: one Q16 count per frame is
         * 1/65536, which no eye and no instrument in this program can see. */
        g_gy16 -= (g_gy16 - w16 + ((1u << JD_GY_ATK) - 1)) >> JD_GY_ATK;
    } else {
        g_gy16 += (w16 - g_gy16 + ((1u << JD_GY_REL) - 1)) >> JD_GY_REL;
    }

    if (g_gy16 > 65536u)                     g_gy16 = 65536u;
    if (g_gy16 < ((uint32_t)JD_GY_MIN << 8)) g_gy16 = (uint32_t)JD_GY_MIN << 8;
    g_gyield = g_gy16 >> 8;
    if (!g_gy_on) g_gyield = 256;            /* JD_GY=0: measure, do nothing */

    if ((frame & 127) == 0)
        TR("GY f=%d D=%u cov=%u gluma=%u gate=%u want=%u y=%u clum=%u\n",
           frame, D, cov, g_gluma, gate, want, g_gyield, clum);
}
```

(`static uint32_t g_gy_fault = 0;` alongside the other state; `g_gy_on` initialised once in `engine_init` from `getenv("JD_GY")`, defaulting to 1.)

### 3.4 Application — replace the ground composite block (line ~2585)

```c
    /* ---- ground: normalized, so the frame never pulses ---- */
    /* GROUND YIELDING is applied HERE, folded into the weights this block was
     * already applying.  The single-ground path was a memcpy; span_scale is a
     * memcpy when w >= 256, so an idle yield is byte-identical to 3.0.4 and
     * free.  The handover path was already a weighted sum; carrying the
     * attenuation in wa and wb costs two multiplies per FRAME and nothing per
     * pixel.  A separate span_scale pass over 7.46 Mpx would have cost 0.5-0.7
     * ms -- 6% of the 10.5 ms render+blend budget -- to do the same thing. */
    if (ng == 2) {
        uint64_t a = g_L[gnd[0]].w16, b = g_L[gnd[1]].w16;
        if (!(a + b)) {
            gy_sample_ground(gnd, 1, 0, npix);
            span_scale(fb, g_L[gnd[0]].buf, npix, g_gyield);
        } else {
            uint32_t wb = (uint32_t)(b * 256 / (a + b));
            gy_sample_ground(gnd, 2, wb, npix);
            span_lerp2(fb, g_L[gnd[0]].buf, g_L[gnd[1]].buf, npix,
                       ((256 - wb) * g_gyield) >> 8, (wb * g_gyield) >> 8);
        }
    } else {
        gy_sample_ground(gnd, 1, 0, npix);
        span_scale(fb, g_L[gnd[0]].buf, npix, g_gyield);
    }

    /* ---- overlays, bottom up ---- */
    for (int k = 0; k < no; k++) {
        /* ... unchanged ... */
    }

    /* GROUND YIELDING: measure and advance the controller.  This call must
     * stay exactly here -- after the overlays, before the boot fade, before
     * span_gain, before the emblem.  Move it below any of those and the
     * controller starts observing corrections it did not make, the loop
     * closes through g_gain, and the picture breathes on a ~10 s period.
     * That is the failure this ordering exists to prevent. */
    gy_update(fb, npix, frame);
```

### 3.5 Resize

Add `g_gtapn = 0;` to the resize branch alongside `g_sig_n = 0;` — the taps were sampled at the old stride and one frame of mismatched demand is pointless when it is free to discard.

---

## 4. Interaction with the existing guards

### 4.1 `g_gain` — the ordering, and the invariant

**The loop is opened structurally, not damped.**

| Stage | Reads | Writes | Sees the yield? |
|---|---|---|---|
| Layer render | — | `L->buf` | No |
| Strobe detector | `L->buf` | `frozen`, `w_peak`, `t_out` | **No** |
| Dark-ground guard | `g_L[0].buf` | `g_gluma`, `t_out` | **No** |
| Audition guard | both ground bufs | `SHADOW.live` | **No** |
| Ground composite | `L->buf` | `fb` | applies it |
| Overlays | `fb`, `L->buf` | `fb` | inherits it |
| **`gy_update`** | `fb`, `g_gtap` | `g_gy16` | measures it (see 4.2) |
| Boot fade | `fb` | `fb` | downstream |
| **Dead-air `g_gain`** | `fb` | `fb` | downstream |
| Audio bloom `g_gain` | `fb` | `fb` | downstream |
| Emblem / HUD | `fb` | `fb` | downstream |

- `gy_update` sits **above every gain stage**. `g_gain` is not in the yield loop at any point. There is no path from `g_gyield` → `g_gain` → yield measurement. A first-order lag on an open loop cannot oscillate; it can only lag.
- **The invariant:** `g_gain > 260` and `g_gyield < 256` must never both hold.
  - `g_gain` only rises above 256 when composite luma < `JD_LUMA_FLOOR` (30).
  - `g_gyield` only descends while pre-gain composite luma ≥ `JD_GY_GUARD` (45), and is released to 256 in ~0.8 s once it is not.
  - The 15-count band between 30 and 45, combined with the 32:1 speed ratio, means yield is already retreating before `g_gain` has begun to move.
  - Worth asserting under `JD_INSTR`: `assert(!(g_gain > 260 && g_gyield < 256))`.

### 4.2 The one loop that does close, and why it is a feature

`d = lc − gs` for a `B_MIX` overlay depends on the attenuated ground: `d = (w/256)(A − aG)`. So attenuating **raises** the measured delta, which **lowers** demand, which **relaxes** the yield. That is negative feedback, and it is the servo doing its job — the law stops dimming the instant separation is achieved rather than driving to the floor.

Stability of that loop:
- **Single dominant pole** (τ = 256 f). The only other pole in the path is the 1-frame measurement delay — 0.4° of phase at the loop's own bandwidth. Irrelevant.
- **Incremental loop gain < 1.** The output is bounded to `(256 − 179)/256 = 0.30` of the reference, and demand is monotone-decreasing in separation, so `|∂D/∂a · ∂a/∂D| < 0.30` at any operating point.
- One pole, loop gain 0.30, negative sign: unconditionally stable, settles in 3τ ≈ 13 s, no overshoot, no limit cycle. Contrast with what you get if you sample after `span_gain`: two integrators (τ 256 and τ 32) with a sign inversion around them — an oscillator with a ~10 s period. That is the "breathing" this design exists to avoid, and it is one misplaced line away.

### 4.3 The strobe detector — immune, and the trap

- The detector diffs `L->buf` against `g_lsig[s]` at 512 taps. The yield never touches `L->buf`. Immune.
- **The trap, and it is a bad one.** If anyone ever "optimises" this by scaling the ground layer buffer in place instead of at composite time, two catastrophes fire simultaneously:
  1. During the 256-frame attack ramp, the ground buffer changes every frame by the ramp *in addition to* its own animation. `dq8` inflates. If it crosses 4096 the ground is declared a strobe: `strikes++`, then `frozen = 1`, and its handover is pulled to `frame + 60`. **The ground stops animating and is retired early — because it was asked to dim.**
  2. `g_gluma` is computed from the same buffer. A 30% attenuation on a nominal ground takes `g_gluma` from 60 to 42, and on a ground already at 30 takes it to 21 — one count above the `g_gluma < 20` trip. Grounds start handing over 4 s early, continuously, and the engine churns.
- Both are silent — no crash, no log, just a show that never settles. **`L->buf` is read-only to this law. Non-negotiable.**

### 4.4 The dark-ground handover guard — where the gate is blind

- `g_gluma` is only updated when `g_L[0].live && !g_L[JD_SHADOW].live && g_L[0].sl > 60`.
- **Therefore `g_gluma` is stale for the entire duration of a handover** — it holds the *outgoing* ground's value through all 180 frames of the crossfade. If the incoming ground is much darker, the feed-forward gate is running on the wrong number for 3 s.
- This is the single place the gate cannot protect the frame, and it is exactly why the live `clum < JD_GY_GUARD` interlock exists. The two are deliberately redundant: the gate is the cheap feed-forward limit, the interlock is the exact live backstop.
- After handover, `g_gluma = 60` is pinned for 60 frames (`sl <= 60`), giving gate = `(60−40)/35` = 57% authority for one second before it tracks the real ground. Conservative in the right direction.

### 4.5 `motion_probe` / `g_motion`

- `motion_probe` reads `fb`, so it sees the yield's drift. Max slew 0.30 Q8 counts/frame on a channel value of ~130 = **0.15 per channel per frame**, against a `g_motion > 5.5` jitter threshold — **2.7% of threshold**, and only at full slew, which lasts a few frames per attack.
- Bias direction: upward, i.e. very slightly more likely to blame a layer for jitter. Below the noise floor of a 0.9/0.1 EWMA. No action needed; recorded so it is not rediscovered as a mystery.

### 4.6 `B_DIFF` overlays

- `B_DIFF` can make a pixel *darker* than the ground. `lc ≤ gs + DON` skips it, so a difference-blend accent registers zero coverage and demands nothing.
- **This is correct, not a gap.** An accent that reads by darkening is helped by a *brighter* ground, not a dimmer one. The law declines to act rather than acting backwards. Noted as a known limitation: `B_DIFF` accents get no assistance from this law and need a separate one if they turn out to matter.

---

## 5. The floor: `JD_GY_MIN` = 179 (0.699×)

Three independent bounds, computed separately, agreeing within 4%. The tightest binds.

| # | Bound | Value | Reasoning |
|---|---|---|---|
| 1 | **Dead-air separation** | `a ≥ 0.70` | The yield must never be able to provoke `g_gain`. The `JD_GY_GATE_HI` case: a ground at `g_gluma` = 75 attenuated by `a` must stay above `JD_GY_GUARD` = 45, which is itself 1.5× `JD_LUMA_FLOOR` = 30. `75a ≥ 45` → **`a ≥ 0.60`**. Add margin for the sparse-tap estimator's variance (512 taps on 7.46 Mpx; ±3 luma at 1σ on typical content) → **`a ≥ 0.70`**. |
| 2 | **Perceptual** | `a ≥ 0.70` | 0.70× is **−0.51 stop**. Below roughly −0.7 stop the change stops reading as "the accent came forward" and starts reading as "the picture got darker", which is the failure J explicitly forbade. −0.5 stop is inside that, with the separation gain (≥1.43×, and 1.9× for `B_MIX` per §2.3) sitting where it does the work. |
| 3 | **Quantisation** | `a ≥ 0.25` | `span_scale`/`span_lerp2` are Q8 per channel. At `w` = 179 the ground's 256 input levels map onto 179 output levels — under 1 LSB of loss, no banding in the smooth gradients these patterns are full of. Banding on a slow ramp needs the level count under ~64, i.e. `w` < 64. **Not binding** — stated so the real limit is on record rather than assumed. |

**Chosen: `JD_GY_MIN` = 179 Q8 = 0.699×.** Bounds 1 and 2 agree; bound 3 is 2.8× looser and would only matter if this were ever deepened.

**What that buys, by ground brightness — note the law is self-targeting:**

| `g_gluma` | Gate | Max attenuation | Contrast gain (min) | `B_MIX` delta gain |
|---|---|---|---|---|
| ≤ 40 | 0% | 1.000× | 1.00 | 1.00 |
| 50 | 29% | 0.914× | 1.09 | 1.15 |
| 60 (nominal) | 57% | 0.828× | 1.21 | 1.29 |
| 75+ (the complaint) | 100% | 0.699× | **1.43** | **1.50** |

The grounds that get pulled hardest are precisely the bright, busy ones J is complaining about. Dark and moody grounds — "approved content", in the file's own words — are untouched.

---

## 6. Global versus local

**Verdict: global. A per-pixel mask is not affordable, not resolvable, and not artistically right — in that order of certainty.**

### 6.1 Cost, at 3456×2160 = 7,464,960 px

| Stage | Traffic | Realistic M5 cost |
|---|---|---|
| Build mask from accent buffers (luma + threshold) | 30 MB read, 7.5 MB write | 0.3–0.4 ms |
| **Blur the mask** — separable box ×3 at r ≈ 110–220 px (5–10% of frame height) | ~6 passes × ~15 MB each way ≈ 90 MB, and the vertical passes are cache-hostile | **1.5–2.5 ms** |
| Apply: 3-input per-pixel op instead of 2-input | +7.5 MB read, and it displaces the *free* fold into `span_lerp2` | 0.3–0.5 ms |
| **Total** | | **2.1–3.4 ms** |

- `BUDGET_Q8` is 10.5 ms for render+blend; `g_ewma_ms` idles at 6.0 and the engine goes `g_hot` above 13.5.
- Spending 2–3.4 ms is **20–32% of the entire render+blend budget**. The admission test (`cost > BUDGET_Q8`) would start refusing layers, and `g_hot` would engage half-rate decimation.
- **You would be deleting sprites in order to make sprites visible.** That is the whole argument.
- The blur cannot be skipped: a hard mask edge is a visible rectangle around every accent, which is a worse artefact than the one being fixed.

### 6.2 Downscaled mask — cheap enough, and useless

- Build and blur at 1/8 linear (432×270 = 117k px): ~0.05 ms. Bilinear upsample during application: ~0.3 ms. **Affordable.**
- **But it cannot see the thing it is for.** A "nuanced flash" 20 px across is 2.5 px in the mask; after the blur required to avoid upsample blocking, it is gone. The mask would resolve only large accent *masses* — which are exactly the ones that already read.
- The cheap version helps the case that does not need help and misses the case that does.

### 6.3 The artistic objection, which stands regardless of cost

- A local pulldown around every sprite is a **halo**. It is large-radius unsharp masking, and its characteristic failure — grey, muddy, ring-bounded midtones — *is* "foggy clutter". Local dimming risks producing J's complaint by a different route.
- Compositors do local pulldowns with a matte **tied to the element's own geometry**. This compositor has no such matte: layers are finished RGB buffers with no alpha. Deriving one by thresholding luma is a chroma key with no key colour — it will grab bright *ground* pixels wherever they exceed the threshold, and pull the ground down under itself.
- Global attenuation reads as **exposure**, which is a thing pictures do. Local attenuation reads as **a process**, which is a thing pictures should not visibly do.

### 6.4 Two middle paths, named so they are on record

- **Coarse spatial bias (available, not recommended).** Bin the existing 512 taps into a 3×3 grid, produce 9 demand scalars, apply a bilinearly-interpolated 3×3 attenuation field. Essentially free — 9 extra accumulators and a 4-tap weight lerp per pixel. Risk: a 3×3 field over a 3456×2160 frame has ~1150 px between control points, and a slow diagonal gradient across a flat ground is visible as a soft seam. Only worth it if global proves insufficient in review.
- **The genuinely local fix costs nothing per pixel, and it is a scheduling change.** The overlays that fail to separate are the `B_ADD`/`B_SCREEN`/`B_MAX` ones, because those *preserve* the ground underneath. `B_MIX` already replaces the ground proportionally under the accent — it is per-pixel local by construction. **Biasing accent- and spark-slot blend selection toward `B_MIX` gives local separation at zero per-pixel cost.** That is a complementary change, not an alternative, and belongs in its own document.

---

## 7. Validation

- **`JD_GY=0`** kills the law while leaving the measurement and the `GY` trace running: A/B against the same ground with the same seed by toggling one env var. `g_run` is per-launch, so pair the runs with a fixed seed.
- **`GY` trace every 128 frames** logs `D cov gluma gate want y clum`. What to look for:
  - `y` should move in ramps of hundreds of frames. **Any `y` excursion completing in under ~120 frames is a pump** — the loop is faster than designed and something is wrong.
  - `g_gy_fault` should be **near zero**. More than ~1 firing/minute means `JD_GY_GATE_LO`/`_HI` are mistuned, not that the release is wrong.
  - `clum` should never approach 30 with `y < 256`. If it does, the invariant in §4.1 is broken.
- **The pump test.** 120 BPM four-on-the-floor with a heavy spark load, 5 minutes. Plot `y` against frame. Peak-to-peak ripple at the beat period must stay under **2.6 Q8 counts (1% of ground)**. The derivation predicts 1.35.
- **The handover test.** Force a bright ground → dark ground handover with `y` at the floor. `clum` must not go under `JD_LUMA_FLOOR`, and `g_gain` must not exceed 260 at any point. This is the §4.4 blind spot and it is the test most likely to find a real bug.
- **The regression test.** With `JD_GY=0`, the single-ground path must be **byte-identical** to 3.0.4 (`span_scale` at w = 256 is a `memcpy`), and the handover path byte-identical (`span_lerp2(…, 256−wb, wb)` is the old `span_lerp`). Hash a few hundred frames and compare.

---

## 8. Failure modes, ranked by how badly they end

| # | Failure | Cause | Symptom | Guard |
|---|---|---|---|---|
| 1 | **Ground frozen and churned** | Scaling `L->buf` in place instead of at composite time | Ground stops animating; grounds hand over every few seconds forever | §4.3. `L->buf` is read-only to this law. |
| 2 | **~10 s breathing** | `gy_update` placed after `span_gain` | Whole picture pulses on a long period | §4.1 ordering. The call site comment says so. |
| 3 | **Pump at 1.3 s** | Q8 controller state with the file's round-up idiom | Ground steps visibly on every accent | §2.6. Q16 state. |
| 4 | **Integrator stall** | Q8 state without round-up | Law silently never engages; looks like it "didn't help" | §2.6. Q16 + round-up. |
| 5 | **Dark frame during handover** | `g_gluma` stale for 180 f across a crossfade | Brief dim on an already-dark incoming ground | §4.4 live `clum` interlock, 0.8 s recovery. |
| 6 | **Frame reads empty** | Floor set too deep | "The picture got darker", J's stated non-goal | §5. Floor 179, three agreeing bounds. |
| 7 | **Lone sprite drags the frame** | Demand normalised over *failing* taps instead of all taps | One spark dims the whole picture | §3.3. `deficit >> 9`, always /512. |
| 8 | **Layers dropped to pay for the law** | Per-pixel mask | Fewer sprites, which is the opposite of the goal | §6.1. Folded into existing weights; ~0 ms. |
| 9 | **`B_DIFF` accents unhelped** | Darkening accents register no coverage | Some accents unchanged | §4.6. Known and accepted; declining is better than acting backwards. |
| 10 | **Stale taps after resize** | `g_gtap` sampled at the old stride | One frame of wrong demand | `g_gtapn = 0` in the resize branch. |

---

## 9. Summary

- **Signal:** coverage-weighted separation deficit — the compositor's matte-and-delta pair — measured as `composite − ground` on the existing 512-tap lattice, normalised against the *unattenuated* ground, and divided over **all** taps so coverage enters multiplicatively.
- **Law:** asymmetric single-pole in Q16. Attack τ = 256 f (4.3 s), release τ = 1024 f (17 s), fault release τ = 16 f. Attack derived from a 1%-of-ground per-beat ripple budget; realised ripple 0.53%. Release 4× slower so the yield is a per-passage bias, not a per-event gesture.
- **Hook:** folded into the ground composite's own weights via `span_lerp2`. **Zero added per-pixel cost**; byte-identical to 3.0.4 when idle.
- **Composition with `g_gain`:** the measurement is taken upstream of every gain stage, so the loop is open by construction — not damped, open. Invariant: `g_gain > 260` and `g_gyield < 256` cannot both hold.
- **`g_gluma` and the strobe detector:** both read layer buffers, both structurally immune — provided nobody ever scales a layer buffer in place. That is failure mode #1 and it is the one to guard in review.
- **Scope:** global. Local costs 2–3.4 ms of a 10.5 ms budget, cannot resolve the sprites it exists to protect, and risks producing the very artefact it is meant to remove.
- **Floor:** 0.699× (`JD_GY_MIN` = 179 Q8), from three independent bounds that agree within 4%. Gated by `g_gluma`, so a nominal ground gives 17% and only the bright, cluttered grounds give the full 30% — the law acts hardest exactly where the complaint is.
