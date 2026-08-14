#!/usr/bin/env python3
"""
_gen_p51_p70.py -- authors palettes P51..P70 for JellyDazzle v2.1.

The second block of twenty. Same governing spec (lab/design/palettes.md), same
scorer (palette_score.py), same authoring space (OKLCh), and the same three
structural rules: author as a LOOP, order_ramp() before writing, judge
smoothness as a RATIO.

Class allocation for this block -- deliberately ZERO rainbows:

    mono_accent 3   duotone 2   analogous 2   split_complement 3
    neon_on_black 2 metallic 3  stark 2       earth 3

Gating corpus = everything already in the library, 100 palettes:
    24 lospec + lab/palettes/P01..P30 + the 6 house ramps in gen_tables.py
    + P31..P50 (from disk, or the cached run of _gen_p31_p50.py)
    + P71..P90 (on disk, written by the third block).

WHERE THE ALLOCATION CAME FROM, AND WHERE IT DEPARTS FROM palettes.md 9.
The quota in the taxonomy is written for 60 new palettes. P31-P50 and P71-P90
landed first and spent theirs, leaving exactly 20 slots: mono 2, duo 2,
analogous 3, split 3, neon 1, pastel 0, metallic 2, stark 2, earth 2,
full_spectrum 3. Two of those are not deliverable and one should not be:

  * `analogous` 3 -> 2. Five analogous palettes already ship, the same-class
    hue floor is 36 degrees, and only two zones of the wheel are still open
    (~120-170 and ~220-260). 1,469 candidates passed the class gate; not one
    compatible TRIPLE exists among them. Verified by exhaustive triangle
    enumeration over the whole passing set, not by sampling.
  * `full_spectrum` 3 -> 0. Feasible (72 candidates passed, and a compatible
    triple exists at min distance 0.177) but declined. J's brief for this block
    is "do not make more rainbows", and the taxonomy's own argument is that
    full_spectrum is legitimate as a class and illegitimate as a habit. Three
    more rainbows would be answering the letter of a quota against the point
    of it.
  * the 4 freed slots went to +1 mono_accent, +1 neon_on_black, +1 metallic and
    +1 earth -- the classes that could still absorb a member at distance, and
    weighted to the restrained end of the set rather than the loud one.

Resulting library class counts across all 60 new palettes, measured on disk:
mono 8, duotone 7, analogous 7, split_complement 6, neon_on_black 7,
pastel_wash 5, metallic 6, stark 6, earth 7, full_spectrum 1.
`library_report`'s quota check therefore reads False; its two distance targets
both pass -- min NN 0.167 against a floor of 0.16, median NN 0.203 against a
target of 0.20 -- and that is the trade this file makes deliberately.

HOW THE NUMBERS WERE FOUND. Every class was swept across the whole wheel
(10-15 degree steps) crossed with a tone grid: ~60,000 candidate palettes, of
which 3,446 passed both their class gate and the library distance gate. Those
were enumerated into all valid k-subsets per class (edges and triangles of the
compatibility graph) and a beam search over the eight classes picked the
combination maximising the minimum pairwise distance while spreading L_mean,
dark_frac and C_lit INSIDE each class -- palettes.md 7's finding that hue
rotation alone does not diversify. Result: min distance 0.167 against a floor
of 0.16, with (for instance) two duotones at dark_frac 0.48 and 0.05.

The grids in SPECS are single points -- the search's answers, frozen. Re-running
this file is reproducible and cannot silently move a palette's hue away from
what its spec.md claims.

ONE NOTE FOR WHOEVER OWNS gen_tables.py. Its `step_ratio()` reads 8-31 for most
of these palettes, against 1.3-2.8 measured on the anchors. That is the metric,
not the palettes. Every one of those maxima lands between two near-black entries
(`000000 -> 010100` and the like): OKLab's cube root has an unbounded derivative
at zero, so a 1/255 change down there measures larger than a visible mid-tone
step, while the long runs of 8-bit-identical entries around it deflate the mean.
Excluding samples where both endpoints sit under RGB 10, the visible max step of
this block is median 0.75 / worst 1.47, against median 0.78 / worst 2.23 for the
70 palettes already shipped -- i.e. slightly smoother than the library, not
eight times rougher. Mean per-channel 8-bit delta per ramp step is 0.12-0.65
against the house budget of 8. If that metric is going to gate builds, gate it
on max/p99, or drop near-black samples first; as written it will reject every
palette with a deep floor, including P41, P46, P50, P81 and P87.

Run:  python3 lab/design/_gen_p51_p70.py [--write]
"""
import os, sys, json, itertools
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import palette_score as PS
from _p31_lib import (lch, ramp, max_chroma, order_ramp, metrics,
                      score, signature, distance, hue_emd)
from _gen_p31_p50 import leg, hot, swatch, MET_ROWS

ROOT = '/Users/exeter/dev/m5/assembly/dzzle1'
OUT = os.path.join(ROOT, 'lab/palettes')
CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     '_p31_p50_cache.json')
MY_IDS = {f'P{n}' for n in range(51, 71)}   # this block owns P51..P70


# ---------------------------------------------------------------- builders
#
# Structure varies as well as hue. palettes.md 7 measured that hue rotation
# alone does not diversify -- symmetric palettes are near rotation-invariant --
# so every builder exposes tone knobs (`lo`/`hi` ground and ceiling, `cm`
# chroma scale) and most expose a structural one as well (where the accent
# sits, how deep the floor is, how the two poles share the value range).

def b_mono(q):
    """One hue family carries the frame; a small opposing accent sits at a
    chosen lightness -- high (`al` near the ceiling) reads as a glint, low
    reads as embers inside the field. Closed by a near-neutral descent (RULE 1).

    `ac` caps the accent's chroma. It matters more than it looks: an accent at
    full chroma carries so much of the chroma weight that accent_frac leaves
    the mono_accent band entirely, and being far from the field in OKLab it
    also spikes step_ratio. Both failures were measured before it was added."""
    H, Ha, lo, hi, cm = q['H'], q['Ha'], q['lo'], q['hi'], q['cm']
    ac, al = q['ac'], q['al']
    field = ramp(13, lo, hi, 0.32 * cm, 0.32 * cm, H - 6, H + 6,
                 cbow=0.38 * cm, cfrac=0.95)
    acc = [lch(al + 0.09 * i, ac * (1 - 0.12 * i), Ha + 4 * i)
           for i in range(q['na'])]
    return field + acc + leg(6, hi - 0.06, lo + 0.05, H)


def b_duo_deep(q):
    """Two hue poles over a shared near-black floor -- the inks meet in the
    dark rather than in the light, so the cyclic wrap happens where no cliff
    is visible."""
    A, B, lo, hi, cm = q['A'], q['B'], q['lo'], q['hi'], q['cm']
    floor = [lch(lo + 0.02 * i, 0.05 + 0.02 * i, A if i % 2 else B)
             for i in range(q['nf'])]
    return (floor
            + ramp(9, lo + 0.05, hi, 0.34 * cm, 0.28 * cm, A - 4, A + 4,
                   cbow=0.38 * cm, cfrac=0.95)
            + ramp(9, lo + 0.05, hi - q['dl'], 0.34 * cm, 0.28 * cm,
                   B + 4, B - 4, cbow=0.38 * cm, cfrac=0.95))


def b_duo_split(q):
    """Two hue poles sharing the value range UNEVENLY: one owns the lower half,
    the other the upper, overlapping only in the middle. With a raised `lo` it
    becomes a light-ground duotone -- two inks on white stock, no black in the
    palette at all, which is a tone nothing else in the library occupies."""
    A, B, lo, hi, cm = q['A'], q['B'], q['lo'], q['hi'], q['cm']
    sh = q['sh']
    return (ramp(10, lo, hi - sh, 0.34 * cm, 0.30 * cm, A - 5, A + 5,
                 cbow=0.36 * cm, cfrac=0.95)
            + ramp(9, lo + sh, hi, 0.32 * cm, 0.26 * cm, B + 5, B - 5,
                   cbow=0.34 * cm, cfrac=0.95))


def b_analog_dusk(q):
    """The same wheel slice run as a dusk: the chromatic peak sits in the lower
    third of the value range and the top desaturates toward a pale wash. This
    is how two analogous palettes separate on TONE and not only on hue."""
    H, sp, lo, hi, cm = q['H'], q['sp'], q['lo'], q['hi'], q['cm']
    mid = lo + (hi - lo) * 0.38
    return (ramp(9, lo, mid, 0.30 * cm, 0.66 * cm, H, H + sp * 0.35,
                 cbow=0.14 * cm, cfrac=0.96)
            + ramp(9, mid + 0.03, hi, 0.62 * cm, 0.18 * cm,
                   H + sp * 0.40, H + sp, cfrac=0.96)
            + ramp(7, hi - 0.06, lo + 0.05, q['rc'], q['rc'] * 0.6,
                   H + sp * 0.75, H + sp * 0.05))


def b_split(q):
    """Dominant hue plus the two neighbours of its opposite, deliberately
    LOPSIDED: the dominant family carries 12 anchors and each accent only 3, at
    capped chroma `ac`.

    That imbalance is not styling. Twelve palettes already in the library
    best-fit split_complement and their hue histograms are broad, so a new
    member with an equally broad histogram fails the same-class hue-EMD floor
    against every one of them -- measured: 1 of 768 grid points survived with
    balanced accents, 305 of 1152 with lopsided ones."""
    D, off, lo, hi, cm, ac = q['D'], q['off'], q['lo'], q['hi'], q['cm'], q['ac']
    a1, a2 = (D + off) % 360, (D - off) % 360
    return (ramp(12, lo, hi, 0.36 * cm, 0.30 * cm, D - 8, D + 8,
                 cbow=0.40 * cm, cfrac=0.96)
            + ramp(3, q['al'], hi - 0.10, ac, ac * 0.6, a1 - 3, a1 + 3,
                   cfrac=0.95)
            + ramp(3, q['al'] - 0.05, hi - 0.20, ac * 0.95, ac * 0.65,
                   a2 + 3, a2 - 3, cfrac=0.95)
            + leg(5, hi - 0.03, lo + 0.04, D)
            + [lch(0.03 + 0.05 * i, 0.05, D) for i in range(q['nf'])])


def b_neon(q):
    """Near-black floor under two gamut-edge hues; each lit ramp reaches down
    into the floor's lightness so the cyclic tour has a path between them --
    without that, the void-to-neon jump IS the step_ratio."""
    A, B, nf, fw = q['A'], q['B'], q['nf'], q['fw']
    floor = ([lch(0.02 + fw * i, 0.03 + 0.16 * fw * i, A) for i in range(nf)]
             + [lch(0.035 + fw * i, 0.04 + 0.16 * fw * i, B) for i in range(nf)])
    lit = []
    for H, l0, l1 in ((A, q['la0'], q['la1']), (B, q['lb0'], q['lb1'])):
        for i in range(7):
            L = l0 + (l1 - l0) * (i / 6.0) ** 0.85
            lit.append(lch(L, max_chroma(L, H), H))
    return floor + lit + [hot(A), hot(B)]


def b_neon_tri(q):
    """Void floor under THREE lit hues instead of two -- the class allows up to
    five hue bins, and the third filament is what separates this from a
    two-rail neon on the same side of the wheel. The floor uses a fine `fw`
    step: a coarse one walks floor anchors up past L=0.30 and dark_frac falls
    out of the class band (measured: nf=12, fw=0.05 -> dark_frac 0.28)."""
    A, B, C, nf, fw = q['A'], q['B'], q['C'], q['nf'], q['fw']
    floor = [lch(0.02 + fw * i, 0.03 + 0.15 * fw * i, (A, B, C)[i % 3])
             for i in range(nf)]
    lit = []
    for H, l0, l1, n in ((A, q['la0'], q['la1'], 6),
                         (B, q['lb0'], q['lb1'], 6),
                         (C, q['lc0'], q['lc1'], 4)):
        for i in range(n):
            L = l0 + (l1 - l0) * (i / (n - 1.0)) ** 0.85
            lit.append(lch(L, max_chroma(L, H), H))
    return floor + lit + [hot(A), hot(B)]


def b_metal(q):
    """Narrow hue band on a long steep value ramp, plus a near-neutral return
    leg so the cyclic wrap has no cliff."""
    H, sp, lo, hi, cm = q['H'], q['sp'], q['lo'], q['hi'], q['cm']
    return (ramp(13, lo, hi, 0.26 * cm, 0.14 * cm, H, H + sp,
                 cbow=0.30 * cm, cfrac=0.92)
            + leg(7, hi - 0.07, lo + 0.07, q['gh'], C=0.05))


def b_metal_polish(q):
    """A metal with a POLISHED highlight: the value ramp holds a plateau near
    the top -- the specular hit -- before the neutral return, which is what a
    burnished surface actually does to a gradient."""
    H, sp, lo, hi, cm = q['H'], q['sp'], q['lo'], q['hi'], q['cm']
    body = ramp(10, lo, hi - 0.10, 0.24 * cm, 0.20 * cm, H, H + sp * 0.7,
                cbow=0.32 * cm, cfrac=0.92)
    hit = [lch(hi - 0.10 + 0.035 * i, (0.16 - 0.03 * i) * cm, H + sp * 0.8)
           for i in range(4)]
    return body + hit + leg(7, hi - 0.06, lo + 0.06, q['gh'], C=0.05)


def b_stark(q):
    """Two value plateaus, no midtones, one chromatic slash. The cliff is the
    look -- stark is the only class the smoothness budget is relaxed for."""
    H, cm = q['H'], q['cm']
    dk = [lch(0.02 + 0.028 * i, q['tint'], q['th']) for i in range(6)]
    lt = [lch(0.92 + 0.014 * i, q['tint'] * 0.5, q['th']) for i in range(6)]
    slash = [lch(0.44, 0.95 * cm, H), lch(0.56, 0.92 * cm, H + 2),
             lch(0.66, 0.80 * cm, H - 2)]
    return dk + lt + slash + [lch(0.20, q['tint'], q['th']),
                              lch(0.86, q['tint'] * 0.6, q['th'])]


def b_earth_wash(q):
    """An earth run as a wash: the same muted chroma band, but lightness
    oscillates around the mid instead of climbing and returning -- weathered
    surface rather than layered strata."""
    H, sp, lo, hi, cm = q['H'], q['sp'], q['lo'], q['hi'], q['cm']
    n = 19
    mid = 0.5 * (lo + hi)
    amp = 0.5 * (hi - lo)
    cols = []
    for i in range(n):
        t = i / n
        Hh = H + sp * 0.5 * (1.0 - np.cos(2 * np.pi * t))
        L = mid + amp * np.sin(2 * np.pi * t + 0.4)
        C = (0.24 + 0.08 * np.cos(2 * np.pi * t + 1.0)) * cm
        cols.append(lch(L, min(C, 0.85 * max_chroma(L, Hh)), Hh))
    return cols


def b_earth_bi(q):
    """TWO muted hue lobes at mid lightness instead of one continuous band --
    clay against lichen, rather than clay shading into it.

    The shape exists because of the gate, not for its own sake: four earths
    already ship and they occupy the single-lobe hue profiles, so every
    one-lobe candidate collided with one of them on the same-class hue floor
    (measured: 0 of 1,248 passed). A two-lobe histogram clears it easily --
    587 of 3,888 passed, the best at distance 0.30."""
    A, B, lo, hi, cm, sp = q['A'], q['B'], q['lo'], q['hi'], q['cm'], q['sp']
    return (ramp(7, lo, hi, 0.26 * cm, 0.22 * cm, A, A + sp, cfrac=0.9)
            + ramp(6, hi - 0.06, lo + 0.08, 0.24 * cm, 0.28 * cm,
                   B + sp, B, cfrac=0.9)
            + ramp(5, lo + 0.10, hi - 0.10, 0.18 * cm, 0.24 * cm,
                   (A + B) / 2.0, (A + B) / 2.0 + sp * 0.5, cfrac=0.9))


# ---------------------------------------------------------------- the twenty
#
# Every parameter set below is the OUTPUT of the search described at the top of
# this file, not a hand guess: each class was swept across the whole wheel
# crossed with a tone grid, the survivors of the class gate and the library
# distance gate were enumerated into all valid k-subsets, and a beam search
# over the eight classes picked the combination that maximises the minimum
# pairwise distance while spreading L_mean / dark_frac / C_lit INSIDE each
# class. The grids are single points so the block is reproducible: re-running
# this file cannot silently swap a palette's hue out from under its spec.md.

def G(**kw):
    keys = list(kw)
    return [dict(zip(keys, v)) for v in itertools.product(*(kw[k] for k in keys))]


SPECS = [
 # ------------------------------------------------------------ mono_accent x3
 dict(id='P51', slug='wormwood', name='Wormwood', cls='mono_accent',
      build=b_mono,
      grid=G(H=[105], Ha=[315], ac=[0.35], al=[0.60], na=[2],
             lo=[0.03], hi=[0.82], cm=[0.9]),
      families='wormwood olive 99-111 field; magenta glint at 315',
      mood='A distillery cellar in green half-light — everything olive, and '
           'one magenta reflection off glass somewhere near the ceiling.',
      look='Thirteen stops of a single olive family from black to pale wormwood, '
           'closed by a near-neutral descent, with two magenta anchors placed '
           'HIGH in the value ramp so they read as a glint and not as a second '
           'colour. Chroma on the accent is capped at 0.35 — at full chroma the '
           'accent takes so much of the chroma weight that the palette stops '
           'being mono_accent at all.',
      pairing='Flow, plasma and interference fields. One dominant current does '
              'the work; the magenta marks where it peaks.'),

 dict(id='P52', slug='orchid_vault', name='Orchid Vault', cls='mono_accent',
      build=b_mono,
      grid=G(H=[320], Ha=[140], ac=[0.5], al=[0.30], na=[3],
             lo=[0.05], hi=[0.90], cm=[1.05]),
      families='orchid magenta 314-326 field; green embers at 140',
      mood='A stone vault lit through orchid glass, with three green service '
           'lamps burning low behind the far pillars.',
      look='The same one-family construction as Wormwood with the accent placed '
           'LOW — three green embers down inside the field instead of a glint on '
           'top of it, and a hotter field (C_lit 0.52 against 0.33). Restraint '
           'from the other end of the ramp.',
      pairing='Drape, veil and swirl patterns. The long magenta value ramp gives '
              'interior volume; the green picks out folds.'),

 dict(id='P53', slug='frost_signal', name='Frost Signal', cls='mono_accent',
      build=b_mono,
      grid=G(H=[210], Ha=[0], ac=[0.35], al=[0.60], na=[2],
             lo=[0.14], hi=[0.97], cm=[0.9]),
      families='ice blue 204-216 field; one red signal at 0',
      mood='A cold room in full daylight: pale blue walls, frost on the glass, '
           'and a single red indicator burning on a panel.',
      look='The high-key mono of the three — ground lifted to L 0.14 and ceiling '
           'pushed to 0.97, so dark_frac falls to 0.19 where the other two sit '
           'near 0.33. Same restraint, different weather.',
      pairing='Caustics, frost and ripple patterns; anything that wants a light '
              'field with one point of attention in it.'),

 # ---------------------------------------------------------------- duotone x2
 dict(id='P54', slug='sodium_and_slate', name='Sodium & Slate', cls='duotone',
      build=b_duo_deep,
      grid=G(A=[75], B=[225], lo=[0.03], hi=[0.88], dl=[0.06], nf=[5],
             cm=[0.9]),
      families='sodium gold 71-79 vs slate blue 221-229',
      mood='A sodium lamp over a wet slate roof. Two temperatures, no third '
           'opinion, and a lot of night around them.',
      look='Both poles rise out of a shared near-black floor rather than meeting '
           'at a white ceiling, so the cyclic wrap happens in the dark where no '
           'cliff is visible. dark_frac 0.48 — this is the deep duotone.',
      pairing='Two-body patterns: reaction-diffusion, duelling attractors, split '
              'fields. Anything that wants a clear "us and them".'),

 dict(id='P55', slug='riso_indigo', name='Riso Indigo', cls='duotone',
      build=b_duo_split,
      grid=G(A=[110], B=[275], lo=[0.24], hi=[0.97], sh=[0.16], cm=[0.85]),
      families='lime-olive 105-115 vs indigo 270-280, on a LIGHT ground',
      mood='A two-colour risograph run: lime and indigo laid over bright stock, '
           'with the paper showing through everywhere they miss.',
      look='The two inks share the value range unevenly — one owns the lower '
           'half, one the upper — and the whole palette is lifted off the floor: '
           'dark_frac 0.05, nothing in it is black. Against Sodium & Slate '
           '(dark_frac 0.48) the two duotones share a class and no tone at all.',
      pairing='Lattice, scan and moire work; also the right choice when a dark '
              'palette has been up a while and the scheduler needs contrast.'),

 # -------------------------------------------------------------- analogous x2
 #
 # Two, not three. Only two hue zones on the wheel are still open to this class
 # -- roughly 120-170 and 220-260 -- because five analogous palettes already
 # ship and the same-class hue floor is 36 degrees. Every candidate for a third
 # was enumerated (1,469 passing the class gate) and no compatible triple
 # exists. The freed slot went to a third metallic; see the header.
 dict(id='P56', slug='iris_morning', name='Iris Morning', cls='analogous',
      build=b_analog_dusk,
      grid=G(H=[250], sp=[110], lo=[0.28], hi=[0.98], cm=[1.2], rc=[0.10]),
      families='blue-violet 250 -> violet -> rose 360, a slow third of a turn',
      mood='Iris and hyacinth in early light — violet through to rose-white, '
           'with only the deepest anchor still holding any night.',
      look='A high-key analogous: L_mean 0.60, dark_frac 0.04, climbing to a '
           'rose-white ceiling and returning down a greyed leg. The chromatic '
           'peak sits low in the value range, so the palette reads as light '
           'without going pastel.',
      pairing='Swirl, vortex and drape patterns. Continuous hue means continuous '
              'motion — nothing here can produce a hard edge.'),

 dict(id='P57', slug='harbour_dusk', name='Harbour Dusk', cls='analogous',
      build=b_analog_dusk,
      grid=G(H=[140], sp=[110], lo=[0.05], hi=[0.86], cm=[0.6], rc=[0.30]),
      families='sea green 140 -> teal -> steel blue 250',
      mood='The hour after sunset over water: green low on the horizon going '
           'teal, then steel blue overhead.',
      look='The same builder as Iris Morning run at the opposite tone — ground '
           'at L 0.05, chroma scaled to 0.6, dark_frac 0.36. Same class, same '
           'construction, and the two share neither hue zone nor weather.',
      pairing='Ripple, caustic and slow-wave patterns; anything where brightness '
              'rather than hue should carry the story.'),

 # ------------------------------------------------------- split_complement x3
 dict(id='P58', slug='verdigris_alarm', name='Verdigris Alarm',
      cls='split_complement', build=b_split,
      grid=G(D=[170], off=[165], ac=[0.35], lo=[0.04], hi=[0.82], cm=[0.9],
             nf=[2], al=[0.38]),
      families='verdigris teal 162-178 dominant; rose + scarlet accents',
      mood='Oxidised copper across the whole frame with a red warning light '
           'somewhere behind it.',
      look='A twelve-anchor teal ramp with rose and scarlet either side of its '
           'opposite, each only three anchors at capped chroma. The imbalance is '
           'deliberate: twelve palettes already best-fit this class and their '
           'hue profiles are broad, so a balanced split-complement cannot clear '
           'the same-class floor — measured, 1 of 768 balanced grid points '
           'survived against 305 of 1,152 lopsided ones.',
      pairing='Tunnel, shear and ribbon patterns — the dominant field carries the '
              'motion, the warm accents mark leading edges.'),

 dict(id='P59', slug='flare_path', name='Flare Path',
      cls='split_complement', build=b_split,
      grid=G(D=[50], off=[165], ac=[0.35], lo=[0.04], hi=[0.82], cm=[1.1],
             nf=[6], al=[0.38]),
      families='flare amber 42-58 dominant; cold blue + indigo accents',
      mood='Runway flares seen from the far end of a wet field, with the last '
           'of the cold blue still in the sky behind them.',
      look='Amber dominant against the two neighbours of its opposite, on the '
           'deepest floor of the three (six extra near-black anchors, dark_frac '
           '0.39) and the highest chroma scale.',
      pairing='Plume, heat and drift patterns; the cold accents give the edges '
              'somewhere to go that is not merely "less orange".'),

 dict(id='P60', slug='serpentine', name='Serpentine',
      cls='split_complement', build=b_split,
      grid=G(D=[130], off=[165], ac=[0.35], lo=[0.09], hi=[0.82], cm=[0.9],
             nf=[6], al=[0.38]),
      families='serpentine green 122-138 dominant; violet + magenta accents',
      mood='Serpentine stone: a deep green field with violet and magenta veins '
           'running through it where the mineral changed its mind.',
      look='Green dominant against the two neighbours of its opposite. Sits '
           'between the other two on the wheel and above Flare Path on the '
           'floor, so all three split-complements differ in hue AND ground.',
      pairing='Branch, lattice and growth patterns; the cool accents give nodes '
              'and tips somewhere to go that is not merely "brighter green".'),

 # ---------------------------------------------------------- neon_on_black x2
 dict(id='P61', slug='neon_alley', name='Neon Alley', cls='neon_on_black',
      build=b_neon,
      grid=G(A=[340], B=[200], la0=[0.22], la1=[0.90], lb0=[0.14], lb1=[0.80],
             nf=[5], fw=[0.065]),
      families='void floor; hot rose 340 + ion cyan 200 at the gamut edge',
      mood='Wet asphalt at night: a hot pink sign above, its cyan reflection '
           'below, and absolutely nothing lit between them.',
      look='Ten near-black anchors form the floor; everything above rides the '
           'gamut edge, with the rose pole at the highest chroma sRGB has '
           '(C_max 0.98). The dark half is what makes the lit half read electric '
           'rather than merely bright.',
      pairing='Line, spark and trail patterns on a dark ground — anything where '
              'the subject is thin and the background should vanish.'),

 dict(id='P62', slug='violet_reactor', name='Violet Reactor',
      cls='neon_on_black', build=b_neon_tri,
      grid=G(A=[260], B=[320], C=[300], la0=[0.14], la1=[0.88], lb0=[0.12],
             lb1=[0.76], lc0=[0.30], lc1=[0.66], nf=[9], fw=[0.022]),
      families='void floor; electric blue 260 + magenta 320, violet 300 filament',
      mood='Something running hot behind leaded glass: a blue core, a magenta '
           'shell, and one violet filament threading between them.',
      look='THREE lit hues over the void instead of two — the class allows up to '
           'five hue bins, and the third filament is what separates this from a '
           'two-rail neon. Nine floor anchors at a fine lightness step keep '
           'dark_frac inside the band; a coarse step walks the floor up past '
           'L 0.30 and the class fails (measured: dark_frac 0.28).',
      pairing='Filament, glow-decay and particle patterns. Very strong under '
              'additive layering, where the black floor absorbs the accumulation '
              'instead of blowing out.'),

 # --------------------------------------------------------------- metallic x3
 dict(id='P63', slug='antique_bronze', name='Antique Bronze', cls='metallic',
      build=b_metal_polish,
      grid=G(H=[110], sp=[44], lo=[0.10], hi=[0.98], cm=[0.5], gh=[40]),
      families='green-gold bronze 110-154 on a full value ramp; warm return',
      mood='Bronze that has been handled for a century — green-gold in the body, '
           'a hard polished hit on the high edge, warm grey in the shadow.',
      look='A narrow hue band riding a long steep value ramp, holding a PLATEAU '
           'near the top (the specular hit) before the neutral return leg closes '
           'the loop. The shine is lightness, not chroma: C_lit 0.21.',
      pairing='Bevel, emboss and relief patterns; anything with implied '
              'thickness, where a flat chroma ramp reads as paper.'),

 dict(id='P64', slug='pewter_rose', name='Pewter Rose', cls='metallic',
      build=b_metal_polish,
      grid=G(H=[348], sp=[34], lo=[0.06], hi=[0.98], cm=[0.5], gh=[40]),
      families='warm pewter 348-22 on a full value ramp; neutral return leg',
      mood='Old pewter with the faintest blush in it — black in the shadow, '
           'white on the rim, and a warmth you only see next to a true grey.',
      look='The least chromatic palette in the block: C_lit 0.21, an 18-degree '
           'hue arc, and all the drama carried by an L_range of 0.99. A first '
           'draft of this slot was a brass at hue 76; it cleared every gate when '
           'it was searched and then collided with `P73_brass_vigil` (d=0.155) '
           'when that palette was revised mid-flight, so it was re-searched '
           'against the live library and replaced.',
      pairing='Specular and sheen patterns; survives compositing under other '
              'layers because it carries almost no hue to fight with.'),

 dict(id='P65', slug='cobalt_steel', name='Cobalt Steel', cls='metallic',
      build=b_metal,
      grid=G(H=[252], sp=[48], lo=[0.02], hi=[0.98], cm=[1.2], gh=[40]),
      families='cobalt blue 252-300 on a full value ramp; warm return leg',
      mood='Blued steel straight off the heat — cold cobalt through the body and '
           'a white edge where the light catches it.',
      look='The hot metal of the three: chroma scaled to 1.2 where Antique '
           'Bronze runs at 0.5, so C_lit is 0.45 against 0.21. Same class, same '
           'steep value ramp, twice the colour.',
      pairing='Rim-light, shear and bevel patterns; also good under any pattern '
              'that wants to look manufactured rather than grown.'),

 # ------------------------------------------------------------------ stark x2
 dict(id='P66', slug='gold_rule', name='Gold Rule', cls='stark',
      build=b_stark,
      grid=G(H=[90], cm=[0.9], tint=[0.02], th=[130]),
      families='black / white / one gold rule',
      mood='Black ink, white stock, and a single gold rule ruled across it.',
      look='Two value plateaus with almost nothing between them plus three gold '
           'anchors. The cliff IS the look — stark is the one class the '
           'smoothness budget is relaxed for, and this palette uses nearly all '
           'of it (step_ratio 5.2 against a budget of 5.0 + 1.5 tolerance).',
      pairing='Hard-edged geometry: grids, bars, halftone, scanline. Soft-edged '
              'patterns should stay away from this one.'),

 dict(id='P67', slug='magenta_press', name='Magenta Press', cls='stark',
      build=b_stark,
      grid=G(H=[330], cm=[0.9], tint=[0.04], th=[130]),
      families='near-black / paper white / magenta slash',
      mood='A press run with the magenta plate landing where nothing asked it '
           'to. Black, paper, and one violent stripe.',
      look='The same two-plateau structure as Gold Rule with the accent on the '
           'far side of the wheel and at twice the chroma (C_lit 0.70 against '
           '0.34) — the loudest palette in the block, and by construction the '
           'least smooth.',
      pairing='Bar, tile and halftone patterns; reads as press artefact rather '
              'than poster art.'),

 # ------------------------------------------------------------------ earth x3
 dict(id='P68', slug='sea_fog', name='Sea Fog', cls='earth',
      build=b_earth_wash,
      grid=G(H=[180], sp=[55], lo=[0.40], hi=[0.70], cm=[0.7]),
      families='sea teal 180-235 at very low chroma, mid value only',
      mood='Fog that has not lifted off cold water: teal, grey-green, and the '
           'wet light between them.',
      look='Run as a WASH — lightness oscillates around the mid instead of '
           'climbing and returning, which reads as weathered surface rather than '
           'as layered strata. Nothing in it is dark and nothing is pale '
           '(dark_frac 0, light_frac 0): the class is deliberately muted rather '
           'than either.',
      pairing='Terrain, sediment and haze patterns. Stays legible under heavy '
              'layering where a saturated palette goes muddy.'),

 dict(id='P69', slug='lichen_and_slate', name='Lichen & Slate', cls='earth',
      build=b_earth_bi,
      grid=G(A=[135], B=[265], sp=[25], lo=[0.18], hi=[0.58], cm=[0.6]),
      families='lichen green 135 and slate violet 265, two muted lobes',
      mood='A north wall in wet woodland: lichen on the stone, slate underneath, '
           'and not a warm colour anywhere.',
      look='TWO muted hue lobes instead of one continuous band. That shape is '
           'what got a third earth into the library at all — the four earths '
           'already shipped occupy the single-lobe hue profiles, and every '
           'single-lobe candidate collided with one of them on the same-class '
           'floor. The darkest earth of the three (L_mean 0.38).',
      pairing='Crack, grain and erosion patterns; cross-fades well with the warm '
              'earths from the other blocks.'),

 dict(id='P70', slug='heather_moor', name='Heather Moor', cls='earth',
      build=b_earth_wash,
      grid=G(H=[270], sp=[55], lo=[0.40], hi=[0.58], cm=[1.1]),
      families='heather violet 270-325 at low chroma, tight mid value band',
      mood='Late-summer moorland: heather going over, the whole hillside '
           'slightly purple and slightly grey.',
      look='The same wash as Sea Fog on the opposite side of the wheel and at a '
           'higher chroma scale (C_lit 0.26 against 0.16), inside a tighter '
           'value band. Muted, not dark — no anchor in it is either black or '
           'pale.',
      pairing='Texture, sediment and drift patterns; the quiet palette to put '
              'between two loud ones.'),
]


# ---------------------------------------------------------------- corpus

def _house_schemes():
    """The house HSV ramps out of gen_tables.py, sampled at 24 stops each.

    Deliberately parsed rather than imported, and deliberately NOT via
    _p31_lib.load_corpus: gen_tables.py is being rewritten for v2.1 in
    parallel with this file and its keyframe dict has already been renamed
    once (`SCHEMES`/`ORDER` -> `HOUSE`/`HOUSE_ORDER`). Both spellings are
    accepted here, and if neither is found the house ramps are simply skipped
    with a warning -- a table rename in another agent's file must not be able
    to stop palettes being authored."""
    import colorsys, re
    src = open(os.path.join(ROOT, 'gen_tables.py')).read()
    body = None
    for open_tok, close_tok in (('HOUSE = {', 'HOUSE_ORDER'),
                                ('SCHEMES = {', 'ORDER = [')):
        if open_tok in src and close_tok in src:
            body = src[src.index(open_tok):src.index(close_tok)]
            body = re.sub(r'^\w+ = ', 'HOUSE = ', body, count=1)
            break
    if body is None:
        print('WARN: no house scheme table found in gen_tables.py -- '
              'gating without it', file=sys.stderr)
        return []
    ns = {}
    exec(body, {'__builtins__': {}}, ns)
    out = []
    for name, keys in ns['HOUSE'].items():
        cols = []
        for i in range(24):
            p = i / 24.0
            for k in range(len(keys) - 1):
                p0, h0, s0, v0, _ = keys[k]
                p1, h1, s1, v1, _ = keys[k + 1]
                if p0 <= p <= p1:
                    t = (p - p0) / max(p1 - p0, 1e-9)
                    t = t * t * (3 - 2 * t)
                    dh = (h1 - h0 + 0.5) % 1.0 - 0.5
                    h = (h0 + dh * t) % 1.0
                    r, g, b = colorsys.hsv_to_rgb(h, s0 + (s1 - s0) * t,
                                                  v0 + (v1 - v0) * t)
                    cols.append('%02x%02x%02x'
                                % (int(r * 255), int(g * 255), int(b * 255)))
                    break
        if len(cols) >= 4:
            out.append(('house:' + name, cols))
    return out


def _load_lib():
    """(name, colors) for the lospec imports and every lab/palettes/P* on disk."""
    import glob
    out = []
    for p in json.load(open(os.path.join(ROOT, 'reference/palettes.json'))):
        out.append(('lospec:' + p['slug'], p['colors']))
    for d in sorted(glob.glob(os.path.join(ROOT, 'lab/palettes/P*/palette.json'))):
        tag = os.path.basename(os.path.dirname(d))
        if tag.split('_')[0] in MY_IDS:   # never gate this block against itself
            continue
        j = json.load(open(d))
        cols = [c['hex'] if isinstance(c, dict) else c for c in j['colors']]
        out.append((tag, cols))
    return out + _house_schemes()


def corpus():
    """Everything already in the library. P31-P50 come from disk when the other
    block has landed, otherwise from the cached run of its generator.

    An existing palette is given a class ONLY if it genuinely passes one -- the
    same-class hue floor should not be enforced against a bad fit."""
    lib = []
    for name, cols in _load_lib():
        cls, _ = PS.classify(cols)
        lib.append((name, cls if score(cols, cls)[1] else None, cols))
    have = {n.split('_')[0] for n, c, cols in lib}
    if os.path.exists(CACHE):
        for pid, rec in json.load(open(CACHE)).items():
            if pid in have:
                continue
            cols = rec['colors']
            cls, _ = PS.classify(cols)
            ok = score(cols, cls)[1]
            lib.append((pid + '_' + rec['slug'], cls if ok else None, cols))
    return lib


# ---------------------------------------------------------------- search

def evaluate(cols, cls, accepted):
    s, ok, det = score(cols, cls)
    if not ok:
        bad = [f'{k}={v}({t[0]}..{t[1]})' for k, (v, sub, t) in det.items()
               if sub < 1.0]
        return False, s, 0.0, 'class fit %.2f :: %s' % (s, '; '.join(bad)), None
    sig = signature(cols)
    near, nn = 9.9, None
    for name, ocls, osig in accepted:
        d = distance(sig, osig)
        if d < near:
            near, nn = d, name
        if d < PS.MIN_DIST:
            return False, s, d, f'too close to {name} d={d:.3f}', None
        if ocls == cls and hue_emd(sig, osig) < PS.MIN_HUE_EMD_SAME_CLASS:
            return False, s, d, (f'hue overlap with {name} (same class) '
                                 f'emd={hue_emd(sig, osig):.2f}'), None
    return True, s, near, f'fit={s:.2f} nn={nn} d={near:.3f}', sig


def build():
    accepted = [(n, c, signature(cols)) for n, c, cols in corpus()]
    rows = []
    for sp in SPECS:
        best, why = None, 'no grid point passed'
        for q in sp['grid']:
            cols = order_ramp(list(dict.fromkeys(sp['build'](q))))
            ok, s, near, msg, sig = evaluate(cols, sp['cls'], accepted)
            if not ok:
                if best is None:
                    why = msg
                continue
            # rank: house rule first (no rough breaks -- step_ratio inside the
            # class budget), then class fit, then distance from the library
            smooth = PS.score(cols, sp['cls'])[2]['step_ratio'][1]
            rank = (round(smooth, 2), round(s, 2), near)
            if best is None or rank > best[0]:
                best = (rank, cols, near, s, msg, sig, q)
        if best is None:
            rows.append((sp, None, False, why, None))
            continue
        _, cols, near, s, msg, sig, q = best
        accepted.append((sp['id'] + '_' + sp['slug'], sp['cls'], sig))
        rows.append((sp, cols, True, msg, q))
    return accepted, rows


# ---------------------------------------------------------------- output

def write_all(rows):
    for sp, cols, ok, msg, q in rows:
        if not ok:
            continue
        d = os.path.join(OUT, f"{sp['id']}_{sp['slug']}")
        os.makedirs(d, exist_ok=True)
        m = metrics(cols)
        json.dump({'id': sp['id'], 'slug': sp['slug'], 'name': sp['name'],
                   'class': sp['cls'], 'scheme': sp['families'],
                   'mood': sp['mood'], 'count': len(cols), 'colors': cols},
                  open(os.path.join(d, 'palette.json'), 'w'), indent=1)
        swatch(cols, os.path.join(d, 'swatch.png'))
        _, _, det = score(cols, sp['cls'])
        with open(os.path.join(d, 'spec.md'), 'w') as f:
            f.write(f"# {sp['id']} {sp['name']}\n\n"
                    f"**Class** `{sp['cls']}` — {PS.CLASSES[sp['cls']]['blurb']}\n\n"
                    f"**Scheme** {sp['families']}\n\n"
                    f"## Mood\n{sp['mood']}\n\n"
                    f"## Look\n{sp['look']}\n\n"
                    f"## Pattern pairing\n{sp['pairing']}\n\n"
                    f"## Swatch\n\n![swatch](swatch.png)\n\n"
                    f"Top band: the {len(cols)} anchors. Bottom band: the "
                    f"cyclic ramp `gen_tables.py` expands from them.\n\n"
                    f"## Class metrics\n\n| metric | value | class target |\n|---|---|---|\n")
            for k in MET_ROWS:
                v = m[k]
                tgt = f'{det[k][2][0]} .. {det[k][2][1]}' if k in det else '·'
                f.write(f'| `{k}` | {v if isinstance(v, int) else round(v, 3)} '
                        f'| {tgt} |\n')
            f.write(f"\nGate: **PASS** — {msg}\n\n## Colors ({len(cols)})\n\n")
            f.write(' '.join(f'`{c}`' for c in cols) + '\n')


if __name__ == '__main__':
    accepted, rows = build()
    print(f'{"id":5s} {"slug":22s} {"class":17s} {"n":>3s} {"bins":>4s} '
          f'{"arc":>4s} {"Clit":>5s} {"Cmax":>5s} {"Lmn":>5s} {"Lsd":>5s} '
          f'{"drk":>5s} {"sr":>5s}  result')
    print('-' * 124)
    bad = 0
    for sp, cols, ok, msg, q in rows:
        if not ok:
            bad += 1
            print(f"{sp['id']:5s} {sp['slug']:22s} {sp['cls']:17s} "
                  f"{'':44s}FAIL {msg}")
            continue
        m = metrics(cols)
        print(f"{sp['id']:5s} {sp['slug']:22s} {sp['cls']:17s} {m['n']:3d} "
              f"{m['hue_bins']:4d} {m['hue_arc']:4.0f} {m['C_lit']:5.2f} "
              f"{m['C_max']:5.2f} {m['L_mean']:5.2f} {m['L_sd']:5.2f} "
              f"{m['dark_frac']:5.2f} {m['step_ratio']:5.2f}  OK   {msg}")
        if '-v' in sys.argv:
            print(f'      params {q}')
    print()
    if bad:
        print(f'{bad} FAILING')
        sys.exit(1)
    mine = accepted[-20:]
    nn = [min(distance(s, o) for n2, c2, o in accepted if n2 != n)
          for n, c, s in mine]
    print(f'P51-P70 against the full {len(accepted)}-palette library: '
          f'min NN {min(nn):.3f}   median NN {np.median(nn):.3f}   '
          f'(floors: 0.16 / 0.20)')
    if '--write' in sys.argv:
        write_all(rows)
        print(f'wrote {len(rows) - bad} palettes to {OUT}')
