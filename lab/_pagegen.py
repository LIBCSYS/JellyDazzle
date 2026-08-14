#!/usr/bin/env python3
"""Generate the GitHub Pages front page (../index.html) from lab/CATALOG.md.

Run from anywhere:  python3 lab/_pagegen.py
Reads:  lab/CATALOG.md, lab/patterns/NNN_slug/, lab/palettes/PNN_slug/palette.json
Writes: index.html at the repo root (Pages serves the root of master).
"""
import html
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LAB = ROOT / "lab"

TOP10 = ["011", "042", "004", "019", "100", "018", "067", "089", "014", "064"]
HERO_IMGS = TOP10[:6]  # preloaded crossfade stack — keep the page light

STATS = [
    ("1,232", "lines of assembly"),
    ("24", "routines on the wheel"),
    ("100", "lab patterns"),
    ("30", "palettes"),
    ("~175", "fps, single thread"),
    ("0", "floating-point ops"),
]


def parse_tables(md: str):
    patterns, palettes, section = [], [], None
    for line in md.splitlines():
        if line.startswith("## "):
            section = ("pat" if line.startswith("## Patterns")
                       else "pal" if line.startswith("## Palettes") else None)
            continue
        if not line.startswith("|") or line.startswith("| #") or line.startswith("| ID") or line.startswith("|--"):
            continue
        cells = [c.strip() for c in line.strip("|").split("|")]
        if section == "pat" and len(cells) >= 7:
            num, name, fam, dz, co, mo, tot = cells[:7]
            desc = cells[7] if len(cells) > 7 else ""
            patterns.append(dict(num=num, name=name, fam=fam, dz=dz, co=co, mo=mo,
                                 tot=tot.strip("* "), desc=desc))
        elif section == "pal" and len(cells) >= 6:
            pid, name, rich, rng, mood, tot = cells[:6]
            desc = cells[6] if len(cells) > 6 else ""
            palettes.append(dict(pid=pid, name=name, rich=rich, rng=rng, mood=mood,
                                 tot=tot.strip("* "), desc=desc))
    return patterns, palettes


def pattern_dir(num: str):
    hits = sorted((LAB / "patterns").glob(f"{num}_*"))
    return hits[0] if hits else None


def palette_dir(pid: str):
    hits = sorted((LAB / "palettes").glob(f"{pid}_*"))
    return hits[0] if hits else None


def rel(p: Path):
    return p.relative_to(ROOT).as_posix()


def pattern_card(p, featured=False, rank=None):
    d = pattern_dir(p["num"])
    if d is None:
        return ""
    img = rel(d / "preview.png")
    badge = f'<span class="rank">TOP&nbsp;10 · №{rank}</span>' if rank else ""
    cls = "card featured" if featured else "card"
    return f'''<article class="{cls}" id="p{p['num']}">
<div class="head"><span class="num">{p['num']}</span><span class="name">{html.escape(p['name'])}</span>{badge}</div>
<a class="shot" href="{img}" data-lightbox="{p['num']} {html.escape(p['name'], quote=True)}"><img src="{img}" alt="Pattern {p['num']} {html.escape(p['name'], quote=True)} preview" loading="lazy" width="1280" height="960"></a>
<p class="desc">{html.escape(p['desc'])}</p>
<div class="scores"><span>DZ {p['dz']}</span><span>CO {p['co']}</span><span>MO {p['mo']}</span><span class="tot">Σ {p['tot']}</span></div>
</article>'''


def palette_card(p):
    d = palette_dir(p["pid"])
    if d is None:
        return ""
    colors = json.loads((d / "palette.json").read_text()).get("colors", [])
    chips = "".join(f'<i style="background:#{c}" title="#{c}"></i>' for c in colors)
    return f'''<article class="card pal" id="{p['pid']}">
<div class="head"><span class="num">{p['pid']}</span><span class="name">{html.escape(p['name'])}</span><span class="fam">{len(colors)} colors</span></div>
<div class="chips" role="img" aria-label="{html.escape(p['name'], quote=True)} color swatches">{chips}</div>
<p class="desc">{html.escape(p['desc'])}</p>
<div class="scores"><span>RICH {p['rich']}</span><span>RANGE {p['rng']}</span><span>MOOD {p['mood']}</span><span class="tot">Σ {p['tot']}</span></div>
</article>'''


def build():
    patterns, palettes = parse_tables((LAB / "CATALOG.md").read_text())
    by_num = {p["num"]: p for p in patterns}

    hero_layers = []
    for i, num in enumerate(HERO_IMGS):
        d = pattern_dir(num)
        hero_layers.append(
            f'<img class="hero-slide" style="animation-delay:{i * 5}s" src="{rel(d / "preview.png")}" alt="" aria-hidden="true">')

    stat_html = "".join(
        f'<div class="stat"><b>{n}</b><span>{label}</span></div>' for n, label in STATS)

    top_html = "".join(pattern_card(by_num[n], featured=True, rank=i + 1)
                       for i, n in enumerate(TOP10) if n in by_num)

    families, order = {}, []
    for p in patterns:
        if p["fam"] not in families:
            families[p["fam"]] = []
            order.append(p["fam"])
        families[p["fam"]].append(p)

    fam_nav = "".join(
        f'<a href="#f{i}">{html.escape(f)}</a>' for i, f in enumerate(order))
    lab_html = ""
    for i, fam in enumerate(order):
        cards = "".join(pattern_card(p) for p in families[fam])
        lab_html += f'<h3 class="famhead" id="f{i}">{html.escape(fam)} <span>{families[fam][0]["num"]}–{families[fam][-1]["num"]}</span></h3>\n<div class="grid">{cards}</div>\n'

    pal_html = "".join(palette_card(p) for p in palettes)

    page = f'''<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>JellyDazzle — DAZZLE.EXE reborn in Apple Silicon assembly</title>
<meta name="description" content="An homage to DAZZLE.EXE, the magical DOS kaleidoscope — hand-written in ARMv9.2-A assembly for Apple Silicon. 100 lab patterns, 30 palettes, zero floats.">
<meta property="og:title" content="JellyDazzle">
<meta property="og:description" content="Never the same pattern, never the same colors. DAZZLE.EXE reborn in hand-written ARM assembly.">
<meta property="og:image" content="https://libcsys.github.io/JellyDazzle/lab/patterns/011_plasma_mandala/preview.png">
<link rel="icon" href="lab/patterns/011_plasma_mandala/preview.png">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Bungee&family=IBM+Plex+Mono:wght@400;600&family=IBM+Plex+Sans:wght@400;600&display=swap" rel="stylesheet">
<style>
:root {{
  --void:#08050f; --panel:#130c20; --edge:#251a3a; --ink:#efe9ff; --dim:#978db3;
  --magenta:#ff3ea5; --volt:#ffd23c; --aqua:#45e6d0;
}}
* {{ box-sizing:border-box; margin:0; padding:0; }}
html {{ scroll-behavior:smooth; }}
body {{ background:var(--void); color:var(--ink);
  font:16px/1.55 "IBM Plex Sans",-apple-system,sans-serif; }}
a {{ color:var(--aqua); }}
a:focus-visible, button:focus-visible {{ outline:3px solid var(--volt); outline-offset:2px; }}
.mono {{ font-family:"IBM Plex Mono",monospace; }}

/* ---------- hero: attract mode ---------- */
.hero {{ position:relative; min-height:100svh; display:flex; flex-direction:column;
  justify-content:center; align-items:center; text-align:center; overflow:hidden;
  padding:4rem 1.2rem 5.5rem; isolation:isolate; }}
.hero-slide {{ position:absolute; inset:0; width:100%; height:100%; object-fit:cover;
  opacity:0; z-index:-3; animation:attract 30s infinite; filter:saturate(1.15); }}
@keyframes attract {{
  0% {{ opacity:0; transform:scale(1); }}
  4% {{ opacity:1; }}
  16.6% {{ opacity:1; transform:scale(1.07); }}
  22% {{ opacity:0; transform:scale(1.09); }}
  100% {{ opacity:0; }}
}}
.hero::before {{ content:""; position:absolute; inset:0; z-index:-2;
  background:radial-gradient(ellipse at center, rgba(8,5,15,.25) 0%, rgba(8,5,15,.88) 78%); }}
.hero::after {{ content:""; position:absolute; inset:0; z-index:-1; pointer-events:none;
  background:repeating-linear-gradient(0deg, transparent 0 2px, rgba(0,0,0,.22) 2px 4px); }}
.eyebrow {{ font-family:"IBM Plex Mono",monospace; font-size:.78rem; letter-spacing:.22em;
  color:var(--volt); text-transform:uppercase; }}
.wordmark {{ font-family:"Bungee",sans-serif; font-weight:400; color:var(--ink);
  font-size:clamp(2.6rem, 9vw, 6.5rem); line-height:1.05; margin:.5rem 0 .8rem;
  text-shadow:-4px 0 var(--magenta), 4px 0 var(--aqua), 0 0 46px rgba(255,62,165,.35); }}
.tagline {{ font-size:clamp(1.05rem, 2.4vw, 1.45rem); color:var(--ink); max-width:34em; }}
.tagline em {{ color:var(--volt); font-style:normal; }}
.ctas {{ display:flex; gap:.9rem; flex-wrap:wrap; justify-content:center; margin-top:1.8rem; }}
.btn {{ display:inline-block; padding:.85rem 1.5rem; border-radius:8px; font-weight:600;
  text-decoration:none; font-size:1rem; border:1px solid var(--edge); }}
.btn.primary {{ background:var(--magenta); color:#14060e; border-color:var(--magenta); }}
.btn.primary:hover {{ background:var(--volt); border-color:var(--volt); }}
.btn.ghost {{ color:var(--ink); background:rgba(19,12,32,.72); }}
.btn.ghost:hover {{ border-color:var(--aqua); color:var(--aqua); }}
.stats {{ position:absolute; left:0; right:0; bottom:0; display:flex; flex-wrap:wrap;
  justify-content:center; gap:0 2.6rem; padding:1rem 1.2rem 1.3rem;
  background:linear-gradient(transparent, rgba(8,5,15,.92) 40%); }}
.stat {{ font-family:"IBM Plex Mono",monospace; text-align:center; padding:.3rem 0; }}
.stat b {{ display:block; color:var(--volt); font-size:1.35rem; }}
.stat span {{ font-size:.72rem; color:var(--dim); text-transform:uppercase; letter-spacing:.13em; }}

/* ---------- shared sections ---------- */
section {{ max-width:1400px; margin:0 auto; padding:4.5rem 1.2rem 0; }}
h2 {{ font-family:"Bungee",sans-serif; font-weight:400; color:var(--volt);
  font-size:clamp(1.4rem, 3.4vw, 2.1rem); letter-spacing:.04em; margin-bottom:.4rem; }}
.sub {{ color:var(--dim); max-width:52em; margin-bottom:1.6rem; }}
.sub a {{ color:var(--aqua); }}
.story {{ display:grid; grid-template-columns:1fr 1fr; gap:2.5rem; align-items:start; }}
.story blockquote {{ font-size:1.25rem; line-height:1.6; border-left:4px solid var(--magenta);
  padding-left:1.2rem; color:var(--ink); }}
.story blockquote footer {{ margin-top:.7rem; font-size:.85rem; color:var(--dim);
  font-family:"IBM Plex Mono",monospace; }}
.story .body p {{ margin-bottom:1rem; color:var(--dim); }}
.story .body strong {{ color:var(--ink); }}
@media (max-width:820px) {{ .story {{ grid-template-columns:1fr; }} }}

/* ---------- cards ---------- */
.grid {{ display:grid; grid-template-columns:repeat(auto-fill,minmax(340px,1fr));
  gap:1.1rem; margin-bottom:1.4rem; }}
.grid.wide {{ grid-template-columns:repeat(auto-fill,minmax(420px,1fr)); }}
.card {{ background:var(--panel); border:1px solid var(--edge); border-radius:12px;
  padding:.9rem; transition:transform .18s ease, border-color .18s ease, box-shadow .18s ease; }}
.card:hover {{ transform:translateY(-3px); border-color:var(--magenta);
  box-shadow:0 10px 34px rgba(255,62,165,.16); }}
.card .head {{ display:flex; align-items:baseline; gap:.6rem; margin-bottom:.55rem; }}
.num {{ font-family:"IBM Plex Mono",monospace; font-weight:600; font-size:1.35rem; color:var(--volt); }}
.name {{ font-weight:600; font-size:1.02rem; }}
.fam, .rank {{ margin-left:auto; font-family:"IBM Plex Mono",monospace; font-size:.68rem;
  color:var(--dim); text-transform:uppercase; letter-spacing:.09em; white-space:nowrap; }}
.rank {{ color:var(--magenta); }}
.shot {{ display:block; border-radius:7px; overflow:hidden; background:#000; }}
.shot img {{ width:100%; height:auto; display:block; }}
.desc {{ color:var(--dim); font-size:.86rem; margin-top:.6rem; }}
.scores {{ display:flex; gap:.5rem; margin-top:.6rem;
  font-family:"IBM Plex Mono",monospace; font-size:.76rem; font-weight:600; }}
.scores span {{ background:var(--void); border:1px solid var(--edge); border-radius:5px;
  padding:.14rem .5rem; color:var(--aqua); }}
.scores .tot {{ color:var(--volt); margin-left:auto; }}
.famhead {{ font-family:"IBM Plex Mono",monospace; font-size:.95rem; letter-spacing:.14em;
  text-transform:uppercase; color:var(--ink); border-bottom:1px solid var(--edge);
  padding:1.6rem 0 .5rem; margin-bottom:1rem; }}
.famhead span {{ color:var(--dim); font-size:.78rem; margin-left:.6rem; }}
.famnav {{ display:flex; flex-wrap:wrap; gap:.45rem .5rem; margin-bottom:.6rem; }}
.famnav a {{ font-family:"IBM Plex Mono",monospace; font-size:.74rem; letter-spacing:.06em;
  text-decoration:none; color:var(--aqua); border:1px solid var(--edge); border-radius:99px;
  padding:.3rem .8rem; }}
.famnav a:hover {{ border-color:var(--aqua); }}
.chips {{ display:flex; height:34px; border-radius:6px; overflow:hidden;
  border:1px solid var(--edge); }}
.chips i {{ flex:1; }}

/* ---------- lightbox ---------- */
dialog {{ border:none; background:rgba(8,5,15,.94); padding:1.2rem; width:100vw; height:100vh;
  max-width:100vw; max-height:100vh; display:none; flex-direction:column; align-items:center;
  justify-content:center; gap:.8rem; }}
dialog[open] {{ display:flex; }}
dialog img {{ max-width:min(96vw,1280px); max-height:82vh; border-radius:8px; }}
dialog p {{ font-family:"IBM Plex Mono",monospace; color:var(--volt); }}
dialog button {{ font:inherit; background:var(--panel); color:var(--ink);
  border:1px solid var(--edge); border-radius:8px; padding:.5rem 1.2rem; cursor:pointer; }}

footer.site {{ margin-top:5rem; border-top:1px solid var(--edge); padding:2.2rem 1.2rem 3rem;
  text-align:center; color:var(--dim); font-size:.9rem; }}
footer.site .mono {{ font-size:.76rem; letter-spacing:.14em; text-transform:uppercase;
  margin-top:.5rem; }}

@media (prefers-reduced-motion: reduce) {{
  html {{ scroll-behavior:auto; }}
  .hero-slide {{ animation:none; opacity:0; }}
  .hero-slide:first-of-type {{ opacity:1; }}
  .card, .card:hover {{ transition:none; transform:none; }}
}}
</style>
</head>
<body>

<header class="hero">
{chr(10).join(hero_layers)}
<p class="eyebrow">DAZZLE.EXE homage · hand-written ARMv9.2-A assembly · Apple Silicon</p>
<h1 class="wordmark">JELLY<br>DAZZLE</h1>
<p class="tagline"><em>Never the same pattern, never the same colors.</em><br>
The magical DOS kaleidoscope, rebuilt from scratch — every pixel drawn by hand-written ARM64 assembly.</p>
<div class="ctas">
<a class="btn primary" href="https://github.com/LIBCSYS/JellyDazzle/releases">Download JellyDazzle.app</a>
<a class="btn ghost" href="https://github.com/LIBCSYS/JellyDazzle/blob/master/draw.s">Read the assembly</a>
</div>
<div class="stats">{stat_html}</div>
</header>

<section id="story">
<h2>Thirty years of staring at the same screen</h2>
<div class="story">
<blockquote>"I spent hours and hours staring at that magical kaleidoscope. Never the same pattern, never the same color scheme. It was amazing, and everything looked cool."
<footer>— the reason this exists</footer></blockquote>
<div class="body">
<p><strong>The engine</strong> is a 24-routine wheel running at ~175 fps on a single M-series core — interference kaleidoscopes, radius-sheared twisters, tunnels, moiré eyes, spirographs that draw themselves over 34 seconds — several reverse-engineered frame by frame from video of the original. A 66-line SDL2 shim stands in for INT&nbsp;10h; everything else is <a href="https://github.com/LIBCSYS/JellyDazzle/blob/master/draw.s">1,232 lines of assembly</a>. Integer math only: 16-bit interpolated sine tables, fixed point, octagonal norms — the way 1994 would have wanted it.</p>
<p><strong>The lab below</strong> is the expansion roadmap: 100 numbered pattern prototypes and 30 palettes waiting to be promoted into the engine, one verified port at a time. Each release names the lab numbers it absorbed.</p>
</div>
</div>
</section>

<section id="top10">
<h2>The Top 10</h2>
<p class="sub">Highest curator scores across dazzle-vibe (DZ), cohesion (CO) and motion-promise (MO). These get ported to assembly first.</p>
<div class="grid wide">{top_html}</div>
</section>

<section id="lab">
<h2>The Lab — 100 Patterns</h2>
<p class="sub">Every prototype is addressed by its number. Ten families, ten patterns each. Click any card for the full frame.</p>
<div class="famnav">{fam_nav}</div>
{lab_html}
</section>

<section id="palettes">
<h2>30 Palettes</h2>
<p class="sub">Six house palettes plus community palettes from <a href="https://lospec.com">Lospec</a> — PICO-8, NES, Apollo, resurrect-64 and friends. Scored for richness, range and mood. Swatches below are the real values from each <span class="mono">palette.json</span>.</p>
<div class="grid">{pal_html}</div>
</section>

<footer class="site">
<p>MIT licensed · built by <a href="https://github.com/LIBCSYS">LIBCSYS</a> · live pattern lab at <a href="https://dazzle.jelia.nyc">dazzle.jelia.nyc</a></p>
<p class="mono">drawn by hand in assembly, like 1994 intended</p>
</footer>

<dialog id="lb"><img id="lbimg" src="" alt=""><p id="lbcap"></p><button id="lbclose">Close&nbsp;·&nbsp;ESC</button></dialog>
<script>
const lb = document.getElementById("lb"), lbimg = document.getElementById("lbimg"), lbcap = document.getElementById("lbcap");
document.querySelectorAll("a.shot").forEach(a => a.addEventListener("click", e => {{
  e.preventDefault();
  lbimg.src = a.getAttribute("href");
  lbcap.textContent = a.dataset.lightbox;
  lb.showModal();
}}));
document.getElementById("lbclose").addEventListener("click", () => lb.close());
lb.addEventListener("click", e => {{ if (e.target === lb) lb.close(); }});
</script>
</body>
</html>
'''
    (ROOT / "index.html").write_text(page)
    print(f"index.html: {len(page):,} bytes · {len(patterns)} patterns · {len(palettes)} palettes · {len(order)} families")


if __name__ == "__main__":
    build()
