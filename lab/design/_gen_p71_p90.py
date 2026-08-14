#!/usr/bin/env python3
"""
_gen_p71_p90.py -- authors palettes P71..P90 for JellyDazzle v2.1.

The second half of the taxonomy fill. P31..P50 took 20 of the 60 quota slots;
this file takes 20 more, leaving 20 for P51..P70. Class allocation here:

    mono_accent 3   duotone 2   analogous 2   split_complement 1
    neon_on_black 3 pastel_wash 3  metallic 2  stark 2  earth 2
    full_spectrum 0

The split is uneven because the wheel is unevenly occupied -- see the scout
note above SPECS. analogous and split_complement are nearly full; neon_on_black
and pastel_wash, the electric and the soft pole, are wide open, so they take
the slots the crowded classes could not use.

ZERO rainbows. full_spectrum is capped at 4 of 60 across the whole batch and
P50 already took one; the remaining three are left for P51..P70. J's complaint
is that half the shipped library is full-spectrum lookalikes, so this batch
spends every slot on the classes that make palettes DIFFER -- three of the four
classes the v2.0 library has never had a member of (mono_accent, pastel_wash,
stark) get seven slots between them here.

Construction is identical to P31..P50 and deliberately reuses its builders:
author in OKLCh, close the anchor list as a LOOP so the cyclic wrap has no
cliff (RULE 1), order_ramp() before writing (RULE 2), then score against the
class and gate for distance against the entire corpus (24 lospec + every
lab/palettes/P* on disk + the 6 house HSV schemes).

Hue families are chosen to sit clear of the same-class members already written,
because within a class the tone vector is pinned by the class definition and
ALL separation has to come from hue (palettes.md 7).

Run:  python3 lab/design/_gen_p71_p90.py [--write]
"""
import os, sys, json
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import palette_score as PS
from _corpus_safe import load_corpus
from _p31_lib import (order_ramp, metrics, score, signature,
                      distance, hue_emd, fast_scores, lch, ramp)
# the ten builders and the output plumbing are already written and validated by
# P31..P50; re-implementing them here would be two copies to keep in sync
from _gen_p31_p50 import (b_duo, b_analog, b_split, b_neon, b_pastel,
                          b_metal, b_stark, b_earth, G, leg, swatch, MET_ROWS)

ROOT = '/Users/exeter/dev/m5/assembly/dzzle1'
OUT = os.path.join(ROOT, 'lab/palettes')


# ---------------------------------------------------------------- the twenty
#
# Every hue site below was chosen by _scout_p71.py, which sweeps each builder
# over the whole wheel and reports which sites clear BOTH the class gate and
# the same-class hue floor against the corpus on disk. Guessing hue families by
# eye does not work any more: the library is dense enough that most of the
# wheel is already spoken for in most classes.
#
# What the scout found, and why the allocation below is not the naive even
# split:
#
#   split_complement  1 free site  (13 corpus members already best-fit it)
#   duotone          51 free sites
#   neon_on_black    74 free sites
#   pastel_wash      18 free sites
#   stark            16 free sites
#   metallic         18 free sites
#   mono_accent      16 free sites
#   earth            12 free sites
#   analogous         3 free sites (P36/P37/P38 + house:ember + fantasy-24
#                                   between them hold most of the wheel)
#
# So this batch takes 1 split_complement and 2 analogous rather than 2 and 3,
# and spends the two freed slots on neon_on_black and pastel_wash -- the
# electric and the soft pole, which is exactly the axis J asked to be widened.


def b_mono2(q):
    """b_mono with an accent-chroma knob.

    b_mono fixes the accent at 0.62 chroma, which is fine for a cobalt field
    (max in-gamut C_norm ~0.49 at 240 deg) and wrong for a jade one (~0.71 at
    144): the same accent takes a much larger share of the total chroma weight
    and pushes accent_frac over the class ceiling of 0.22. `ac` scales the two
    accent anchors so the field/accent BALANCE, not the absolute chroma, is
    what the grid searches over."""
    H, Ha, lo, hi, cm, ac = q['H'], q['Ha'], q['lo'], q['hi'], q['cm'], q['ac']
    return (ramp(11, lo, hi, 0.30 * cm, 0.34 * cm, H - 5, H + 5,
                 cbow=0.36 * cm, cfrac=0.95)
            + [lch(q['al'], 0.62 * ac, Ha), lch(q['al'] + 0.18, 0.50 * ac, Ha + 3)]
            + leg(6, hi - 0.06, lo + 0.05, H))


SPECS = [

 # ---- mono_accent x3 ------------------------------------------------------
 # P31 fields cobalt 262, P32 fields oxblood 22. These three take jade, plum
 # and brass, so the five mono_accents in the library sit at 22 / 84 / 144 /
 # 262 / 336 -- five different quadrants, which is the whole point of the
 # same-class hue floor.
 dict(id='P71', slug='jade_lantern', name='Jade Lantern', cls='mono_accent',
      build=b_mono2,
      grid=G(H=[138, 144, 150], Ha=[28, 40], al=[0.50, 0.58, 0.66],
             lo=[0.04, 0.08], hi=[0.86, 0.93], cm=[0.9, 1.05],
             ac=[0.45, 0.6, 0.75]),
      families='jade 138-150 field; ember orange 28-40 accent',
      mood='A jade room lit from one corner — everything in it is green stone '
           'except the single paper lantern burning at the far end.',
      look='Eleven stops of one jade family from near-black to frost, a '
           'near-neutral descent closing the loop, and exactly two ember '
           'anchors holding under a fifth of the chroma. Jade sits high in the '
           'sRGB gamut, so the accent had to be scaled DOWN to keep the class '
           'ceiling — see `b_mono2`.',
      pairing='Flow, caustic and interference fields — anything with a single '
              'dominant current. The ember reads as a spark inside the field, '
              'never as a competing subject.'),

 dict(id='P72', slug='damson_hour', name='Damson Hour', cls='mono_accent',
      build=b_mono2,
      grid=G(H=[330, 336, 342], Ha=[76, 88], al=[0.50, 0.58, 0.66],
             lo=[0.04, 0.09], hi=[0.86, 0.93], cm=[0.85, 1.0],
             ac=[0.4, 0.55, 0.7]),
      families='damson plum 330-342 field; old gold 76-88 accent',
      mood='Plums in a dark bowl, and one brass lamp behind them. Almost the '
           'whole frame is the fruit.',
      look='A single plum-rose family from bruise-black to blossom-white, '
           'returned by a desaturated descent, with two gold anchors as the '
           'only warm-neutral thing in it. Sits a clear 60 degrees off P32 '
           'Oxblood Chapel so the two red-family mono_accents never read the '
           'same.',
      pairing='Drape, bloom and vortex patterns. The gold accent marks leading '
              'edges without ever turning the scheme into a duotone.'),

 dict(id='P73', slug='brass_vigil', name='Brass Vigil', cls='mono_accent',
      build=b_mono2,
      grid=G(H=[78, 84, 90], Ha=[258, 272], al=[0.44, 0.54, 0.64],
             lo=[0.04, 0.10], hi=[0.86, 0.94], cm=[0.85, 1.0],
             ac=[0.4, 0.55, 0.7]),
      families='brass 78-90 field; cold indigo 258-272 accent',
      mood='Lamplit brass and old paper, with one cold blue window behind it.',
      look='A warm brass family from bark to bone-white with a muted return '
           'leg, and exactly two indigo anchors — the exact inverse of P31 '
           'Cobalt Vigil, which is a blue field carrying warm accents.',
      pairing='Grain, sediment and slow-wash patterns; the long warm value '
              'ramp does the modelling and the indigo marks depth.'),

 # ---- duotone x2 ----------------------------------------------------------
 # P33 teal/coral, P34 orchid/absinthe, P35 blue/amber, P39 cardinal/jade are
 # taken. The two pole pairs below are the widest-open sites the scout found.
 dict(id='P74', slug='violet_and_wheat', name='Violet & Wheat', cls='duotone',
      build=b_duo,
      grid=G(A=[280, 288, 296], B=[70, 78, 86], lo=[0.05, 0.11],
             hi=[0.86, 0.94], dl=[0.0, 0.10], cm=[0.9, 1.1]),
      families='violet 280-296 vs wheat gold 70-86',
      mood='Two inks that should not work together and do: a cold violet and a '
           'dry wheat gold, with nothing between them.',
      look='Two full ramps a little over 150 degrees apart, meeting only at '
           'the shared floor and ceiling so the loop closes at both ends and '
           'no third hue family ever appears.',
      pairing='Two-body patterns — duelling attractors, reaction-diffusion, '
              'split fields. The pole pair keeps "us and them" legible.'),

 dict(id='P75', slug='fuchsia_and_verdigris', name='Fuchsia & Verdigris',
      cls='duotone', build=b_duo,
      grid=G(A=[312, 320, 328], B=[194, 202, 210], lo=[0.05, 0.11],
             hi=[0.86, 0.94], dl=[0.0, 0.10], cm=[0.9, 1.1]),
      families='fuchsia 312-328 vs verdigris 194-210',
      mood='Hot pink neon reflected in oxidised copper. Two inks, both loud, '
           'and no referee between them.',
      look='Fuchsia is the highest-chroma hue sRGB has (C_norm 0.95 against '
           'red\'s 0.79), verdigris one of the lowest — so this duotone is '
           'lopsided on purpose, and the teal side reads as the ground the '
           'pink is sitting on.',
      pairing='Lattice, moire and shear patterns — two interfering families '
              'stay readable here where a full-spectrum palette turns to mud.'),

 # ---- analogous x2 --------------------------------------------------------
 # Only 3 free sites on the whole wheel: P36 green->cyan, P37 gold->green,
 # P38 azure->magenta, house:ember and fantasy-24 hold the rest between them.
 dict(id='P76', slug='claret_run', name='Claret Run', cls='analogous',
      build=b_analog,
      grid=G(H=[306, 314, 322, 330], sp=[64, 76, 88, 100],
             lo=[0.05, 0.09, 0.13], hi=[0.84, 0.90, 0.95],
             cm=[0.85, 1.0, 1.15], rc=[0.18, 0.26, 0.34]),
      families='orchid 312-328 -> claret -> ember, roughly a fifth of a turn',
      mood='The last of the light through a bottle — orchid at the neck, '
           'claret through the body, ember where it touches the table.',
      look='A short continuous slice of the wheel that starts violet and ends '
           'warm, climbing to a pale rose and returning down a dusty brick '
           'leg. The purely warm quarter-turn was not available: house:ember '
           'and fantasy-24 already hold it and the same-class hue floor '
           'rejects anything within 36 degrees of them.',
      pairing='Plume, drift and drape patterns; also slow radial washes, where '
              'a cool hue would read as a hole punched in the middle.'),

 dict(id='P77', slug='reed_bed', name='Reed Bed', cls='analogous',
      build=b_analog,
      grid=G(H=[72, 80, 88], sp=[88, 100, 112], lo=[0.09, 0.14],
             hi=[0.90, 0.95], cm=[1.15, 1.3], rc=[0.06, 0.09, 0.13, 0.18]),
      families='chartreuse 72-88 -> green -> teal, roughly a third of a turn',
      mood='Late sun through a reed bed — chartreuse at the tips, green down '
           'the stems, and teal in the water underneath.',
      look='The jade->teal->azure slice this palette started as could not be '
           'shipped: at every chroma the builder can reach, that arc scores '
           '1.00 as `earth` and only 0.97 as `analogous`, because teal and '
           'azure sit low in the sRGB gamut (max C_norm 0.47 and 0.42) and a '
           'muted mid-lightness cool ramp IS an earth palette by definition. '
           'This site clears it: chartreuse and green reach C_norm 0.53-0.75, '
           'so a high-chroma climb over a near-neutral return leg pushes C_sd '
           'past what `earth` allows.',
      pairing='Ripple, caustic and cellular-noise patterns. With hue held to a '
              'narrow slice, brightness does all the storytelling.'),

 # ---- split_complement x1 -------------------------------------------------
 # The one free site in the entire class. Thirteen palettes already in the
 # library best-fit split_complement; the scout found exactly one dominant hue
 # whose profile clears the 1.2-bin floor against all of them.
 dict(id='P78', slug='lagoon_triad', name='Lagoon Triad',
      cls='split_complement', build=b_split,
      grid=G(D=[188, 195, 202], off=[112, 122, 132, 142],
             lo=[0.06, 0.09, 0.12], hi=[0.86, 0.90, 0.94],
             cm=[0.85, 0.95, 1.05], nf=[5, 7, 9], al=[0.24, 0.32, 0.40]),
      families='lagoon cyan dominant; fuchsia + gold accents',
      mood='Shallow water over pale sand with something pink living in it. The '
           'lagoon owns the frame and the two warm notes are what is in it.',
      look='A long cyan ramp against fuchsia and gold — a true triad rather '
           'than a strict split, which is what the one remaining free site in '
           'this class turned out to be. The mirror of P39 Cardinal & Jade: '
           'there a warm dominant with cool accents, here the reverse.',
      pairing='Tunnel, shear and ribbon patterns; the dominant field carries '
              'the motion and the two warm accents mark leading edges.'),

 # ---- neon_on_black x3 ----------------------------------------------------
 # The most open class on the board (74 free sites) and the one that reads as
 # "amazing", so it gets the slot analogous could not use. P41 green/magenta
 # and P42 red/violet are taken.
 dict(id='P79', slug='marsh_light', name='Marsh Light', cls='neon_on_black',
      build=b_neon,
      grid=G(A=[136, 140, 146], B=[40, 46, 52], la0=[0.24, 0.32],
             la1=[0.84, 0.90], lb0=[0.22, 0.30], lb1=[0.80, 0.88],
             nf=[4, 5, 6], fw=[0.05, 0.065]),
      families='void floor; acid green 136-146 + ember orange 40-52 at the '
               'gamut edge',
      mood='Will-o\'-the-wisp over black water: a cold green light and a warm '
           'one, both a long way off, and nothing lit between here and them.',
      look='This started as a two-green neon and could not be shipped that '
           'way: a single hue family climbing out of black over a steep value '
           'ramp scores 1.00 as `metallic` — that is the literal definition of '
           'the class — and only 0.95 as `neon_on_black`. Opening the pair to '
           '96 degrees of hue arc kills the metallic reading outright while '
           'keeping both lit ramps on the gamut edge.',
      pairing='Line, spark and trail patterns on a dark ground — anything '
              'where the subject is thin and the background should vanish.'),

 dict(id='P80', slug='sodium_rose', name='Sodium Rose', cls='neon_on_black',
      build=b_neon,
      grid=G(A=[334, 342], B=[74, 84], la0=[0.24, 0.32, 0.40],
             la1=[0.78, 0.86], lb0=[0.22, 0.30, 0.38], lb1=[0.80, 0.88],
             nf=[4, 5, 6, 7], fw=[0.05, 0.065]),
      families='void floor; rose 334-342 + sodium gold 74-84 at the gamut edge',
      mood='A motorway at three in the morning: sodium lamps overhead, a rose '
           'neon sign at the exit, and nothing lit in between.',
      look='Rose sits at the sRGB chroma maximum, so this one carries the '
           'highest C_max of the three neons while the gold side stays '
           'comparatively restrained — a deliberately unbalanced pair.',
      pairing='Filament, spark and glow-decay patterns. Very strong under '
              'additive layering, where the black floor absorbs the '
              'accumulation instead of blowing out.'),

 dict(id='P81', slug='ion_trail', name='Ion Trail', cls='neon_on_black',
      build=b_neon,
      grid=G(A=[274, 282], B=[194, 204], la0=[0.24, 0.32, 0.40],
             la1=[0.78, 0.86], lb0=[0.22, 0.30, 0.38], lb1=[0.80, 0.88],
             nf=[4, 5, 6, 7], fw=[0.05, 0.065]),
      families='void floor; violet 274-282 + ion cyan 194-204 at the gamut edge',
      mood='A long-exposure of something moving fast in the dark — violet at '
           'the head of the trail, cyan where it has already cooled.',
      look='The coldest of the three neons: both lit hues are on the blue half '
           'of the wheel, which no other neon in the library is. The violet '
           'carries the chroma and the cyan carries the reach.',
      pairing='Trail, comet and motion-blur patterns; also anything additive, '
              'where two cool hues stack into white instead of into mud.'),

 # ---- pastel_wash x3 ------------------------------------------------------
 # The soft pole, and the class with the second-most room. P43 warm blush and
 # P44 cool mint are taken; these three split the rest of the circle.
 dict(id='P82', slug='lilac_powder', name='Lilac Powder', cls='pastel_wash',
      build=b_pastel,
      grid=G(H=[306, 315, 324], sp=[70, 90, 110], base=[0.82, 0.85],
             amp=[0.05, 0.07], cm=[0.75, 0.9]),
      families='lilac and orchid pastels, all L 0.76-0.93',
      mood='Powdered sugar over something purple. Lilac, orchid and the '
           'palest mauve, none of it louder than a whisper.',
      look='A tight high value band with chroma held low, walking out and back '
           'across the orchid slice. The loop closes because it never leaves '
           'the band — which is why this class needs no return leg, and could '
           'not afford one anyway without breaking the L_sd budget.',
      pairing='Bloom, blur and slow-drift patterns. This is where the '
              'scheduler goes when the composition needs to get quiet.'),

 dict(id='P83', slug='buttermilk_field', name='Buttermilk Field',
      cls='pastel_wash', build=b_pastel,
      grid=G(H=[80, 90, 100], sp=[70, 90, 110], base=[0.83, 0.86],
             amp=[0.05, 0.07], cm=[0.8, 0.95]),
      families='buttermilk through celadon pastels, all L 0.77-0.94',
      mood='Sunlight on a painted wall — buttermilk, straw, and the green a '
           'garden throws back onto plaster.',
      look='The yellow-green third of the wheel at pastel weight. Between this, '
           'P43 (warm blush), P44 (cool mint) and P82 (lilac) the pastels are '
           'spread evenly around the circle instead of clustering the way the '
           'shipped library does.',
      pairing='Haze, bloom and slow-wave patterns; excellent under heavy '
              'layering, where saturated palettes accumulate into sludge.'),

 dict(id='P84', slug='periwinkle_hour', name='Periwinkle Hour',
      cls='pastel_wash', build=b_pastel,
      grid=G(H=[246, 255, 264], sp=[70, 90, 110], base=[0.82, 0.85],
             amp=[0.05, 0.07], cm=[0.8, 0.95]),
      families='periwinkle and cornflower pastels, all L 0.76-0.93',
      mood='Six in the morning through a net curtain — periwinkle, cornflower '
           'and the faintest grey-blue.',
      look='The coolest of the pastels and the only one on the blue side of '
           'the wheel that is not mint. Runs marginally darker than Buttermilk '
           'Field so the three new pastels separate on tone as well as hue.',
      pairing='Bloom, haze and slow-wave patterns; pairs especially well '
              'cross-fading into the neon sets, since it is their exact '
              'opposite in every metric the taxonomy measures.'),

 # ---- metallic x2 ---------------------------------------------------------
 # P45 copper 30 is the only metallic in the library. These take the two ends
 # the scout left widest open.
 dict(id='P85', slug='rose_gold', name='Rose Gold', cls='metallic',
      build=b_metal,
      grid=G(H=[314, 322, 330], sp=[14, 22, 30], lo=[0.03, 0.06],
             hi=[0.94, 0.98], cm=[0.85, 1.0], gh=[0, 120, 200]),
      families='rose gold 314-360 on a full value ramp; neutral return leg',
      mood='Polished rose gold under a single lamp: black in the recess, white '
           'on the rim, and every value between is the metal.',
      look='A narrow hue band riding a long steep value ramp, closed by a '
           'near-neutral descent so the cyclic wrap has no cliff. Sits a full '
           '290 degrees off P45 Hammered Copper — the two warm metals are '
           'nowhere near each other.',
      pairing='Specular, sheen and bevel patterns. The shine here is lightness '
              'rather than chroma, so it survives being composited under other '
              'layers.'),

 dict(id='P86', slug='verdigris_steel', name='Verdigris Steel', cls='metallic',
      build=b_metal,
      grid=G(H=[182, 190, 198], sp=[14, 22, 30], lo=[0.03, 0.06],
             hi=[0.94, 0.98], cm=[0.85, 1.0], gh=[40, 60, 300]),
      families='verdigris steel 182-228 on a full value ramp; warm return leg',
      mood='Oxidised bronze on a cold day — tin, patina, and the blue-green '
           'that lives in weathered metal.',
      look='The cold twin of the warm metallics: the same steep specular value '
           'ramp on the opposite side of the wheel, with a warm near-neutral '
           'return leg instead of a cool one.',
      pairing='Bevel, edge and specular patterns; also anything that should '
              'read as machined rather than painted.'),

 # ---- stark x2 ------------------------------------------------------------
 # P46 red slash, P47 cyan slash. These take lime and violet.
 dict(id='P87', slug='hazard_tape', name='Hazard Tape', cls='stark',
      build=b_stark,
      grid=G(H=[126, 132, 140], cm=[0.9, 1.0], tint=[0.0, 0.02],
             th=[0, 120, 140]),
      families='black / white / hazard lime',
      mood='Black, white, and one lime slash across both. Signage, not '
           'painting.',
      look='Two value plateaus with almost nothing between them plus three '
           'lime anchors. The cliff IS the look — stark is the one class whose '
           'smoothness budget is deliberately relaxed, to step_ratio 5.',
      pairing='Hard-edged geometry: grids, bars, halftone and scanline work. '
              'Soft-edged patterns should not draw this one.'),

 dict(id='P88', slug='ultraviolet_press', name='Ultraviolet Press', cls='stark',
      build=b_stark,
      grid=G(H=[282, 288, 296], cm=[0.9, 1.0], tint=[0.02, 0.04],
             th=[240, 280, 300]),
      families='near-black / chalk white / ultraviolet',
      mood='A hand-pressed poster in two inks, and someone ran a violet plate '
           'over the top of it slightly out of register.',
      look='The same two-plateau structure as the other starks with a violet '
           'accent and faintly violet-tinted blacks. No midtone anywhere.',
      pairing='Grid, wire and edge-detect patterns; reads as print rather than '
              'as light.'),

 # ---- earth x2 ------------------------------------------------------------
 # P48 terracotta and P49 olive-slate hold the warm and green sides; every
 # other earth-classified palette in the corpus is warm too. These two are the
 # first cool and the first violet earths the library has ever had.
 dict(id='P89', slug='driftwood_and_slate', name='Driftwood & Slate',
      cls='earth', build=b_earth,
      grid=G(H=[196, 206, 216], sp=[70, 85, 100], lo=[0.24, 0.30, 0.36],
             hi=[0.64, 0.70, 0.76], cm=[0.8, 0.95]),
      families='weathered blue-grey -> slate -> driftwood, all low chroma',
      mood='A grey beach in winter: bleached wood, wet slate, and the blue '
           'that only shows up when the light is flat.',
      look='The cold end of the earth family — the same muted mid-lightness '
           'construction as the warm earths, moved to the blue side, which no '
           'earth palette in the library has ever occupied.',
      pairing='Terrain, sediment and crack patterns; cross-fades beautifully '
              'with the warm earth sets when the scheduler moves between '
              'them.'),

 dict(id='P90', slug='mushroom_and_mauve', name='Mushroom & Mauve',
      cls='earth', build=b_earth,
      grid=G(H=[310, 320, 330], sp=[60, 75, 90], lo=[0.26, 0.32],
             hi=[0.64, 0.70, 0.76], cm=[0.85, 1.0]),
      families='mushroom -> mauve -> dusty rose, all low chroma',
      mood='A forager\'s basket in flat light: cap-brown, gill-grey, and the '
           'mauve that bruised mushrooms go.',
      look='Muted violet-reds inside a mid lightness band. Nothing here is '
           'black and nothing is pale, so it never reads as a value ramp — and '
           'it is the only earth in the library on the cool side of red.',
      pairing='Texture, grain, erosion and sediment patterns. Stays legible '
              'under heavy layering where a saturated palette goes muddy.'),
]

assert len(SPECS) == 20
assert len({s['id'] for s in SPECS}) == 20


# ---------------------------------------------------------------- search

def evaluate(cols, cls, accepted, hue_floor=True):
    """(passes, class fit, nearest distance, message, signature)."""
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
        if hue_floor and ocls == cls:
            e = hue_emd(sig, osig)
            if e < PS.MIN_HUE_EMD_SAME_CLASS:
                return False, s, d, (f'hue overlap with {name} (same class) '
                                     f'emd={e:.2f}'), None
    return True, s, near, f'fit={s:.2f} nn={nn} d={near:.3f}', sig


def build():
    accepted = [(n, c, signature(cols)) for n, c, cols
                in load_corpus(ROOT, skip={sp['id'] for sp in SPECS})]
    rows = []
    for sp in SPECS:
        best, why = None, 'no grid point passed'
        for q in sp['grid']:
            cols = order_ramp(list(dict.fromkeys(sp['build'](q))))
            ok, s, near, msg, sig = evaluate(cols, sp['cls'], accepted,
                                             hue_floor=not sp.get('no_hue_floor'))
            if not ok:
                if best is None:
                    why = msg
                continue
            # rank: house rule first (no rough breaks -- step_ratio inside the
            # class budget), then class fit, then distance from the library.
            # the declared class must also be the BEST-fitting one, or the
            # palette is mud wearing a label.
            # STRICT argmax, not the >= of P31..P50. A tie is not good enough:
            # load_corpus files every palette under classify()'s winner, and
            # classify() breaks ties by dict order -- so a palette that scores
            # 1.00 as both `analogous` and `mono_accent` gets filed as
            # mono_accent and is then gated on hue against the wrong class.
            # That is exactly how P76 collided with P72 on the first pass.
            _, allsc = fast_scores(cols)
            own = allsc[sp['cls']][0]
            other = max(v[0] for c, v in allsc.items() if c != sp['cls'])
            argmax = own > other + 1e-9
            det_sr = PS.score(cols, sp['cls'])[2]['step_ratio']
            smooth = det_sr[1]                       # banded sub-score, 0..1
            # ...and then the RAW step_ratio. The sub-score saturates at 1.0
            # for anything inside the class budget, so on its own it lets the
            # search pick a ramp at 2.60 over an equally-legal one at 1.4 --
            # both score 1.0. J's "no rough breaks" is about the raw number,
            # not about clearing a threshold, so break ties toward the
            # genuinely smoother ramp. `stark` is exempt: there the cliff IS
            # the look and minimising it would destroy the class.
            # It is BUCKETED at 0.5, though. Ranking on the bare number makes
            # smoothness lexicographically dominant and the search then trades
            # away diversity for hundredths of step_ratio: unbucketed, five of
            # the twenty landed within 0.003 of the 0.16 distance floor and one
            # could not be placed at all. Half a unit of step_ratio is roughly
            # where the difference becomes visible; inside a bucket, distance
            # from the rest of the library decides.
            raw = (0.0 if sp['cls'] == 'stark'
                   else -float(np.ceil(det_sr[0] / 0.5)))
            rank = (argmax, round(smooth, 2), raw, round(s, 2), near)
            if best is None or rank > best[0]:
                best = (rank, cols, near, s, msg, sig, q)
        if best is None:
            rows.append((sp, None, False, why, None))
            continue
        rank, cols, near, s, msg, sig, q = best
        if not rank[0]:
            # no grid point made the declared class the strict winner; ship
            # nothing rather than a mislabelled palette, and say which class
            # actually won so the grid can be widened toward it
            _, allsc = fast_scores(cols)
            won = max(allsc, key=lambda c: allsc[c][0])
            rows.append((sp, None, False,
                         f'not strict argmax — reads as {won} '
                         f'({allsc[won][0]:.2f}) over {sp["cls"]} '
                         f'({allsc[sp["cls"]][0]:.2f})', q))
            continue
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
    print(f'P71-P90 against the full {len(accepted)}-palette library: '
          f'min NN {min(nn):.3f}   median NN {np.median(nn):.3f}   '
          f'(floors: 0.16 / 0.20)')
    if '--write' in sys.argv:
        write_all(rows)
        print(f'wrote 20 palette directories under {OUT}')
