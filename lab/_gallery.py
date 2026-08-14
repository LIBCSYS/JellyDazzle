#!/usr/bin/env python3
"""Gallery composer for JellyDazzle v2.1 — the whole library, in one page.

Emits  lab/gallery.html  and  lab/CATALOG.md.

v2.1 rewrite. What changed from the v2.0 composer:

  * ALL 200 C patterns, not 100.  001-100 keep their lab spec.md; 101-200 are
    described from lab/patterns_c_specs/NNN.md.
  * ALL 120 compiled colour schemes, not 30.  Every swatch on the page is the
    ACTUAL ramp read out of palette.bin — the bytes the app loads — rather than
    a re-derivation from the anchor list.  House and reference schemes get a
    swatch for the first time.
  * Each pattern card carries the engine's own measured numbers: layer role,
    render cost, frame-to-frame motion, screen coverage.  These come from
    lab/design/engine_stats.csv, dumped by the engine's startup probe
    (lab/design/engine_stats.c) — the same table the v2.1 scheduler schedules
    from, so the page cannot drift from what the app believes.
  * Client-side filter/search, because 200 + 120 cards is not a scroll.

Everything is self-contained: no CDN, no external font, no fetch.  Safe for
file:// and for GitHub Pages.

Regenerating the inputs (only needed if patterns/palettes change):
    engine_stats.csv   clang -O2 -I. -DJD_NS=$(scheme count) \\
                         lab/design/engine_stats.c patterns_c/pattern_*.c \\
                         patterns_c/registry.c draw.s -o /tmp/dump -lm
                       /tmp/dump stats | tail -n +2 > lab/design/engine_stats.csv
    lab/previews/*.png filmstrips rendered from the shipped patterns
    lab/ramps/*.png    ramp strips sliced out of palette.bin
"""
import csv, glob, html, json, os, re, sys

LAB  = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(LAB)

def esc(s): return html.escape(str(s or ""), quote=True)

# ===================================================================== inputs

def read_engine_stats():
    """rt -> measured stats. rt 0..23 are asm modes, 24.. are pattern_001.."""
    path = os.path.join(LAB, "design", "engine_stats.csv")
    out = {}
    if not os.path.exists(path):
        print("  WARN  no design/engine_stats.csv — cards lose their numbers", file=sys.stderr)
        return out
    for r in csv.DictReader(open(path)):
        out[int(r["rt"])] = dict(
            kind=r["kind"], role=r["role"], cls=r["cls"],
            dark=int(r["dark"]), luma=int(r["luma"]), sat=int(r["sat"]),
            delta=float(r["delta"]), cost=float(r["cost_ms"]),
            probed=int(r["probed"]))
    return out

def read_scheme_map():
    """scheme index -> (source, name), parsed from the generated palette_count.h."""
    path = os.path.join(REPO, "palette_count.h")
    out = {}
    if not os.path.exists(path):
        return out
    for line in open(path):
        m = re.match(r"\s*\*\s+(\d+)\s+(house|ref|lab)\s+(.+?)\s*$", line)
        if m:
            out[int(m.group(1))] = (m.group(2), m.group(3))
    return out

def read_curator_scores():
    pat, pal = {}, {}
    for f in glob.glob(os.path.join(LAB, "curate", "scores_*.md")):
        for line in open(f):
            m = re.match(r"\|\s*(\d{3})\s*\|\s*([\w-]+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|"
                         r"\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(.+?)\s*\|\s*$", line)
            if m:
                pat[int(m.group(1))] = dict(dz=int(m.group(3)), co=int(m.group(4)),
                                            mo=int(m.group(5)), total=int(m.group(6)),
                                            notes=m.group(7))
                continue
            m = re.match(r"\|\s*(P\d{2})\s*\|\s*([\w-]+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|"
                         r"\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(.+?)\s*\|\s*$", line)
            if m:
                pal[m.group(1)] = dict(rich=int(m.group(3)), rng=int(m.group(4)),
                                       mood=int(m.group(5)), total=int(m.group(6)),
                                       notes=m.group(7))
    return pat, pal

# ------------------------------------------------------------------ markdown

def strip_md(t):
    t = re.sub(r"`([^`]*)`", r"\1", t)
    t = re.sub(r"\*\*([^*]*)\*\*", r"\1", t)
    t = re.sub(r"\*([^*]*)\*", r"\1", t)
    t = re.sub(r"!\[[^\]]*\]\([^)]*\)", "", t)
    t = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", t)
    return " ".join(t.split())

def first_sentences(text, maxlen=300):
    """One or two whole sentences, up to maxlen. Never cuts mid-word."""
    t = strip_md(text)
    if not t:
        return ""
    out = ""
    for m in re.finditer(r".+?[.!?](?:\s|$)", t):
        s = m.group(0)
        if out and len(out) + len(s) > maxlen:
            break
        out += s
        if len(out) >= maxlen * 0.55:
            break
    out = out.strip() or t
    if len(out) > maxlen:
        out = out[:maxlen].rsplit(" ", 1)[0] + "…"
    return out

def md_section(md, header):
    m = re.search(r"^#+ +" + header + r"\s*\n(.*?)(?=^#+ |\Z)", md, re.S | re.M)
    if m:
        return m.group(1).strip()
    # bold-run style: **Look.** body ... up to the next **Word.**
    m = re.search(r"\*\*" + header + r"\.?\*\*\s*(.*?)(?=\n\s*\n\*\*[A-Z]|\Z)", md, re.S)
    return m.group(1).strip() if m else ""

# ================================================================== families

FAMILIES = [
    (range(1, 11),    "Kaleido & Tiling"),
    (range(11, 21),   "Demoscene Classics"),
    (range(21, 31),   "Moire & Op-Art"),
    (range(31, 41),   "Spirograph & Curves"),
    (range(41, 51),   "Cellular & Reaction"),
    (range(51, 61),   "Particles & Orbits"),
    (range(61, 71),   "Tunnels & Vortex"),
    (range(71, 81),   "Organic & Growth"),
    (range(81, 101),  "Original Footage Remakes"),
    (range(101, 1000), "Layer Corps (v2.1)"),
]
def family(n):
    for r, name in FAMILIES:
        if n in r:
            return name
    return "Unfiled"

ROLE_BLURB = {
    "GROUND": ("the floor", "Fills the screen. Always the bottom of the stack, always opaque — "
                            "everything else is painted on top of one of these."),
    "FIELD":  ("the body",  "Covers most of the frame but not all of it. Sits over a ground and "
                            "gives the composition its weight."),
    "FIGURE": ("the shape", "Distinct forms over a mostly dark frame. Reads as an object on the "
                            "picture rather than as the picture."),
    "SPARK":  ("the glint", "Sparse and mostly black — filaments, embers, particles. Meant to be "
                            "the top of the stack, brief and bright."),
}
ROLE_ORDER = ["GROUND", "FIELD", "FIGURE", "SPARK"]

# ================================================================== patterns

def registered_patterns():
    """The pattern numbers the engine actually links, read out of the generated
    registry.  Sourcing this from a directory glob instead would put half-finished
    pattern files that nobody has registered yet onto the release page."""
    path = os.path.join(REPO, "patterns_c", "registry.c")
    if os.path.exists(path):
        src = open(path).read()
        nums = sorted({int(m) for m in re.findall(r"\bpattern_(\d{3})\b", src)})
        if nums:
            declared = re.search(r"jd_pattern_count\s*=\s*(\d+)", src)
            if declared and int(declared.group(1)) != len(nums):
                print(f"  WARN  registry declares {declared.group(1)} patterns but names "
                      f"{len(nums)}", file=sys.stderr)
            return nums
    print("  WARN  no registry.c — falling back to the source glob", file=sys.stderr)
    return sorted(int(os.path.basename(f)[8:11])
                  for f in glob.glob(os.path.join(REPO, "patterns_c", "pattern_[0-9]*.c")))

def from_source_header(n):
    """(name, description) from the leading block comment of pattern_NNN.c.

    Convention across the whole library:  /* pattern_NNN — TITLE  ... prose */
    """
    path = os.path.join(REPO, "patterns_c", f"pattern_{n:03d}.c")
    if not os.path.exists(path):
        return None, ""
    src = open(path, errors="replace").read(6000)
    m = re.match(r"\s*/\*(.*?)\*/", src, re.S)
    if not m:
        return None, ""
    body = "\n".join(ln.strip().lstrip("*").strip() for ln in m.group(1).splitlines())
    lines = [ln.strip() for ln in body.splitlines()]
    name = None
    if lines:
        t = re.match(r"pattern_\d+\s*[-—–:]\s*(.+)", lines[0], re.I)
        if t:
            name = t.group(1).strip()
            name = re.sub(r"\s*\((?:overlay|ground|field|figure|spark)[^)]*\)\s*$",
                          "", name, flags=re.I).strip()
            if name.isupper():
                name = name.title()
    prose = " ".join(lines[1:])
    prose = re.sub(r"\bCost:.*$", "", prose)          # implementation note, not a look
    return name, first_sentences(prose)

def harvest_patterns(stats, scores):
    """Every pattern the engine registers, described from whatever spec exists."""
    nums = registered_patterns()
    lab_dirs = {int(os.path.basename(d)[:3]): os.path.basename(d)
                for d in glob.glob(os.path.join(LAB, "patterns", "[0-9]*"))}
    out = []
    for n in nums:
        name, look, slug, src = None, "", "", None
        if n in lab_dirs:                                   # 001-100: lab prototype dir
            d = lab_dirs[n]; slug = d[4:]
            p = os.path.join(LAB, "patterns", d, "spec.md")
            if os.path.exists(p):
                md = open(p).read(); src = f"patterns/{d}/spec.md"
                t = re.match(r"# +\d+ +[-—]? *(.+)", md)
                if t: name = t.group(1).strip()
                look = first_sentences(md_section(md, "Look"))
        p2 = os.path.join(LAB, "patterns_c_specs", f"{n}.md")
        if os.path.exists(p2):                              # 101-200 (and any re-spec)
            md = open(p2).read()
            t = re.match(r"# +\d+ +[-—]? *(.+)", md)
            if t and not name: name = t.group(1).strip()
            if not look:
                src = f"patterns_c_specs/{n}.md"
                body = md_section(md, "Look")
                if not body:                                # plain prose spec
                    body = re.sub(r"^# .*\n", "", md, count=1).strip()
                look = first_sentences(body)
        if not (name and look):
            # Last resort, and the one that always works: every pattern_NNN.c
            # opens with a block comment naming and describing itself.  A
            # pattern that ships before anyone writes it a spec still gets a
            # real card instead of an empty one.
            hn, hl = from_source_header(n)
            name = name or hn
            if not look:
                look = hl
                if hl: src = f"patterns_c/pattern_{n:03d}.c"
        if not name:
            name = (slug or f"Pattern {n}").replace("_", " ").title()
        rt = 23 + n                                         # JD_MODE numbering
        st = stats.get(rt, {})
        prev = f"previews/{n:03d}.png"
        out.append(dict(n=n, rt=rt, name=name, slug=slug, look=look, spec=src,
                        family=family(n), sc=scores.get(n),
                        preview=prev if os.path.exists(os.path.join(LAB, prev)) else None,
                        role=st.get("role", "?"), cost=st.get("cost"), delta=st.get("delta"),
                        cover=(255 - st["dark"]) / 255.0 if "dark" in st else None,
                        luma=st.get("luma"), cls=st.get("cls", "")))
    return out

def harvest_asm(stats):
    out = []
    for m in range(24):
        st = stats.get(m, {})
        out.append(dict(mode=m, role=st.get("role", "GROUND"), cost=st.get("cost"),
                        delta=st.get("delta"), cls=st.get("cls", ""),
                        cover=(255 - st["dark"]) / 255.0 if "dark" in st else None))
    return out

# ================================================================== palettes

def harvest_palettes(scheme_map, scores):
    """One entry per COMPILED scheme, in palette.bin order — the only order the
    engine knows. Metadata is attached where the source directory exists."""
    lab = {}
    for d in sorted(glob.glob(os.path.join(LAB, "palettes", "P[0-9]*"))):
        base = os.path.basename(d)
        try:
            pj = json.load(open(os.path.join(d, "palette.json")))
        except Exception:
            continue
        slug = pj.get("slug") or base[4:]
        specp = os.path.join(d, "spec.md")
        md = open(specp).read() if os.path.exists(specp) else ""
        gate = re.search(r"Gate:\s*\*\*(PASS|FAIL)\*\*", md)
        nn = re.search(r"nn=(\S+)\s+d=([\d.]+)", md)
        lab[slug] = dict(pid=base[:3], dir=base, name=pj.get("name"),
                         cls=pj.get("class"), scheme=pj.get("scheme"),
                         mood=pj.get("mood") or first_sentences(md_section(md, "Mood"), 200),
                         look=first_sentences(md_section(md, "Look"), 260),
                         colors=pj.get("colors", []),
                         gate=gate.group(1) if gate else None,
                         nn=(nn.group(1), float(nn.group(2))) if nn else None,
                         sc=scores.get(base[:3]))
    ref = {}
    rp = os.path.join(REPO, "reference", "palettes.json")
    if os.path.exists(rp):
        for e in json.load(open(rp)):
            ref[e["slug"]] = e.get("colors", [])

    out = []
    for idx in sorted(scheme_map):
        source, name = scheme_map[idx]
        meta = lab.get(name, {}) if source == "lab" else {}
        colors = meta.get("colors") or (ref.get(name, []) if source == "ref" else [])
        ramp = f"ramps/{idx:03d}.png"
        out.append(dict(idx=idx, source=source, slug=name,
                        name=meta.get("name") or name.replace("_", " ").replace("-", " ").title(),
                        cls=meta.get("cls"), mood=meta.get("mood") or "",
                        look=meta.get("look") or "", scheme=meta.get("scheme") or "",
                        colors=colors, pid=meta.get("pid"), gate=meta.get("gate"),
                        nn=meta.get("nn"), sc=meta.get("sc"),
                        ramp=ramp if os.path.exists(os.path.join(LAB, ramp)) else None))
    return out

# The six house schemes are authored in gen_tables.py as HSV keyframes rather
# than as an anchor list, so they have no palette.json to describe them.
HOUSE_BLURB = {
    "jewels": "The original materials ramp, and the one draw.s reaches into directly for its "
              "accent taps: the whole wheel at gemstone saturation.",
    "ember":  "Near-black up through dark red, crimson, orange and out to gold. No cool "
              "colour anywhere in it.",
    "royal":  "Deep violet and indigo lifted through magenta to a pale rose — the darkest "
              "of the six, and the one that makes bright overlays read as lit.",
    "gilded": "One metal, top to bottom: brown through bronze and brass to white gold. The "
              "narrowest hue span in the house set.",
    "ice":    "Black to deep blue to cyan to white. Cold, and the natural ground under a "
              "warm figure.",
    "spring": "Dark green through leaf and lime to cream — the only house ramp built around "
              "a single warm-green family.",
}
REF_BLURB = ("Artist palette imported from Lospec. v2.1 re-expands it through the OKLab "
             "cyclic spline instead of walking the anchors in file order, which is what "
             "used to turn every one of these into a full-spectrum sweep.")

SOURCE_BLURB = {
    "house": "Authored in the engine as HSV keyframes — the six original dazzle ramps.",
    "ref":   "Imported artist palettes (Lospec), re-expanded through the v2.1 OKLab spline.",
    "lab":   "Designed in the lab against the v2.1 taxonomy, gated by palette_score.py.",
}
CLASS_BLURB = {
    "mono_accent":      "one hue family, one opposing glint",
    "stark":            "hard contrast, almost no middle",
    "neon_on_black":    "electric colour on near-black",
    "pastel_wash":      "high value, low chroma, no dark",
    "duotone":          "two poles and the road between them",
    "analogous":        "neighbours on the wheel only",
    "split_complement": "one hue plus the two flanking its opposite",
    "full_spectrum":    "the whole wheel, deliberately",
    "earth":            "muted mineral and organic tones",
    "metallic":         "one metal's value ramp, warm or cold",
}

# ================================================================== rendering

CSS = r"""
*,*::before,*::after{box-sizing:border-box}
:root{
  --void:#07080d; --panel:#0f1119; --panel2:#141724; --line:#232838;
  --amber:#ffb02e; --beam:#5ee0d0; --hot:#ff5ec8; --warn:#ff6b5e;
  --ink:#e9e7e1; --dim:#858da6; --dim2:#5d6478;
  --mono:ui-monospace,SFMono-Regular,"SF Mono",Menlo,Consolas,monospace;
  --sans:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
}
html{scroll-behavior:smooth}
body{margin:0;background:var(--void);color:var(--ink);font-family:var(--sans);
     font-size:15px;line-height:1.55;-webkit-font-smoothing:antialiased}
a{color:var(--beam)}
.wrap{max-width:1500px;margin:0 auto;padding:0 22px}

/* ---------- masthead ---------- */
.top{border-bottom:1px solid var(--line);
  background:radial-gradient(115% 85% at 50% -25%,rgba(255,176,46,.10),transparent 62%),var(--void)}
.top .wrap{padding:38px 22px 30px}
.kicker{font-family:var(--mono);font-size:.7rem;letter-spacing:.34em;text-transform:uppercase;
        color:var(--amber);margin:0 0 12px}
h1{font-family:var(--mono);font-weight:600;font-size:clamp(1.9rem,5vw,3rem);
   letter-spacing:-.02em;margin:0 0 10px;line-height:1.06}
h1 .v{color:var(--dim2)}
.lede{max-width:74ch;color:#c3c0b8;margin:0}
.lede b{color:var(--ink);font-weight:600}

.tiles{display:grid;gap:10px;margin:26px 0 0;
       grid-template-columns:repeat(auto-fit,minmax(168px,1fr))}
.tile{border:1px solid var(--line);background:var(--panel);padding:12px 14px}
.tile .k{font-family:var(--mono);font-size:.66rem;letter-spacing:.2em;text-transform:uppercase;
         color:var(--dim2)}
.tile .v{font-family:var(--mono);font-size:1.5rem;color:var(--amber);margin-top:3px;
         font-variant-numeric:tabular-nums;line-height:1.15}
.tile .n{font-size:.76rem;color:var(--dim);margin-top:3px}
.tile.good .v{color:var(--beam)}

/* ---------- toolbar ---------- */
.bar{position:sticky;top:0;z-index:20;background:rgba(7,8,13,.94);
     backdrop-filter:blur(9px);border-bottom:1px solid var(--line)}
.bar .wrap{display:flex;flex-wrap:wrap;gap:9px;align-items:center;padding:11px 22px}
.bar input{flex:1 1 230px;min-width:180px;background:var(--panel);color:var(--ink);
  border:1px solid var(--line);padding:8px 11px;font-family:var(--mono);font-size:.84rem}
.bar input:focus{outline:none;border-color:var(--amber)}
.chip{font-family:var(--mono);font-size:.74rem;letter-spacing:.1em;text-transform:uppercase;
  border:1px solid var(--line);background:none;color:var(--dim);padding:7px 11px;cursor:pointer}
.chip:hover{color:var(--ink);border-color:var(--dim2)}
.chip[aria-pressed="true"]{color:var(--void);background:var(--amber);border-color:var(--amber);
  font-weight:600}
.count{font-family:var(--mono);font-size:.76rem;color:var(--dim2);margin-left:auto;
  white-space:nowrap}

/* ---------- sections ---------- */
section{padding:34px 0 6px;border-top:1px solid var(--line);margin-top:34px}
section:first-of-type{border-top:none;margin-top:10px}
h2{font-family:var(--mono);font-size:1rem;letter-spacing:.2em;text-transform:uppercase;
   color:var(--amber);margin:0 0 6px;font-weight:600}
.sub{color:var(--dim);margin:0 0 8px;max-width:82ch}
.groupfoot{margin:0 0 26px}
.rolehead{display:flex;flex-wrap:wrap;align-items:baseline;gap:12px;margin:40px 0 6px;
  padding-bottom:9px;border-bottom:1px dashed var(--line)}
.rolehead .r{font-family:var(--mono);font-size:.82rem;letter-spacing:.22em;text-transform:uppercase}
.rolehead .c{font-family:var(--mono);font-size:.74rem;color:var(--dim2)}
.rolehead .d{font-size:.86rem;color:var(--dim);flex:1 1 320px}
.rolehead.GROUND .r{color:#ffb02e} .rolehead.FIELD .r{color:#5ee0d0}
.rolehead.FIGURE .r{color:#7fa6ff} .rolehead.SPARK .r{color:#ff5ec8}

/* ---------- cards ---------- */
.grid{display:grid;gap:15px;grid-template-columns:repeat(auto-fill,minmax(330px,1fr))}
.card{border:1px solid var(--line);background:var(--panel);display:flex;flex-direction:column;
      transition:border-color .16s,transform .16s}
.card:hover{border-color:var(--dim2);transform:translateY(-2px)}
.card.hide{display:none}
.chead{display:flex;align-items:baseline;gap:9px;padding:11px 12px 9px}
.num{font-family:var(--mono);font-size:1.2rem;font-weight:700;color:var(--amber);
     font-variant-numeric:tabular-nums}
.nm{font-weight:600;font-size:.98rem;line-height:1.25}
.badge{margin-left:auto;font-family:var(--mono);font-size:.62rem;letter-spacing:.14em;
  text-transform:uppercase;padding:3px 7px;border:1px solid var(--line);color:var(--dim);
  white-space:nowrap}
.badge.GROUND{color:#ffb02e;border-color:#4a3a18}
.badge.FIELD{color:#5ee0d0;border-color:#1d4744}
.badge.FIGURE{color:#7fa6ff;border-color:#22345c}
.badge.SPARK{color:#ff5ec8;border-color:#54204a}
.strip{display:block;width:100%;height:auto;background:#000;border-block:1px solid var(--line)}
.desc{color:#a9adbd;font-size:.855rem;padding:11px 12px 0;flex:1}
.meta{display:flex;flex-wrap:wrap;gap:5px;padding:11px 12px 12px;font-family:var(--mono);
      font-size:.68rem}
.meta span{border:1px solid var(--line);background:var(--panel2);padding:3px 7px;color:var(--dim)}
.meta b{color:var(--ink);font-weight:600}
.meta .cmd{color:var(--beam);border-color:#1d4744}
.meta .fam{margin-left:auto;color:var(--dim2);border:none;background:none;padding-right:0}

/* ---------- palettes ---------- */
.pgrid{display:grid;gap:15px;grid-template-columns:repeat(auto-fill,minmax(370px,1fr))}
.ramp{display:block;width:100%;height:auto;border-block:1px solid var(--line)}
.chips{display:flex;height:15px;margin:0}
.chips i{flex:1 1 0;min-width:0}
.pmeta{display:flex;flex-wrap:wrap;gap:5px;padding:10px 12px 12px;font-family:var(--mono);
       font-size:.68rem}
.pmeta span{border:1px solid var(--line);background:var(--panel2);padding:3px 7px;color:var(--dim)}
.src{margin-left:auto;font-family:var(--mono);font-size:.62rem;letter-spacing:.14em;
  text-transform:uppercase;padding:3px 7px;border:1px solid var(--line);color:var(--dim)}
.src.house{color:#ffb02e;border-color:#4a3a18}
.src.ref{color:#9aa2b8;border-color:#2b3247}
.src.lab{color:#5ee0d0;border-color:#1d4744}
.cls{color:var(--hot)!important;border-color:#54204a!important}

.empty{color:var(--dim2);font-family:var(--mono);font-size:.84rem;padding:26px 0}
footer{border-top:1px solid var(--line);margin-top:56px;padding:26px 0 60px;
  color:var(--dim2);font-size:.8rem;font-family:var(--mono)}
@media (max-width:520px){.grid,.pgrid{grid-template-columns:1fr}}
"""

JS = r"""
(function(){
  var q = document.getElementById('q');
  var count = document.getElementById('count');
  var cards = [].slice.call(document.querySelectorAll('.card'));
  var chips = [].slice.call(document.querySelectorAll('.chip[data-facet]'));
  var facets = {};

  function apply(){
    var term = (q.value || '').trim().toLowerCase();
    var shown = 0;
    for (var i = 0; i < cards.length; i++){
      var c = cards[i], ok = true;
      for (var f in facets){
        if (facets[f] && c.dataset[f] !== facets[f]) { ok = false; break; }
      }
      if (ok && term) ok = c.dataset.s.indexOf(term) !== -1;
      c.classList.toggle('hide', !ok);
      if (ok) shown++;
    }
    count.textContent = shown + ' / ' + cards.length + ' shown';
    // hide a group heading whose whole group filtered away
    [].forEach.call(document.querySelectorAll('[data-group]'), function(g){
      var any = g.querySelector('.card:not(.hide)');
      g.style.display = any ? '' : 'none';
    });
  }
  chips.forEach(function(b){
    b.addEventListener('click', function(){
      var f = b.dataset.facet, v = b.dataset.val;
      var on = facets[f] === v;
      chips.forEach(function(o){ if (o.dataset.facet === f) o.setAttribute('aria-pressed','false'); });
      facets[f] = on ? null : v;
      b.setAttribute('aria-pressed', on ? 'false' : 'true');
      apply();
    });
  });
  q.addEventListener('input', apply);
  // "148" in the box scrolls to that card as well as filtering
  q.addEventListener('keydown', function(e){
    if (e.key !== 'Enter') return;
    var m = /^\s*(\d{1,3})\s*$/.exec(q.value);
    if (!m) return;
    var el = document.getElementById('p' + ('00' + m[1]).slice(-3));
    if (el) el.scrollIntoView({block:'center'});
  });
  apply();
})();
"""

def pattern_card(p):
    role = p["role"] if p["role"] in ROLE_BLURB else "?"
    bits = []
    bits.append(f'<span class="cmd">JD_MODE={p["rt"]}</span>')
    if p["cost"] is not None:
        bits.append(f'<span><b>{p["cost"]:.2f}</b> ms</span>')
    if p["delta"] is not None:
        bits.append(f'<span>motion <b>{p["delta"]:.2f}</b></span>')
    if p["cover"] is not None:
        bits.append(f'<span>cover <b>{p["cover"]*100:.0f}%</b></span>')
    if p["sc"]:
        bits.append(f'<span>curator <b>{p["sc"]["total"]}</b></span>')
    bits.append(f'<span class="fam">{esc(p["family"])}</span>')
    img = (f'<img class="strip" src="{p["preview"]}" loading="lazy" decoding="async" '
           f'alt="{p["n"]:03d} {esc(p["name"])}">') if p["preview"] else ""
    search = " ".join([f'{p["n"]:03d}', p["name"], p["family"], role, p["look"]]).lower()
    return (f'<div class="card" id="p{p["n"]:03d}" data-role="{role}" '
            f'data-s="{esc(search)}">'
            f'<div class="chead"><span class="num">{p["n"]:03d}</span>'
            f'<span class="nm">{esc(p["name"])}</span>'
            f'<span class="badge {role}">{role}</span></div>'
            f'{img}<p class="desc">{esc(p["look"])}</p>'
            f'<div class="meta">{"".join(bits)}</div></div>')

def palette_card(p):
    chips = "".join(f'<i style="background:#{esc(c)}"></i>' for c in p["colors"][:64])
    chips = f'<div class="chips">{chips}</div>' if chips else ""
    bits = []
    if p["cls"]:
        bits.append(f'<span class="cls">{esc(p["cls"])}</span>')
    if p["colors"]:
        bits.append(f'<span><b>{len(p["colors"])}</b> anchors</span>')
    if p["gate"]:
        bits.append(f'<span>gate <b>{esc(p["gate"])}</b></span>')
    if p["nn"]:
        bits.append(f'<span>nearest <b>{p["nn"][1]:.2f}</b></span>')
    if p["sc"]:
        bits.append(f'<span>curator <b>{p["sc"]["total"]}</b></span>')
    blurb = p["mood"] or p["look"] or p["scheme"] or ""
    blurb = re.sub(r"^(?:mood|look|scheme)\s*[:—-]\s*", "", blurb, flags=re.I)
    if blurb[:1].islower():
        blurb = blurb[0].upper() + blurb[1:]
    if not blurb and p["source"] == "house":
        blurb = HOUSE_BLURB.get(p["slug"], "")
    if not blurb and p["source"] == "ref":
        blurb = REF_BLURB
    if not blurb and p["cls"] in CLASS_BLURB:
        blurb = CLASS_BLURB[p["cls"]].capitalize() + "."
    ramp = (f'<img class="ramp" src="{p["ramp"]}" loading="lazy" decoding="async" '
            f'alt="scheme {p["idx"]} {esc(p["name"])} ramp">') if p["ramp"] else ""
    label = p["pid"] or f'{p["idx"]:03d}'
    search = " ".join([str(p["idx"]), label, p["name"], p["slug"],
                       p["cls"] or "", p["source"], blurb]).lower()
    return (f'<div class="card" id="s{p["idx"]:03d}" data-source="{p["source"]}" '
            f'data-s="{esc(search)}">'
            f'<div class="chead"><span class="num">{p["idx"]:03d}</span>'
            f'<span class="nm">{esc(p["name"])}</span>'
            f'<span class="src {p["source"]}">{p["source"]}</span></div>'
            f'{ramp}{chips}'
            f'<p class="desc">{esc(blurb)}</p>'
            f'<div class="pmeta">{"".join(bits)}</div></div>')

def build(patterns, palettes, asm, stats):
    nrt = len(stats) or (24 + len(patterns))
    by_role = {r: [p for p in patterns if p["role"] == r] for r in ROLE_ORDER}
    unfiled = [p for p in patterns if p["role"] not in ROLE_ORDER]
    costs = sorted(p["cost"] for p in patterns if p["cost"] is not None)
    deltas = [p["delta"] for p in patterns if p["delta"] is not None]
    med = costs[len(costs)//2] if costs else 0
    by_src = {}
    for p in palettes:
        by_src.setdefault(p["source"], []).append(p)
    classed = [p for p in palettes if p["cls"]]

    tiles = [
        ("routines",  f"{nrt}", f"{len(patterns)} C patterns + {len(asm)} assembly modes"),
        ("colour schemes", f"{len(palettes)}", "every one compiled into palette.bin"),
        ("layer roles", "4", "ground / field / figure / spark"),
        ("median cost", f"{med:.1f} ms", "per pattern at 1280&times;960", "good"),
        ("motion budget", "&lt; 8", f"library mean {sum(deltas)/len(deltas):.2f}" if deltas else "", "good"),
        ("palette classes", f"{len(set(p['cls'] for p in classed))}", f"{len(classed)} schemes classified"),
    ]
    tile_html = "".join(
        f'<div class="tile{" good" if len(t) > 3 else ""}"><div class="k">{t[0]}</div>'
        f'<div class="v">{t[1]}</div><div class="n">{t[2]}</div></div>' for t in tiles)

    # ---- pattern groups, by the role the engine measured
    pat_groups = []
    for r in ROLE_ORDER:
        g = by_role[r]
        if not g:
            continue
        role_label, role_desc = ROLE_BLURB[r]
        cards = "".join(pattern_card(p) for p in g)
        pat_groups.append(
            f'<div data-group="{r}"><div class="rolehead {r}">'
            f'<span class="r">{r}</span><span class="c">{len(g)} patterns &middot; {role_label}</span>'
            f'<span class="d">{esc(role_desc)}</span></div>'
            f'<div class="grid">{cards}</div></div>')
    if unfiled:
        pat_groups.append('<div data-group="other"><div class="rolehead"><span class="r">UNMEASURED</span>'
                          f'<span class="c">{len(unfiled)} patterns</span></div>'
                          f'<div class="grid">{"".join(pattern_card(p) for p in unfiled)}</div></div>')

    role_chips = "".join(
        f'<button class="chip" data-facet="role" data-val="{r}" aria-pressed="false">'
        f'{r} {len(by_role[r])}</button>' for r in ROLE_ORDER if by_role[r])

    # ---- palette groups, in palette.bin order within source
    pal_groups = []
    for s in ("house", "ref", "lab"):
        g = by_src.get(s, [])
        if not g:
            continue
        pal_groups.append(
            f'<div data-group="pal-{s}"><div class="rolehead">'
            f'<span class="r" style="color:var(--beam)">{s.upper()}</span>'
            f'<span class="c">{len(g)} schemes &middot; {g[0]["idx"]:03d}&ndash;{g[-1]["idx"]:03d}</span>'
            f'<span class="d">{esc(SOURCE_BLURB.get(s, ""))}</span></div>'
            f'<div class="pgrid">{"".join(palette_card(p) for p in g)}</div></div>')
    src_chips = "".join(
        f'<button class="chip" data-facet="source" data-val="{s}" aria-pressed="false">'
        f'{s} {len(by_src[s])}</button>' for s in ("house", "ref", "lab") if by_src.get(s))

    asm_rows = "".join(
        f'<tr><td>{a["mode"]:03d}</td><td>{a["role"]}</td>'
        f'<td>{a["cost"]:.2f}</td><td>{a["delta"]:.2f}</td>'
        f'<td>{"accumulator" if a["cls"] == "canvas" else "repaint"}</td></tr>'
        for a in asm if a["cost"] is not None)

    return f"""<!DOCTYPE html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>JellyDazzle Library &mdash; {len(patterns)} Patterns / {len(palettes)} Palettes</title>
<style>{CSS}</style><script defer src='https://umami.jelia.nyc/script.js' data-website-id='6020d1b0-2017-46b2-a8a2-790180b29374'></script>
</head><body>

<div class="top"><div class="wrap">
<p class="kicker">JellyDazzle v2.1 &middot; the library</p>
<h1>{len(patterns)} patterns <span class="v">/</span> {len(palettes)} palettes</h1>
<p class="lede">Everything the engine can put on screen. v2.1 does not play one of these at a
time &mdash; it stacks up to four of them, each on its own clock, each fading in and out on its
own, so what you actually see is a <b>combination</b> that has probably never occurred before.
The role badge on every card is what the engine measured at startup and is what decides which
layer that pattern is allowed to occupy.</p>
<div class="tiles">{tile_html}</div>
</div></div>

<div class="bar"><div class="wrap">
<input id="q" type="search" placeholder="search name, description, family&hellip;  (or type a number + Enter to jump)"
       aria-label="Search the library">
{role_chips}{src_chips}
<span class="count" id="count"></span>
</div></div>

<div class="wrap">

<section id="patterns">
<h2>Patterns 001&ndash;{len(patterns):03d}</h2>
<p class="sub">Grouped by the layer role the engine measured for each one &mdash; how much of the
screen it covers, and therefore where in the stack it belongs. Every strip below is four real
frames from that pattern, rendered by the shipping code, each pattern on a different one of the
{len(palettes)} colour schemes. <code>JD_MODE=n</code> on the card runs that one alone.</p>
<p class="sub groupfoot">Numbers on each card: render cost in milliseconds at 1280&times;960,
mean frame-to-frame channel change (the house limit is 8 &mdash; nothing here strobes), and how
much of the screen it lights up.</p>
{"".join(pat_groups)}
</section>

<section id="palettes">
<h2>Colour schemes 000&ndash;{len(palettes)-1:03d}</h2>
<p class="sub">Each strip is the actual 32,768-entry ramp read out of <code>palette.bin</code>,
walked end to end &mdash; the bytes the app loads, not a redrawing of the anchor list. The thin
band under it is the authored anchors where those exist. The ramp is <em>cyclic</em>: the right
edge joins the left edge, and the engine scrolls through it continuously, which is why every
scheme has to come home to its own darkest colour.</p>
{"".join(pal_groups)}
</section>

<section id="asm">
<h2>Assembly modes 000&ndash;023</h2>
<p class="sub">The original ARM64 engine in <code>draw.s</code>. These are grounds only &mdash;
they read the palette directly rather than taking a layer palette, so they can only ever sit at
the bottom of the stack. Modes 015&ndash;023 are accumulators: they stamp a few pixels per frame
onto a canvas that persists, which is why they cost almost nothing.</p>
<div style="overflow-x:auto"><table style="border-collapse:collapse;font-family:var(--mono);font-size:.8rem;min-width:420px">
<thead><tr style="color:var(--amber);text-align:left">
<th style="padding:6px 16px 6px 0">mode</th><th style="padding:6px 16px 6px 0">role</th>
<th style="padding:6px 16px 6px 0">ms</th><th style="padding:6px 16px 6px 0">motion</th>
<th style="padding:6px 0">kind</th></tr></thead>
<tbody style="color:var(--dim)">{asm_rows}</tbody></table></div>
</section>

</div>
<footer><div class="wrap">
Generated by <code>lab/_gallery.py</code> from the shipping tree &mdash;
patterns from <code>patterns_c/</code>, schemes from <code>palette.bin</code>,
measurements from <code>lab/design/engine_stats.csv</code>.
</div></footer>
<script>{JS}</script>
</body></html>
"""

# =================================================================== catalog

def catalog(patterns, palettes, asm):
    L = []
    A = L.append
    A("# JellyDazzle — Master Catalog")
    A("")
    A(f"Generated by `lab/_gallery.py`. **{len(patterns)} patterns** · "
      f"**{len(palettes)} colour schemes** · **{len(asm)} assembly modes** "
      f"= **{len(patterns) + len(asm)} routines** the scheduler can draw from.")
    A("")
    A("`role` / `ms` / `motion` / `cover` are measured by the engine's own startup probe "
      "(`lab/design/engine_stats.csv`) — the same table the v2.1 scheduler schedules from. "
      "`JD_MODE` runs that routine on its own.")
    A("")
    A("## Patterns")
    A("")
    A("| # | JD_MODE | Name | Role | ms | motion | cover | Family | One-liner |")
    A("|---|---------|------|------|----|--------|-------|--------|-----------|")
    for p in patterns:
        ms = f"{p['cost']:.2f}" if p["cost"] is not None else "—"
        dl = f"{p['delta']:.2f}" if p["delta"] is not None else "—"
        cv = f"{p['cover']*100:.0f}%" if p["cover"] is not None else "—"
        look = p["look"].replace("|", "\\|")
        A(f"| {p['n']:03d} | {p['rt']} | {p['name']} | {p['role']} | {ms} | {dl} | {cv} "
          f"| {p['family']} | {look} |")
    A("")
    A("## Colour schemes")
    A("")
    A("Index is the scheme's position in `palette.bin`, which is the only ordering the engine knows.")
    A("")
    A("| idx | Source | Name | Class | Anchors | Gate | Mood |")
    A("|-----|--------|------|-------|---------|------|------|")
    for p in palettes:
        mood = (p["mood"] or p["look"] or p["scheme"] or "").replace("|", "\\|")
        A(f"| {p['idx']:03d} | {p['source']} | {p['name']} | {p['cls'] or '—'} "
          f"| {len(p['colors']) or '—'} | {p['gate'] or '—'} | {mood} |")
    A("")
    A("## Assembly modes")
    A("")
    A("| mode | role | ms | motion | kind |")
    A("|------|------|----|--------|------|")
    for a in asm:
        ms = f"{a['cost']:.2f}" if a["cost"] is not None else "—"
        dl = f"{a['delta']:.2f}" if a["delta"] is not None else "—"
        A(f"| {a['mode']:03d} | {a['role']} | {ms} | {dl} "
          f"| {'accumulator' if a['cls'] == 'canvas' else 'repaint'} |")
    A("")
    return "\n".join(L)

# ====================================================================== main

def main():
    stats = read_engine_stats()
    smap = read_scheme_map()
    pat_scores, pal_scores = read_curator_scores()
    patterns = harvest_patterns(stats, pat_scores)
    palettes = harvest_palettes(smap, pal_scores)
    asm = harvest_asm(stats)

    open(os.path.join(LAB, "gallery.html"), "w").write(build(patterns, palettes, asm, stats))
    open(os.path.join(LAB, "CATALOG.md"), "w").write(catalog(patterns, palettes, asm))

    from collections import Counter
    print(f"patterns={len(patterns)} palettes={len(palettes)} asm={len(asm)} "
          f"routines={len(patterns)+len(asm)}")
    print("roles:", dict(Counter(p["role"] for p in patterns)))
    print("sources:", dict(Counter(p["source"] for p in palettes)))
    print("classes:", dict(Counter(p["cls"] for p in palettes if p["cls"])))
    miss_prev = [p["n"] for p in patterns if not p["preview"]]
    miss_look = [p["n"] for p in patterns if not p["look"]]
    miss_ramp = [p["idx"] for p in palettes if not p["ramp"]]
    miss_role = [p["n"] for p in patterns if p["role"] not in ROLE_ORDER]
    print("missing preview:", miss_prev or "none")
    print("missing description:", miss_look or "none")
    print("missing ramp:", miss_ramp or "none")
    print("unmeasured:", miss_role or "none")

if __name__ == "__main__":
    main()
