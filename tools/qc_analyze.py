#!/usr/bin/env python3
"""qc_analyze.py — turn a jd_qc CSV (+ JD_DEBUG stderr) into the QC tables.

  python3 tools/qc_analyze.py /tmp/qc_long.csv /tmp/qc_long.err [warmup]
"""
import sys, re, csv, statistics as st

csv_path = sys.argv[1]
err_path = sys.argv[2] if len(sys.argv) > 2 else None
warm = int(sys.argv[3]) if len(sys.argv) > 3 else 600

rows = []
with open(csv_path) as f:
    for r in csv.DictReader(f):
        rows.append((int(r['frame']), float(r['ms']), float(r['delta'])))
f0 = rows[0][0]
byframe = {f: (m, d) for f, m, d in rows}

def pct(v, p):
    v = sorted(v)
    return v[min(len(v) - 1, int(len(v) * p))]

def block(name, sel):
    ms = [m for f, m, d in rows if sel(f)]
    dl = [d for f, m, d in rows if sel(f) and d >= 0]
    if not ms:
        return
    print(f"{name:22s} n={len(ms):6d}  ms p50 {pct(ms,.5):6.2f} p90 {pct(ms,.9):6.2f} "
          f"p99 {pct(ms,.99):6.2f} max {max(ms):7.2f}  | fps p50 {1000/pct(ms,.5):6.1f} "
          f"p99 {1000/pct(ms,.99):6.1f} worst {1000/max(ms):6.1f}  | delta mean {st.mean(dl):5.3f} "
          f"med {pct(dl,.5):5.3f} p99 {pct(dl,.99):5.3f} max {max(dl):5.3f} over8 {sum(1 for d in dl if d>8)}")

print("=== whole run ===")
block("all frames", lambda f: True)
block(f"startup (<{warm}f)", lambda f: f - f0 < warm)
block(f"steady (>= {warm}f)", lambda f: f - f0 >= warm)

# --- events -----------------------------------------------------------------
spawns, jumps, calms, trims, nospawn = [], [], [], [], []
if err_path:
    for line in open(err_path, errors='ignore'):
        m = re.match(r'SPAWN f=(\d+) slot=(\d+) rt=(\d+) role=(\d+) blend=(\d+) peak=(\d+) life=(\d+)', line)
        if m:
            spawns.append(tuple(int(x) for x in m.groups()))
        m = re.match(r'JUMP f=(\d+) slot=(\d+) rt=(\d+)', line)
        if m: jumps.append(tuple(int(x) for x in m.groups()))
        m = re.match(r'CALM f=(\d+) slot=(\d+) rt=(\d+)', line)
        if m: calms.append(tuple(int(x) for x in m.groups()))
        m = re.match(r'TRIM f=(\d+) slot=(\d+) rt=(\d+)', line)
        if m: trims.append(tuple(int(x) for x in m.groups()))
        if line.startswith('NOSPAWN'): nospawn.append(line.strip())

print(f"\nevents: spawns={len(spawns)} jumps={len(jumps)} calms={len(calms)} "
      f"trims={len(trims)} nospawn={len(nospawn)}")

def win(f, lo=-20, hi=40):
    ds = [byframe[x][1] for x in range(f + lo, f + hi + 1) if x in byframe and byframe[x][1] >= 0]
    ms = [byframe[x][0] for x in range(f + lo, f + hi + 1) if x in byframe]
    return (max(ds) if ds else -1, max(ms) if ms else -1)

print("\n=== motion at layer entries (peak delta / peak ms in f-20..f+40) ===")
worst = []
for s in spawns:
    f, slot, rt = s[0], s[1], s[2]
    if f - f0 < warm: continue
    d, m = win(f)
    if d < 0: continue
    worst.append((d, f, slot, rt, m))
worst.sort(reverse=True)
for d, f, slot, rt, m in worst[:15]:
    print(f"  f={f} slot={slot} rt={rt:3d}  peak delta {d:5.3f}  peak ms {m:6.2f}")
if worst:
    print(f"  entries sampled {len(worst)}  max {worst[0][0]:.3f}  "
          f"median {sorted(x[0] for x in worst)[len(worst)//2]:.3f}")

print("\n=== motion at cadence boundaries ===")
for label, mod in (("palette leg %1024", 1024), ("asm canvas %2048", 2048)):
    pts = [f for f, m, d in rows if f % mod == 0 and f - f0 >= warm]
    ws = [(win(f)[0], f) for f in pts]
    ws = [w for w in ws if w[0] >= 0]
    ws.sort(reverse=True)
    if ws:
        print(f"  {label:18s} n={len(ws):3d}  max {ws[0][0]:5.3f} (f={ws[0][1]})  "
              f"median {sorted(x[0] for x in ws)[len(ws)//2]:5.3f}")

print("\n=== worst single frames (delta) ===")
for f, m, d in sorted(rows, key=lambda r: -r[2])[:10]:
    print(f"  f={f} delta {d:6.3f} ms {m:6.2f}")
print("\n=== worst single frames (ms), steady only ===")
for f, m, d in sorted([r for r in rows if r[0] - f0 >= warm], key=lambda r: -r[1])[:10]:
    print(f"  f={f} ms {m:7.2f} delta {d:6.3f}")
