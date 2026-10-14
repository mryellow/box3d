#!/bin/bash
# Interleaved A/B on post_field only: per round, alternate which side runs first.
# base = main + post_field scene (d1b8e3b cherry-picked)
# full = base + b3ShapeTimeOfImpact called with maxFraction 1.0f (the TOI interval is always [0, 1])
S=/tmp/claude-1000/-home-user-box3d/4d4ac9d1-ac4c-473d-b0b5-287e55d7d0e5/scratchpad
OUT=$S/results; mkdir -p $OUT/raw $OUT/cwd
CSV=$OUT/results.csv
echo "round,scene,side,order,threads,run_ms,load1_before,memavail_mb_before" > $CSV
ROUNDS=${1:-10}
MAXLOAD=1.5
sc=post_field
run() { # round side order
  local r=$1 side=$2 ord=$3
  local raw=$OUT/raw/r${r}_${sc}_${side}.txt
  local load=$(cut -d' ' -f1 /proc/loadavg) mem=$(awk '/MemAvailable/{print int($2/1024)}' /proc/meminfo)
  (cd $OUT/cwd && $S/wt-$side/build/bin/benchmark -b=$sc -t=8 -r=1 > $raw 2>&1)
  awk -v r=$r -v sc=$sc -v side=$side -v o=$ord -v l=$load -v m=$mem '
    /^thread count:/ {t=$3}
    /^run [0-9]+ :/ {print r","sc","side","o","t","$4","l","m}' $raw >> $CSV
}
for r in $(seq 1 $ROUNDS); do
  w=0; while [ "$(awk -v m=$MAXLOAD '{print ($1>m)}' /proc/loadavg)" = 1 ] && [ $w -lt 60 ]; do sleep 30; w=$((w+1)); done
  echo "round $r start $(date +%T) load=$(cut -d' ' -f1-3 /proc/loadavg) waited=${w}x30s" >> $OUT/progress.log
  if [ $(( r % 2 )) -eq 0 ]; then first=base; second=full; else first=full; second=base; fi
  run $r $first 1; run $r $second 2
  echo "round $r done $(date +%T)" >> $OUT/progress.log
done
echo ALLDONE >> $OUT/progress.log
