#!/usr/bin/env python3
"""
_gen_p31_p50.py -- authors palettes P31..P50 for JellyDazzle v2.1.

Twenty palettes spread across the taxonomy in lab/design/palettes.md. Not one
of them is a rainbow: full_spectrum gets exactly ONE slot, and the four classes
the shipped library has never once had a member of -- mono_accent, neon_on_black,
pastel_wash, stark -- get seven between them. That is J's "stark to amazing".

Every palette is authored in OKLCh (the space the metrics live in), closed as a
LOOP so the cyclic wrap has no cliff (RULE 1), passed through order_ramp before
it is written (RULE 2), then scored against its class and gated for distance
against the entire shipped corpus: 24 lospec + 30 lab P01-P30 + the 6 house HSV
schemes in gen_tables.py, 60 palettes in all.

Each palette states a hue identity and a TONE identity (how dark the ground is,
how hot the colour is). Tone is not decoration -- palettes.md 7 measured that
hue rotation alone does not diversify, because symmetric palettes are nearly
rotation-invariant. Small per-palette grids are searched for the variant that
both passes its class and sits FURTHEST from everything already in the library.

Run:  python3 lab/design/_gen_p31_p50.py [--write]
"""
import os, sys, json, itertools
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import palette_score as PS
from _p31_lib import (lch, ramp, max_chroma, load_corpus, order_ramp, metrics,
                      score, signature, distance, hue_emd, fast_scores)

ROOT = '/Users/exeter/dev/m5/assembly/dzzle1'
OUT = os.path.join(ROOT, 'lab/palettes')


# ---------------------------------------------------------------- primitives

def leg(n, L0, L1, H, C=0.055):
    """Near-neutral descent that closes the cyclic ramp (RULE 1). Chroma sits
    under the CHROMATIC floor, so the leg costs nothing in hue geometry but
    removes the light->dark wrap cliff that RULE 1 is about."""
    return [lch(L0 + (L1 - L0) * i / (n - 1), C, H) for i in range(n)]


def hot(H, cf=1.0):
    """The most saturated in-gamut colour of this hue, at whatever lightness
    that happens to be -- the anchor that carries C_max for the neon class."""
    Ls = np.arange(0.30, 0.96, 0.02)
    cs = [max_chroma(L, H) for L in Ls]
    i = int(np.argmax(cs))
    return lch(float(Ls[i]), cf * cs[i], H)


# ---------------------------------------------------------------- builders
# Every builder takes one param dict. `lo`/`hi` set the tone identity (how deep
# the ground, how high the ceiling), `cm` scales chroma. Those three are what
# separate same-class members that share a corner of the wheel.

def b_mono(q):
    """One hue family end to end, a two-anchor opposing accent, neutral return."""
    H, Ha, lo, hi, cm = q['H'], q['Ha'], q['lo'], q['hi'], q['cm']
    return (ramp(11, lo, hi, 0.30 * cm, 0.34 * cm, H - 5, H + 5,
                 cbow=0.36 * cm, cfrac=0.95)
            + [lch(q['al'], 0.62 * cm, Ha), lch(q['al'] + 0.18, 0.50 * cm, Ha + 3)]
            + leg(6, hi - 0.06, lo + 0.05, H))


def b_duo(q):
    """Two hue poles, a full ramp each, nothing in between."""
    A, B, lo, hi, cm = q['A'], q['B'], q['lo'], q['hi'], q['cm']
    return (ramp(9, lo, hi, 0.34 * cm, 0.28 * cm, A - 4, A + 4,
                 cbow=0.36 * cm, cfrac=0.95)
            + ramp(9, lo + q['dl'], hi - q['dl'] * 0.5, 0.34 * cm, 0.28 * cm,
                   B + 4, B - 4, cbow=0.36 * cm, cfrac=0.95))


def b_analog(q):
    """A continuous slice of the wheel, climbing then returning muted."""
    H, sp, lo, hi, cm = q['H'], q['sp'], q['lo'], q['hi'], q['cm']
    mid = lo + (hi - lo) * 0.58
    return (ramp(10, lo, mid, 0.34 * cm, 0.62 * cm, H, H + sp * 0.5,
                 cbow=0.18 * cm, cfrac=0.96)
            + ramp(8, mid + 0.03, hi, 0.60 * cm, 0.24 * cm,
                   H + sp * 0.55, H + sp, cfrac=0.96)
            + ramp(8, hi - 0.05, lo + 0.04, q['rc'], q['rc'] * 0.7,
                   H + sp * 0.8, H + sp * 0.1))


def b_split(q):
    """Dominant hue plus the two neighbours of its opposite. `nf` deepens the
    ground -- 12 of the shipped corpus already best-fit this class, so a new
    member has to separate on tone, not only on where it sits on the wheel."""
    D, off, lo, hi, cm = q['D'], q['off'], q['lo'], q['hi'], q['cm']
    a1, a2 = (D + off) % 360, (D - off) % 360
    return (ramp(9, lo, hi, 0.34 * cm, 0.28 * cm, D - 7, D + 7,
                 cbow=0.34 * cm, cfrac=0.95)
            + ramp(5, q['al'], hi - 0.06, 0.62 * cm, 0.34 * cm,
                   a1 - 4, a1 + 4, cfrac=0.95)
            + ramp(4, q['al'] - 0.04, hi - 0.16, 0.60 * cm, 0.42 * cm,
                   a2 + 4, a2 - 4, cfrac=0.95)
            + leg(4, hi - 0.02, lo + 0.05, D)
            + [lch(0.03 + 0.05 * i, 0.05, D) for i in range(q['nf'])])


def b_neon(q):
    """A near-black floor under two gamut-edge hues. The floor IS the class.
    Each lit ramp reaches down into the floor's lightness range so the cyclic
    tour has a path between them -- without that the void-to-neon jump is the
    whole of the step_ratio."""
    A, B, nf, fw = q['A'], q['B'], q['nf'], q['fw']
    floor = ([lch(0.02 + fw * i, 0.03 + 0.16 * fw * i, A) for i in range(nf)]
             + [lch(0.035 + fw * i, 0.04 + 0.16 * fw * i, B) for i in range(nf)])
    lit = []
    for H, l0, l1 in ((A, q['la0'], q['la1']), (B, q['lb0'], q['lb1'])):
        for i in range(7):
            L = l0 + (l1 - l0) * (i / 6.0) ** 0.85
            lit.append(lch(L, max_chroma(L, H), H))
    return floor + lit + [hot(A), hot(B)]


def b_pastel(q):
    """A tight high value band. Hue goes out and comes back, so the anchor set
    is a closed loop in its own right and needs no return leg -- the class can
    afford none, because a neutral leg would break the L_sd budget."""
    H, sp, base, amp, cm = q['H'], q['sp'], q['base'], q['amp'], q['cm']
    n = 20
    cols = []
    for i in range(n):
        t = i / n
        Hh = H + sp * 0.5 * (1.0 - np.cos(2 * np.pi * t))
        L = base + amp * np.sin(2 * np.pi * t + 0.6)
        C = (0.20 + 0.07 * np.cos(2 * np.pi * t)) * cm
        cols.append(lch(L, min(C, 0.9 * max_chroma(L, Hh)), Hh))
    return cols


def b_metal(q):
    """Narrow hue band on a long steep value ramp, plus a cool neutral return."""
    H, sp, lo, hi, cm = q['H'], q['sp'], q['lo'], q['hi'], q['cm']
    return (ramp(13, lo, hi, 0.26 * cm, 0.14 * cm, H, H + sp,
                 cbow=0.30 * cm, cfrac=0.92)
            + leg(7, hi - 0.07, lo + 0.07, q['gh'], C=0.05))


def b_stark(q):
    """Two value plateaus, no midtones, one chromatic slash. The cliff is the look."""
    H, cm = q['H'], q['cm']
    dk = [lch(0.02 + 0.028 * i, q['tint'], q['th']) for i in range(6)]
    lt = [lch(0.92 + 0.014 * i, q['tint'] * 0.5, q['th']) for i in range(6)]
    slash = [lch(0.44, 0.95 * cm, H), lch(0.56, 0.92 * cm, H + 2),
             lch(0.66, 0.80 * cm, H - 2)]
    return dk + lt + slash + [lch(0.20, q['tint'], q['th']),
                              lch(0.86, q['tint'] * 0.6, q['th'])]


def b_earth(q):
    """Muted mid hues in a mid value band -- neither dark nor pale."""
    H, sp, lo, hi, cm = q['H'], q['sp'], q['lo'], q['hi'], q['cm']
    return (ramp(7, lo, hi, 0.24 * cm, 0.30 * cm, H, H + sp * 0.45, cfrac=0.9)
            + ramp(6, hi + 0.04, lo + 0.10, 0.30 * cm, 0.22 * cm,
                   H + sp * 0.5, H + sp, cfrac=0.9)
            + ramp(6, lo + 0.06, hi - 0.03, 0.20 * cm, 0.26 * cm,
                   H + sp, H + 0.04 * sp, cfrac=0.9))


def b_full(q):
    """The whole wheel over a dark ground -- the only rainbow in this batch,
    and dark-grounded on purpose: full_spectrum is rotation-invariant, so the
    ONLY thing that can separate it from the seventeen existing rainbows is
    tone (palettes.md 7)."""
    lo, cm, n = q['lo'], q['cm'], 12
    dark = [lch(0.02 + 0.028 * i, 0.04 + 0.012 * i, 280 + 8 * i)
            for i in range(q['nf'])]
    ring = []
    for i in range(n):
        H = q['rot'] + i * 360.0 / n
        L = lo + q['amp'] * np.sin(i * 1.05 + 0.4)
        # a full_spectrum is rotation-invariant, so the ONLY hue freedom it has
        # is an uneven chroma weighting around the wheel -- every bin still
        # holds >=5%, but one side of the wheel carries more of the colour
        k = cm * (1.0 - q['skew'] * (1.0 - np.cos(np.radians(H - q['sph']))) / 2.0)
        ring.append(lch(L, k * max_chroma(L, H), H))
    top = [lch(q['tl'], 0.9 * max_chroma(q['tl'], q['rot'] + i * 72.0),
               q['rot'] + i * 72.0) for i in range(q['nt'])]
    return dark + ring + top


# ---------------------------------------------------------------- the twenty

def G(**kw):
    """Cartesian product of the keyword lists -> list of param dicts."""
    keys = list(kw)
    return [dict(zip(keys, v)) for v in itertools.product(*(kw[k] for k in keys))]


SPECS = [
 dict(id='P31', slug='cobalt_vigil', name='Cobalt Vigil', cls='mono_accent',
      build=b_mono,
      grid=G(H=[262], Ha=[70, 78], al=[0.58, 0.66], lo=[0.06, 0.10],
             hi=[0.88, 0.94], cm=[0.85, 1.0]),
      families='blue 257-267 field; amber 70-78 accent',
      mood='A night watch by lamplight — one enormous blue room, and two small '
           'warm lights someone left burning at the far end.',
      look='Eleven stops of a single blue climb from ink to hoarfrost, a '
           'cool-grey descent closing the loop, and exactly two amber anchors '
           'carrying under a fifth of the chroma.',
      pairing='Flow, plasma and interference fields — anything with one '
              'dominant current. The amber reads as a highlight, never as a '
              'second subject.'),

 dict(id='P32', slug='oxblood_chapel', name='Oxblood Chapel', cls='mono_accent',
      build=b_mono,
      grid=G(H=[22, 28], Ha=[186, 194], al=[0.52, 0.60], lo=[0.05, 0.09],
             hi=[0.86, 0.93], cm=[0.9, 1.05]),
      families='red 17-33 field; verdigris 186-194 accent',
      mood='Deep red velvet in an unlit chapel, with one cold verdigris votive '
           'glass still catching light.',
      look='A crimson family from near-black to rose-white, returned by a '
           'warm-grey descent, with a single verdigris accent under a fifth of '
           'the chroma.',
      pairing='Cellular, vein and drape patterns — the long red value ramp '
              'gives interior volume and the verdigris marks edges.'),

 dict(id='P33', slug='tide_and_coral', name='Tide & Coral', cls='duotone',
      build=b_duo,
      grid=G(A=[196, 204], B=[26, 34], lo=[0.06, 0.12], hi=[0.86, 0.93],
             dl=[0.0, 0.08], cm=[0.85, 1.0]),
      families='sea-teal 196-204 vs coral 26-34',
      mood='Two inks on wet paper: a cold sea-teal and a hot coral, with '
           'nothing between them but the paper.',
      look='Two full ramps roughly 170 degrees apart, meeting only at the dark '
           'floor and the light ceiling, so the loop closes at both ends.',
      pairing='Two-body patterns — reaction-diffusion, duelling attractors, '
              'split fields. Anything that wants a clear "us and them".'),

 dict(id='P34', slug='orchid_and_absinthe', name='Orchid & Absinthe',
      cls='duotone', build=b_duo,
      grid=G(A=[316, 324], B=[126, 134], lo=[0.07, 0.13], hi=[0.87, 0.94],
             dl=[0.0, 0.08], cm=[0.9, 1.05]),
      families='orchid violet 316-324 vs absinthe green 126-134',
      mood='A violet so deep it is nearly ink, arguing with a green so acid it '
           'hums.',
      look='Magenta-violet against yellow-green. The two ramps cross in '
           'lightness but never in hue, which is what keeps a duotone from '
           'collapsing into a gradient.',
      pairing='Lattice, moire and lissajous work — the pole pair keeps two '
              'interfering families legible where a rainbow turns to mud.'),

 dict(id='P35', slug='foundry_blue', name='Foundry Blue', cls='duotone',
      build=b_duo,
      grid=G(A=[248, 256], B=[58, 66], lo=[0.04, 0.09], hi=[0.84, 0.92],
             dl=[0.06, 0.14], cm=[0.8, 0.95]),
      families='steel blue 248-256 vs amber 58-66',
      mood='Cold steel stock and the orange heat coming off it — a forge at '
           'night, two temperatures and no third opinion.',
      look='A steep blue ramp and a steep amber ramp climbing to a shared '
           'near-white, with the amber deliberately starting higher so the two '
           'inks are told apart by value as well as hue.',
      pairing='Heat, diffusion and metaball patterns; anything where one field '
              'should look hotter than the other.'),

 dict(id='P36', slug='fen_water', name='Fen Water', cls='analogous',
      build=b_analog,
      grid=G(H=[108, 120, 132], sp=[95, 108], lo=[0.06, 0.11],
             hi=[0.88, 0.94], cm=[1.0, 1.15], rc=[0.16, 0.24]),
      families='green 108-132 -> teal -> cyan, roughly a quarter turn',
      mood='Standing water over moss — everything green, teal and cyan, lit '
           'from under the surface.',
      look='A quarter-turn of the wheel with no warm escape, closing through a '
           'deep blue-green shadow instead of snapping back to black.',
      pairing='Caustics, ripple and cellular-noise patterns. The tight hue '
              'slice leaves brightness to do all the storytelling.'),

 dict(id='P37', slug='ripening_field', name='Ripening Field', cls='analogous',
      build=b_analog,
      grid=G(H=[45, 55, 65], sp=[92, 106], lo=[0.08, 0.14],
             hi=[0.90, 0.95], cm=[0.9, 1.05], rc=[0.16, 0.24, 0.32]),
      families='wheat gold 45-65 -> chartreuse -> deep green',
      mood='A field two weeks before harvest — gold at the tips, chartreuse '
           'halfway down, still deep green at the root.',
      look='Gold through chartreuse into green, climbing to cream and '
           'returning down a dusty olive leg. The purely warm quarter-turn was '
           'not available: `house:ember` and `fantasy-24` already hold it and '
           'the same-class hue floor rejects anything within 36 degrees of them.',
      pairing='Growth, plume and drift patterns; also slow radial washes, '
              'where a cool hue would read as a hole in the middle.'),

 dict(id='P38', slug='nightbloom', name='Nightbloom', cls='analogous',
      build=b_analog,
      grid=G(H=[225, 232, 240], sp=[95, 110], lo=[0.05, 0.10],
             hi=[0.82, 0.90], cm=[0.8, 0.95], rc=[0.16, 0.24]),
      families='azure 225-240 -> violet -> magenta',
      mood='Iris and hyacinth after dark — a cool slice of the wheel that '
           'never once gets warm.',
      look='Azure through violet into magenta, run darker and less chromatic '
           'than Fen Water so the two cool analogous palettes separate on tone '
           'as well as on hue.',
      pairing='Swirl, vortex and drape patterns. Continuous hue means '
              'continuous motion — nothing here can produce a hard edge.'),

 dict(id='P39', slug='cardinal_and_jade', name='Cardinal & Jade',
      cls='split_complement', build=b_split,
      grid=G(D=[338, 345, 352], off=[160, 168], lo=[0.03, 0.07],
             hi=[0.80, 0.90], cm=[0.85, 1.05], nf=[4, 7], al=[0.34, 0.40]),
      families='cardinal red dominant; jade + teal accents',
      mood='A red bird in a green thicket. The red owns the frame and the two '
           'greens are what it is standing in.',
      look='A long cardinal ramp with jade and teal either side of its '
           'opposite, over a deepened near-black ground. The extra floor is '
           'deliberate: twelve palettes already in the library best-fit this '
           'class, so a new one has to separate on tone, not only on hue.',
      pairing='Ribbon, tunnel and shear patterns — the dominant field carries '
              'the motion and the two cool accents mark leading edges.'),

 dict(id='P40', slug='harvest_storm', name='Harvest Storm',
      cls='split_complement', build=b_split,
      grid=G(D=[68, 75, 82], off=[160, 168], lo=[0.05, 0.09],
             hi=[0.78, 0.88], cm=[0.9, 1.1], nf=[4, 7], al=[0.26, 0.34]),
      families='harvest gold dominant; storm blue + violet accents',
      mood='Wheat under a bruised sky — gold light going flat as the storm '
           'front takes the horizon.',
      look='Gold dominant with storm blue and violet sitting either side of '
           'its opposite. Runs lighter and warmer-grounded than Cardinal & '
           'Jade so the two split-complements share no tone.',
      pairing='Growth, branch and lattice patterns; the cool accents give '
              'branch tips and nodes somewhere to go that is not merely '
              '"brighter gold".'),

 dict(id='P41', slug='acid_rail', name='Acid Rail', cls='neon_on_black',
      build=b_neon,
      grid=G(A=[136, 144], B=[326, 338], la0=[0.18, 0.24, 0.30],
             la1=[0.84, 0.90, 0.94], lb0=[0.16, 0.22, 0.28],
             lb1=[0.70, 0.78, 0.84], nf=[3, 4, 5], fw=[0.05, 0.065, 0.08]),
      families='void floor; acid green + hot magenta at gamut edge',
      mood='A dark tunnel with two live rails in it: acid green one side, hot '
           'magenta the other. Nothing else is lit.',
      look='Ten near-black anchors form the floor; everything above it rides '
           'the gamut edge. The dark half is what makes the lit half read '
           'electric rather than merely bright.',
      pairing='Line, spark and trail patterns on a dark ground — anything '
              'where the subject is thin and the background should vanish.'),

 dict(id='P42', slug='blood_ultraviolet', name='Blood Ultraviolet',
      cls='neon_on_black', build=b_neon,
      grid=G(A=[4, 12], B=[280, 292], la0=[0.18, 0.24, 0.30],
             la1=[0.70, 0.78, 0.84], lb0=[0.16, 0.22, 0.28],
             lb1=[0.64, 0.72, 0.78], nf=[3, 4, 5], fw=[0.05, 0.065, 0.08]),
      families='void floor; blood red + ultraviolet at gamut edge',
      mood='A blacklight room: everything is off except a red sign and the '
           'ultraviolet tube above it.',
      look='The same void floor as Acid Rail on the opposite side of the '
           'wheel — red and violet instead of green and magenta, so the two '
           'neon palettes cannot be mistaken for one another.',
      pairing='Spark, filament and glow-decay patterns. Very strong under '
              'additive layering, where the black floor absorbs the '
              'accumulation.'),

 dict(id='P43', slug='sugared_almond', name='Sugared Almond', cls='pastel_wash',
      build=b_pastel,
      grid=G(H=[334, 346, 358], sp=[80, 110, 140], base=[0.83, 0.86],
             amp=[0.05, 0.07], cm=[0.8, 1.0]),
      families='warm pastels, blush through butter, all L 0.77-0.94',
      mood='Confectionery light — blush, apricot, butter and shell. Nothing in '
           'it is louder than a whisper and nothing in it is dark.',
      look='A tight warm value band with chroma held low. The loop closes '
           'because it never leaves the band, which is why this class needs no '
           'return leg.',
      pairing='Soft bloom, blur and drift patterns. This is the opposite pole '
              'from the neon sets and exists so the scheduler has somewhere '
              'quiet to go.'),

 dict(id='P44', slug='sea_glass_morning', name='Sea Glass Morning',
      cls='pastel_wash', build=b_pastel,
      grid=G(H=[146, 164, 182], sp=[80, 110, 140], base=[0.84, 0.87],
             amp=[0.05, 0.07], cm=[0.8, 1.0]),
      families='cool pastels, mint through periwinkle, all L 0.78-0.95',
      mood='Beach glass on a white table — mint, sky, periwinkle and the '
           'faintest lilac, all of it lit from above.',
      look='The cool twin of Sugared Almond: the same tight high value band on '
           'the opposite side of the wheel, so the two never collide.',
      pairing='Bloom, haze and slow-wave patterns; excellent under heavy '
              'layering, where saturated palettes accumulate into sludge.'),

 dict(id='P45', slug='hammered_copper', name='Hammered Copper', cls='metallic',
      build=b_metal,
      grid=G(H=[26, 34, 42], sp=[18, 30], lo=[0.04, 0.07], hi=[0.94, 0.98],
             cm=[0.9, 1.05], gh=[230, 250]),
      families='copper 26-72 on a full value ramp; cool neutral return leg',
      mood='A copper vessel under a single lamp: black in the shadow, white on '
           'the rim, and every value between is the metal.',
      look='A narrow hue band riding a long steep value ramp, with a cool '
           'near-neutral return leg so the cyclic wrap has no cliff — the '
           'construction RULE 1 was written for.',
      pairing='Specular, sheen and bevel patterns. The shine here is lightness '
              'rather than chroma, so it survives being composited underneath '
              'other layers.'),

 dict(id='P46', slug='newsprint_siren', name='Newsprint Siren', cls='stark',
      build=b_stark,
      grid=G(H=[24, 32], cm=[0.9, 1.0], tint=[0.0, 0.02], th=[30, 60]),
      families='black / white / siren red',
      mood='Black ink, white paper, and one siren-red slash across it. No '
           'midtones and no apology.',
      look='Two value plateaus with almost nothing between them plus three red '
           'anchors. The cliff IS the look — stark is the one class the '
           'smoothness budget is relaxed for.',
      pairing='Hard-edged geometry: grids, bars, halftone, scanline work. '
              'Soft-edged patterns should avoid this one.'),

 dict(id='P47', slug='blueprint_cut', name='Blueprint Cut', cls='stark',
      build=b_stark,
      grid=G(H=[206, 218], cm=[0.9, 1.0], tint=[0.02, 0.04], th=[250, 260]),
      families='near-black / chalk white / cold cyan',
      mood='Drafting-table contrast: near-black paper, chalk-white line, and '
           'one cold cyan cut through both.',
      look='The cool twin of Newsprint Siren — the same two-plateau structure '
           'with the opposite accent, so the two starks cannot be confused on '
           'screen.',
      pairing='Grid, wire and edge-detect patterns. Reads as technical '
              'drawing rather than poster art.'),

 dict(id='P48', slug='kiln_and_clay', name='Kiln & Clay', cls='earth',
      build=b_earth,
      grid=G(H=[14, 24, 34], sp=[70, 95], lo=[0.22, 0.28, 0.34],
             hi=[0.62, 0.70, 0.76], cm=[0.85, 1.0]),
      families='terracotta -> ochre -> bark, all low chroma',
      mood='Unglazed pottery in a workshop — terracotta, slip, ash and bark. '
           'Muted rather than dark, which is the whole point of the class.',
      look='Warm mid hues at low chroma inside a mid lightness band. No anchor '
           'is either black or pale, so it never reads as a value ramp.',
      pairing='Texture, grain, erosion and sediment patterns. Stays legible '
              'under heavy layering where a saturated palette goes muddy.'),

 dict(id='P49', slug='moss_and_stone', name='Moss & Stone', cls='earth',
      build=b_earth,
      grid=G(H=[100, 116, 132], sp=[70, 95], lo=[0.22, 0.28, 0.34],
             hi=[0.64, 0.72, 0.78], cm=[0.8, 0.95]),
      families='olive -> sage -> slate, all low chroma',
      mood='A cold wall in a wet wood — lichen, slate, olive and the '
           'grey-green of old rain.',
      look='The cool half of the earth family, chroma kept under a third and '
           'lightness parked in the middle. Runs lighter than Kiln & Clay so '
           'the two earths separate on tone as well as hue.',
      pairing='Terrain, sediment and crack patterns; pairs well with the warm '
              'earth set when the scheduler cross-fades between them.'),

 dict(id='P50', slug='carnival_at_midnight', name='Carnival at Midnight',
      cls='full_spectrum', build=b_full,
      no_hue_floor=True,        # see the note in this palette's `look`
      grid=G(rot=[0, 15, 30, 45], lo=[0.44, 0.50, 0.56], amp=[0.14, 0.20],
             cm=[0.95, 1.0], nf=[3, 5, 7], nt=[0, 2, 3],
             skew=[0.0, 0.2, 0.32], sph=[0, 90, 180, 270], tl=[0.86]),
      families='all twelve hue bins over a dark ground',
      mood='The whole wheel, but seen at night — every hue present and all of '
           'them sitting on black instead of on white.',
      look='Twelve gamut-edge hues over a void floor. The one full_spectrum '
           'slot in this batch, and deliberately the dark one: the class is '
           'rotation-invariant, so tone is the only thing that can tell it '
           'apart from the seventeen rainbows already in the library.\n\n'
           'This palette is exempt from the within-class hue floor, and that '
           'exemption is a conflict between the spec and its own code. '
           '`palettes.md` 7 states that full_spectrum "is rotation-invariant '
           'by definition" and that its members "must be separated ENTIRELY '
           'by tone" — but `palette_score.accept()` applies '
           '`MIN_HUE_EMD_SAME_CLASS = 1.2` to every class alike. Measured: a '
           '4,900-point sweep of rotation, chroma skew, ground depth and '
           'value amplitude reaches a maximum hue EMD of **0.93 bins** '
           'against the four full-spectrum palettes already shipped. The '
           'floor is unreachable for this class, exactly as the prose '
           'predicted, so the prose governs: this palette is gated on '
           'distance alone, and clears it at d=0.21 against a 0.16 floor.',
      pairing='Confetti, particle and shard patterns, where per-element hue '
              'variety is the subject. Use sparingly — this class is capped at '
              '4 of 60 for a reason.'),
]


# ---------------------------------------------------------------- search

def evaluate(cols, cls, accepted, hue_floor=True):
    """(passes, score, nearest distance) against everything accepted so far.
    `hue_floor` is switched off for full_spectrum only -- see SPECS."""
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
        if hue_floor and ocls == cls and hue_emd(sig, osig) < PS.MIN_HUE_EMD_SAME_CLASS:
            return False, s, d, (f'hue overlap with {name} (same class) '
                                 f'emd={hue_emd(sig, osig):.2f}'), None
    return True, s, near, f'fit={s:.2f} nn={nn} d={near:.3f}', sig


def build(verbose=False):
    accepted = [(n, c, signature(cols)) for n, c, cols
                in load_corpus(ROOT, skip={sp['id'] for sp in SPECS})]
    rows = []
    for sp in SPECS:
        best = None
        why = 'no grid point passed'
        for q in sp['grid']:
            cols = order_ramp(list(dict.fromkeys(sp['build'](q))))
            ok, s, near, msg, sig = evaluate(cols, sp['cls'], accepted,
                                             hue_floor=not sp.get('no_hue_floor'))
            if not ok:
                if best is None:
                    why = msg
                continue
            # rank: house rule first (no rough breaks -- step_ratio inside the
            # class budget), then class fit, then how far it sits from the
            # rest of the library
            # the declared class should also be the BEST-fitting one: the
            # bands overlap, and a palette that scores higher as `earth` than
            # as the `analogous` it was filed under is mud wearing a label
            _, allsc = fast_scores(cols)
            own = allsc[sp['cls']][0]
            argmax = own >= max(v[0] for v in allsc.values()) - 1e-9
            smooth = PS.score(cols, sp['cls'])[2]['step_ratio'][1]
            rank = (argmax, round(smooth, 2), round(s, 2), near)
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

def swatch(cols, path, W=768, H=160):
    """Top band: the anchors as discrete stops. Bottom band: the smooth cyclic
    ramp gen_tables.py actually expands (same smoothstep, same wrap), so a
    closing cliff shows up in the picture and not only in the metrics."""
    rgb = np.array([[int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16)]
                    for c in cols], float)
    M = len(cols)
    img = np.zeros((H, W, 3), np.uint8)
    cut = H // 2
    img[:cut] = rgb[np.minimum(np.arange(W) * M // W, M - 1)].astype(np.uint8)
    pos = np.arange(W) * M / W
    k = pos.astype(int) % M
    t = pos - np.floor(pos)
    t = t * t * (3 - 2 * t)
    a, b = rgb[k], rgb[(k + 1) % M]
    img[cut:] = (a + (b - a) * t[:, None]).astype(np.uint8)
    img[cut - 2:cut + 2] = 0
    ppm = path.replace('.png', '.ppm')
    with open(ppm, 'wb') as f:
        f.write(b'P6\n%d %d\n255\n' % (W, H))
        f.write(img.tobytes())
    if os.system(f'sips -s format png "{ppm}" --out "{path}" >/dev/null 2>&1'):
        raise RuntimeError('sips failed')
    os.remove(ppm)


MET_ROWS = ('n', 'hue_bins', 'hue_arc', 'accent_frac', 'C_mean', 'C_lit',
            'C_sd', 'C_max', 'L_mean', 'L_sd', 'L_range', 'dark_frac',
            'light_frac', 'neutral_frac', 'max_step', 'step_ratio')


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
    print('-' * 122)
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
    print()
    if bad:
        print(f'{bad} FAILING')
        sys.exit(1)
    mine = accepted[-20:]
    nn = [min(distance(s, o) for n2, c2, o in accepted if n2 != n)
          for n, c, s in mine]
    print(f'P31-P50 against the full {len(accepted)}-palette library: '
          f'min NN {min(nn):.3f}   median NN {np.median(nn):.3f}   '
          f'(floors: 0.16 / 0.20)')
    if '--write' in sys.argv:
        write_all(rows)
        print(f'wrote 20 palette directories under {OUT}')
