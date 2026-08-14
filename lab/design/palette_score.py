#!/usr/bin/env python3
"""
palette_score.py — the measurable definition of a JellyDazzle palette class.

Governs the 60 new v2.1 palettes. Two jobs:

  1. score(colors, cls)      -> does this palette actually BELONG to its class?
  2. accept(colors, cls, ..) -> is it far enough from every palette we already have?

Everything perceptual is measured in OKLab / OKLCh, not HSV. HSV lies about
lightness (pure blue and pure yellow are both V=1.0) and that lie is exactly
why the v2.0 set reads flat: schemes with wildly different HSV numbers land in
the same perceptual place.

Input `colors` is the ANCHOR list -- the artist-chosen stops, in ramp order --
as hex strings ('0d0b10'), the same list that lab/palettes/PNN_*/palette.json
holds and that gen_tables.py expands into a 32768-entry ramp.

Dependencies: numpy only.
"""

import numpy as np

# --------------------------------------------------------------------------
# colour space
# --------------------------------------------------------------------------

C_NORM = 0.33          # sRGB max OKLCh chroma ~0.32; normalises C into 0..1
HUE_BINS = 12          # 30-degree bins
CHROMATIC = 0.08       # C_norm below this = a neutral, gets no vote on hue


def _hex_to_rgb(colors):
    a = np.array([[int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16)]
                  for c in (h.lstrip('#') for h in colors)], float) / 255.0
    return a


def _srgb_to_linear(c):
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def to_oklab(colors):
    """hex list -> (L, a, b) arrays. L in 0..1."""
    rgb = _srgb_to_linear(_hex_to_rgb(colors))
    r, g, b = rgb[:, 0], rgb[:, 1], rgb[:, 2]
    l = 0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b
    m = 0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b
    s = 0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b
    l_, m_, s_ = np.cbrt(l), np.cbrt(m), np.cbrt(s)
    L = 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_
    A = 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_
    B = 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_
    return L, A, B


def to_oklch(colors):
    """hex list -> (L 0..1, C_norm 0..1, H degrees 0..360)."""
    L, A, B = to_oklab(colors)
    C = np.sqrt(A * A + B * B) / C_NORM
    H = np.degrees(np.arctan2(B, A)) % 360.0
    return L, C, H


def delta_e(colors):
    """OKLab distance between ADJACENT anchors in ramp order, cyclic. x100."""
    L, A, B = to_oklab(colors)
    P = np.stack([L, A, B], 1)
    return np.sqrt(((np.roll(P, -1, 0) - P) ** 2).sum(1)) * 100.0


# --------------------------------------------------------------------------
# metrics
# --------------------------------------------------------------------------

def order_ramp(colors):
    """
    Reorder anchors into a smooth CYCLIC tour in OKLab (greedy nearest-neighbour
    from the darkest anchor, then 2-opt to convergence).

    This is not cosmetic. gen_tables.py expands the anchor list in FILE ORDER,
    so a hue-grouped authoring list ('...ruby glow, gold deep...') writes a
    ~80-unit cliff straight into the ramp -- which is precisely the "rough
    break" J is seeing. Author grouped by hue family for readability, then run
    the list through here before expansion. max_step is measured on the OUTPUT.
    """
    L, A, B = to_oklab(colors)
    P = np.stack([L, A, B], 1)
    n = len(colors)
    if n < 4:
        return list(colors)
    D = np.sqrt(((P[:, None, :] - P[None, :, :]) ** 2).sum(-1))

    tour = [int(np.argmin(L))]
    left = set(range(n)) - set(tour)
    while left:
        last = tour[-1]
        nxt = min(left, key=lambda j: D[last, j])
        tour.append(nxt)
        left.discard(nxt)

    def tlen(t):
        return sum(D[t[i], t[(i + 1) % n]] for i in range(n))

    improved = True
    while improved:                               # 2-opt, cyclic
        improved = False
        for i in range(n - 1):
            for k in range(i + 2, n if i else n - 1):
                a, b = tour[i], tour[(i + 1) % n]
                c, d = tour[k], tour[(k + 1) % n]
                if D[a, b] + D[c, d] > D[a, c] + D[b, d] + 1e-12:
                    tour[i + 1:k + 1] = reversed(tour[i + 1:k + 1])
                    improved = True
    return [colors[i] for i in tour]


def _hue_hist(C, H):
    """chroma-weighted 12-bin hue histogram, sums to 1 (uniform if all-neutral)."""
    w = np.where(C >= CHROMATIC, C, 0.0)
    if w.sum() <= 1e-9:
        return np.full(HUE_BINS, 1.0 / HUE_BINS)
    idx = (H / (360.0 / HUE_BINS)).astype(int) % HUE_BINS
    h = np.bincount(idx, weights=w, minlength=HUE_BINS)
    return h / h.sum()


def _hue_arc(C, H, frac=0.90):
    """Smallest arc in degrees holding `frac` of the chroma weight."""
    w = np.where(C >= CHROMATIC, C, 0.0)
    if w.sum() <= 1e-9:
        return 0.0
    o = np.argsort(H)
    hh, ww = H[o], w[o]
    ww = ww / ww.sum()
    keep = ww > 0
    hh, ww = hh[keep], ww[keep]
    n = len(hh)
    if n == 1:
        return 0.0
    hh2 = np.concatenate([hh, hh + 360.0])
    ww2 = np.concatenate([ww, ww])
    best = 360.0
    for i in range(n):                       # window start
        acc, j = 0.0, i
        while acc < frac and j < i + n:
            acc += ww2[j]
            j += 1
        if acc >= frac - 1e-9:
            best = min(best, hh2[j - 1] - hh2[i])
    return float(min(best, 360.0))


def _dominant_arc_weight(C, H, arc=60.0):
    """Largest share of chroma weight capturable by any `arc`-degree window."""
    w = np.where(C >= CHROMATIC, C, 0.0)
    if w.sum() <= 1e-9:
        return 1.0
    w = w / w.sum()
    best = 0.0
    for start in np.arange(0.0, 360.0, 2.0):
        d = (H - start) % 360.0
        best = max(best, w[d < arc].sum())
    return float(best)


def metrics(colors):
    """All governing numbers for one palette. Anchor list in ramp order."""
    L, C, H = to_oklch(colors)
    hist = _hue_hist(C, H)
    step = delta_e(colors)
    chro = C >= CHROMATIC
    lit = chro & (L >= 0.35)          # the anchors you actually read AS colour
    return {
        'n':            len(colors),
        # --- hue geometry
        'hue_bins':     int((hist >= 0.05).sum()),        # bins holding >=5% weight
        'hue_arc':      _hue_arc(C, H),                   # deg holding 90% weight
        'accent_frac':  round(1.0 - _dominant_arc_weight(C, H, 60.0), 4),
        # --- chroma
        # C_mean is dragged down by dark anchors, so it measures "how much of
        # this palette is colour at all". C_lit measures "how saturated is the
        # colour that IS there" -- neon vs earth is a C_lit distinction, and
        # conflating the two is what let 30 different-looking specs converge.
        'C_mean':       round(float(C.mean()), 4),
        'C_lit':        round(float(C[lit].mean()) if lit.any() else 0.0, 4),
        'C_sd':         round(float(C.std()), 4),
        'C_max':        round(float(C.max()), 4),
        # --- lightness
        'L_mean':       round(float(L.mean()), 4),
        'L_sd':         round(float(L.std()), 4),
        'L_range':      round(float(L.max() - L.min()), 4),
        'dark_frac':    round(float((L < 0.30).mean()), 4),
        'light_frac':   round(float((L > 0.80).mean()), 4),
        'neutral_frac': round(float((~chro).mean()), 4),
        # --- smoothness ("no rough breaks")
        # The ramp is 32768 entries over n anchors, so an absolute step of 30
        # is ~1800 samples of smooth interpolation -- invisible. What the eye
        # catches as a "break" is one segment moving far FASTER than its
        # neighbours as the ramp scrolls. That is a ratio, not a magnitude.
        'max_step':     round(float(step.max()), 2),
        'mean_step':    round(float(step.mean()), 2),
        'step_ratio':   round(float(step.max() / max(step.mean(), 1e-6)), 2),
    }


# --------------------------------------------------------------------------
# class definitions
#
# Each entry: metric -> (lo, hi, tol). Score 1.0 inside [lo,hi], ramping to 0
# at lo-tol / hi+tol. Metrics absent from a class are unconstrained.
# `weight` marks the 2-3 metrics that DEFINE the class -- they are scored
# hard (a miss there fails the palette outright, not just docks points).
# --------------------------------------------------------------------------

INF = 1e9

CLASSES = {
 'mono_accent': {
    'blurb': 'One hue family carries the whole frame; a single opposing accent '
             'appears in under a fifth of it. Restraint is the point.',
    'key':   ['hue_bins', 'accent_frac'],
    'spec': {
        'hue_bins':    (1,    3,    1),
        'accent_frac': (0.04, 0.22, 0.07),
        'C_lit':       (0.30, 0.75, 0.15),
        'L_range':     (0.60, 1.00, 0.12),
        'step_ratio':  (0,    2.6,  0.9),
    }},

 'duotone': {
    'blurb': 'Exactly two hue poles, 110-180 deg apart, each with a full ramp. '
             'No third family. Reads as a two-ink print.',
    'key':   ['hue_bins', 'pole_sep'],
    'spec': {
        'hue_bins':    (2,    4,    1),
        'pole_sep':    (110,  180,  25),
        'hue_arc':     (110,  250,  40),
        'C_lit':       (0.35, 0.90, 0.15),
        'L_range':     (0.60, 1.00, 0.12),
        'step_ratio':  (0,    2.6,  0.9),
    }},

 'analogous': {
    'blurb': 'Neighbouring hues only -- a continuous slice of the wheel, no '
             'more than a third of a turn. Warm and cool instances are separate '
             'palettes and the diversity gate keeps them apart.',
    'key':   ['hue_arc', 'hue_bins'],
    'spec': {
        'hue_arc':     (50,   125,  25),
        'hue_bins':    (3,    5,    1),
        'accent_frac': (0.00, 0.32, 0.12),
        'C_lit':       (0.30, 0.85, 0.15),
        'L_range':     (0.60, 1.00, 0.12),
        'step_ratio':  (0,    2.4,  0.8),
    }},

 'split_complement': {
    'blurb': 'A dominant hue against the two neighbours of its opposite (or a '
             'true triad). Tension without the mud of a straight complement.',
    'key':   ['hue_bins', 'hue_arc'],
    'spec': {
        'hue_bins':    (3,    6,    1),
        'hue_arc':     (170,  290,  35),
        'accent_frac': (0.20, 0.60, 0.12),
        'C_lit':       (0.40, 0.90, 0.15),
        'L_range':     (0.65, 1.00, 0.10),
        'step_ratio':  (0,    2.8,  1.0),
    }},

 'neon_on_black': {
    'blurb': 'Half the ramp is near-black; what is lit is at maximum chroma. '
             'Electric, and the dark floor is what makes it read that way.',
    'key':   ['dark_frac', 'C_max'],
    'spec': {
        'dark_frac':   (0.40, 0.70, 0.12),
        'C_max':       (0.80, 1.00, 0.10),
        'C_lit':       (0.60, 1.00, 0.15),
        'L_mean':      (0.18, 0.48, 0.10),
        'hue_bins':    (2,    5,    1),
        'step_ratio':  (0,    3.0,  1.0),
    }},

 'pastel_wash': {
    'blurb': 'High lightness, low chroma, tight value band. Nothing in it is '
             'dark. The soft end of the whole set.',
    'key':   ['L_mean', 'dark_frac'],
    'spec': {
        'C_mean':      (0.06, 0.30, 0.08),
        'L_mean':      (0.72, 0.92, 0.08),
        'L_sd':        (0.00, 0.14, 0.05),
        'dark_frac':   (0.00, 0.08, 0.05),
        'L_range':     (0.10, 0.45, 0.12),
        'step_ratio':  (0,    2.4,  0.8),
    }},

 'metallic': {
    'blurb': 'A narrow warm or cold hue band riding a long, steep value ramp -- '
             'the specular curve of gold, copper, steel, bronze. Chroma stays '
             'modest; the shine comes from lightness, not colour. Must include '
             'a return leg (see RULE 3) or the cyclic wrap cliffs.',
    'key':   ['hue_arc', 'L_sd'],
    'spec': {
        'hue_arc':     (0,    60,   20),
        'L_sd':        (0.22, 0.40, 0.07),
        'L_range':     (0.75, 1.00, 0.10),
        'C_lit':       (0.20, 0.60, 0.12),
        'light_frac':  (0.08, 0.40, 0.10),
        'step_ratio':  (0,    2.8,  1.0),
    }},

 'stark': {
    'blurb': 'Two or three values, almost no midtones, chroma near zero or '
             'slammed. Graphic and hard -- the opposite pole from pastel. The '
             'only class permitted a high step_ratio: the cliff IS the look.',
    'key':   ['L_sd', 'neutral_frac'],
    'spec': {
        'L_sd':        (0.28, 0.50, 0.08),
        'neutral_frac':(0.35, 1.00, 0.15),
        'L_range':     (0.85, 1.00, 0.08),
        'hue_bins':    (1,    3,    1),
        'C_mean':      (0.00, 0.30, 0.10),
        'step_ratio':  (0,    5.0,  1.5),
    }},

 'earth': {
    'blurb': 'Ochre, clay, moss, bark, stone. Warm-biased mid hues at low '
             'chroma and mid lightness -- the only class that is deliberately '
             'muted rather than dark or pale.',
    'key':   ['C_lit', 'L_mean'],
    'spec': {
        'C_lit':       (0.12, 0.40, 0.08),
        'C_sd':        (0.00, 0.16, 0.06),
        'hue_arc':     (40,   160,  35),
        'L_mean':      (0.38, 0.65, 0.10),
        'L_sd':        (0.12, 0.28, 0.07),
        'step_ratio':  (0,    2.4,  0.8),
    }},

 'full_spectrum': {
    'blurb': 'The whole wheel. Legitimate as a CLASS -- illegitimate as half '
             'the library, which is what v2.0 was. Capped at 4 of 60.',
    'key':   ['hue_bins'],
    'spec': {
        'hue_bins':    (9,    12,   1),
        'hue_arc':     (240,  360,  40),
        'C_lit':       (0.45, 0.95, 0.15),
        'L_range':     (0.70, 1.00, 0.12),
        'step_ratio':  (0,    2.6,  0.9),
    }},
}

# how many of the 60 new palettes each class gets
QUOTA = {
    'mono_accent': 7, 'duotone': 7, 'analogous': 8, 'split_complement': 6,
    'neon_on_black': 6, 'pastel_wash': 5, 'metallic': 5, 'stark': 6,
    'earth': 6, 'full_spectrum': 4,
}
assert sum(QUOTA.values()) == 60


def _pole_sep(colors):
    """Angular gap between the two heaviest hue lobes. For duotone."""
    L, C, H = to_oklch(colors)
    hist = _hue_hist(C, H)
    order = np.argsort(hist)[::-1]
    a = order[0]
    for b in order[1:]:
        d = abs(a - b) % HUE_BINS
        d = min(d, HUE_BINS - d)
        if d >= 2:                        # skip the adjacent spill bin
            return float(d * (360.0 / HUE_BINS))
    return 0.0


def _band(v, lo, hi, tol):
    if lo <= v <= hi:
        return 1.0
    d = (lo - v) if v < lo else (v - hi)
    return float(max(0.0, 1.0 - d / tol)) if tol > 0 else 0.0


def score(colors, cls):
    """
    Score a palette against a class.
      -> (score 0..1, passed bool, detail dict metric -> (value, sub, target))
    passed  = every KEY metric is 1.0 and overall score >= 0.80.
    """
    if cls not in CLASSES:
        raise KeyError(f'unknown class {cls!r}; have {sorted(CLASSES)}')
    spec = CLASSES[cls]
    m = metrics(colors)
    m['pole_sep'] = _pole_sep(colors)

    detail, subs, key_ok = {}, [], True
    for name, (lo, hi, tol) in spec['spec'].items():
        v = m[name]
        sub = _band(v, lo, hi, tol)
        detail[name] = (v, round(sub, 3), (lo, hi))
        subs.append(sub)
        if name in spec['key'] and sub < 1.0:
            key_ok = False
    s = float(np.mean(subs))
    return round(s, 3), bool(key_ok and s >= 0.80), detail


def classify(colors):
    """Best-fitting class and its score -- for auditing an existing palette."""
    return max(((c, score(colors, c)[0]) for c in CLASSES), key=lambda x: x[1])


# --------------------------------------------------------------------------
# diversity
# --------------------------------------------------------------------------

TONE_KEYS = ('L_mean', 'L_sd', 'C_mean', 'C_sd', 'dark_frac', 'light_frac')
TONE_SCALE = np.array([0.30, 0.18, 0.30, 0.15, 0.45, 0.30])


def signature(colors):
    """(hue histogram 12, tone vector 6) -- the palette's identity."""
    L, C, H = to_oklch(colors)
    m = metrics(colors)
    return _hue_hist(C, H), np.array([m[k] for k in TONE_KEYS])


def _cyclic_emd(p, q):
    """Earth-mover distance on a cyclic histogram, in bins. Max = 6 for n=12."""
    f = np.cumsum(p - q)
    return float(np.abs(f - np.median(f)).sum())


def hue_emd(a, b):
    """Hue-profile separation in BINS (1 bin = 30 deg). Max 6."""
    return round(_cyclic_emd(a[0], b[0]), 4)


def distance(a, b):
    """
    Perceptual distance between two palettes, 0..~1.0.
      hue term  -- where the colour actually is on the wheel (cyclic EMD)
      tone term -- how light / how saturated / how much black
    A palette can be far in either one and be a genuinely different look.
    """
    (ha, ta), (hb, tb) = a, b
    hue = _cyclic_emd(ha, hb) / 6.0                       # 0..1
    tone = float(np.linalg.norm((ta - tb) / TONE_SCALE)) / np.sqrt(len(TONE_KEYS))
    return round(0.60 * hue + 0.40 * min(tone, 1.5), 4)


# ---- floors, calibrated against the shipped corpus (24 lospec + 15 lab, n=54)
# Measured nearest-neighbour distance in that corpus:
#     min 0.064   p25 0.098   MEDIAN 0.115   p75 0.147   max 0.242
# 0.115 is the number behind "they are all basically similar". The gate's
# worst offenders are exactly the pairs a human would name:
#     resurrect-64 ~ endesga-32  d=0.064
#     cyberpunk-neons ~ ink-crimson d=0.072
# MIN_DIST is set at ~2x the corpus median, above the corpus p25 of all-pairs
# (0.186). Same-class pairs get the tone term for free (the class pins tone),
# so they are additionally required to separate on HUE alone.
# Feasibility was simulated before fixing the floor: 60 palettes built on the
# class x hue-rotation grid reach median NN 0.148; adding tone variation inside
# each class band lifts that to 0.189. A floor of 0.22 admitted only 13 of 54 --
# infeasible. 0.16 sits above the shipped corpus p75 (0.147) and is reachable.
MIN_DIST = 0.16
MIN_HUE_EMD_SAME_CLASS = 1.2      # bins; 1 bin == 30 deg of hue-profile shift

# Corpus-level targets for the finished 60 (see library_report).
TARGET_MEDIAN_NN = 0.20           # shipped v2.0 library scores 0.115


def accept(colors, cls, existing):
    """
    Gate a candidate palette against its class and the whole existing library.
      existing: list of (name, cls, colors) already accepted.
      -> (ok bool, reason str, detail dict)
    """
    s, passed, detail = score(colors, cls)
    if not passed:
        bad = [f'{k}={v}' for k, (v, sub, t) in detail.items() if sub < 1.0]
        return False, f'class fit {s:.2f} -- off-target: {", ".join(bad)}', detail

    sig = signature(colors)
    nearest, nname = 9.9, None
    for name, ocls, ocols in existing:
        osig = signature(ocols)
        d = distance(sig, osig)
        if d < nearest:
            nearest, nname = d, name
        if d < MIN_DIST:
            return False, (f'too close to {name} (d={d:.3f} < {MIN_DIST})'), detail
        if ocls == cls:
            e = hue_emd(sig, osig)
            if e < MIN_HUE_EMD_SAME_CLASS:
                return False, (f'same class as {name} and hue profiles overlap '
                               f'(emd={e:.2f} < {MIN_HUE_EMD_SAME_CLASS} bins) '
                               f'-- rotate this one to a different hue family'), detail
    return True, f'ok (fit {s:.2f}, nearest {nname} d={nearest:.3f})', detail


def library_report(lib):
    """
    Corpus-level gate for the finished set. Per-pair `accept` stops duplicates;
    this stops the whole library from clustering the way v2.0 did.
      lib: list of (name, cls, colors)
      -> dict, with 'ok' True when the set is genuinely varied.
    """
    sigs = [(n, c, signature(cols)) for n, c, cols in lib]
    k = len(sigs)
    D = np.full((k, k), 9.0)
    for i in range(k):
        for j in range(k):
            if i != j:
                D[i, j] = distance(sigs[i][2], sigs[j][2])
    nn = D.min(1)
    worst = int(np.argmin(nn))
    counts = {}
    for _, c, _ in lib:
        counts[c] = counts.get(c, 0) + 1
    return {
        'n':            k,
        'min_nn':       round(float(nn.min()), 4),
        'median_nn':    round(float(np.median(nn)), 4),
        'tightest':     (sigs[worst][0], sigs[int(np.argmin(D[worst]))][0]),
        'class_counts': counts,
        'quota_ok':     counts == QUOTA,
        'ok': bool(nn.min() >= MIN_DIST
                   and np.median(nn) >= TARGET_MEDIAN_NN
                   and counts == QUOTA),
    }


# --------------------------------------------------------------------------

if __name__ == '__main__':
    import json, sys, glob, os
    lab = '/Users/exeter/dev/m5/assembly/dzzle1/lab/palettes'
    rows = []
    for d in sorted(glob.glob(os.path.join(lab, 'P*/palette.json'))):
        j = json.load(open(d))
        cols = j['colors'] if isinstance(j.get('colors'), list) else []
        cols = [c['hex'] if isinstance(c, dict) else c for c in cols]
        if cols:
            rows.append((os.path.basename(os.path.dirname(d)), cols))
    print(f'{"palette":26s} {"best class":18s} fit  bins arc  Cmn Lmn Lsd drk step')
    for name, cols in rows:
        c, s = classify(cols)
        m = metrics(cols)
        print(f'{name:26s} {c:18s} {s:.2f} {m["hue_bins"]:3d} '
              f'{m["hue_arc"]:4.0f} {m["C_mean"]:.2f} {m["L_mean"]:.2f} '
              f'{m["L_sd"]:.2f} {m["dark_frac"]:.2f} {m["max_step"]:5.1f}')
