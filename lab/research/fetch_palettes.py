#!/usr/bin/env python3
"""Mine Lospec palettes across tags/moods, dedupe against reference/palettes.json."""
import json, re, subprocess, time, sys

BASE = "/Users/exeter/dev/m5/assembly/dzzle1"
existing = {p["slug"] for p in json.load(open(f"{BASE}/reference/palettes.json"))}

HEX6 = re.compile(r"^[0-9a-fA-F]{6}$")

def fetch(tag, page):
    url = (f"https://lospec.com/palette-list/load?colorNumberFilterType=min"
           f"&colorNumber=8&page={page}&tag={tag}&sortingType=downloads")
    try:
        out = subprocess.run(["curl", "-s", "--max-time", "30", url],
                             capture_output=True, text=True, timeout=40).stdout
        return json.loads(out).get("palettes", [])
    except Exception as e:
        print(f"  ! {tag} p{page}: {e}", file=sys.stderr)
        return []

# (tag, pages) — '' = untagged deeper pages
PLAN = [
    ("neon", 2), ("pastel", 2), ("nature", 2), ("metallic", 1),
    ("gameboy", 1), ("sunset", 2), ("vaporwave", 1), ("horror", 1),
    ("gold", 1), ("retro", 1), ("space", 1), ("autumn", 1),
    ("winter", 1), ("ocean", 1), ("forest", 1), ("desert", 1),
    ("cyberpunk", 1), ("warm", 1), ("cold", 1), ("monochrome", 1),
    ("earthy", 1), ("fantasy", 1), ("night", 1), ("candy", 1),
    ("", 6),  # untagged deep pages 2..7
]

pool = []       # ordered result
seen = set(existing)
tag_hits = {}   # tag -> [slugs collected under it]

for tag, npages in PLAN:
    start = 2 if tag == "" else 0
    for page in range(start, start + npages):
        pals = fetch(tag, page)
        label = tag or "untagged"
        for p in pals:
            slug = p.get("slug")
            colors = [c.lower() for c in p.get("colors", []) if HEX6.match(str(c))]
            if not slug or slug in seen:
                continue
            if not (8 <= len(colors) <= 64):
                continue
            seen.add(slug)
            pool.append({"slug": slug, "colors": colors,
                         "_tag": label, "_n": len(colors),
                         "_downloads": p.get("downloads", "")})
            tag_hits.setdefault(label, []).append(slug)
        print(f"{label} p{page}: {len(pals)} fetched, pool now {len(pool)}")
        time.sleep(0.6)

# save pool (strip private fields)
out = [{"slug": p["slug"], "colors": p["colors"]} for p in pool]
with open(f"{BASE}/lab/research/palettes_pool.json", "w") as f:
    json.dump(out, f, indent=2)

with open(f"{BASE}/lab/research/_meta.json", "w") as f:
    json.dump({"tag_hits": tag_hits,
               "pool": [{k: p[k] for k in ("slug", "_tag", "_n", "_downloads")} for p in pool]},
              f, indent=2)

print(f"\nTOTAL NEW: {len(pool)} (deduped against {len(existing)} existing)")
