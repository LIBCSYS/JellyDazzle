#!/usr/bin/env python3
"""ct_pool.py - pool `gate contrast` tenancy CSVs and put a confidence
interval on the headline numbers.

    ct_pool.py SLOT tenancy_*.csv       (slot 2 = accent, 5 = all overlays)

The unit of independent information is a TENANCY, not a frame: consecutive
frames inside one tenancy are almost perfectly correlated, so a run of 18k
frames is worth ~8 accent observations, not 3600.  Everything below is a
bootstrap over tenancies, weighted by how long each one was on screen."""
import sys, csv, random, statistics as st

slot = int(sys.argv[1]); rows = []
for path in sys.argv[2:]:
    with open(path) as f:
        for r in csv.DictReader(f):
            if int(r["slot"]) == slot and int(r["present"]) > 0:
                rows.append((int(r["present"]), float(r["vis_frac"]),
                             float(r["v50"]), float(r["gluma"])))
if not rows: sys.exit("no tenancies for slot %d" % slot)

def wmean(sample, idx):
    return (sum(r[0] * r[1 + idx] for r in sample) / sum(r[0] for r in sample))

def boot(idx, n=4000, conf=0.95):
    pt = wmean(rows, idx)
    xs = sorted(wmean([random.choice(rows) for _ in rows], idx) for _ in range(n))
    lo = xs[int((1 - conf) / 2 * n)]; hi = xs[int((1 - (1 - conf) / 2) * n)]
    return pt, lo, hi

random.seed(1)
print("tenancies n = %d   (median %d samples each)" %
      (len(rows), int(st.median(r[0] for r in rows))))
for name, idx in (("visible fraction", 0), ("median V", 1), ("ground luma", 2)):
    p, lo, hi = boot(idx)
    print("%-18s %7.4f   95%% CI [%.4f, %.4f]   half-width %.4f"
          % (name, p, lo, hi, (hi - lo) / 2))
