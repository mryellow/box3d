#!/bin/bash
# Interleaved A/B: per round, per scene, alternate which side runs first.
S=/tmp/claude-1000/-home-user-box3d/413a2160-2fc7-46df-b3a3-9f951ac9be05/scratchpad
OUT=$S/results; mkdir -p $OUT/raw $OUT/cwd
CSV=$OUT/results.csv
echo "round,scene,side,order,threads,run_ms,load1_before,memavail_mb_before" > $CSV
ROUNDS=${1:-10}
MAXLOAD=1.5
SCENES="convex_pile joint_grid junkyard large_pyramid large_world many_pyramids mesh_drop post_field rain sleep sleeping_grid spinner trees100 trees50 trees25 village washer"
run() { # round scene side order
  local r=$1 sc=$2 side=$3 ord=$4
  local raw=$OUT/raw/r${r}_${sc}_${side}.txt
  local load=$(cut -d' ' -f1 /proc/loadavg) mem=$(awk '/MemAvailable/{print int($2/1024)}' /proc/meminfo)
  (cd $OUT/cwd && $S/wt-$side/build/bin/benchmark -b=$sc -t=8 -r=1 > $raw 2>&1)
  awk -v r=$r -v sc=$sc -v side=$side -v o=$ord -v l=$load -v m=$mem '
    /^thread count:/ {t=$3}
    /^run [0-9]+ :/ {print r","sc","side","o","t","$4","l","m}' $raw >> $CSV
}
for r in $(seq 1 $ROUNDS); do
  # wait for a quiet machine (load1 decays slowly, so also log if we waited)
  w=0; while [ "$(awk -v m=$MAXLOAD '{print ($1>m)}' /proc/loadavg)" = 1 ] && [ $w -lt 60 ]; do sleep 30; w=$((w+1)); done
  echo "round $r start $(date +%T) load=$(cut -d' ' -f1-3 /proc/loadavg) waited=${w}x30s" >> $OUT/progress.log
  i=0
  for sc in $SCENES; do
    i=$((i+1))
    if [ $(( (r+i) % 2 )) -eq 0 ]; then first=main; second=master; else first=master; second=main; fi
    if [ "$sc" = post_field ] || [ "$sc" = sleeping_grid ]; then run $r $sc master 1; continue; fi
    run $r $sc $first 1; run $r $sc $second 2
  done
  echo "round $r done $(date +%T)" >> $OUT/progress.log
done
echo ALLDONE >> $OUT/progress.log
