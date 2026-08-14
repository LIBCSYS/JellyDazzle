#!/usr/bin/env python3
"""Scout: for each class, scan a wide hue grid and report which hue sites pass
the class gate AND the same-class hue floor against the existing corpus.
Read-only; used to pick the hue families for P71..P90 before writing prose."""
import os, sys, itertools
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import palette_score as PS
from _corpus_safe import load_corpus
from _p31_lib import (order_ramp, metrics, score, signature,
                      distance, hue_emd, lch, ramp, max_chroma)
from _gen_p31_p50 import (b_mono, b_duo, b_analog, b_split, b_neon, b_pastel,
                          b_metal, b_stark, b_earth, G, leg, hot)

ROOT = '/Users/exeter/dev/m5/assembly/dzzle1'
LIB = [(n, c, signature(cols)) for n, c, cols
       in load_corpus(ROOT, skip={'P%d' % i for i in range(71, 91)})]


def b_mono2(q):
    """b_mono with an accent-chroma knob: jade/green fields are gamut-limited,
    so a fixed accent chroma pushes accent_frac over the 0.22 ceiling."""
    H, Ha, lo, hi, cm, ac = q['H'], q['Ha'], q['lo'], q['hi'], q['cm'], q['ac']
    return (ramp(11, lo, hi, 0.30 * cm, 0.34 * cm, H - 5, H + 5,
                 cbow=0.36 * cm, cfrac=0.95)
            + [lch(q['al'], 0.62 * ac, Ha), lch(q['al'] + 0.18, 0.50 * ac, Ha + 3)]
            + leg(6, hi - 0.06, lo + 0.05, H))


def check(cols, cls, extra=()):
    s, ok, det = score(cols, cls)
    if not ok:
        bad = [k for k, (v, sub, t) in det.items() if sub < 1.0]
        return None, 'CLS ' + ','.join(bad)
    sig = signature(cols)
    near, nn = 9.9, None
    for name, ocls, osig in list(LIB) + list(extra):
        d = distance(sig, osig)
        if d < near:
            near, nn = d, name
        if d < PS.MIN_DIST:
            return None, f'near {name} {d:.3f}'
        if ocls == cls and hue_emd(sig, osig) < PS.MIN_HUE_EMD_SAME_CLASS:
            return None, f'hue {name} {hue_emd(sig, osig):.2f}'
    return (near, nn), 'ok'


def scan(tag, build, cls, grid, key, top=12, extra=()):
    res = {}
    for q in grid:
        cols = order_ramp(list(dict.fromkeys(build(q))))
        r, why = check(cols, cls, extra)
        k = tuple(q[x] for x in key)
        if r and (k not in res or r[0] > res[k][0][0]):
            res[k] = (r, q)
    print(f'\n### {tag} [{cls}] {len(res)} passing hue sites')
    for k, (r, q) in sorted(res.items(), key=lambda x: -x[1][0][0])[:top]:
        print('   %-22s nn=%.3f (%s)  %s' % (k, r[0], r[1],
              {a: b for a, b in q.items() if a not in key}))
    return res


if __name__ == '__main__':
    what = sys.argv[1] if len(sys.argv) > 1 else 'all'

    if what in ('all', 'mono'):
        scan('mono_accent', b_mono2, 'mono_accent',
             G(H=list(range(0, 360, 12)), Ha=[0], al=[0.56], lo=[0.07],
               hi=[0.90], cm=[0.95], ac=[0.5, 0.7, 0.9]),
             ['H'], top=30)

    if what in ('all', 'duo'):
        scan('duotone', b_duo, 'duotone',
             G(A=list(range(0, 360, 15)), B=[0], lo=[0.05, 0.10], hi=[0.90],
               dl=[0.0, 0.08], cm=[0.95]),
             ['A'], top=30)

    if what in ('all', 'analog'):
        scan('analogous', b_analog, 'analogous',
             G(H=list(range(0, 360, 10)), sp=[80, 95, 110], lo=[0.06, 0.11],
               hi=[0.90], cm=[0.95, 1.1], rc=[0.20]),
             ['H'], top=36)

    if what in ('all', 'split'):
        scan('split_complement', b_split, 'split_complement',
             G(D=list(range(0, 360, 10)), off=[150, 160, 170], lo=[0.04, 0.09],
               hi=[0.84], cm=[0.95], nf=[4, 7], al=[0.30]),
             ['D'], top=36)

    if what in ('all', 'neon'):
        print('\nmax in-gamut normalised chroma by hue:')
        print('   ' + '  '.join('%d:%.2f' % (h, max(max_chroma(L, h)
              for L in np.arange(0.3, 0.95, 0.05))) for h in range(0, 360, 20)))
        scan('neon', b_neon, 'neon_on_black',
             G(A=list(range(0, 360, 20)), B=[0], la0=[0.24], la1=[0.88],
               lb0=[0.22], lb1=[0.82], nf=[4], fw=[0.065]),
             ['A'], top=40)

    if what in ('all', 'pastel'):
        scan('pastel', b_pastel, 'pastel_wash',
             G(H=list(range(0, 360, 15)), sp=[90, 120], base=[0.85],
               amp=[0.06], cm=[0.9]),
             ['H'], top=30)

    if what in ('all', 'metal'):
        scan('metallic', b_metal, 'metallic',
             G(H=list(range(0, 360, 10)), sp=[22], lo=[0.05], hi=[0.96],
               cm=[0.9, 1.0], gh=[0, 90, 180, 270]),
             ['H'], top=36)

    if what in ('all', 'stark'):
        scan('stark', b_stark, 'stark',
             G(H=list(range(0, 360, 12)), cm=[0.95], tint=[0.0, 0.03],
               th=[0, 120, 240]),
             ['H'], top=30)

    if what in ('all', 'earth'):
        scan('earth', b_earth, 'earth',
             G(H=list(range(0, 360, 10)), sp=[70, 95], lo=[0.24, 0.32],
               hi=[0.66, 0.76], cm=[0.85, 1.0]),
             ['H'], top=36)
