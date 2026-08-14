#!/usr/bin/env python3
"""
_p31_lib.py -- authoring helpers for palettes P31..P50.

Authoring is done in OKLCh (L, C_norm 0..1, H deg) because that is exactly the
space palette_score.py measures in. Hand-picking hex and hoping is how the v2.0
set converged; here every anchor's L / C / H is stated, so the class metrics are
a direct consequence of the author's numbers.
"""
import os, sys
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from palette_score import (C_NORM, to_oklch, metrics, score, signature,
                           distance, hue_emd, order_ramp, classify,
                           CLASSES, MIN_DIST, MIN_HUE_EMD_SAME_CLASS)

# ---------------------------------------------------------------- OKLab -> sRGB

def _oklab_to_linear(L, a, b):
    l_ = L + 0.3963377774 * a + 0.2158037573 * b
    m_ = L - 0.1055613458 * a - 0.0638541728 * b
    s_ = L - 0.0894841775 * a - 1.2914855480 * b
    l, m, s = l_ ** 3, m_ ** 3, s_ ** 3
    r = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s
    g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s
    bb = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
    return r, g, bb


def _lin_to_srgb(c):
    c = max(0.0, min(1.0, c))
    return 12.92 * c if c <= 0.0031308 else 1.055 * (c ** (1 / 2.4)) - 0.055


def _in_gamut(L, Cn, H):
    a = Cn * C_NORM * np.cos(np.radians(H))
    b = Cn * C_NORM * np.sin(np.radians(H))
    r, g, bb = _oklab_to_linear(L, a, b)
    e = -1e-4
    return r >= e and g >= e and bb >= e and r <= 1.0001 and g <= 1.0001 and bb <= 1.0001


def lch(L, Cn, H):
    """OKLCh (L 0..1, C normalised by 0.33, H deg) -> 6-digit hex, gamut-clipped
    by reducing chroma only (preserves L and H, which the metrics care about)."""
    L = float(np.clip(L, 0.0, 1.0))
    if not _in_gamut(L, Cn, H):
        lo, hi = 0.0, Cn
        for _ in range(40):
            mid = 0.5 * (lo + hi)
            if _in_gamut(L, mid, H):
                lo = mid
            else:
                hi = mid
        Cn = lo
    a = Cn * C_NORM * np.cos(np.radians(H))
    b = Cn * C_NORM * np.sin(np.radians(H))
    r, g, bb = _oklab_to_linear(L, a, b)
    return '%02x%02x%02x' % tuple(int(round(_lin_to_srgb(x) * 255)) for x in (r, g, bb))


def max_chroma(L, H):
    """Largest in-gamut normalised chroma at this L and H."""
    lo, hi = 0.0, 1.6
    for _ in range(40):
        mid = 0.5 * (lo + hi)
        if _in_gamut(L, mid, H):
            lo = mid
        else:
            hi = mid
    return lo


def ramp(n, L0, L1, C0, C1, H0, H1, cbow=0.0, lbias=1.0, cfrac=None):
    """n anchors sweeping L, C, H. `cbow` adds a mid-ramp chroma bulge,
    `lbias` gamma-shapes the lightness march, `cfrac` caps C as a fraction of
    the in-gamut maximum at each stop (use 1.0 to ride the gamut edge)."""
    out = []
    for i in range(n):
        t = i / (n - 1) if n > 1 else 0.0
        tl = t ** lbias
        L = L0 + (L1 - L0) * tl
        C = C0 + (C1 - C0) * t + cbow * np.sin(np.pi * t)
        H = H0 + (H1 - H0) * t
        if cfrac is not None:
            C = min(C, cfrac * max_chroma(L, H))
        out.append(lch(L, C, H))
    return out


# ---------------------------------------------------------------- corpus

def load_corpus(root='/Users/exeter/dev/m5/assembly/dzzle1', skip=()):
    """Every palette already shipped: 24 lospec + lab/palettes/P* + 6 house
    HSV schemes from gen_tables.py, each as (name, class_or_None, colors).
    `skip` drops palette ids being re-authored, so a rerun does not measure a
    candidate against the copy of itself it wrote last time."""
    import json, glob, colorsys, re
    out = []
    for p in json.load(open(os.path.join(root, 'reference/palettes.json'))):
        out.append(('lospec:' + p['slug'], p['colors']))
    for d in sorted(glob.glob(os.path.join(root, 'lab/palettes/P*/palette.json'))):
        j = json.load(open(d))
        if j.get('id') in skip:
            continue
        cols = [c['hex'] if isinstance(c, dict) else c for c in j['colors']]
        out.append((os.path.basename(os.path.dirname(d)), cols))

    # the 6 house HSV schemes: sample each ramp at 24 stops
    src = open(os.path.join(root, 'gen_tables.py')).read()
    ns = {}
    body = src[src.index('SCHEMES = {'):src.index("ORDER = [")]
    exec(body, {'__builtins__': {}}, ns)
    for name, keys in ns['SCHEMES'].items():
        cols = []
        for i in range(24):
            p = i / 24.0
            for k in range(len(keys) - 1):
                p0, h0, s0, v0, _ = keys[k]
                p1, h1, s1, v1, _ = keys[k + 1]
                if p0 <= p <= p1:
                    t = (p - p0) / (p1 - p0); t = t * t * (3 - 2 * t)
                    dh = (h1 - h0 + 0.5) % 1.0 - 0.5
                    h = (h0 + dh * t) % 1.0
                    r, g, b = colorsys.hsv_to_rgb(h, s0 + (s1 - s0) * t,
                                                  v0 + (v1 - v0) * t)
                    cols.append('%02x%02x%02x' % (int(r * 255), int(g * 255), int(b * 255)))
                    break
        out.append(('house:' + name, cols))

    # give each existing palette a class ONLY if it genuinely passes one --
    # the same-class hue floor should not be enforced against a bad fit
    lib = []
    for name, cols in out:
        cls, fit = classify(cols)
        ok = score(cols, cls)[1]
        lib.append((name, cls if ok else None, cols))
    return lib


def fast_scores(cols):
    """Score against ALL ten classes from ONE metrics pass. classify() recomputes
    metrics ten times per call, which is the whole cost of the grid search."""
    from palette_score import _band, _pole_sep
    m = metrics(cols)
    m['pole_sep'] = _pole_sep(cols)
    out = {}
    for cls, spec in CLASSES.items():
        subs, key_ok = [], True
        for name, (lo, hi, tol) in spec['spec'].items():
            sub = _band(m[name], lo, hi, tol)
            subs.append(sub)
            if name in spec['key'] and sub < 1.0:
                key_ok = False
        sc = float(np.mean(subs))
        out[cls] = (round(sc, 3), bool(key_ok and sc >= 0.80))
    return m, out


def report(cols, cls):
    m = metrics(cols)
    s, ok, det = score(cols, cls)
    bad = [f'{k}={v}({t[0]}..{t[1]})' for k, (v, sub, t) in det.items() if sub < 1.0]
    return s, ok, bad, m


def gate(name, cls, cols, existing):
    """accept() but reporting the nearest neighbour even on success."""
    s, ok, bad, m = report(cols, cls)
    if not ok:
        return False, f'CLASS FAIL fit={s:.2f} :: ' + '; '.join(bad)
    sig = signature(cols)
    near, nn = 9.9, None
    for oname, ocls, ocols in existing:
        d = distance(sig, signature(ocols))
        if d < near:
            near, nn = d, oname
        if d < MIN_DIST:
            return False, f'TOO CLOSE to {oname} d={d:.3f}'
        if ocls == cls:
            e = hue_emd(sig, signature(ocols))
            if e < MIN_HUE_EMD_SAME_CLASS:
                return False, f'HUE OVERLAP with {oname} (same class) emd={e:.2f}'
    return True, f'fit={s:.2f} nn={nn} d={near:.3f}'
