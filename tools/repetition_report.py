#!/usr/bin/env python3
"""repetition_report.py — repetition report for JellyDazzle telemetry CSVs (JD_TELEMETRY).

  repetition_report.py session REGISTRY session.csv      one long run
  repetition_report.py launches REGISTRY launch_*.csv    many short launches
"""
import csv, re, sys, statistics as st
from collections import Counter, defaultdict

FPS = 60
NASM = 24
ROLE = ["ground", "field", "figure", "spark"]


def families(reg):
    txt = open(reg).read()
    body = txt.split("jd_pattern_family[] = {", 1)[1].split("};", 1)[0]
    return [int(x) for x in re.findall(r"\d+", body)]


def fam_of(fam, v):
    return 100 if v < NASM else fam[v - NASM]


def spawns(path):
    with open(path) as f:
        return [r for r in csv.DictReader(f) if r["event"] == "spawn"]


def pct(xs, p):
    xs = sorted(xs)
    return xs[min(len(xs) - 1, int(p / 100 * len(xs)))] if xs else 0


def gaps(rows, key):
    last, out = {}, []
    for r in rows:
        k, f = key(r), int(r["frame"])
        if k in last:
            out.append((f - last[k]) / FPS)
        last[k] = f
    return out


def session(reg, path):
    fam = families(reg)
    rows = spawns(path)
    nlib = NASM + len(fam)
    f0, f1 = int(rows[0]["frame"]), int(rows[-1]["frame"])
    mins = (f1 - f0) / FPS / 60
    print(f"session: {len(rows)} spawns over {mins:.1f} min "
          f"({len(rows) / mins:.1f}/min), library {nlib} routines")
    seen = {int(r["routine"]) for r in rows}
    print(f"coverage: {len(seen)} distinct routines = {100 * len(seen) / nlib:.0f}% of library")
    by_role = defaultdict(list)
    for r in rows:
        by_role[int(r["role"])].append(r)
    for k, rr in sorted(by_role.items()):
        d = {int(r["routine"]) for r in rr}
        print(f"  {ROLE[k]:6}: {len(rr):4} spawns, {len(d):3} distinct")

    g = gaps(rows, lambda r: r["routine"])
    print(f"\nsame ROUTINE returns: {len(g)} repeats; gap min {min(g, default=0):.0f}s, "
          f"p10 {pct(g, 10):.0f}s, median {pct(g, 50):.0f}s")
    for w in (60, 180, 300):
        print(f"  back within {w:3}s: {sum(x < w for x in g)}")
    c = Counter(r["name"] for r in rows)
    print("  most played:", ", ".join(f"{n} x{k}" for n, k in c.most_common(6)))

    fg = gaps([r for r in rows if fam_of(fam, int(r["routine"])) not in (0, 100)],
              lambda r: fam_of(fam, int(r["routine"])))
    print(f"\nsame SHAPE FAMILY returns (tagged families): {len(fg)}; "
          f"median gap {pct(fg, 50):.0f}s; within 30s: {sum(x < 30 for x in fg)}, "
          f"within 60s: {sum(x < 60 for x in fg)}")
    fc = Counter(fam_of(fam, int(r["routine"])) for r in rows)
    tot = len(rows)
    print("  family share of spawns:", ", ".join(
        f"{'asm' if k == 100 else 'f' + str(k)} {100 * v / tot:.0f}%" for k, v in fc.most_common(8)))

    gr = [r for r in rows if r["slot"] == "0"]
    gg = gaps(gr, lambda r: r["routine"])
    print(f"\nGROUND (slot 0): {len(gr)} changes, every {(f1 - f0) / FPS / max(1, len(gr)):.0f}s; "
          f"{len({r['routine'] for r in gr})} distinct; repeat gap min {min(gg, default=0):.0f}s "
          f"median {pct(gg, 50):.0f}s")

    legs = {}
    for r in rows:
        legs[int(r["leg"])] = r["schemeA"]
    sch = [legs[k] for k in sorted(legs)]
    sg = []
    last = {}
    for i, s in enumerate(sch):
        if s in last:
            sg.append((i - last[s]) * 1024 / FPS)
        last[s] = i
    print(f"\nCOLOUR legs sampled: {len(sch)}, distinct schemes {len(set(sch))}; "
          f"scheme back within 5 min: {sum(x < 300 for x in sg)}")


def launches(reg, paths):
    fam = families(reg)
    opens, ofam, oscheme = [], [], []
    for p in sorted(paths, key=lambda s: int(re.findall(r"\d+", s)[-1])):
        rows = spawns(p)
        if not rows:
            continue
        o = next((r for r in rows if r["slot"] == "0"), rows[0])
        opens.append(o["name"])
        ofam.append(fam_of(fam, int(o["routine"])))
        oscheme.append(o["schemeA"])
    n = len(opens)
    print(f"launches: {n}")
    print(f"  distinct opening grounds: {len(set(opens))}/{n}; repeats: "
          f"{[k for k, v in Counter(opens).items() if v > 1] or 'none'}")
    back = sum(opens[i] in opens[max(0, i - 10):i] for i in range(n))
    print(f"  opener repeated within previous 10 launches: {back}")
    print(f"  opening family repeated vs previous launch: "
          f"{sum(ofam[i] == ofam[i - 1] for i in range(1, n))}/{n - 1}")
    print(f"  distinct opening colour schemes: {len(set(oscheme))}/{n}; same as previous launch: "
          f"{sum(oscheme[i] == oscheme[i - 1] for i in range(1, n))}")
    print("  first 12 openers:", " | ".join(opens[:12]))


if __name__ == "__main__":
    mode, reg, *files = sys.argv[1:]
    (session if mode == "session" else launches)(reg, *([files[0]] if mode == "session" else [files]))
