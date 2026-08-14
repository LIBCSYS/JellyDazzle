# JellyDazzle v2.1 — SCHEDULER

What plays, when, next to what. This document owns **selection and timing**:
the routine bag, the palette bag, the layer schedule, and the taste rules that
decide which routine may occupy which layer.

It does **not** own compositing, easing, or the palette crossfade — those are
`transitions.md`. It does not own palette authoring — that is `palettes.md`.
Where this document touches those, it cites them and stays compatible.
§11 lists the three places the three documents currently disagree.

Every number below was measured on this machine, on this code, today. The
harnesses are in §10 so any of it can be re-run.

---

## 0. What was measured

All 100 patterns were benchmarked and analysed at 1280×960. Full table in §9.

| Instrument | What it produced |
|---|---|
| `harness.c bench` ×100 | render cost, ms/frame |
| `harness.c delta 300` ×100 | frame-to-frame mean channel delta |
| new coverage analyser ×100 | mean luma, screen coverage, hue bins, accumulation growth |
| composite micro-bench | cost of every blend operator at 1280×960 |
| SDL micro-bench | texture upload + present cost |
| scheduler simulation | 6 wall-hours of scheduling, before/after |

### 0.1 The brief's numbers, confirmed

Reproduced by simulating `bridge.c:29-31` exactly:

| Claim | Measured | Verdict |
|---|---|---|
| 110 of 124 routines in 300 segments | **110/124** | exact |
| 39 repeats within a 20-segment window | **40** (39 by the brief's off-by-one window) | exact |
| Palette A-usage 5..16 over 300 segments | **min 5, max 16** | exact |

### 0.2 Three things found by measuring that the brief did not have

**A. The palette pool is worse than "half are rainbows", and it is a generator bug.**
Measured over `palette.bin`: **19 of 30 schemes have a hue span ≥ 0.75** — they wrap
three-quarters or more of the colour wheel. The six house schemes are the *only*
consistently narrow ones (`gilded` 0.06, `ice` 0.11, `spring` 0.12, `ember` 0.14).
All 24 downloaded palettes come out wide, and `gen_tables.py:92-95` says why:

```python
pos = i * M / N ; k = int(pos) % M      # walks ALL M artist colours in ramp order
```

Any Lospec palette with more than ~6 anchors is *expanded into* a full-spectrum sweep,
regardless of how restrained the artist's palette was. Curation cannot fix this;
the expansion has to change. This is `palettes.md`'s territory — flagged here because
the scheduler cannot schedule variety that the generator has averaged away.

**B. The pattern library is shaped backwards for layering.**
Classifying by measured screen coverage:

| role | coverage | count | consumed by the schedule |
|---|---|---|---|
| GROUND | ≥ 0.80 | **42** | 1 draw / 527 s |
| FIELD | 0.45–0.80 | **22** | 1 draw / 41 s |
| FIGURE | 0.15–0.45 | **23** | 1 draw / 25 s |
| SPARK | < 0.15 | **13** | 1 draw / 23 s |

Consumption is inverted against supply. Layering burns dark, sparse routines ~20×
faster than it burns full-screen ones, and those are the two smallest bags. With
today's pool the SPARK bag recycles every **0.8 minutes**. This drives the pool-shape
directive in §8 — and it materially changes what "double the routines" should mean.

**C. The 60 fps target is not the real constraint; the display is.**
`SDL_RenderPresent` blocks on vsync on the Metal backend *regardless of the
`PRESENTVSYNC` flag*. Measured on the hidden-window path: **8.337 ms/frame**, which is
exactly one refresh of the built-in 120 Hz ProMotion panel. SDL reports two displays:

```
display 0: 3440x1440 @  60 Hz   (external)
display 1: 1352x878  @ 120 Hz   (built-in)
```

So the app's frame budget is 16.67 ms on the external monitor and 8.33 ms on the
laptop panel. A build tuned to 16.67 ms will alternate 8.3/16.7 ms on the built-in
display — **uneven pacing, which reads as judder on slow material.** See §7.3: the
recommendation is an explicit 60 fps cap, not a faster renderer.

---

## 1. Model: slots, not segments

v2.0's unit of composition is the 2048-frame segment: one routine owns the screen,
then is replaced by a hard cut. v2.1 replaces it with **four independently-clocked
layer slots**. There is no global segment boundary at all — nothing in the engine
ever changes all at once again, which is most of "no rough breaks" for free.

| slot | name | role | what it is |
|---|---|---|---|
| 0 | `base` | GROUND \| FIELD | the ground. Always present, always opaque. |
| 1 | `mid` | FIELD \| FIGURE | the body of the composition. |
| 2 | `accent` | FIGURE \| SPARK | figures over the body. |
| 3 | `spark` | SPARK \| FIGURE | brief highlights. |

Each slot owns a private full-resolution canvas (`transitions.md` §1), draws its own
routine, runs its own lifetime clock, and fades independently. The scheduler's job is
to keep the slots filled with routines that (i) have not been seen recently, (ii) fit
the frame budget, and (iii) look good together.

This design requires **zero changes to any pattern**. The plug-in contract already
supplies `sl` and `seed`; the scheduler simply makes `sl` layer-local
(`transitions.md` §5.1). Verified against the library: exactly **one** pattern
(`pattern_044`) reads `fb` before writing it; every other pattern is a self-contained
repaint and composites cleanly.

> **Hazard, must be enforced:** every pattern holds file-static state
> (`p51_acc`, `p51_bx`, …). Two slots running the *same* routine would corrupt each
> other's state. The bag scheme in §2 makes this structurally impossible, and the
> admission predicate asserts it anyway (§5.2, rule 1). Do not weaken either.

---

## 2. (a) The routine bag

### 2.1 The requirement

Every routine plays once before any repeat; reshuffle with a new seed each cycle;
no duplicate across the seam.

### 2.2 The design that does not work — and why, so it is not re-attempted

The obvious implementation is one global shuffled bag of all 224 routines, drawn
with a filter for role/cost/motion. **This was simulated and it fails.** Filtering a
single bag reintroduces exactly the disease being cured, because a heavily-constrained
slot (`spark` needs a dark, cheap, low-motion routine) rejects most of the bag,
exhausts it, and forces a reshuffle — so the few routines that pass the filter get
drawn over and over.

Measured, 3 simulated hours, single global bag with filtering:

```
distinct routines used   90/100
min gap between repeats   1 draw
usage spread            1..66      <-- worse than v2.0's 1..8
```

The failure mode is silent: the bag *looks* correct in isolation and only misbehaves
under admission control. Two structural fixes are required.

### 2.3 Fix 1 — one bag per role

Partition the routine pool by measured role. Each bag is a true permutation over its
own members, so the once-per-cycle guarantee is local to the bag and cannot be
destroyed by a different slot's constraints.

```c
enum { JD_GROUND = 0, JD_FIELD, JD_FIGURE, JD_SPARK, JD_NROLE };

typedef struct {
    const uint16_t *items;   /* routine ids belonging to this role */
    uint16_t  n;
    uint16_t  buf[JD_MAXBAG];/* the live permutation, consumed from the front */
    uint16_t  head;          /* next index to consider */
    uint16_t  hist[JD_SEAM]; /* ring of the last JD_SEAM ids drawn */
    uint8_t   hpos;
    uint32_t  seed;          /* advances every cycle */
} jd_bag;

static jd_bag g_bag[JD_NROLE];
```

### 2.4 Fix 2 — reject by deferring, never by re-drawing

When a candidate fails admission it is **moved to the back of the current cycle**, not
discarded and re-rolled. It stays in the permutation, so it still plays exactly once
per cycle — just later, when the stack around it has changed and it may well pass.

```c
/* Draw the next admissible routine from bag b.
 * Rotates rejects to the back of the CURRENT cycle so the permutation survives.
 * Returns 0xFFFF only if nothing in the whole cycle is admissible right now. */
static uint16_t bag_draw(jd_bag *b, int (*ok)(uint16_t, void *), void *ctx)
{
    if (b->head >= b->n) bag_refill(b);
    for (uint16_t tried = 0; tried < b->n; tried++) {
        uint16_t v = b->buf[b->head];
        /* rotate the front element to the back of the remaining cycle */
        for (uint16_t i = b->head; i + 1 < b->n; i++) b->buf[i] = b->buf[i + 1];
        b->buf[b->n - 1] = v;
        if (ok(v, ctx)) {
            b->head++;                       /* consumed */
            b->hist[b->hpos++ & (JD_SEAM-1)] = v;
            return v;
        }
        /* not consumed: head stays put, v is now at the back */
    }
    return 0xFFFF;                           /* caller retries in ~2 s */
}
```

> The linear rotate is O(n) with n ≤ 64 and runs at most a few times per second —
> roughly 200 ns. A ring buffer is tidier; it is not worth the bug surface here.

### 2.5 Reshuffle and the seam

```c
#define JD_SEAM 16        /* must be a power of two */

static void bag_refill(jd_bag *b)
{
    b->seed = mix32(b->seed ^ 0x9E3779B9u);
    uint32_t r = b->seed;

    for (uint16_t i = 0; i < b->n; i++) b->buf[i] = b->items[i];
    for (uint16_t i = b->n - 1; i > 0; i--) {          /* Fisher-Yates */
        r = mix32(r);
        uint16_t j = (uint16_t)(r % (uint32_t)(i + 1));
        uint16_t t = b->buf[i]; b->buf[i] = b->buf[j]; b->buf[j] = t;
    }

    /* Seam guard: nothing in the first W entries may appear in the last W drawn.
     * Swap offenders deep into the tail. W is capped at n/2 so a swap target
     * always exists. */
    uint16_t W = b->n / 2 < JD_SEAM ? b->n / 2 : JD_SEAM;
    for (uint16_t i = 0; i < W; i++) {
        for (int guard = 0; guard < 128 && recently_drawn(b, b->buf[i], W); guard++) {
            r = mix32(r);
            uint16_t j = W + (uint16_t)(r % (uint32_t)(b->n - W));
            uint16_t t = b->buf[i]; b->buf[i] = b->buf[j]; b->buf[j] = t;
        }
    }
    b->head = 0;
}
```

`recently_drawn` scans the `hist` ring. The guaranteed property is
**min-gap ≥ W draws**, and measured min-gap comfortably exceeds W because the
interior of the permutation adds slack.

Measured, N = 224, 600 draws:

| seam window W | min gap achieved | all unique per cycle |
|---|---|---|
| 8 | 13 | yes |
| 16 | **32** | yes |
| 24 | 35 | yes |
| 48 | 54 | yes |

W = 16 is the recommended setting: it more than doubles the naive gap and costs
nothing.

### 2.6 Measured result

Simulated against the exact v2.0 expression, 300 draws:

| | v2.0 `mix32(seg)%124` | shuffled bag |
|---|---|---|
| distinct routines | 110 / 124 | **124 / 124** |
| repeats in any 20-draw window | 40 | **2** |
| min gap between repeats | 2 | **12** |
| usage spread | 1..8 | **2..3** |

At N = 224 (after doubling): 224/224 distinct, **1** in-window repeat, min gap 16.

Over a 6-hour run with per-role bags and deferred admission, usage is flat to ±1 in
every bag (`11..12`, `14..15`, `14..15`) — the 1..66 collapse of the naive design is
gone.

---

## 3. (b) The palette bag

`transitions.md` §3.3 already specifies the palette bag mechanism (Fisher-Yates over
`JD_NS`, epoch seam guard, `scheme_at(leg)` chained so `B(leg) == A(leg+1)`). **Adopt
that as-is.** This section adds the one thing it does not have, which is the part that
answers J's actual complaint.

### 3.1 A uniform bag does not fix "they are all basically similar"

A plain bag fixes *frequency* — every palette appears once per epoch, usage spread
collapses from 5..16 to exactly 1. It does nothing about *adjacency*. With
`full_spectrum` and its neighbours making up a large share of the pool, a uniform
shuffle still routinely plays two indistinguishable rainbows back to back, and the
viewer reads that as "no change happened".

Measured, uniform bag over the 60-palette target pool, 1200 draws:

```
same-class adjacent   130
LOUD-class adjacent    92      (full_spectrum | neon_on_black | split_complement)
```

### 3.2 Class-adjacency repair

After shuffling, walk the permutation once and swap forward past any element whose
class matches its predecessor. This is a **repair, not a re-roll** — the permutation
is preserved intact, so every min-gap and uniformity guarantee from `transitions.md`
§3.3 still holds exactly.

Classes and pool counts are `palettes.md` §5, used verbatim:

```c
/* palettes.md §5 taxonomy — 60 palettes */
enum { PC_MONO_ACCENT, PC_DUOTONE, PC_ANALOGOUS, PC_SPLIT_COMP, PC_NEON_ON_BLACK,
       PC_PASTEL_WASH, PC_METALLIC, PC_STARK, PC_EARTH, PC_FULL_SPECTRUM, PC_N };

static const uint8_t pal_class[JD_NS];        /* generated with palette.bin */

/* "LOUD" = classes that read as high-energy. Two in a row is visual shouting;
   two of the SAME loud class in a row is the thing J is complaining about. */
static inline int pal_loud(uint8_t c) {
    return c == PC_FULL_SPECTRUM || c == PC_NEON_ON_BLACK || c == PC_SPLIT_COMP;
}

static void bag_repair_classes(uint8_t *perm, int n, uint8_t prev_class)
{
    uint8_t pc = prev_class;
    for (int i = 0; i < n; i++) {
        int bad = (pal_class[perm[i]] == pc) || (pal_loud(pc) && pal_loud(pal_class[perm[i]]));
        if (bad) {
            for (int j = i + 1; j < n; j++) {
                uint8_t cj = pal_class[perm[j]];
                if (cj != pc && !(pal_loud(pc) && pal_loud(cj))) {
                    uint8_t t = perm[i]; perm[i] = perm[j]; perm[j] = t;
                    break;                    /* tail may have no candidate: accept */
                }
            }
        }
        pc = pal_class[perm[i]];
    }
}
```

Measured over the same 1200 draws:

| | uniform bag | + class repair | + LOUD repair |
|---|---|---|---|
| min gap | 21 | 21 | **21** |
| usage spread | 20..20 | 20..20 | **20..20** |
| same-class adjacent | 130 | 3 | **3** |
| LOUD adjacent | 92 | 71 | **0** |
| two rainbows in a row | 8 | 0 | **0** |

The repair costs nothing in uniformity: over 6000 draws each class lands on its
target share to within 0.1 % (`full_spectrum` target 6.7 %, actual 6.7 %). The
residual 3 same-class adjacencies out of 1200 (0.25 %) are permutation-tail cases
where no differing class remains; that is below the perceptual floor and is not worth
a second pass.

At `JD_LEG = 1024` frames, one 60-palette epoch is **17.1 minutes**, and the min-gap
of 21 draws guarantees **≥ 5.7 minutes** between any palette and itself.

### 3.3 Layers share the palette, with a window

`transitions.md` §3.1 is right that all layers must share one palette walk — different
palettes per layer read as two images fighting. But identical palettes per layer make
the stack muddy, because every layer lands on the same hues.

Give each layer a **window** into the shared blended ramp instead of its own palette:

```c
/* Derive a layer's palette from the shared one. Cost: 32768 entries ~ 0.007 ms,
   so this is free even if rebuilt every frame — but rebuild it only when the
   layer is born or the shared ramp's t8 ticks. */
static void layer_palette(uint32_t *dst, const uint32_t *shared,
                          uint16_t span, uint16_t offset)
{
    for (int i = 0; i < 32768; i++)
        dst[i] = shared[(((i * span) >> 15) + offset) & JD_PAL_MASK];
}
```

| slot | span | offset | effect |
|---|---|---|---|
| base | 32768 (1.0×) | 0 | the whole ramp — the ground states the scheme |
| mid | 22000 (0.67×) | `seed % 32768` | a two-thirds slice, rotated |
| accent | 11000 (0.34×) | `seed % 32768` | a third of the ramp — reads as a hue family |
| spark | 4000 (0.12×) | `seed % 32768` | near-monochrome highlight |

This is what makes "stark to amazing" reachable from one pool: a `mono_accent`
scheme with narrow layer windows gives a restrained, near-monochrome frame; a
`full_spectrum` scheme with the base at full span gives the blazing one. Same
machinery, opposite ends.

---

## 4. (c) The layer schedule

### 4.1 The entry cascade — J's spec, literally

> *"use 1, then 3 seconds later, another, then 1 second another, building layer upon layer"*

```
t = 0 s     base   enters, fades in over 3.0 s
t = 3.0 s   mid    enters, fades in over 3.0 s
t = 4.0 s   accent enters, fades in over 2.5 s
t = 9.0 s   spark  enters, fades in over 2.0 s
```

The 0 → +3 s → +1 s cascade is exactly as specified. `spark` is held back to +9 s so
the stack builds to a readable three-layer composition before the highlights start;
after the first cascade it is opportunistic (§4.3).

### 4.2 Lifetimes — prime seconds, so the stack never re-syncs

Each slot picks its lifetime from a set of **primes, distinct across slots**. Because
no two slot periods share a factor, the stack configuration does not repeat on any
humanly observable timescale — the composition is always changing, which is the
requirement, and it needs no extra state to achieve.

| slot | lifetime pool (s) | rest after death (s) | fade in | fade out | peak weight |
|---|---|---|---|---|---|
| `base` | 43, 47, 53, 59, 61, 67 | 0 (handover, §4.4) | 3.0 s | 3.0 s | 1.00 |
| `mid` | 29, 31, 37, 41 | 2–5 | 3.0 s | 3.5 s | 0.65 |
| `accent` | 17, 19, 23 | 3–8 | 2.5 s | 3.0 s | 0.45 |
| `spark` | 11, 13, 17 | 6–14 | 2.0 s | 2.5 s | 0.28 |

Fades are `ease_ss` (`transitions.md` §4.1) and every fade-out is **longer than its
fade-in** — a slow exit dissolves, a fast one cuts. All durations clear the 120-frame
floor that `transitions.md` §4.3 derives from the motion budget, so no fade can ever
be the thing that makes the frame jerky.

The rest gap matters more than it looks: it is what keeps the fast slots from
churning through their bags. Adding a 6–14 s rest to `spark` roughly halves its draw
rate and doubles its min-gap.

### 4.3 Slot state machine

```
        ┌──────── rest (slot empty, timer running) ◀────────┐
        ▼                                                   │
    [ spawn ] ──fade in──▶ [ hold ] ──fade out──▶ [ retire ]┘
       │                                             ▲
       └── admission refused ──▶ retry in 2 s ───────┘
```

`base` never enters `rest` — it hands over instead (§4.4). All other slots rest, then
respawn. A refused admission is not a failure; the slot simply stays empty for another
2 s and tries again. Over 6 simulated hours, refusals resolve on retry in every case;
the stack is never starved.

### 4.4 Base handover — the one place two grounds are live at once

The base cannot simply die: there would be a hole. Instead the outgoing base fades
out *through* the incoming one over 3.0 s, both rendering:

```
base_old   ▓▓▓▓▓▓▓▓▓▓▓▓╲__________
base_new             ___╱▓▓▓▓▓▓▓▓▓▓▓
                     └─ 3.0 s ─┘
```

That is a fifth render slot for 180 frames. Two rules keep it inside budget:

1. During a handover **no new layer may spawn** in any slot.
2. The admission cost test for `base_new` uses the stack cost **including
   `base_old`** — so a heavy incoming ground is simply refused until a cheaper one
   comes up in the bag.

### 4.5 Maximum layers and measured occupancy

`JD_LAYERS = 4`, plus the transient 5th during a base handover.

Simulated over 6 wall-hours with the v2.1 pool:

```
1 layer   2%
2 layers 23%
3 layers 59%      <-- the normal state
4 layers 15%
```

Three concurrent layers is the resting state of the machine and four is a regular
peak. That is what "layer upon layer" should feel like.

---

## 5. (d) How a layer picks its routine

### 5.1 The tag table

Selection is driven by four measured properties per routine, not by hand-labelling.
Full table in §9; the generator is in §10.

| tag | measurement | used for |
|---|---|---|
| `role` | screen coverage at frame 320 and 1900 | which slot may host it |
| `cost_ms` | `harness bench` | the frame budget |
| `delta` | `harness delta 300` | the motion budget |
| `accum` | coverage or luma growth ≥ 1.35× over a segment | the "never two accumulators" rule |

Role thresholds:

```c
role = cov >= 0.80 ? GROUND : cov >= 0.45 ? FIELD : cov >= 0.15 ? FIGURE : SPARK;
```

The reasoning is compositional, not arbitrary. A GROUND routine writes a bright pixel
almost everywhere, so it can only be a ground — stacked as an overlay it obliterates
whatever is under it. A SPARK routine is mostly near-black with bright figures, so
under a screen or normalized blend its dark ground contributes nothing and only its
figures survive. That is precisely what makes a layer read as *added to* the image
rather than *replacing* it.

### 5.2 Slot eligibility and the admission predicate

```c
static const uint8_t slot_roles[4][2] = {
    /* base   */ { JD_GROUND, JD_FIELD  },   /* weighted 3:1 toward GROUND */
    /* mid    */ { JD_FIELD,  JD_FIGURE },   /* 2:2 */
    /* accent */ { JD_FIGURE, JD_SPARK  },   /* 2:3 */
    /* spark  */ { JD_SPARK,  JD_FIGURE },   /* 3:1 */
};

#define JD_BUDGET_MS   11.0f    /* sum of layer renders + composite, §7 */
#define JD_DELTA_CAP    6.0f    /* weighted motion sum; house rule is 8 */

static int admissible(uint16_t r, int slot)
{
    /* 1. no routine may be live twice — static pattern state would corrupt */
    for (int i = 0; i < JD_LAYERS; i++)
        if (g_layer[i].live && g_layer[i].routine == r) return 0;

    /* 2. never two heavy accumulators: they both build up, and the stack turns
     *    to mud with no negative space left for either to read against */
    if (jd_tag[r].accum && live_accumulators() >= 1) return 0;

    /* 3. frame budget: renders + the composite pass for the resulting depth */
    if (stack_cost_ms(r) + composite_cost_ms(live_count() + 1) > JD_BUDGET_MS) return 0;

    /* 4. motion budget: weight each layer's measured delta by its peak opacity.
     *    This is the no-strobe rule, enforced at selection time. */
    if (stack_delta(r, slot) > JD_DELTA_CAP) return 0;

    /* 5. asm modes (0..23) are ground-only: they read jd_palette directly and
     *    cannot take a layer palette window (§3.3). draw.s is resolution-agnostic,
     *    so this is taste, not a technical limit. */
    if (r < 24 && slot != 0) return 0;

    return 1;
}
```

Rule 4 is the important one and it is why the motion budget must be checked at
*selection*, not just verified afterwards. The four jumpiest routines sum to a
weighted delta of **24.1** against a house cap of 8 — a scheduler that ignores motion
will assemble a strobing stack out of four individually-compliant patterns. Likewise
the four most expensive sum to **26.4 ms**, four times over budget. Both budgets are
load-bearing; neither is decoration.

Measured over 6 hours with both budgets active: composite delta p50 **2.74**,
p95 **5.89**, max **6.00** — never once near the cap of 8.

### 5.3 Why "never two accumulators" is rule 2 and not a style note

26 of 100 routines are accumulators by measurement. An accumulator's whole idea is
that the frame fills up over its lifetime — negative space is the thing it is
spending. Two of them co-resident both spend it, and by the second minute there is no
unlit pixel left for either to draw against. The measured signature is unambiguous
(pattern 042 goes from 0.104 coverage at frame 320 to 0.943 at frame 1900), so this
does not need a human judgement call — it is a flag on the tag table.

---

## 6. Pseudocode against `bridge.c`

Structures follow `transitions.md` §1; only the scheduling members are new.

```c
/* ---- tags, generated alongside palette.bin (see §10) ---- */
typedef struct {
    uint8_t  role;      /* JD_GROUND | JD_FIELD | JD_FIGURE | JD_SPARK */
    uint8_t  accum;     /* 1 = heavy accumulator */
    uint16_t cost_q8;   /* render ms, Q8 fixed point */
    uint16_t delta_q8;  /* frame-to-frame channel delta, Q8 */
} jd_tag_t;
extern const jd_tag_t jd_tag[];          /* 24 asm + jd_pattern_count entries */

/* ---- scheduler state ---- */
typedef struct {
    int      t_rest;     /* frame at which this slot may spawn again */
    uint8_t  handover;   /* base only: an outgoing tenant is still fading */
} jd_slot;

static jd_slot  g_slot[JD_LAYERS];
static jd_bag   g_bag[JD_NROLE];

static const uint16_t PEAK_Q8[JD_LAYERS] = { 256, 166, 115, 72 };  /* 1.0 .65 .45 .28 */
static const uint16_t LIFE_S[JD_LAYERS][6] = {
    { 43, 47, 53, 59, 61, 67 }, { 29, 31, 37, 41, 0, 0 },
    { 17, 19, 23,  0,  0,  0 }, { 11, 13, 17,  0, 0, 0 },
};

/* ---- budget accounting, all Q8 milliseconds ---- */
static uint32_t composite_cost_q8(int n)
{
    static const uint16_t c[5] = { 0, 0, 66, 144, 181 };   /* 0 / .256 / .562 / .708 ms */
    return c[n < 4 ? n : 4];
}

static uint32_t stack_cost_q8(uint16_t extra)
{
    uint32_t s = 0; int n = 0;
    for (int i = 0; i < JD_LAYERS; i++)
        if (g_layer[i].live) { s += jd_tag[g_layer[i].routine].cost_q8; n++; }
    if (extra != 0xFFFF) { s += jd_tag[extra].cost_q8; n++; }
    return s + composite_cost_q8(n);
}

static uint32_t stack_delta_q8(uint16_t extra, int slot)
{
    uint32_t d = 0;
    for (int i = 0; i < JD_LAYERS; i++)
        if (g_layer[i].live)
            d += (jd_tag[g_layer[i].routine].delta_q8 * PEAK_Q8[i]) >> 8;
    if (extra != 0xFFFF)
        d += (jd_tag[extra].delta_q8 * PEAK_Q8[slot]) >> 8;
    return d;
}

/* ---- per-frame scheduler tick, called from jd_frame() before rendering ---- */
static void jd_schedule(int frame)
{
    for (int s = 0; s < JD_LAYERS; s++) {
        jd_layer *L = &g_layer[s];

        /* retire */
        if (L->live && frame >= L->t_end) {
            L->live = 0;
            if (s == 0) g_slot[0].handover = 0;
            else g_slot[s].t_rest = frame + rest_frames(s, mix32(frame ^ s));
        }

        /* base handover: start the successor while the incumbent still fades */
        if (s == 0 && L->live && !g_slot[0].handover && frame >= L->t_out) {
            if (try_spawn(0, frame)) g_slot[0].handover = 1;   /* into the shadow slot */
            continue;
        }

        /* spawn */
        if (!L->live && frame >= g_slot[s].t_rest && !g_slot[0].handover)
            if (!try_spawn(s, frame))
                g_slot[s].t_rest = frame + 120;                /* refused: retry in 2 s */
    }
}

static int try_spawn(int slot, int frame)
{
    uint32_t r = mix32((uint32_t)frame * 2654435761u + (uint32_t)slot);
    int role   = pick_role(slot, r);                  /* weighted, §5.2 */
    uint16_t v = bag_draw(&g_bag[role], admissible_cb, &slot);
    if (v == 0xFFFF) return 0;

    jd_layer *L = &g_layer[slot];
    L->routine = v;
    L->seed    = mix32(r ^ 0xA5A5A5A5u);
    L->live    = 1;
    L->blend   = (slot == 0) ? JD_MIX : JD_SCREEN;    /* transitions.md §2.4 */

    int fin  = FADE_IN_F[slot], fout = FADE_OUT_F[slot];
    int hold = LIFE_S[slot][r % life_count(slot)] * 60;
    L->t_in   = frame;
    L->t_full = frame + fin;
    L->t_out  = frame + fin + hold;
    L->t_end  = frame + fin + hold + fout;

    layer_palette(L->pal, g_blend, SPAN[slot], (uint16_t)(L->seed & JD_PAL_MASK));
    return 1;
}

/* ---- render, replacing the body of jd_frame() ---- */
void jd_frame(uint32_t *fb, int w, int h, int frame)
{
    if (jd_mode_override(fb, w, h, frame)) return;    /* JD_MODE=N, unchanged */

    jd_palette_walk(frame, &g_A, &g_B, &g_t);         /* transitions.md §3.2 */
    palette_update(g_A, g_B, g_t);                    /* cached, transitions.md §3.5 */
    jd_schedule(frame);

    int idx[JD_LAYERS]; uint16_t nw[JD_LAYERS];
    int n = composite_prepare(frame, idx, nw);        /* transitions.md §2.2 */

    for (int k = 0; k < n; k++) {
        jd_layer *L = &g_layer[idx[k]];
        int sl = frame - L->t_in;                     /* LAYER-LOCAL: the whole trick */
        if (falling(L, frame) && nw[k] < JD_FREEZE_W) continue;   /* §5.3 of transitions */
        if (L->routine < 24) {
            g_mode = L->routine; g_sl = sl; g_seed = L->seed;
            draw_frame(L->buf, w, h, frame);
        } else {
            jd_patterns[L->routine - 24](L->buf, w, h, frame, sl, L->seed, L->pal);
        }
    }
    jd_composite(fb, w * h, n, idx, nw);
}
```

Note the single most important line: `int sl = frame - L->t_in`. Making `sl`
layer-local is what lets an accumulator clear on its own frame 0 — which is a frame
where its weight is 0, so the blank happens off-screen. No pattern has to change, and
the hard blank J has been seeing every 34 seconds simply stops existing.

---

## 7. Performance budget

### 7.1 Measured components, 1280×960

| Component | Measured |
|---|---|
| Pattern render, min / median / p90 / max | 0.19 / 2.87 / 4.06 / **9.11** ms |
| Composite n=2 (specialized lerp) | 0.256 ms |
| Composite n=3 (fixed-n, unrolled) | 0.562 ms |
| Composite n=4 (fixed-n, unrolled) | 0.708 ms |
| Composite n=3 (generic variable loop) | 2.308 ms |
| Composite n=4 (generic variable loop) | 2.867 ms |
| Palette rebuild, 32768 entries | 0.007 ms |
| `SDL_UpdateTexture` (4.9 MB) | 0.412 ms |
| SDL clear + copy + present, CPU side | ~0.4 ms |

> **Specialize the compositor by layer count.** `transitions.md` §2.3 gives a generic
> `compN` with a variable-length inner loop. That loop cannot vectorize: measured
> **2.31 ms at n=3 and 2.87 ms at n=4**, versus **0.56 / 0.71 ms** for fixed-n
> unrolled versions of identical arithmetic. A 4× regression hiding in the innermost
> loop of the engine. Write `comp3()` and `comp4()` explicitly and switch on `n`.

### 7.2 The budget

```
16.67 ms   one frame at 60 Hz
 −0.80     SDL upload + present, CPU side
 −0.71     composite, worst case (n=4)
 −0.01     palette rebuild
 =15.15    available for layer renders
```

`JD_BUDGET_MS = 11.0` for renders + composite, leaving **~4.9 ms** of headroom for
thermal throttling, other applications, and the base-handover transient. Simulated
over 6 wall-hours:

```
frame work (renders + composite)   p50 9.22   p95 10.85   max 11.00 ms
worst frame incl. SDL                                     11.80 ms
implied floor                                             85 fps
```

The engine never comes within 4.8 ms of missing a 60 Hz frame.

### 7.3 The 120 Hz problem — cap the frame rate

§0.2C measured `RenderPresent` blocking for exactly one refresh of whichever display
owns the window: 16.67 ms external, 8.33 ms on the built-in ProMotion panel. At
9–11 ms of work the app cannot make 120 Hz, so on the laptop screen it will present
on alternating refreshes and the *pacing* will wobble between 8.3 and 16.7 ms.

For slow, relaxing material that wobble is visible and it is exactly the class of
defect J has rejected builds for. **Cap the app at 60 fps explicitly** rather than
racing a target it cannot hit:

```c
/* main.c — even pacing beats maximum pacing for a screensaver */
uint64_t next = SDL_GetPerformanceCounter();
const uint64_t period = SDL_GetPerformanceFrequency() / 60;
/* ... after RenderPresent ... */
next += period;
int64_t slack = (int64_t)(next - SDL_GetPerformanceCounter());
if (slack > 0) SDL_Delay((uint32_t)(slack * 1000 / SDL_GetPerformanceFrequency()));
```

This also halves power draw on the laptop, which matters for something left running.

### 7.4 Optional: half-resolution upper layers

Measured, rendering a layer at 640×480 and upscaling on composite:

| | measured |
|---|---|
| Render speedup at half res | **3.8–4.4×** (consistent across 10 patterns) |
| Naive scalar bilinear upscale + screen | 1.75 ms |
| **Packed two-lane** upscale + screen | **0.96 ms** |

With the packed version, break-even is a full-res cost of ~1.3 ms, so half-res wins
for essentially every routine in the library (median 2.87 ms). It is **not needed** —
§7.2 already fits with 4.9 ms to spare — but it is the lever to reach if the doubled
pattern set turns out heavier than the current one, or if a 120 Hz target is ever
wanted. Keep the base at full resolution regardless; upscaling the ground is visible.

---

## 8. Directive for the "double the routines and palettes" work

This falls out of §0.2B and is the part most likely to be got wrong. **Doubling the
library uniformly will not fix repetition**, because the schedule does not consume
roles uniformly — it burns dark, sparse routines about 20× faster than full-screen
ones.

Measured draw rates and the resulting no-repeat interval:

| role | v2.0 pool | draw rate | min gap today | target pool | min gap achieved |
|---|---|---|---|---|---|
| GROUND | 42 | 1 / 527 s | never repeats | **56** | never repeats |
| FIELD | 22 | 1 / 41 s | 1.4 min | **44** | **4.8 min** |
| FIGURE | 23 | 1 / 25 s | 1.7 min | **60** | **5.9 min** |
| SPARK | 13 | 1 / 23 s | **0.8 min** | **64** | **3.8 min** |

So the 100 new patterns should be authored roughly:

```
+14 GROUND   (cov >= 0.80)     full-screen grounds — the library already has plenty
+22 FIELD    (cov 0.45-0.80)
+37 FIGURE   (cov 0.15-0.45)   bright figures on a dark ground
+27 SPARK    (cov < 0.15)      sparse highlights on near-black
```

**Roughly two thirds of the new work should be dark-ground, low-coverage material.**
Those are the routines layering actually consumes, and they are what the current
library is starved of. Two further constraints:

- Keep new routines under **4 ms** at 1280×960. Twelve of the existing 100 exceed
  that and they are the ones the budget keeps refusing; a 9.11 ms routine
  (`027 Wedge Ripples`) can essentially only ever be a solo ground.
- Keep new routines under **delta 4**. Ten existing patterns exceed it and they
  crowd out two other layers each under the motion budget.

For palettes, `palettes.md` §5 already sets the class targets and caps
`full_spectrum` at 4 of 60. That cap is what makes §3.2's adjacency repair work —
above about 10 of 60 the tail of the permutation runs out of non-rainbow candidates
and same-class adjacencies come back.

---

## 9. Measured tag table

Cost in ms at 1280×960; `delta` is mean per-channel frame-to-frame change;
`cov` is the greater of coverage at frame 320 and frame 1900; `acc` marks
accumulators. This is the data `jd_tag[]` is generated from.

| #  | name | role | cost ms | delta | cov | acc |
|----|------|------|---------|-------|-----|-----|
| 001 | Kaleido Rose           | GROUND |  2.23 | 2.26 | 1.00 | . |
| 002 | P4M Quilt              | GROUND |  1.85 | 1.07 | 1.00 | Y |
| 003 | Hex Snowfold           | GROUND |  2.31 | 1.85 | 1.00 | Y |
| 004 | Mirror Truchet         | FIGURE |  3.86 | 0.75 | 0.44 | Y |
| 005 | Breathing Fold         | GROUND |  2.00 | 2.90 | 1.00 | . |
| 006 | Tri Morph              | GROUND |  2.95 | 1.55 | 1.00 | . |
| 007 | Pinwheel Spiral        | GROUND |  1.90 | 1.04 | 1.00 | . |
| 008 | Octa Mandala           | GROUND |  1.73 | 0.84 | 1.00 | . |
| 009 | Lissa Web              | FIGURE |  3.53 | 2.43 | 0.39 | . |
| 010 | Mosaic Quilt           | GROUND |  2.20 | 0.96 | 0.92 | Y |
| 011 | Plasma Mandala         | FIELD  |  3.08 | 5.28 | 0.69 | . |
| 012 | Rotozoom Kaleido       | FIELD  |  2.91 | 5.77 | 0.66 | . |
| 013 | Tunnel Bloom           | GROUND |  2.82 | 5.91 | 0.98 | . |
| 014 | Copper Octarings       | GROUND |  1.11 | 1.68 | 0.84 | Y |
| 015 | Twister Star           | FIELD  |  4.87 | 1.08 | 0.63 | . |
| 016 | Shadebob Rosette       | FIELD  |  2.84 | 1.97 | 0.68 | . |
| 017 | Metaball Kiss          | GROUND |  3.06 | 1.26 | 1.00 | . |
| 018 | Kefrens Spiral         | FIELD  |  3.32 | 4.04 | 0.76 | . |
| 019 | Moire Silk             | GROUND |  4.06 | 3.14 | 0.95 | . |
| 020 | Feedback Fractal       | GROUND |  4.73 | 0.51 | 0.93 | . |
| 021 | Ripple Duet            | GROUND |  3.95 | 3.45 | 0.85 | Y |
| 022 | XOR Rings              | FIELD  |  1.29 | 5.78 | 0.73 | . |
| 023 | Silk Gratings          | GROUND |  4.02 | 3.28 | 0.96 | . |
| 024 | Munch Frost            | FIELD  |  0.37 | 1.39 | 0.74 | . |
| 025 | Ember Triad            | GROUND |  3.52 | 5.33 | 0.87 | . |
| 026 | Spoke Moire            | FIELD  |  4.99 | 4.69 | 0.62 | . |
| 027 | Wedge Ripples          | GROUND |  9.11 | 6.65 | 0.97 | Y |
| 028 | Quasicrystal           | FIELD  |  3.34 | 5.61 | 0.62 | . |
| 029 | Tartan Beat            | FIELD  |  3.24 | 1.87 | 0.68 | . |
| 030 | Octa Facets            | FIELD  |  2.28 | 2.65 | 0.76 | . |
| 031 | Spirograph Bloom       | FIGURE |  1.33 | 0.22 | 0.23 | . |
| 032 | Harmonograph Veil      | FIGURE |  1.34 | 0.10 | 0.39 | . |
| 033 | Rose Engine            | FIGURE |  1.35 | 0.10 | 0.34 | . |
| 034 | Lissajous Weave        | FIGURE |  1.32 | 0.32 | 0.27 | . |
| 035 | String Cardioid        | FIELD  |  1.25 | 0.15 | 0.48 | . |
| 036 | Epicycle Lace          | FIGURE |  1.36 | 0.10 | 0.41 | Y |
| 037 | Pendulum Web           | FIGURE |  1.24 | 0.13 | 0.44 | Y |
| 038 | Guilloche Rings        | FIGURE |  1.25 | 0.06 | 0.26 | Y |
| 039 | Butterfly Lace         | SPARK  |  1.40 | 0.05 | 0.12 | . |
| 040 | Maurer Rose            | FIGURE |  1.23 | 0.12 | 0.37 | Y |
| 041 | Cyclic Bloom           | GROUND |  1.05 | 0.77 | 0.87 | . |
| 042 | BZ Pinwheel            | GROUND |  2.88 | 1.70 | 0.94 | Y |
| 043 | Turing Labyrinth       | GROUND |  2.16 | 0.71 | 0.83 | Y |
| 044 | Majority Quilt         | GROUND |  2.19 | 0.90 | 0.90 | Y |
| 045 | Lenia Bloom            | GROUND |  2.31 | 1.29 | 0.83 | Y |
| 046 | RPS Trichrome          | GROUND |  3.05 | 2.99 | 1.00 | . |
| 047 | Target Choir           | GROUND |  2.66 | 1.36 | 0.96 | Y |
| 048 | Excitable Spirals      | GROUND |  2.09 | 1.17 | 0.93 | Y |
| 049 | Voronoi Breath         | GROUND |  2.84 | 2.14 | 0.85 | . |
| 050 | Turing Spots           | GROUND |  2.84 | 1.39 | 1.00 | Y |
| 051 | Firework Garden        | FIELD  |  3.40 | 0.95 | 0.47 | . |
| 052 | Orbit Weave            | SPARK  |  3.42 | 1.12 | 0.06 | . |
| 053 | Gravity Rose           | SPARK  |  3.38 | 1.99 | 0.08 | . |
| 054 | Starfield Warp         | SPARK  |  3.29 | 1.16 | 0.06 | . |
| 055 | Comet Carousel         | SPARK  |  3.30 | 2.35 | 0.05 | . |
| 056 | Ribbon Swarm           | SPARK  |  3.27 | 4.26 | 0.11 | . |
| 057 | Galaxy Pinwheel        | SPARK  |  3.30 | 1.20 | 0.08 | . |
| 058 | Fountain Arcs          | FIGURE |  3.19 | 2.79 | 0.19 | . |
| 059 | Binary Dance           | SPARK  |  3.52 | 0.62 | 0.02 | . |
| 060 | Meteor Veil            | SPARK  |  3.72 | 1.06 | 0.06 | . |
| 061 | Checker Tunnel         | FIELD  |  1.95 | 1.03 | 0.71 | . |
| 062 | Twist Corridor         | GROUND |  3.08 | 1.28 | 0.91 | Y |
| 063 | Spiral Zoom            | GROUND |  1.05 | 0.42 | 0.99 | . |
| 064 | Starburst Forge        | FIELD  |  2.32 | 1.63 | 0.73 | . |
| 065 | Echo Mandala           | GROUND |  2.31 | 3.38 | 1.00 | . |
| 066 | Hex Tunnel             | FIGURE |  4.09 | 0.98 | 0.24 | . |
| 067 | Twin Tunnels           | FIELD  |  2.17 | 1.92 | 0.70 | . |
| 068 | Wormhole Pond          | FIELD  |  3.03 | 2.84 | 0.74 | . |
| 069 | Pillar Hall            | FIGURE |  3.12 | 0.79 | 0.31 | . |
| 070 | Vortex Petals          | GROUND |  1.74 | 1.13 | 0.89 | . |
| 071 | Silk Currents          | SPARK  |  4.24 | 1.66 | 0.08 | . |
| 072 | Vine Waltz             | FIGURE |  3.00 | 0.40 | 0.19 | . |
| 073 | Frost Court            | SPARK  |  3.16 | 0.18 | 0.08 | . |
| 074 | Slow Lightning         | SPARK  |  3.31 | 0.07 | 0.06 | . |
| 075 | Coral Lace             | FIGURE |  2.85 | 0.67 | 0.31 | Y |
| 076 | Kelp Cathedral         | FIGURE |  2.82 | 0.27 | 0.33 | . |
| 077 | Mycelium Veil          | FIGURE |  2.91 | 0.81 | 0.30 | Y |
| 078 | Turing Garden          | GROUND |  2.88 | 0.08 | 1.00 | . |
| 079 | Golden Bloom           | FIGURE |  2.93 | 1.43 | 0.16 | . |
| 080 | Tendril Rose           | FIGURE |  2.86 | 0.52 | 0.21 | Y |
| 081 | Poly Morph             | FIGURE |  3.34 | 0.35 | 0.19 | . |
| 082 | Greek Key              | GROUND |  6.07 | 1.80 | 0.96 | . |
| 083 | Patch Quilt            | GROUND |  3.93 | 0.70 | 0.84 | . |
| 084 | Gear Rosettes          | GROUND |  4.27 | 2.25 | 0.98 | . |
| 085 | Vector Machine         | FIELD  |  6.18 | 0.37 | 0.61 | Y |
| 086 | Gem Orbit              | SPARK  |  2.37 | 1.02 | 0.10 | . |
| 087 | Chevron Court          | FIELD  |  2.70 | 0.49 | 0.71 | . |
| 088 | Star Cross             | GROUND |  5.03 | 1.02 | 0.83 | . |
| 089 | Oval Drums             | GROUND |  1.50 | 2.13 | 0.99 | . |
| 090 | Diamond Burst          | FIGURE |  3.80 | 0.61 | 0.26 | . |
| 091 | Diamond Confetti       | FIGURE |  1.30 | 1.39 | 0.36 | . |
| 092 | Greek Key Panel        | GROUND |  0.29 | 0.77 | 1.00 | Y |
| 093 | Cathedral Fan          | FIGURE |  3.87 | 0.53 | 0.38 | . |
| 094 | Thread Web             | GROUND |  0.19 | 0.46 | 0.99 | . |
| 095 | Ring Machine           | FIELD  |  3.29 | 0.32 | 0.49 | . |
| 096 | Scanline Butterfly     | FIELD  |  2.41 | 0.78 | 0.51 | . |
| 097 | Magenta Fireworks      | GROUND |  0.33 | 0.15 | 1.00 | Y |
| 098 | Gear Flower Quad       | GROUND |  2.62 | 0.57 | 1.00 | . |
| 099 | Racetrack Drums        | GROUND |  2.00 | 1.80 | 0.86 | . |
| 100 | Pinwheel Swirl         | FIGURE |  2.05 | 0.59 | 0.53 | Y |

The 24 asm modes still need the same treatment. `draw.s` is fully
resolution-agnostic (no hardcoded 1280/960 anywhere), so the same harness works —
point it at `draw_frame` with `g_mode` set.

---

## 10. Verification

### 10.1 Regenerate the tag table

```sh
# cost + motion, all patterns
cd patterns_c
for p in pattern_*.c; do n=${p#pattern_}; n=${n%.c}
  clang -O2 -DPATTERN=pattern_$n harness.c $p -o /tmp/t -lm
  echo "$n $(/tmp/t bench) $(/tmp/t delta 300)"
done

# coverage / role / accumulation: the analyser renders to frame 320 and 1900
# and reports mean luma, coverage, hue bins for each.
```

### 10.2 Scheduler assertions (unit tests, no rendering)

| Property | Assertion |
|---|---|
| bag completeness | over `n` consecutive draws from a bag, all `n` ids appear exactly once |
| seam | min gap between repeats ≥ `W` over 10 000 draws |
| uniformity | per-id usage within ±1 over 100 cycles |
| **deferred admission** | with a predicate rejecting 80 % of items, completeness and uniformity **still hold** — this is the test the naive design fails |
| no duplicate live | no two slots ever hold the same routine, over a 100 000-frame sim |
| accumulators | `live_accumulators() <= 1` on every frame |
| budget | `stack_cost_ms <= JD_BUDGET_MS` on every frame |
| motion | `stack_delta <= JD_DELTA_CAP` on every frame |
| palette classes | no two consecutive palettes share a class; no two consecutive LOUD |

### 10.3 Visual, non-negotiable

- Watch one full base handover. There must be no darkening or brightening at the
  crossover point.
- Watch an accumulator retire. It must dissolve, never blank.
- Confirm the frame is never empty: `sum(w_i) > 0` asserted in the debug build.
- Whole-engine motion check **around scheduled transitions**, not at random frames —
  transitions are the only place the budget is at risk:

```sh
clang -O2 -I. <harness using jd_frame> bridge.c patterns_c/pattern_*.c \
      patterns_c/registry.c draw.s -o /tmp/dump
# for each t_in / t_out, dump [t-30, t+D+30] and assert mean delta < 8
```

---

## 11. Conflicts with the sibling documents

Three places where this document and `transitions.md` disagree. They need one
decision each before implementation.

**1. Layer occupancy.** `transitions.md` §1.2 schedules
`1→25% 2→45% 3→25% 4→5%`. This document schedules `1→2% 2→23% 3→59% 4→15%`
(measured over 6 simulated hours). J asked for "building layer upon layer"; a
distribution that is 70 % one-or-two layers does not deliver that, and the measured
budget shows three layers costs 9.2 ms of a 15.15 ms allowance — there is no
performance reason to stay thin. **Recommend this document's schedule.**

**2. Composite implementation.** `transitions.md` §2.3's generic `compN` measures
2.31 ms at n=3 and 2.87 ms at n=4 because the variable-length inner loop blocks
vectorization. Fixed-n `comp3`/`comp4` with identical arithmetic measure 0.56/0.71 ms.
**Recommend fixed-n specializations**; the budget in §7.2 assumes them.

**3. Palette per layer.** `transitions.md` §3.1 says layers share the palette
outright. This document proposes a shared *walk* with a per-layer window (§3.3), which
preserves the "one image changing shape" property while keeping the stack from
collapsing onto identical hues. The window costs 0.007 ms per layer.
**Recommend the windowed form**; it is a strict superset — `span = 32768, offset = 0`
reduces exactly to `transitions.md`'s behaviour.

Additionally, for `palettes.md`: the `gen_tables.py` cyclic-ramp expansion
(§0.2A) turns restrained artist palettes into full-spectrum sweeps. No amount of
curation or scheduling can recover hue restraint that the generator has already
averaged out. That expansion has to change alongside the new palette authoring.

---

## 12. Build order

Each step builds, runs, and strictly improves on the one before.

1. **Tag table.** Generate `jd_tag[]` from the harness. Pure data, zero risk, and
   nothing else can be tested without it.
2. **Routine bag in the existing single-routine dispatcher.** No layers yet. Fixes
   measured problem 1 on its own: 110/124 → 124/124 distinct, 40 → 2 in-window
   repeats. Independently shippable.
3. **Palette class repair** on top of `transitions.md` §3.3's bag. Independently
   shippable.
4. **Two slots**, base + mid, dissolve only. The `n == 1` path must be byte-identical
   to today's output — that is the regression test.
5. **Admission control** (cost, motion, accumulator, duplicate). Assert the budgets
   every frame in the debug build.
6. **Four slots**, base handover, layer palette windows.
7. **60 fps cap** in `main.c` (§7.3) and the fixed-n compositors (§7.1).
8. Half-res upper layers (§7.4) — only if the doubled pattern set needs it.
