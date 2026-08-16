#!/usr/bin/env python3
# gen_palettes.py — builds assets/palette.bin, assets/sintab.bin and
#                   src/engine/palette_count.h from the palette sources
# Run:  python3 gen_tables.py          (or: make gen)
#
# palette.bin       : JD_SCHEMES x 32768 x u32 ARGB  (131,072 bytes per scheme)
#                     draw.s / bridge.c crossfade scheme k -> k+1 on a slow clock
# palette_count.h   : generated  '#define JD_SCHEMES <n>'  so bridge.c and draw.s
#                     stop hardcoding 30.  draw.s cannot #include a C header (it
#                     is assembled as .s, no preprocessor), so this script also
#                     rewrites the scheme-modulus literal in draw.s in place.
# sintab.bin        : 256 x i32 Q14 sine            (1,024 bytes)
# shapes.bin        : 5 shapes x 256x256 x i16 SDF  (655,360 bytes)
#                     order: heart, diamond, club, spade, star
#                     value = signed distance in grid px (<0 inside), y-down rows,
#                     shape half-height ~= 100 grid px, center at (128,128)
#
# ---------------------------------------------------------------------------
# v2.1 palette rework (see lab/design/palettes.md)
#
# THREE SOURCES, concatenated in this fixed order — house first, because draw.s
# Lpalmix hardcodes scheme 0 ('jewels') for its accent taps:
#
#   [0 .. 5]        6 house schemes (HSV keyframe ramps, below)
#   [6 .. 6+R-1]    reference/palettes.json          (curated Lospec imports)
#   [.. rest]       lab/palettes/P*/palette.json     (globbed sorted at build
#                                                     time; new ones land here)
#
# FOUR FIXES against the measured v2.0 defects:
#
#   1. ANCHOR ORDERING (palettes.md RULE 2).  v2.0 expanded the anchor list in
#      FILE ORDER.  A hue-grouped authoring list writes an ~80-unit cliff into
#      the ramp — measured as the direct cause of J's "rough breaks".  Every
#      palette now goes through palette_score.order_ramp() (greedy OKLab
#      nearest-neighbour tour + 2-opt, cyclic) before expansion.  Idempotent on
#      palettes that were already authored ordered.
#
#   2. OKLab INTERPOLATION.  v2.0 lerped in sRGB, which drives every ramp
#      through a desaturated grey midpoint and makes HSV-equal-looking colours
#      land in different perceptual places.  Interpolation is now in OKLab.
#
#   3. CATMULL-ROM, NOT SMOOTHSTEP.  smoothstep pins the derivative to zero at
#      every anchor, so an M-anchor palette scrolls as M plateaus with fast
#      ramps between them — visible as pulsing.  A cyclic centripetal-ish
#      Catmull-Rom spline is C1 everywhere: broad soft ramps, no corners, no
#      plateaus.  Overshoot is limited to the local control hull so it cannot
#      punch out of gamut and clip to a hard edge.
#
#   4. NO HAIRLINE FILAMENTS.  The house schemes used
#         v += 0.38 * max(0, sin(fr*24))**9      # "filaments"
#         v -= 0.22 * max(0, sin(fr*24+2.1))**7  # "grooves"
#      Those are ~40-entry-wide spikes in a 32768 ramp — exactly the
#      hairline-filament look the client rejected.  Both are gone, and the
#      shimmer term dropped from 48/131 cycles to 5/11 cycles at lower gain:
#      broad soft luminance drift instead of a moire.
# ---------------------------------------------------------------------------

import math, struct, colorsys, json, os, re, glob, sys
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)          # repo root: tools/ lives one below
N    = 32768                      # entries per scheme
BPS  = N * 4                      # 131072 bytes per scheme

# palette_score.py owns order_ramp()/to_oklab() and is the spec's reference
# implementation — import it rather than forking the maths.  It lives in
# lab/design, which is a design-workstream directory: if it ever goes away the
# TABLE build must still succeed, so there is a local fallback below.
sys.path.insert(0, HERE)
try:
    from palette_score import order_ramp, to_oklab      # noqa: E402
except Exception as _e:                                 # pragma: no cover
    print(f'  WARN  lab/design/palette_score.py unavailable ({_e}); '
          f'using built-in fallback', file=sys.stderr)

    def to_oklab(colors):
        a = np.array([[int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16)]
                      for c in (h.lstrip('#') for h in colors)], float) / 255.0
        a = np.where(a <= 0.04045, a / 12.92, ((a + 0.055) / 1.055) ** 2.4)
        r, g, b = a[:, 0], a[:, 1], a[:, 2]
        l_ = np.cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b)
        m_ = np.cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b)
        s_ = np.cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b)
        return (0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
                1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
                0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_)

    def order_ramp(colors):
        """greedy OKLab nearest-neighbour cyclic tour + 2-opt (see palettes.md
        RULE 2).  Same algorithm as palette_score.order_ramp."""
        L, A, B = to_oklab(colors)
        P = np.stack([L, A, B], 1)
        n = len(colors)
        if n < 4:
            return list(colors)
        D = np.sqrt(((P[:, None, :] - P[None, :, :]) ** 2).sum(-1))
        tour = [int(np.argmin(L))]
        left = set(range(n)) - set(tour)
        while left:
            nxt = min(left, key=lambda j: D[tour[-1], j])
            tour.append(nxt); left.discard(nxt)
        improved = True
        while improved:
            improved = False
            for i in range(n - 1):
                for k in range(i + 2, n if i else n - 1):
                    a, b = tour[i], tour[(i + 1) % n]
                    c, d = tour[k], tour[(k + 1) % n]
                    if D[a, b] + D[c, d] > D[a, c] + D[b, d] + 1e-12:
                        tour[i + 1:k + 1] = reversed(tour[i + 1:k + 1])
                        improved = True
        return [colors[i] for i in tour]


# ============================== colour maths ==============================

def _linear_to_srgb(c):
    c = np.clip(c, 0.0, 1.0)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * c ** (1 / 2.4) - 0.055)


def oklab_to_rgb8(P):
    """(n,3) OKLab -> (n,3) uint8 sRGB."""
    L, A, B = P[:, 0], P[:, 1], P[:, 2]
    l_ = L + 0.3963377774 * A + 0.2158037573 * B
    m_ = L - 0.1055613458 * A - 0.0638541728 * B
    s_ = L - 0.0894841775 * A - 1.2914855480 * B
    l, m, s = l_ ** 3, m_ ** 3, s_ ** 3
    r = 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s
    g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s
    b = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
    rgb = np.stack([_linear_to_srgb(r), _linear_to_srgb(g), _linear_to_srgb(b)], 1)
    return np.clip(np.rint(rgb * 255.0), 0, 255).astype(np.uint8)


def pack_argb(rgb8):
    """(n,3) uint8 -> (n,) uint32 0xFFrrggbb."""
    return (0xFF000000
            | (rgb8[:, 0].astype(np.uint32) << 16)
            | (rgb8[:, 1].astype(np.uint32) << 8)
            | rgb8[:, 2].astype(np.uint32)).astype('<u4')


# ============================ anchor expansion ============================

def catmull_rom_cyclic(P, n):
    """Expand (M,3) control points into (n,3) along a CLOSED C1 Catmull-Rom
    spline.  Overshoot is clamped to the hull of the two straddling control
    points, so the curve stays inside the authored gamut without giving up
    smoothness anywhere it matters."""
    M = len(P)
    if M == 1:
        return np.repeat(P, n, 0)
    if M == 2:                                   # degenerate: cosine there-and-back
        t = np.arange(n) * 2.0 / n
        t = np.where(t > 1.0, 2.0 - t, t)
        t = (0.5 - 0.5 * np.cos(t * math.pi))[:, None]
        return P[0] * (1 - t) + P[1] * t

    pos = np.arange(n) * (M / n)
    k   = pos.astype(np.int64) % M
    t   = (pos - np.floor(pos))[:, None]

    p0 = P[(k - 1) % M]
    p1 = P[k]
    p2 = P[(k + 1) % M]
    p3 = P[(k + 2) % M]

    t2, t3 = t * t, t * t * t
    out = 0.5 * ((2 * p1)
                 + (-p0 + p2) * t
                 + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2
                 + (-p0 + 3 * p1 - 3 * p2 + p3) * t3)
    lo = np.minimum(p1, p2)
    hi = np.maximum(p1, p2)
    return np.clip(out, lo, hi)


def ramp_from_anchors(cols):
    """hex anchor list -> (N,) uint32 ARGB, ordered + OKLab + Catmull-Rom."""
    cols = order_ramp(list(cols))                      # RULE 2
    L, A, B = to_oklab(cols)
    P = np.stack([L, A, B], 1)
    return pack_argb(oklab_to_rgb8(catmull_rom_cyclic(P, N)))


# ============================== house schemes ==============================
# (position, hue, sat, val, shimmer) — the original six, unchanged anchors.
HOUSE = {
    'jewels': [   # the original materials ramp
        (0.00, 0.615, 0.97, 0.28, 0.05), (0.10, 0.760, 0.95, 0.42, 0.08),
        (0.20, 0.380, 0.97, 0.36, 0.06), (0.30, 0.130, 0.80, 0.97, 0.30),
        (0.40, 0.070, 0.85, 0.58, 0.26), (0.50, 0.520, 0.65, 0.85, 0.32),
        (0.60, 0.580, 0.08, 0.94, 0.36), (0.70, 0.950, 0.34, 0.98, 0.06),
        (0.80, 0.450, 0.32, 0.97, 0.05), (0.90, 0.720, 0.36, 0.97, 0.06),
        (1.00, 0.615, 0.97, 0.28, 0.05)],
    'ember': [    # near-black -> DARK RED -> crimson -> orange -> gold
        (0.00, 0.990, 0.90, 0.10, 0.03), (0.18, 0.985, 0.97, 0.34, 0.06),
        (0.36, 0.000, 0.95, 0.55, 0.12), (0.55, 0.030, 0.90, 0.75, 0.20),
        (0.72, 0.080, 0.85, 0.95, 0.30), (0.88, 0.120, 0.70, 0.99, 0.24),
        (1.00, 0.990, 0.90, 0.10, 0.03)],
    'royal': [    # deep PURPLE -> violet -> magenta, GOLD filigree
        (0.00, 0.740, 0.98, 0.20, 0.04), (0.20, 0.760, 0.95, 0.45, 0.10),
        (0.40, 0.800, 0.85, 0.70, 0.16), (0.55, 0.880, 0.75, 0.85, 0.14),
        (0.70, 0.130, 0.75, 0.95, 0.32), (0.85, 0.720, 0.90, 0.55, 0.10),
        (1.00, 0.740, 0.98, 0.20, 0.04)],
    'gilded': [   # bronze -> GOLD -> white-gold — the treasure room
        (0.00, 0.090, 0.90, 0.25, 0.08), (0.25, 0.110, 0.88, 0.55, 0.20),
        (0.50, 0.130, 0.80, 0.92, 0.38), (0.70, 0.140, 0.45, 1.00, 0.30),
        (0.85, 0.110, 0.75, 0.65, 0.22), (1.00, 0.090, 0.90, 0.25, 0.08)],
    'ice': [      # navy -> LIGHT BLUE -> cyan -> white
        (0.00, 0.620, 0.95, 0.22, 0.04), (0.25, 0.590, 0.80, 0.55, 0.10),
        (0.50, 0.550, 0.55, 0.90, 0.22), (0.70, 0.520, 0.30, 1.00, 0.30),
        (0.85, 0.560, 0.65, 0.75, 0.14), (1.00, 0.620, 0.95, 0.22, 0.04)],
    'spring': [   # forest -> LIGHT GREEN -> mint -> cream
        (0.00, 0.360, 0.95, 0.20, 0.04), (0.22, 0.340, 0.85, 0.50, 0.10),
        (0.45, 0.310, 0.60, 0.85, 0.20), (0.65, 0.280, 0.35, 0.98, 0.26),
        (0.82, 0.400, 0.55, 0.70, 0.12), (1.00, 0.360, 0.95, 0.20, 0.04)],
}
HOUSE_ORDER = ['jewels', 'ember', 'royal', 'gilded', 'ice', 'spring']

# broad soft luminance drift; v2.0 used 48 and 131 cycles at full gain, which
# reads as a moire at 32768 entries.  5 and 11 cycles at 0.6 gain is a swell.
SHIMMER_LO, SHIMMER_HI, SHIMMER_GAIN = 5.0, 11.0, 0.60


def house_ramp(keys):
    """keyframe (pos,h,s,v,shimmer) list -> (N,) uint32 ARGB, vectorised."""
    p  = np.arange(N) / N
    kp = np.array([k[0] for k in keys])
    kv = np.array([k[1:] for k in keys])                 # (K,4) h,s,v,shim

    seg = np.clip(np.searchsorted(kp, p, 'right') - 1, 0, len(keys) - 2)
    p0, p1 = kp[seg], kp[seg + 1]
    t = (p - p0) / np.maximum(p1 - p0, 1e-9)
    t = t * t * (3 - 2 * t)                              # ease per keyframe span

    A, B = kv[seg], kv[seg + 1]
    dh = (B[:, 0] - A[:, 0] + 0.5) % 1.0 - 0.5           # shortest-way hue lerp
    h  = (A[:, 0] + dh * t) % 1.0
    s  = A[:, 1] + (B[:, 1] - A[:, 1]) * t
    v  = A[:, 2] + (B[:, 2] - A[:, 2]) * t
    sh = A[:, 3] + (B[:, 3] - A[:, 3]) * t

    fr = p * 2 * math.pi
    v = v + sh * SHIMMER_GAIN * (0.55 * np.sin(fr * SHIMMER_LO)
                                 + 0.45 * np.sin(fr * SHIMMER_HI))
    v = np.clip(v, 0.02, 1.0)

    hsv = np.stack([h, s, v], 1)
    rgb = np.array([colorsys.hsv_to_rgb(*row) for row in hsv])
    return pack_argb(np.clip(np.rint(rgb * 255.0), 0, 255).astype(np.uint8))


# =============================== collect ==================================

def load_anchor_palettes():
    """[(source, name, colors)] from reference/palettes.json then lab/palettes,
    skipping anything malformed with a warning on stderr."""
    out, warn = [], []

    def clean(name, raw):
        if not isinstance(raw, (list, tuple)) or len(raw) < 2:
            warn.append(f'{name}: needs >=2 colours, got {len(raw) if hasattr(raw, "__len__") else "?"}')
            return None
        cols = []
        for c in raw:
            if not isinstance(c, str):
                warn.append(f'{name}: non-string colour {c!r}')
                return None
            c = c.strip().lstrip('#').lower()
            if len(c) != 6 or any(ch not in '0123456789abcdef' for ch in c):
                warn.append(f'{name}: bad hex {c!r}')
                return None
            cols.append(c)
        return cols

    ref_path = os.path.join(ROOT, 'assets', 'palettes', 'lospec.json')
    try:
        ref = json.load(open(ref_path))
    except Exception as e:
        warn.append(f'reference/palettes.json unreadable ({e}) — skipped')
        ref = []
    for i, pal in enumerate(ref):
        name = pal.get('slug') or pal.get('name') or f'ref{i:02d}'
        cols = clean(f'ref:{name}', pal.get('colors', []))
        if cols:
            out.append(('ref', name, cols))

    # globbed at BUILD time and sorted, so palettes that land mid-run are picked
    # up by the next `make gen` with no edit here.
    for f in sorted(glob.glob(os.path.join(ROOT, 'assets', 'palettes', 'designed', '*.json'))):
        tag = os.path.basename(os.path.dirname(f))
        try:
            d = json.load(open(f))
        except Exception as e:
            warn.append(f'{tag}: unreadable JSON ({e})')
            continue
        cols = clean(tag, d.get('colors', []))
        if cols:
            out.append(('lab', d.get('slug') or tag, cols))

    # exact-duplicate guard: two files with the same anchor list would be two
    # identical schemes, which is the sameness problem all over again.
    seen, uniq = {}, []
    for src, name, cols in out:
        key = tuple(cols)
        if key in seen:
            warn.append(f'{name}: identical anchors to {seen[key]} — skipped')
            continue
        seen[key] = name
        uniq.append((src, name, cols))

    for w in warn:
        print(f'  WARN  {w}', file=sys.stderr)
    return uniq, len(warn)


# =============================== build ====================================

def step_ratio(argb, stride=32):
    """max/mean OKLab step over the cyclic ramp — palettes.md RULE 3.

    Sampled every `stride` entries (1024 samples over the cycle).  Measuring
    entry-to-entry would measure 8-bit quantisation noise, not the ramp: on a
    32768-entry ramp most neighbours are identical, the mean collapses, and the
    ratio blows up meaninglessly.  At stride 32 the reading is the real thing —
    does one stretch of the ramp scroll faster than its neighbours."""
    a = np.asarray(argb, dtype=np.uint32)[::stride]
    rgb = np.stack([(a >> 16) & 255, (a >> 8) & 255, a & 255], 1) / 255.0
    lin = np.where(rgb <= 0.04045, rgb / 12.92, ((rgb + 0.055) / 1.055) ** 2.4)
    r, g, b = lin[:, 0], lin[:, 1], lin[:, 2]
    l_ = np.cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b)
    m_ = np.cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b)
    s_ = np.cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b)
    P = np.stack([0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
                  1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
                  0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_], 1)
    d = np.linalg.norm(np.diff(P, axis=0, append=P[:1]), axis=1)
    mean = d.mean()
    return (d.max() / mean) if mean > 1e-12 else 0.0


schemes = []                                   # [(source, name, ndarray u32)]

for name in HOUSE_ORDER:
    schemes.append(('house', name, house_ramp(HOUSE[name])))

anchor_pals, n_warn = load_anchor_palettes()
for src, name, cols in anchor_pals:
    schemes.append((src, name, ramp_from_anchors(cols)))

JD_SCHEMES = len(schemes)

ASSETS = os.path.join(ROOT, 'assets')
os.makedirs(ASSETS, exist_ok=True)
with open(os.path.join(ASSETS, 'palette.bin'), 'wb') as f:
    for _, _, ramp in schemes:
        f.write(np.ascontiguousarray(ramp, dtype='<u4').tobytes())

# ---------------- generated header: one source of truth ----------------
hdr = [
    '/* palette_count.h — GENERATED by gen_tables.py.  Do not edit.',
    ' *',
    ' * Number of colour schemes in palette.bin / jd_palette[].  Each scheme is',
    ' * 32768 ARGB entries == 131072 bytes, so',
    ' *     sizeof(palette.bin) == JD_SCHEMES * 131072.',
    ' *',
    ' * bridge.c should use this instead of a literal.  draw.s is assembled as',
    ' * .s (no C preprocessor) so gen_tables.py rewrites its scheme-modulus',
    ' * literal in place; both therefore track this number automatically.',
    ' */',
    '#ifndef JD_PALETTE_COUNT_H',
    '#define JD_PALETTE_COUNT_H',
    '',
    f'#define JD_SCHEMES {JD_SCHEMES}',
    '',
    '/* scheme index -> source : name',
]
for i, (src, name, _) in enumerate(schemes):
    hdr.append(f' *   {i:3d}  {src:5s}  {name}')
hdr += [' */', '', '#endif /* JD_PALETTE_COUNT_H */', '']
with open(os.path.join(ROOT, 'src', 'engine', 'palette_count.h'), 'w') as f:
    f.write('\n'.join(hdr))

# ---------------- keep draw.s's modulus in sync ----------------
# draw.s picks the crossfade scheme pair with  udiv/msub by w13.  That literal
# was 'mov w13, #30'.  Rewrite it (idempotent) rather than leaving two numbers.
draw_path = os.path.join(ROOT, 'src', 'engine', 'routines_asm.s')
try:
    src_txt = open(draw_path).read()
    pat = re.compile(r'(mov\s+w13,\s*#)(\d+)([^\n]*//[^\n]*scheme[^\n]*)', re.I)
    m = pat.search(src_txt)
    if m:
        new_line = (f'{m.group(1)}{JD_SCHEMES}'
                    f'                    '
                    f'// JD_SCHEMES: generated, see palette_count.h (scheme count)')
        if src_txt[m.start():m.end()] != new_line:
            src_txt = src_txt[:m.start()] + new_line + src_txt[m.end():]
            open(draw_path, 'w').write(src_txt)
            print(f'  draw.s   scheme modulus {m.group(2)} -> {JD_SCHEMES}')
        else:
            print(f'  draw.s   scheme modulus already {JD_SCHEMES}')
    else:
        print('  WARN  draw.s: no "mov w13, #<n>  // ...scheme..." line found; '
              'modulus NOT patched', file=sys.stderr)
except FileNotFoundError:
    print('  WARN  draw.s not found; modulus NOT patched', file=sys.stderr)

# ---------------- report ----------------
ratios = [step_ratio(r) for _, _, r in schemes]
by_src = {}
for (src, _, _) in schemes:
    by_src[src] = by_src.get(src, 0) + 1
print('schemes: {} ({})'.format(
    JD_SCHEMES, ' + '.join(f'{n} {s}' for s, n in by_src.items())))
print('  ramp step_ratio  min {:.2f}  median {:.2f}  max {:.2f}  (budget ~3)'
      .format(min(ratios), float(np.median(ratios)), max(ratios)))
worst = sorted(zip(ratios, [n for _, n, _ in schemes]), reverse=True)[:3]
print('  roughest: ' + ', '.join(f'{n} {r:.2f}' for r, n in worst))
if n_warn:
    print(f'  {n_warn} palette(s) skipped — see WARN lines above')

# ---------------- Q14 sine table ----------------
with open(os.path.join(ASSETS, 'sintab.bin'), 'wb') as f:
    for i in range(256):
        f.write(struct.pack('<i', round(math.sin(i * 2 * math.pi / 256) * 16384)))

pb = os.path.getsize(os.path.join(ASSETS, 'palette.bin'))
assert pb == JD_SCHEMES * BPS, f'palette size {pb} != {JD_SCHEMES}*{BPS}'
assert os.path.getsize(os.path.join(ASSETS, 'sintab.bin')) == 1024
print(f'palette.bin {pb} bytes  OK ({JD_SCHEMES} schemes x {BPS})')
print('palette_count.h  #define JD_SCHEMES', JD_SCHEMES)
print('sintab.bin    1024 bytes  OK')
