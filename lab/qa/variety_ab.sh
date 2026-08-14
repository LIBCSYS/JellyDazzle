#!/bin/sh
# lab/qa/variety_ab.sh — paired A/B launch-variety battery.
#
#   ./lab/qa/variety_ab.sh <before_binary> <after_binary> <outroot> [frames] [w] [h]
#
# The two builds are run as a PAIR per seed, concurrently, because
# probe_step() spends a wall-clock budget: a build that happens to run on a
# quieter machine probes more of the library and looks more varied for
# reasons that have nothing to do with the change under test.  Pairing them
# puts both arms under identical load for each seed.
set -e
BEF=${1:-/tmp/variety}
AFT=${2:-/tmp/variety_after}
ROOT=${3:-/tmp/jdvar/ab}
FR=${4:-3600}
W=${5:-1280}
H=${6:-960}
mkdir -p "$ROOT/before" "$ROOT/after"

SEEDS="123456 417022 1039284 1583421 1996488 2244532 2699421 2938103 3157482 3506611 3812004 4109337"

for s in $SEEDS; do
    "$BEF" "$s" "$FR" "$W" "$H" > "$ROOT/before/launch_$s.txt" 2>/dev/null &
    p1=$!
    "$AFT" "$s" "$FR" "$W" "$H" > "$ROOT/after/launch_$s.txt" 2>/dev/null &
    p2=$!
    wait $p1 $p2
    printf '%s ' "$s"
done
echo "done -> $ROOT"
