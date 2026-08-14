#!/usr/bin/env python3
"""lab/qa/variety_report.py — turn a variety_run.sh output dir into the table.

  ./lab/qa/variety_report.py <outdir> [label]

Reports, per launch: distinct routines, spawns, repeats inside the window,
distinct palette schemes, the frame the probe sweep finished, and the opening
routine.  Then the cross-launch overlap matrix (Jaccard and raw shared count)
for routines and for palettes, plus the library-span of each launch's routine
set — the number that exposes a probe order that is a rotation rather than a
permutation, because a rotated order hands every launch a block of neighbours.
"""
import sys, os, glob, itertools, statistics


def load(path):
    d = {"rts": set(), "pals": set(), "spawns": [], "elig": set()}
    for line in open(path):
        t = line.split()
        if not t:
            continue
        if t[0] == "RUN":
            d["start"], d["g_run"] = int(t[1]), int(t[2])
        elif t[0] == "SPAWN":
            d["spawns"].append((int(t[1]), int(t[2]), int(t[3]), int(t[4]), int(t[5])))
        elif t[0] == "PROG":
            d["probe_i"], d["nprobed"], d["probe_done"] = int(t[1]), int(t[2]), int(t[3])
        elif t[0] == "SUM":
            (d["distinct"], d["nspawn"], d["repeats"], d["npal"],
             d["probe_frame"], d["first_rt"]) = map(int, t[2:8])
        elif t[0] == "RTS":
            d["rts"] = set(map(int, t[1:]))
        elif t[0] == "PALS":
            d["pals"] = set(map(int, t[1:]))
        elif t[0] == "ELIG":
            d["elig"] = set(map(int, t[1:]))
    return d


def families(rts):
    """Set-overlap is a weak proxy for "these two launches look alike".
    Patterns in this library were generated in BATCHES (lab/_gen_p31_p50.py,
    _gen_p51_p70.py, _gen_p71_p90.py, ...), so neighbouring numbers are
    siblings that share a look.  Two launches can share zero routine IDs and
    still be the same show if both drew 8 members of one batch.  Family =
    20-pattern block, which is how the batches were cut."""
    return {(r - 24) // 20 for r in rts if r >= 24}


def span(rts):
    """How wide a slice of the 201-pattern library a launch's material came
    from.  A rotated probe order pins this near the count itself."""
    pats = sorted(r - 24 for r in rts if r >= 24)
    return (max(pats) - min(pats) + 1) if pats else 0


def main():
    outdir = sys.argv[1]
    label = sys.argv[2] if len(sys.argv) > 2 else os.path.basename(outdir)
    files = sorted(glob.glob(os.path.join(outdir, "launch_*.txt")),
                   key=lambda p: int(p.rsplit("_", 1)[1].split(".")[0]))
    L = [load(f) for f in files]
    if not L:
        sys.exit("no launches in " + outdir)

    print(f"### {label} — {len(L)} launches x 3600 frames (60 s @ 1280x960)\n")
    print("| launch (start frame) | distinct routines | spawns | repeats | "
          "distinct palettes | probe done (frame) | opening routine | library span "
          "| families |")
    print("|---|---|---|---|---|---|---|---|---|")
    for d in L:
        pd = d["probe_frame"]
        print(f"| {d['start']} | {d['distinct']} | {d['nspawn']} | {d['repeats']} "
              f"| {d['npal']} | {pd if pd >= 0 else 'NOT DONE'} | {d['first_rt']} "
              f"| {span(d['rts'])} | {len(families(d['rts']))} |")

    dist = [d["distinct"] for d in L]
    reps = [d["repeats"] for d in L]
    spans = [span(d["rts"]) for d in L]
    fams = [len(families(d["rts"])) for d in L]
    print(f"\n- distinct routines per launch: min {min(dist)} mean "
          f"{statistics.mean(dist):.1f} max {max(dist)}")
    print(f"- launches that repeat a routine inside 60 s: "
          f"{sum(1 for r in reps if r)}/{len(L)} (total repeats {sum(reps)})")
    print(f"- library span of a launch's material: mean {statistics.mean(spans):.0f} "
          f"of 201 patterns (min {min(spans)}, max {max(spans)})")
    print(f"- generator families per launch (20-pattern batches, max 11): mean "
          f"{statistics.mean(fams):.1f} (min {min(fams)}, max {max(fams)})")

    # overlap
    def overlap(key):
        pair, shared, jac = [], [], []
        for a, b in itertools.combinations(range(len(L)), 2):
            s = L[a][key] & L[b][key]
            u = L[a][key] | L[b][key]
            pair.append((a, b))
            shared.append(len(s))
            jac.append(len(s) / len(u) if u else 0.0)
        return pair, shared, jac

    for d in L:
        d["fams"] = families(d["rts"])
    for key, name in (("rts", "ROUTINE"), ("fams", "FAMILY"), ("pals", "PALETTE")):
        pair, shared, jac = overlap(key)
        n = len(shared)
        print(f"\n**{name} overlap across all {n} launch pairs**")
        print(f"- mean shared: {statistics.mean(shared):.2f}   "
              f"max shared: {max(shared)}   pairs with 0 shared: "
              f"{sum(1 for s in shared if s == 0)}/{n}")
        print(f"- mean Jaccard: {statistics.mean(jac):.3f}   "
              f"max Jaccard: {max(jac):.3f}")
        worst = max(range(n), key=lambda i: shared[i])
        a, b = pair[worst]
        print(f"- worst pair: launch {L[a]['start']} vs {L[b]['start']} — "
              f"{shared[worst]} shared, {sorted(L[a][key] & L[b][key])}")

    # opening material — what the first 15 s actually shows
    print("\n**Opening 15 s (first 900 frames) — the part a friend sees "
          "before deciding it's the same app twice**")
    print("| launch | routines in first 900 frames |")
    print("|---|---|")
    firsts = []
    for d in L:
        r = sorted({s[2] for s in d["spawns"] if s[0] < 900})
        firsts.append(set(r))
        print(f"| {d['start']} | {r} |")
    pair_sh = [len(a & b) for a, b in itertools.combinations(firsts, 2)]
    print(f"\n- opening-set mean shared: {statistics.mean(pair_sh):.2f}, "
          f"max {max(pair_sh)}, identical openings: "
          f"{sum(1 for a, b in itertools.combinations(firsts, 2) if a == b)}")
    print(f"- distinct opening routines across all launches: "
          f"{len(set().union(*firsts))}")


if __name__ == "__main__":
    main()
