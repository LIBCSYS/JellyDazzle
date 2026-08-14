#!/bin/sh
# lab/qa/variety_run.sh — 12 independent JellyDazzle launches, 60 s each.
#
#   ./lab/qa/variety_run.sh <outdir> [frames] [w] [h]
#
# Start frames are 12 draws from the same domain main.c uses (rand() &
# 0x3FFFFF), fixed here so the battery is reproducible.  Runs are SEQUENTIAL
# on purpose: probe_step() is budgeted in wall-clock ms, so launches racing
# each other for a core would each probe less of the library than a real
# launch does and the variety numbers would be a fiction.
set -e
OUT=${1:-/tmp/jdvar/out}
FR=${2:-3600}
W=${3:-1280}
H=${4:-960}
mkdir -p "$OUT"

SEEDS="123456 417022 1039284 1583421 1996488 2244532 2699421 2938103 3157482 3506611 3812004 4109337"

for s in $SEEDS; do
    /tmp/variety "$s" "$FR" "$W" "$H" > "$OUT/launch_$s.txt" 2>/dev/null
    printf '%s ' "$s"
done
echo "done -> $OUT"
