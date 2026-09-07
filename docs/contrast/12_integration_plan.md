# 12 — Integration Plan: what gets built, in what order, and how it lands

Scope: decide the sequence. The other papers in `docs/contrast/` derive the theory;
this one commits to a build order, a flag scheme, and a pass/fail bar per stage.
Everything below is grounded in `src/engine/compositor.c` and the 1,962-spawn
telemetry capture, not in the other papers — so it stands whether or not they land.

---

## 0. The finding this whole plan turns on

**The layer weight is applied to the source before the blend, not to the blend result.
That makes `w` a hard visibility gate, not an opacity.**

`span_max()` (compositor.c ~1286):

```c
uint32_t srb = (((s & 0x00FF00FFu) * w) >> 8) & 0x00FF00FFu;   /* src scaled first */
dst[i] = ... (dr > sr ? dr : sr) ...                            /* THEN max */
```

So an overlay pixel is visible only where `(src * w) >> 8 > dst`. Rearranged: the
overlay must be **`256/w` times brighter than the ground pixel under it** or it
contributes literally nothing.

Measured peaks, 1,074 overlay tenancies from `spawn_telemetry.csv`:

| Slot | Role | Mean peak (Q8) | Overlay must exceed ground by | Ground luma above which it is invisible even at src=255 |
|---|---|---|---|---|
| 1 mid | FIELD/FIGURE | 143.8 | 1.78x | 144 |
| 2 accent | FIGURE/SPARK | 111.5 | 2.30x | 112 |
| 3 spark | SPARK/FIGURE | 91.1 | 2.81x | 91 |

- A ground is admitted only at `luma >= 55` (`role_from_cov`) and handed over below
  an EWMA of 20 (`DIM` guard). Real grounds sit well above 91.
- **Therefore the spark layer is provably invisible over most of a typical frame,
  and where it does clear the ground it is clipped flat at `w` — a hard-edged,
  low-amplitude speckle with its gradient destroyed.** That is "the nuanced flashes
  and sprites... barely visible", stated as arithmetic.
- The 24.3% that go SCREEN fare no better in kind: src is scaled to 91 first, then
  inverse-multiplied against a ground at 200, yielding 220. A +20 luma smear with
  the sprite's shape compressed out of it. **That is the fog.**
- Only DIFF can move a pixel downward. It is 6.6% of tenancies and its peak is
  clamped to 90. B_ADD is selected **zero** times in 1,962 spawns — dead code.

Two consequences the whole staging follows from:

1. The composite is a monotone non-decreasing function of every overlay. Nothing
   can darken a bright ground, and the ground is 100% opacity, full frame, always.
2. "Can't darken the ground" and "can't out-bright the ground" are **the same
   line of code**, not two problems.

---

## 1. The one-line-of-defence question

**If only one change ships: fix the blend weighting semantics. `out = lerp(dst, blend(dst, src), w)`.**

Argument:

- **It is a correctness fix, not a taste call.** Every other proposal on the table
  is an aesthetic judgement that can be argued about. This one has a right answer:
  layer opacity applies to the composited result. Photoshop, OpenGL, Core Image and
  every compositor written since 1990 do it this way. The current form is
  "fade the overlay to black, then blend" — which for MAX is a threshold.
- **It preserves the invariant the code explicitly protects.** The comment on
  `span_max` says: *"wherever the overlay is darker than the ground, the ground
  survives bit-exact. That is what keeps a stack readable instead of grey."*
  Under result-lerp, `src <= dst` gives `blend = dst`, so `lerp(dst, dst, w) = dst`.
  Bit-exact. The property survives untouched. Same for the mirrored-warp
  out-of-bounds behaviour, which relies on black being a no-op under MAX.
- **It restores the sprite's gradient.** Currently everything above the ground is
  clipped to a plateau at `w`. Result-lerp is proportional, so soft edges stay soft.
  Nuance is exactly what got clipped.
- **It cannot fight the dead-air guard.** The guard only ever lifts (`span_gain`
  runs only when `g_gain > 260`). This change only ever raises composite luma.
  No new control loop, no new oscillation path.
- **It does not touch the strobe machinery.** The `JUMP` detector (`dq8 > 4096`)
  and the `DIM`/`AUDITION` guards all sample `L->buf` and `g_L[0].buf` — pre-blend.
  None of them observe this change. The earned safety properties are out of scope
  by construction.
- **It makes every later stage cheaper.** Once `w` is a real opacity, peaks can come
  *down* — which is what actually removes fog. Under the current semantics you can
  only reduce fog by reducing visibility, because they are the same dial.

Worked numbers (spark, `w = 91`):

| Ground | src | Today | After | Delta |
|---|---|---|---|---|
| 200 | 255 | 200 (invisible) | 220 | +20, visible, still a nuance |
| 120 | 255 | 120 (invisible) | 168 | +48 |
| 60 | 255 | 91 (clipped flat) | 129 | +38, and the gradient survives |
| 160 | 40 | 160 | 160 | unchanged, bit-exact |

---

## 2. Staged table

| # | Change | Flag | Files | LOC | Alters | Risk | Effort | Ships when |
|---|---|---|---|---|---|---|---|---|
| 1 | Blend weight applied to result, not source | `JD_BLENDW=0/1` | compositor.c (`span_max`/`span_screen`/`span_add`/`span_diff`, `blend_span`) | ~40 | every frame with an overlay | **Med-low** — one kernel family, exact revert | 0.5 day | metrics + J's eye |
| 2a | Ground yields as a function of its own brightness | `JD_GTRIM=0..64` | compositor.c (ground composite, ~2585) | ~15 | every frame with an overlay | **Med** — touches composite luma, control-loop adjacency | 0.5 day | after 1 is banked |
| 2b | Peak weights retuned down + normalised by overlay luma | `JD_PEAK=0/1` | compositor.c (`PEAK_LO/HI`, `pick_blend`) | ~12 | spawn decisions only | **Low** — a constant table | 0.5 day | with 2a |
| 3 | Keyed-darkening blend `B_KEY` (added at index 5) + ground-driven selection | `JD_DARK=0/1` | compositor.c (enum, new kernel, `pick_blend`) | ~45 | nothing until selection turns on | **Low→Med** — additive kernel, then a gated selection rule | 1 day | after 2 |
| 4 | Local halo separation (downsampled dilate/blur mask) | `JD_HALO` | compositor.c + scratch buffer | ~120 | new per-frame pass | **High** — perf, new failure modes | 2-3 days | **deferred** |
| — | Family diversity (246 routines in family 0) | — | — | — | — | — | — | **out of scope, explicitly** |

Sequencing rationale: each stage is verifiable alone, and each is a strict
prerequisite for the next being *tunable*. You cannot judge whether a peak is too
high (2b) while `w` is a visibility threshold (1). You cannot judge whether a
darkening blend helps (3) while the ground is at 100% under everything (2a).

---

## 3. Stage 1 — blend weight semantics

**The smallest change that produces a visible improvement.** Four kernels, one
inner-loop line each, one flag, no new state, no new allocation, no new pass.

Shape:

```c
/* JD_BLENDW=0 restores 3.0.4 source-scaling exactly, for A/B. */
static int jd_blendw(void) {
    static int v = -1;
    if (v < 0) { const char *e = getenv("JD_BLENDW"); v = e ? atoi(e) : JD_CONTRAST_DEFAULT_BLENDW; }
    return v;
}

static void span_max_w(uint32_t *dst, const uint32_t *src, int n, uint32_t w)
{
    uint32_t iw = 256 - w;
    for (int i = 0; i < n; i++) {
        uint32_t d = dst[i], s = src[i];
        uint32_t dr = d & 0x00FF0000u, dg = d & 0x0000FF00u, db = d & 0xFFu;
        uint32_t sr = s & 0x00FF0000u, sg = s & 0x0000FF00u, sb = s & 0xFFu;
        uint32_t mr = dr > sr ? dr : sr, mg = dg > sg ? dg : sg, mb = db > sb ? db : sb;
        /* lerp the RESULT back toward the ground by the layer weight */
        uint32_t rb = ((((d & 0x00FF00FFu) * iw) + (((mr | mb)) * w)) >> 8) & 0x00FF00FFu;
        uint32_t g  = (((dg * iw) + (mg * w)) >> 8) & 0x0000FF00u;
        dst[i] = 0xFF000000u | rb | g;
    }
}
```

(Sketch, not final — the packed R/B path needs the usual care that the two 8-bit
lanes cannot carry into each other after two multiplies. If the packed form gets
awkward, the unpacked form is three multiplies and still cheaper than a second
pass over the frame.)

- Apply the same treatment to SCREEN, ADD and DIFF. DIFF already lerps its result
  (`dr*iw + xr*w`) — it is the one kernel that is already correct, which is a
  useful confirmation that this is the intended semantics and MAX/SCREEN/ADD are
  the anomaly.
- `B_MIX` (`span_lerp`) is already correct by definition. Grounds all use MIX
  (888 of 888 ground spawns, blend=0), so **grounds are untouched by Stage 1.**
- Keep `blend_span`'s dispatch identical; branch on `jd_blendw()` inside it, once
  per span, not per pixel.

**Cost.** Doubles the arithmetic in the MAX kernel (69.1% of overlay tenancies).
The engine budgets `n * 120` Q8 = 0.47 ms per blend against `BUDGET_Q8` = 10.5 ms.
At 1280x960 with three overlays this is ~0.5-1.4 ms/frame worst case. Headroom
exists (measured ~2.8x, 167 fps). If it does not, **the failure mode is silent**:
`admissible()` rule 4 refuses candidates on cost, so an under-budgeted kernel
shows up as a *thinner stack at the same fps*, not as dropped frames. See §6.

**Acceptance — measured** (all vs `JD_CONTRAST=0` on the same binary, same START):

| Metric | Direction | Bar |
|---|---|---|
| `HEAD` = P99 luma − P50 luma | up | **+15% or better** |
| `CONT` = luma stddev (`battery` already prints it) | up | **+8% or better** |
| `FOG` = fraction of pixels within ±10 luma of the frame median | down | **−5 points or better** |
| `LUMA` mean | band | within **+15% / −0%** — if it only got brighter, it did not get more separated |
| `MAXD`, `ge8` (`gate maxdelta`) | no regression | **hard fail** on any increase in `ge8`, or `MAXD` above the 3.0.4 baseline max |
| `NLIVE` mean live layers | no regression | **within 0.15** of baseline |
| `FPS` at 1280x960 | no regression | **>= 90%** of baseline |

**Acceptance — eyeballed** (no metric settles this):

- `gate render` at four fixed frames, flag on and off, viewed side by side. Look
  specifically at: do sparks read as *shapes* with soft edges, or as flat clipped
  blobs? Flat blobs mean the lerp is not landing.
- Ten minutes full-screen with a track playing, both flags, and the only question
  that matters: **which one would you leave running.** If the answer is "the old
  one", the metrics were measuring the wrong thing and Stage 2 does not start.
- Watch specifically for a new failure: overlays that were previously invisible now
  appearing *too* insistently on dark grounds. That is Stage 2b's job, not a
  reason to revert Stage 1.

---

## 4. Stage 2 — the ground yields, and the overlays ask for less

Two changes, landed together because they are two halves of one law and each
compensates the other's side effect.

### 2a — ground trim keyed to the ground's own brightness

- Trim the ground **before** the overlay loop, proportional to how bright the
  ground actually is and how much overlay is present:

```c
/* g_gluma is already maintained: the ground buffer's luma EWMA (~0.5 s). */
uint32_t pres = 0;                       /* Q8 overlay presence */
for (k) pres += (L->w_now * (255u - g_st[L->routine].dark)) >> 8;
uint32_t trim = trim_max * clamp01(g_gluma - 90) * clamp01(pres);
/* eased, attack/release like g_gain, so it never steps */
if (trim) span_scale(fb, fb, npix, 256 - trim);
```

- **Bright grounds yield, dark grounds do not.** This is not a cosmetic
  formulation — it is what keeps the trim from ever colliding with the dead-air
  guard, because the guard only fires below luma 30 and the trim is zero below
  ground luma ~90. **No feedback loop is created.** That is the whole reason the
  trim is keyed on `g_gluma` rather than on the composite.
- Cost: near zero. `span_scale` exists; on the single-ground path it replaces a
  `memcpy` (same memory traffic, one multiply per pixel).
- `trim_max` starts at 48/256 (~19%) and is the thing `JD_GTRIM` varies, so J can
  dial it live-ish across A/B runs without a rebuild.

### 2b — peaks down, and normalised by overlay brightness

- **Lower `PEAK_LO/HI` for slot 1 (mid).** At mean 143.8/256 it is the single
  largest fog contributor and, post-Stage-1, it no longer needs that weight to be
  seen. Proposal: `{128,180}` → `{104,150}`, measure, iterate.
- **Normalise peak by the overlay's own measured luma**, which the probe already
  records: `p = p * 128 / max(64, st->luma)`, clamped to `[PEAK_LO, PEAK_HI]`.
  Today a dim overlay and a bright overlay get the same weight distribution, so
  the bright ones blow out and the dim ones fog. This is a two-line change in
  `pick_blend` with a large fog payoff.
- Leave the existing SCREEN cap (`gl >= 160 ? 154 : ...`) alone. Its rationale is
  milkiness over dark grounds, which is a separate and still-valid concern.

**Cache note:** none of Stage 2 changes probe outputs, so `probe_stamp()` stays
valid and the cache does not need busting. But `PEAK_HI` feeds `admissible()`'s
motion budget (rule 5), so **Stage 2b changes which routines get admitted.** That
is expected, and it is why `NLIVE` and the family-clash rate are on the acceptance
list, not just the luma metrics.

**Acceptance:** same metric table as Stage 1, plus:

- `FOG` must fall **another 5 points** — 2a/2b are the fog stages; if fog does not
  move, the trim is too timid or keyed wrong.
- `LUMA` mean is now allowed to fall, but **not below baseline − 10%**, and the
  frame count with composite luma < `JD_LUMA_FLOOR` (30) must stay at **zero**.
  Instrument this with the existing `JD_INSTR` dark-frame block.
- Eyeball: does the ground still read as a picture, or has it become a grey mat
  behind the sprites? A trim that kills the ground has traded one complaint for
  another.

---

## 5. Stage 3 — a darkening mode, added additively

**Land the kernel before the selection rule.** Two commits, not one.

- **3a (zero-risk):** add `B_KEY = 5` to the enum *after* `B_DIFF`, add the kernel,
  add the `blend_span` case. Nothing selects it. Reachable only via a forcing
  env var for inspection. Ships with no behavioural change whatsoever — this is
  the "new blend mode nothing selects yet" that can go in ahead of the argument
  about when to use it.
- **Do not renumber the existing enum.** 1,962 telemetry rows encode blend 0-4;
  renumbering silently invalidates every prior measurement and every comparison in
  this folder. Append only.

The kernel — a keyed local yield, single pass, no blur, no scratch buffer:

```c
/* B_KEY: the ground steps back exactly where the sprite is about to land.
 * sl = luma(src). Where sl ~ 0 this is bit-exact identity (black stays a
 * no-op, as MAX requires). Where sl is high the ground is pushed down and
 * the sprite lands on top, so the sprite carves its own contrast. */
uint32_t sl   = (sr*77 + sg*150 + sb*29) >> 8;
uint32_t yld  = (d * ((sl * KDARK) >> 8)) >> 8;    /* ground yields */
uint32_t base = d - yld;
uint32_t out  = max(base, s);
dst[i] = lerp(d, out, w);
```

- **Honest about its limit:** because the yield is co-located with the sprite, the
  visible gain concentrates at the sprite's *soft edges*, where `sl` is mid-valued.
  That is where separation is perceived, so it works — but it is strictly weaker
  than a dilated halo. The halo is Stage 4 and is deferred on purpose.
- **3b (gated):** `pick_blend` selects `B_KEY` when `g_gluma` is high (bright
  ground, the actual complaint) and the routine's role is FIGURE or SPARK.
  Gated on `JD_DARK=1`, default off until it earns its way on.

**Acceptance:** the metric table, plus a specific eyeball question — *does the
accent read as sitting in front of the ground, or as a hole cut in it?* A hole is
`KDARK` too high. There is no metric for this; it is a look.

Also required: rerun `gate keys` (the C and S live-control paths) and confirm the
post-press delta has not moved. Any new blend mode is a new way for a key press to
strobe.

---

## 6. Feature flags

Follow the engine's existing shape: `getenv` once into a file-static, `atoi`,
never in an inner loop.

```c
/* JD_CONTRAST=0..3 — the ladder, for A/B without a rebuild.
 *   0  3.0.4 behaviour, bit-for-bit
 *   1  + stage 1 (blend weight on the result)
 *   2  + stage 2 (ground trim, retuned peaks)
 *   3  + stage 3 (keyed darkening)
 * Individual JD_BLENDW / JD_GTRIM / JD_PEAK / JD_DARK override the level, so a
 * regression can be bisected to one change without a rebuild. */
static int g_clevel = -1;
static int contrast_level(void) {
    if (g_clevel < 0) { const char *e = getenv("JD_CONTRAST"); g_clevel = e ? atoi(e) : 0; }
    return g_clevel;
}
```

- **Every new flag defaults OFF while its stage is landing**, then flips to default
  ON when the stage is accepted — and **the flag stays for the life of 3.0.x**, so
  `JD_CONTRAST=0` is always a one-word revert to known-good 3.0.4 output. That is
  cheaper than a git bisect against a screensaver, which is killed rather than
  exited and is hard to instrument after the fact.
- `JD_KDARK=n` and `JD_GTRIM=n` take numeric values, not booleans, so the two
  aesthetic constants are dialable across A/B runs from the shell.
- Precedent for a dead enum entry already exists: `B_ADD` is selected zero times in
  1,962 spawns. Adding `B_KEY` unselected carries no novel risk.

**A/B protocol — and one trap that will otherwise waste a day:**

```sh
make gate
for L in 0 1; do
  JD_NOCACHE=1 JD_CONTRAST=$L ./gate battery  1280 960 1700000 3600 60  > /tmp/jd_c$L.txt
  JD_NOCACHE=1 JD_CONTRAST=$L ./gate maxdelta 1280 960 1700000 3600    >> /tmp/jd_c$L.txt
  JD_NOCACHE=1 JD_CONTRAST=$L ./gate render   1280 960 1700000 1703600 /tmp/jd_c$L \
      1700600 1701200 1702400 1703000
done
```

- `g_run = mix32(frame * 2654435761 + 0x1D0F1E55)` at init, so **the harness is
  deterministic given START** — the cast, the bags, the moods and the transforms
  all replay identically.
- **Except**: `recent_note()` calls `probe_cache_save()` on the first spawn of
  every run, and the cross-launch `g_recent` ring then refuses that opener next
  time. **Run N+1 is not run N.** Without `JD_NOCACHE=1` a two-run A/B measures the
  cache, not the change. This is the single easiest way to get a false result here.
- START = `1700000` deliberately: it is the start frame of the existing
  `spawn_telemetry.csv` capture, so the telemetry rows describe the same cast.
- `JD_LAYERS=1` isolates the ground; `JD_LAYERS=2,3,4` walks the stack up one
  overlay at a time. Use it to attribute a metric change to a specific slot.

**Two small harness additions, worth the hour:**

1. `gate contrast` — same driver as `battery`, but per sample prints P50, P95, P99,
   `FOG` and a 10-bucket luma histogram. Everything above is derivable from a
   sorted luma array; the harness already computes mean and stddev.
2. Add live-layer count to `battery`'s `S` line. `jd_now_playing()` already exists
   and is used by `gate keys`. This is the throttle detector and it is one line.
3. Add a `gate` target to the Makefile. There isn't one; the harness is currently
   built ad hoc, which makes the acceptance run harder than it should be.

---

## 7. What NOT to do

- **Do not raise `PEAK_HI`.** It is the tempting one-line "fix" and it makes things
  worse in two directions: it raises the *mid* layer's weight, which is the largest
  fog contributor, and it moves toward the state the code already warns about
  (*"an overlay that reaches 256 has replaced the ground instead of enriching it"*).
  Post-Stage-1 the correct move is peaks **down**.
- **Do not add layers.** `JD_NSLOT` is 4 and `JD_NBUF` is 5. More thin translucent
  layers over a 100%-opacity ground is strictly more mid-band pixel mass, which is
  the literal definition of the fog being complained about. Fix contrast first;
  then re-ask whether more layers even help.
- **Do not reach for global tonemapping or a global S-curve.** The global gain is
  already owned by the dead-air guard, with earned constants
  (`JD_LUMA_MAXGAIN = 384` exists because 2.7x *"clamped the highlights and washed
  the colour out (J caught it)"*). Adding a second global curve puts two control
  loops with different time constants on the same variable. And a global curve
  cannot create *local* separation: the ground and the accent occupy the same luma
  band in the same pixel neighbourhood, which is exactly what a global function
  cannot distinguish.
- **Do not darken the ground by a constant.** It lowers luma everywhere, walks the
  composite toward the dead-air guard's trigger, and leaves relative contrast
  unchanged. A dimmer version of the same fog.
- **Do not touch the strobe guards to make room for a brighter accent.** The
  `dq8 > 4096` JUMP detector, the `DIM` handover, the `AUDITION` redraw and the
  1.06x audio bloom cap were each earned. Nothing in Stages 1-3 needs them moved —
  they all sample pre-blend buffers. If a stage appears to need one loosened, the
  stage is wrong.
- **Do not renumber the blend enum.** Append only. 1,962 rows of telemetry encode
  blend 0-4.
- **Do not A/B without `JD_NOCACHE=1`.** See §6.
- **Do not judge a stage by fps alone.** `admissible()` refuses layers on budget, so
  an over-expensive kernel presents as a thinner stack at unchanged fps. `NLIVE` is
  a required metric, not a nice-to-have.
- **Do not fix this in the routines or the palettes.** 634 routines collapsing into
  9 families is a real problem and J wants it solved — but retuning content cannot
  fix a compositor that clips every overlay at its own weight, and doing both at
  once makes each unattributable. Explicitly out of scope for this pass.
- **Do not land Stage 2 before Stage 1 is banked.** Ground trim and blend semantics
  both move composite luma. Landed together, neither is attributable, and the
  eyeball verdict — which is the one that actually decides — has nothing to compare.

---

## 8. Deferred, and why

| Deferred | Why not now |
|---|---|
| **Local halo separation** (downsample overlay luma, blur, upsample as a ground-yield mask) | The strongest available separation mechanism, and the one that would most directly answer "the background takes the focus". But it is a new per-frame pass with a new scratch buffer, real perf exposure at 1280x960, and a new class of artefact (halo ringing) that none of the existing guards look for. It should land against a build where Stages 1-3 are already banked, so its cost and its benefit are both attributable. |
| **Bloom / glow on the accent** | Additive light on top of an already-additive stack. Until the composite can darken, bloom is more fog with a nicer name. Revisit after Stage 3. |
| **Linear-light / perceptual blending** | Correct, and it would improve every kernel here. But it changes the numeric meaning of every constant in the file — `JD_LUMA_FLOOR`, `JD_LUMA_MAXGAIN`, `dq8 > 4096`, the peak tables, the probe's `luma`/`dark`/`sat` — and would invalidate the probe cache and every measurement in this folder simultaneously. It is a version bump's worth of work, not a stage. |
| **Family diversity (246 routines in family 0)** | Named by J as a separate problem. Keeping it out means the contrast metrics are not confounded by a changed cast. |

---

## 9. Summary

- **Stage 1 is the one that matters, and it is small:** apply the layer weight to
  the blend result rather than the source. Four kernels, ~40 lines, one flag, no
  new state, and it is a correctness fix with a right answer rather than a taste
  call. It preserves every invariant the file protects, touches no guard, creates
  no control loop, and makes the accent visible — which is the stated complaint.
- **Stage 2 is the fog stage:** bright grounds yield, overlays ask for less. Keyed
  on `g_gluma` specifically so it can never collide with the dead-air guard.
- **Stage 3 gives the engine its first darkening mode**, landed additively first
  and selected second.
- **Stage 4 and everything perceptual is deferred**, on purpose, so that each
  landed change is attributable to one flag and revertible to `JD_CONTRAST=0`.
</content>
</invoke>
