#!/usr/bin/env python3
"""
_corpus_safe.py -- version-tolerant corpus loader for the palette gates.

_p31_lib.load_corpus() scrapes the six house HSV ramps out of gen_tables.py by
slicing between the literal strings 'SCHEMES = {' and 'ORDER = ['. gen_tables.py
is being rewritten for v2.1 in parallel with this work and those two names have
already moved once (SCHEMES -> HOUSE, ORDER -> HOUSE_ORDER), which takes the
loader -- and therefore every palette gate -- down with a ValueError.

This does the same job but probes for whichever pair of names is present, and
degrades to "no house schemes" rather than raising if the file is mid-edit.
Everything else is identical to load_corpus: 24 lospec imports + every
lab/palettes/P*/palette.json on disk + the house ramps, each returned as
(name, class_or_None, colors) with a class attached ONLY when the palette
genuinely passes it.
"""
import os, sys, json, glob, colorsys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from palette_score import classify, score

ROOT_DEFAULT = '/Users/exeter/dev/m5/assembly/dzzle1'

# (dict name, order name) pairs, newest first
_HOUSE_NAMES = [('HOUSE = {', 'HOUSE_ORDER = ['), ('SCHEMES = {', 'ORDER = [')]


def _house_schemes(root):
    """The house HSV ramps as 24-stop hex lists. [] if gen_tables has moved on."""
    try:
        src = open(os.path.join(root, 'gen_tables.py')).read()
    except OSError:
        return []
    body = None
    for dict_name, order_name in _HOUSE_NAMES:
        i = src.find(dict_name)
        j = src.find(order_name)
        if i >= 0 and j > i:
            body = src[i:j]
            key = dict_name.split(' =')[0]
            break
    if body is None:
        print('  WARN  _corpus_safe: no house scheme dict found in gen_tables.py '
              '— gating without the house ramps', file=sys.stderr)
        return []
    ns = {}
    try:
        exec(body, {'__builtins__': {}}, ns)
    except Exception as e:
        print(f'  WARN  _corpus_safe: house dict would not evaluate ({e})',
              file=sys.stderr)
        return []
    out = []
    for name, keys in ns[key].items():
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
                    cols.append('%02x%02x%02x'
                                % (int(r * 255), int(g * 255), int(b * 255)))
                    break
        if cols:
            out.append(('house:' + name, cols))
    return out


def load_corpus(root=ROOT_DEFAULT, skip=()):
    """(name, class_or_None, colors) for every palette already in the library."""
    out = []
    for p in json.load(open(os.path.join(root, 'reference/palettes.json'))):
        out.append(('lospec:' + p['slug'], p['colors']))
    for d in sorted(glob.glob(os.path.join(root, 'lab/palettes/P*/palette.json'))):
        j = json.load(open(d))
        if j.get('id') in skip:
            continue
        cols = [c['hex'] if isinstance(c, dict) else c for c in j['colors']]
        out.append((os.path.basename(os.path.dirname(d)), cols))
    out += _house_schemes(root)

    lib = []
    for name, cols in out:
        cls, fit = classify(cols)
        lib.append((name, cls if score(cols, cls)[1] else None, cols))
    return lib
