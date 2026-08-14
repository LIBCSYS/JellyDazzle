#!/usr/bin/env python3
"""Gallery composer — amber-phosphor mode index for JellyDazzle.

Reuses _compose.py's harvest (patterns, palettes, scores) and emits
gallery.html. Design: DOS/VGA-era CRT. Every card carries its real
JD_MODE=NNN launch command, so the page doubles as the app's mode
reference — the numbering is a command, not decoration.
"""
import html, os, re, runpy, sys

LAB = os.path.dirname(os.path.abspath(__file__))
ns = runpy.run_path(os.path.join(LAB, "_compose.py"))
patterns, palettes = ns["patterns"], ns["palettes"]
ranked, top20 = ns["ranked"], ns["top20"]
FAMILIES = [name for _, name in ns["FAMILIES"]]
top10 = [p["n"] for p in ranked[:10]]

def esc(s): return html.escape(s or "")

CSS = """
:root{
  --void:#06070c; --panel:#0c0e16; --line:#1b2030;
  --amber:#ffb02e; --beam:#52e0ff; --hot:#ff5ec8;
  --paper:#edeae2; --dim:#7c839a;
  --mono:'IBM Plex Mono',ui-monospace,SFMono-Regular,Menlo,monospace;
  --sans:'IBM Plex Sans',system-ui,-apple-system,Segoe UI,sans-serif;
}
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{margin:0;background:var(--void);color:var(--paper);font-family:var(--sans);
     font-size:16px;line-height:1.6;-webkit-font-smoothing:antialiased}
a{color:var(--beam)}
.wrap{max-width:1240px;margin:0 auto;padding:0 20px}

/* ---------- boot header ---------- */
.boot{border-bottom:1px solid var(--line);background:
   radial-gradient(120% 90% at 50% -20%,rgba(255,176,46,.10),transparent 60%),var(--void)}
.boot .wrap{padding-top:34px;padding-bottom:30px}
.tag{font-family:var(--mono);font-size:.72rem;letter-spacing:.34em;text-transform:uppercase;
     color:var(--amber);margin:0 0 14px}
h1{font-family:var(--mono);font-weight:600;font-size:clamp(2.1rem,6vw,3.6rem);
   letter-spacing:-.02em;margin:0 0 6px;line-height:1.05}
h1 .dim{color:var(--dim)}
.lede{max-width:62ch;color:#c9c6bd;margin:.4rem 0 0}
.lede b{color:var(--paper);font-weight:600}
.statline{font-family:var(--mono);font-size:.82rem;color:var(--dim);margin-top:18px;
     display:flex;flex-wrap:wrap;gap:8px 22px}
.statline b{color:var(--amber);font-weight:600}
.cta{display:inline-flex;align-items:center;gap:9px;margin-top:22px;padding:11px 20px;
     border:1px solid var(--amber);color:var(--amber);text-decoration:none;
     font-family:var(--mono);font-size:.9rem;background:rgba(255,176,46,.06)}
.cta:hover{background:var(--amber);color:var(--void)}
.cta.ghost{border-color:var(--line);color:var(--paper);background:none;margin-left:10px}
.cta.ghost:hover{border-color:var(--beam);color:var(--beam);background:rgba(82,224,255,.06)}

/* ---------- contact sheet ---------- */
.sheet{margin:26px 0 0;border:1px solid var(--line);position:relative;overflow:hidden;
       background:#000;line-height:0}
.sheet img{width:100%;display:block;opacity:.96}
.sheet::after{content:"";position:absolute;inset:0;pointer-events:none;
  background:repeating-linear-gradient(to bottom,rgba(0,0,0,.22) 0 1px,transparent 1px 3px)}
.sheetcap{font-family:var(--mono);font-size:.72rem;color:var(--dim);
  padding:8px 2px 0;line-height:1.5}

/* ---------- section chrome ---------- */
section{padding:54px 0 10px;border-top:1px solid var(--line);margin-top:44px}
h2{font-family:var(--mono);font-size:1.05rem;letter-spacing:.2em;text-transform:uppercase;
   color:var(--amber);margin:0 0 4px;font-weight:600}
.sub{color:var(--dim);font-size:.92rem;margin:0 0 26px;max-width:70ch}
.jump{display:flex;flex-wrap:wrap;gap:6px;margin:0 0 30px;font-family:var(--mono);font-size:.78rem}
.jump a{border:1px solid var(--line);padding:5px 10px;text-decoration:none;color:var(--dim)}
.jump a:hover{border-color:var(--amber);color:var(--amber)}
.fam{font-family:var(--mono);font-size:.74rem;letter-spacing:.22em;text-transform:uppercase;
     color:var(--dim);margin:38px 0 14px;padding-bottom:8px;border-bottom:1px dashed var(--line)}

/* ---------- mode cards ---------- */
.grid{display:grid;gap:16px;grid-template-columns:repeat(auto-fill,minmax(292px,1fr))}
.card{border:1px solid var(--line);background:var(--panel);display:flex;flex-direction:column;
      transition:border-color .18s,transform .18s}
.card:hover{border-color:var(--amber);transform:translateY(-2px)}
.card.star{border-color:rgba(255,176,46,.45)}
.shot{position:relative;line-height:0;background:#000;border-bottom:1px solid var(--line)}
.shot img{width:100%;display:block}
.shot::after{content:"";position:absolute;inset:0;pointer-events:none;
  background:repeating-linear-gradient(to bottom,rgba(0,0,0,.28) 0 1px,transparent 1px 3px)}
.num{position:absolute;top:0;left:0;z-index:2;font-family:var(--mono);font-weight:600;
     font-size:.86rem;letter-spacing:.06em;color:var(--void);background:var(--amber);
     padding:3px 9px;line-height:1.4}
.card.star .num::after{content:" ★"}
.body{padding:13px 15px 15px;display:flex;flex-direction:column;gap:8px;flex:1}
.name{font-weight:600;font-size:1.02rem;letter-spacing:-.01em;margin:0}
.look{font-size:.86rem;color:#b6b3ab;margin:0;flex:1}
.meta{display:flex;align-items:center;justify-content:space-between;gap:10px;
      font-family:var(--mono);font-size:.72rem;color:var(--dim);
      border-top:1px dashed var(--line);padding-top:9px}
.score{color:var(--beam)}
.cmd{font-family:var(--mono);font-size:.76rem;color:var(--amber);
     background:rgba(255,176,46,.07);border:1px dashed rgba(255,176,46,.35);
     padding:5px 8px;user-select:all}
.cmd::before{content:"$ ";color:var(--dim)}

/* ---------- palettes ---------- */
.pals{display:grid;gap:14px;grid-template-columns:repeat(auto-fill,minmax(320px,1fr))}
.pal{border:1px solid var(--line);background:var(--panel);padding:14px 15px}
.pal h3{font-size:.98rem;margin:0 0 3px;font-weight:600}
.pal .pid{font-family:var(--mono);color:var(--hot);font-size:.78rem}
.ramp{display:flex;height:34px;margin:11px 0 9px;border:1px solid var(--line)}
.ramp i{flex:1}
.pal p{font-size:.83rem;color:#b6b3ab;margin:0}

.steps{counter-reset:s;list-style:none;padding:0;margin:0;max-width:760px;
  display:grid;gap:10px}
.steps li{counter-increment:s;position:relative;padding:13px 16px 13px 54px;
  border:1px solid var(--line);background:var(--panel);font-size:.95rem}
.steps li::before{content:counter(s);position:absolute;left:0;top:0;bottom:0;width:40px;
  display:flex;align-items:center;justify-content:center;font-family:var(--mono);
  font-weight:600;color:var(--void);background:var(--amber)}
.steps b{color:var(--amber);font-weight:600}
.steps i{color:var(--beam);font-style:normal}
.cmd.big{display:block;max-width:760px;font-size:.92rem;padding:13px 16px}
footer{border-top:1px solid var(--line);margin-top:56px;padding:30px 0 60px;
       font-family:var(--mono);font-size:.8rem;color:var(--dim)}
footer a{color:var(--amber)}

@media (prefers-reduced-motion:reduce){*{transition:none!important}}
"""

def card(p):
    s = p["sc"] or {}
    star = " star" if p["n"] in top10 else ""
    return f"""<article class="card{star}" id="p{p['n']:03d}">
  <div class="shot"><span class="num">{p['n']:03d}</span>
    <img loading="lazy" src="{p['preview']}" alt="Pattern {p['n']:03d} — {esc(p['name'])}"></div>
  <div class="body">
    <h3 class="name">{esc(p['name'])}</h3>
    <p class="look">{esc(p['look'])}</p>
    <code class="cmd">JD_MODE={p['n']} ./dazzle64</code>
    <div class="meta"><span>{esc(p['family'])}</span>
      <span class="score">{s.get('total','—')}/30</span></div>
  </div>
</article>"""

def pal_card(p):
    s = p["sc"] or {}
    chips = "".join(f'<i style="background:#{c}"></i>' for c in p["colors"])
    return f"""<article class="pal">
  <span class="pid">{p['pid']}</span>
  <h3>{esc(p['name'])}</h3>
  <div class="ramp">{chips}</div>
  <p>{esc(p['look'])}</p>
  <div class="meta"><span>{len(p['colors'])} colors</span>
    <span class="score">{s.get('total','—')}/30</span></div>
</article>"""

fam_html = []
for fam in FAMILIES:
    members = [p for p in patterns if p["family"] == fam]
    if not members: continue
    fam_html.append(f'<h3 class="fam" id="f{FAMILIES.index(fam)}">{esc(fam)} '
                    f'<span style="color:#3a4256">· {members[0]["n"]:03d}–{members[-1]["n"]:03d}</span></h3>')
    fam_html.append('<div class="grid">' + "".join(card(p) for p in members) + '</div>')

jump = "".join(f'<a href="#f{i}">{esc(f)}</a>' for i, f in enumerate(FAMILIES))

HTML = f"""<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>JellyDazzle — Pattern Lab</title>
<meta name="description" content="{len(patterns)} kaleidoscope pattern modes and {len(palettes)} palettes from JellyDazzle, a DAZZLE.EXE homage written in ARM64 assembly.">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;600&family=IBM+Plex+Sans:wght@400;600&display=swap" rel="stylesheet">
<style>{CSS}</style>
</head><body>

<header class="boot"><div class="wrap">
  <p class="tag">LIBCSystems · jelia.nyc</p>
  <h1>JellyDazzle <span class="dim">Pattern Lab</span></h1>
  <p class="lede">Thirty years after the DOS kaleidoscope <b>DAZZLE.EXE</b>, every routine
    rebuilt in hand-written ARMv9.2-A assembly and C — <b>{len(patterns)} pattern modes</b>
    and <b>{len(palettes)} palettes</b>, shuffled so it never repeats itself.
    Each mode below runs on its own by number.</p>
  <p class="statline">
    <span><b>{len(patterns)}</b> modes</span>
    <span><b>{len(palettes)}</b> palettes</span>
    <span><b>124</b> routines shipping</span>
    <span><b>~175</b> fps, single thread</span>
    <span><b>0</b> floating-point in the assembly core</span>
  </p>
  <a class="cta" href="https://github.com/LIBCSYS/JellyDazzle/releases/latest">Download for macOS</a>
  <a class="cta ghost" href="https://github.com/LIBCSYS/JellyDazzle">Source on GitHub</a>

  <div class="sheet"><img src="patterns_montage.png"
      alt="Contact sheet of all {len(patterns)} JellyDazzle patterns"></div>
  <p class="sheetcap">All {len(patterns)} modes, one frame each · full catalogue below</p>
</div></header>

<div class="wrap">

<section id="open">
  <h2>First launch — 30 seconds, once</h2>
  <p class="sub">macOS says <b>“Apple could not verify JellyDazzle is free of malware.”</b>
    That is Gatekeeper flagging any app without a paid Apple notarization certificate.
    Nothing is wrong with the app — every line of source is public. Here is the way through,
    and you only do it the first time:</p>

  <ol class="steps">
    <li><b>Double-click</b> the app once and let it get refused.</li>
    <li>Open <b>System Settings → Privacy &amp; Security</b>.</li>
    <li>Scroll to the bottom — there is a line saying
        <i>“JellyDazzle was blocked to protect your Mac.”</i></li>
    <li>Click <b>Open Anyway</b>, confirm with Touch ID or your password.</li>
    <li>It launches. Every launch after that is a normal double-click.</li>
  </ol>

  <p class="sub" style="margin:22px 0 8px">Or the one-liner, which works regardless —
    paste it in Terminal after downloading:</p>
  <code class="cmd big">xattr -dr com.apple.quarantine ~/Downloads/JellyDazzle.app</code>

  <p class="sub" style="margin-top:22px">Apple Silicon Macs (M1 or newer) · ESC quits ·
    nothing to install, SDL is bundled inside · this whole dance disappears once the
    Developer ID certificate is in place.</p>
</section>

<section id="modes">
  <h2>Mode index</h2>
  <p class="sub">Every pattern runs standalone — set <code style="color:var(--amber)">JD_MODE</code>
    to its number and that mode fills the screen. ★ marks the ten the curators scored highest.
    Scores are dazzle-vibe + cohesion + motion, out of 30.</p>
  <nav class="jump">{jump}</nav>
  {''.join(fam_html)}
</section>

<section id="palettes">
  <h2>Palette bank</h2>
  <p class="sub">Color schemes the engine crossfades between — curated from community
    palettes and designed harmonies. The running app carries 30 of them.</p>
  <div class="pals">{''.join(pal_card(p) for p in palettes)}</div>
</section>

</div>

<footer><div class="wrap">
  JellyDazzle · MIT · built by J and M5 at
  <a href="https://jelia.nyc">jelia.nyc</a> ·
  <a href="https://github.com/LIBCSYS/JellyDazzle">github.com/LIBCSYS/JellyDazzle</a><br>
  In memory of every hour spent staring at DAZZLE.EXE.
</div></footer>

</body></html>"""

open(os.path.join(LAB, "gallery.html"), "w").write(HTML)
print(f"gallery.html: {len(patterns)} patterns, {len(palettes)} palettes, "
      f"{len(HTML)//1024} KB")
