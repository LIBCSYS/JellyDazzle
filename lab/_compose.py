#!/usr/bin/env python3
"""Lab composer: harvest patterns/palettes/scores -> gallery.html + CATALOG.md"""
import json, re, html, glob, os

LAB = os.path.dirname(os.path.abspath(__file__))

# ---------- families by number ----------
FAMILIES = [
    (range(1, 11),   "Kaleido & Tiling"),
    (range(11, 21),  "Demoscene Classics"),
    (range(21, 31),  "Moire & Op-Art"),
    (range(31, 41),  "Spirograph & Curves"),
    (range(41, 51),  "Cellular & Reaction"),
    (range(51, 61),  "Particles & Orbits"),
    (range(61, 71),  "Tunnels & Vortex"),
    (range(71, 81),  "Organic & Growth"),
    (range(81, 101), "Original Footage Remakes"),
]
def family(n):
    for r, name in FAMILIES:
        if n in r: return name
    return "?"

# ---------- parse curator scores ----------
pat_scores = {}   # n -> dict(dz,co,mo,total,notes)
pal_scores = {}   # 'P16' -> dict(a,b,c,total,notes)
for f in glob.glob(os.path.join(LAB, "curate", "scores_*.md")):
    for line in open(f):
        m = re.match(r"\|\s*(\d{3})\s*\|\s*([\w-]+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(.+?)\s*\|\s*$", line)
        if m:
            n = int(m.group(1))
            pat_scores[n] = dict(slug=m.group(2), dz=int(m.group(3)), co=int(m.group(4)),
                                 mo=int(m.group(5)), total=int(m.group(6)), notes=m.group(7))
            continue
        m = re.match(r"\|\s*(P\d{2})\s*\|\s*([\w-]+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(.+?)\s*\|\s*$", line)
        if m:
            pal_scores[m.group(1)] = dict(slug=m.group(2), rich=int(m.group(3)), rng=int(m.group(4)),
                                          mood=int(m.group(5)), total=int(m.group(6)), notes=m.group(7))

# ---------- harvest patterns ----------
def first_sentence(text, maxlen=170):
    text = " ".join(text.split())
    m = re.match(r"(.+?[.!?])(\s|$)", text)
    s = m.group(1) if m else text
    if len(s) > maxlen:
        s = s[:maxlen].rsplit(" ", 1)[0] + "..."
    return s

def section(md, header):
    m = re.search(r"^## +" + header + r"\s*\n(.*?)(?=^## |\Z)", md, re.S | re.M)
    return m.group(1).strip() if m else ""

patterns = []
for d in sorted(glob.glob(os.path.join(LAB, "patterns", "[0-9]*"))):
    base = os.path.basename(d)
    n = int(base[:3]); slug = base[4:]
    md = open(os.path.join(d, "spec.md")).read()
    title_m = re.match(r"# +\d+ +(.+)", md)
    name = title_m.group(1).strip() if title_m else slug.replace("_", " ").title()
    look = first_sentence(section(md, "Look") or "")
    sc = pat_scores.get(n)
    patterns.append(dict(n=n, slug=slug, dir=base, name=name, look=look, sc=sc,
                         preview=f"patterns/{base}/preview.png", family=family(n)))

# ---------- harvest palettes ----------
palettes = []
for d in sorted(glob.glob(os.path.join(LAB, "palettes", "P[0-9]*"))):
    base = os.path.basename(d)
    pid = base[:3]; slug = base[4:]
    pj = json.load(open(os.path.join(d, "palette.json")))
    md = open(os.path.join(d, "spec.md")).read()
    name = pj.get("name") or slug.replace("_", " ").replace("-", " ").title()
    look = first_sentence(section(md, "Look") or section(md, "Mood") or pj.get("mood", ""))
    palettes.append(dict(pid=pid, slug=slug, dir=base, name=name, look=look,
                         colors=pj["colors"], sc=pal_scores.get(pid),
                         swatch=f"palettes/{base}/swatch.png"))

# ---------- rankings ----------
ranked = sorted([p for p in patterns if p["sc"]],
                key=lambda p: (-p["sc"]["total"], -p["sc"]["dz"], -p["sc"]["co"], p["n"]))
top20 = ranked[:20]

# Port-next: high score + easiest per Integer ARM64 plan (hand-judged from plan text)
PORT_NEXT = [
    (11,  "pure LUT plasma: precomputed phase buffers + palette rotation, 3 table reads/pixel"),
    (19,  "one distance byte table + two window offsets; motion is literal DAC-style palette rotation"),
    (4,   "D8 fold is 3 ops; squared-distance arc test, hash tiles, no sqrt anywhere"),
    (42,  "pure repaint f(x,y,t): angle/radius LUTs + three sine-table reads, palette LUT out"),
    (100, "~6 integer ops + 2 table reads/pixel; hue roll is free DAC ramp rotation"),
    (12,  "textbook 16.16 rotozoom into a wedge buffer + mirror-blit fold map"),
    (14,  "copper-bar architecture verbatim: per-frame 256-entry colorline, 1 table read/pixel"),
    (64,  "all adds/shifts/lookups; chained table sines, saturation clamp LUT"),
    (99,  "static stadium distance map baked once; nearly all motion via DAC rotation, minimal pixel writes"),
    (87,  "incremental \\|x\\|+\\|y\\| walk (no multiplies in inner loop), quadrant mirror blit"),
]

# ---------- CATALOG.md ----------
by_n = {p["n"]: p for p in patterns}
lines = []
lines.append("# DAZZLE LAB — Master Catalog")
lines.append("")
lines.append(f"Composed 2026-08-13. **{len(patterns)} patterns** (all with preview.png) · **{len(palettes)} palettes** (all with swatch.png).")
lines.append("Scores from curator passes (`curate/scores_*.md`): DZ = dazzle-vibe, CO = cohesion, MO = motion-promise, each 1-10.")
lines.append("")
lines.append("## Patterns 001-100")
lines.append("")
lines.append("| # | Name | Family | DZ | CO | MO | Total | One-liner |")
lines.append("|---|------|--------|----|----|----|-------|-----------|")
# v2.1 work adds patterns and palettes faster than the curator passes score
# them, so an unscored entry must render as "—" rather than crash the build.
NO_PAT = dict(dz='—', co='—', mo='—', total='—', notes='')
NO_PAL = dict(rich='—', rng='—', mood='—', total='—', notes='')
for p in patterns:
    s = p["sc"] or NO_PAT
    lines.append(f"| {p['n']:03d} | {p['name']} | {p['family']} | {s['dz']} | {s['co']} | {s['mo']} | **{s['total']}** | {p['look']} |")
lines.append("")
lines.append("## Palettes")
lines.append("")
lines.append("| ID | Name | Richness | Range | Mood | Total | One-liner |")
lines.append("|----|------|----------|-------|------|-------|-----------|")
for p in palettes:
    s = p["sc"] or NO_PAL
    lines.append(f"| {p['pid']} | {p['name']} | {s['rich']} | {s['rng']} | {s['mood']} | **{s['total']}** | {p['look']} |")
lines.append("")
lines.append("## TOP 20 (ranked from curator scores)")
lines.append("")
lines.append("Rank order: total desc, then dazzle-vibe, then cohesion, then number.")
lines.append("")
lines.append("| Rank | # | Name | Family | Total | Why |")
lines.append("|------|---|------|--------|-------|-----|")
for i, p in enumerate(top20, 1):
    note = p["sc"]["notes"]
    lines.append(f"| {i} | {p['n']:03d} | {p['name']} | {p['family']} | {p['sc']['total']} | {note} |")
lines.append("")
lines.append("## PORT NEXT — 10 highest-scoring x easiest Integer ARM64 plans")
lines.append("")
lines.append("Chosen from the top scorers whose `## Integer ARM64 plan` sections are pure LUT/fixed-point")
lines.append("full-repaint work — no simulation state, no heavy per-frame passes, no per-pixel float/trig/div.")
lines.append("")
lines.append("| Order | # | Name | Total | Why it ports easily |")
lines.append("|-------|---|------|-------|---------------------|")
for i, (n, why) in enumerate(PORT_NEXT, 1):
    p = by_n[n]
    lines.append(f"| {i} | {p['n']:03d} | {p['name']} | {p['sc']['total']} | {why} |")
lines.append("")
lines.append("Notes: 089_oval_drums (25) scores above 099 but shares its composition (curator dup-flag: keep one) — 099's plan is the cheaper port, so it carries the slot. First alternates: 061_checker_tunnel (23, pure byte-LUT adds), 030_octa_facets (24, no sine table even needed), 067_twin_tunnels (25, medium: incremental d² + isqrt/mul LUTs).")
lines.append("")
lines.append("## Curator flags (carry-over)")
lines.append("")
lines.append("- **Near-dup pairs — keep one of each:** 089/099 (oval drums), 090/091 (diamond), 082/092 (greek key), 084/098 (gears), 085/095 (ring machine), 043/048 (static maze), 002/010 (quilt), 032/034 (lissajous trail).")
lines.append("- **Redundant clusters:** 052/053/055/059 (sparse dotted orbits — keep at most one), 054/057/060 (dim space dust — 057 only keeper), 009/031/033 (line rosettes — 033 weakest).")
lines.append("- **Cut candidates (lowest totals):** 097 (13), 060 (15), 023/054/059/074/077/094 (16), 029 (17).")
lines.append("- **Palette usage:** P10/P11 are narrow ramps — pair with a wide palette. P06/P13 are mega-palettes — use with a per-pattern subset picker.")
lines.append("")
open(os.path.join(LAB, "CATALOG.md"), "w").write("\n".join(lines))

# ---------- gallery.html ----------
def esc(s): return html.escape(s, quote=True)

cards = []
for p in patterns:
    s = p["sc"] or NO_PAT
    cards.append(f"""<div class="card" id="p{p['n']:03d}">
<div class="head"><span class="num">{p['n']:03d}</span><span class="name">{esc(p['name'])}</span><span class="fam">{esc(p['family'])}</span></div>
<img src="{p['preview']}" alt="{p['n']:03d} {esc(p['name'])} preview" loading="lazy">
<p class="desc">{esc(p['look'])}</p>
<div class="scores"><span>DZ {s['dz']}</span><span>CO {s['co']}</span><span>MO {s['mo']}</span><span class="tot">TOTAL {s['total']}</span></div>
</div>""")

pal_cards = []
for p in palettes:
    s = p["sc"] or NO_PAL
    chips = "".join(f'<i style="background:#{c}"></i>' for c in p["colors"])
    pal_cards.append(f"""<div class="card pal" id="{p['pid']}">
<div class="head"><span class="num">{p['pid']}</span><span class="name">{esc(p['name'])}</span></div>
<img src="{p['swatch']}" alt="{p['pid']} {esc(p['name'])} swatch" loading="lazy">
<div class="chips">{chips}</div>
<p class="desc">{esc(p['look'])}</p>
<div class="scores"><span>RICH {s['rich']}</span><span>RANGE {s['rng']}</span><span>MOOD {s['mood']}</span><span class="tot">TOTAL {s['total']}</span></div>
</div>""")

top10_html = " ".join(f'<a href="#p{p["n"]:03d}">{p["n"]:03d}</a>' for p in ranked[:10])

page = f"""<!DOCTYPE html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>DAZZLE LAB — 100 Patterns / 30 Palettes</title>
<style>
:root {{ --bg:#0b0b10; --panel:#14141c; --edge:#26262f; --ink:#e8e8f0; --dim:#9a9aac; --hot:#ffd23c; --acc:#5ee0d0; }}
* {{ box-sizing:border-box; margin:0; padding:0; }}
body {{ background:var(--bg); color:var(--ink); font:15px/1.45 -apple-system,"Helvetica Neue",Segoe UI,sans-serif; padding:2rem 1.2rem 4rem; }}
header {{ max-width:1400px; margin:0 auto 1.6rem; }}
h1 {{ font-size:1.9rem; letter-spacing:.06em; }}
h1 b {{ color:var(--hot); }}
header p {{ color:var(--dim); margin-top:.4rem; }}
header .top10 {{ margin-top:.6rem; font-size:.95rem; }}
header .top10 a {{ color:var(--acc); text-decoration:none; margin-right:.55rem; font-weight:700; font-variant-numeric:tabular-nums; }}
h2 {{ max-width:1400px; margin:2.4rem auto 1rem; font-size:1.3rem; letter-spacing:.08em; color:var(--hot); border-bottom:1px solid var(--edge); padding-bottom:.4rem; }}
.grid {{ max-width:1400px; margin:0 auto; display:grid; grid-template-columns:repeat(auto-fill,minmax(400px,1fr)); gap:1rem; }}
.card {{ background:var(--panel); border:1px solid var(--edge); border-radius:10px; padding:.8rem; }}
.card img {{ width:100%; height:auto; border-radius:6px; display:block; image-rendering:auto; background:#000; }}
.head {{ display:flex; align-items:baseline; gap:.6rem; margin-bottom:.55rem; }}
.num {{ font-size:1.5rem; font-weight:800; color:var(--hot); font-variant-numeric:tabular-nums; }}
.name {{ font-weight:700; font-size:1.05rem; }}
.fam {{ margin-left:auto; font-size:.72rem; color:var(--dim); text-transform:uppercase; letter-spacing:.08em; }}
.desc {{ color:var(--dim); font-size:.86rem; margin-top:.55rem; }}
.scores {{ display:flex; gap:.5rem; margin-top:.55rem; font-size:.78rem; font-weight:700; }}
.scores span {{ background:#1e1e28; border:1px solid var(--edge); border-radius:5px; padding:.15rem .5rem; color:var(--acc); }}
.scores .tot {{ color:var(--hot); margin-left:auto; }}
.chips {{ display:flex; height:22px; border-radius:5px; overflow:hidden; margin-top:.55rem; border:1px solid var(--edge); }}
.chips i {{ flex:1; }}
@media (max-width:480px) {{ .grid {{ grid-template-columns:1fr; }} }}
</style></head><body>
<header>
<h1>DAZZLE LAB — <b>100 Patterns</b> / <b>30 Palettes</b></h1>
<p>Prototype gallery for the dazzle.exe recreation. Every entry is addressed by its number — test by number. Scores: DZ dazzle-vibe · CO cohesion · MO motion-promise (patterns); RICH · RANGE · MOOD (palettes). Composed 2026-08-13.</p>
<p class="top10"><b style="color:var(--ink)">TOP 10:</b> {top10_html}</p>
</header>
<h2>PATTERNS 001–100</h2>
<div class="grid">
{chr(10).join(cards)}
</div>
<h2>PALETTES P01–P30</h2>
<div class="grid">
{chr(10).join(pal_cards)}
</div>
</body></html>
"""
open(os.path.join(LAB, "gallery.html"), "w").write(page)

# ---------- report ----------
missing = [p["n"] for p in patterns if not p["sc"]] + [p["pid"] for p in palettes if not p["sc"]]
print(f"patterns={len(patterns)} palettes={len(palettes)}")
print("top10=", [f"{p['n']:03d}" for p in ranked[:10]])
print("top20=", [f"{p['n']:03d}" for p in top20])
print("unscored=", missing)
print("no-look=", [p["n"] for p in patterns if not p["look"]] + [p["pid"] for p in palettes if not p["look"]])
