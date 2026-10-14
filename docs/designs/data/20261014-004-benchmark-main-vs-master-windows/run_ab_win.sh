#!/bin/bash
# Interleaved A/B on Windows via cmd.exe. Per round, per scene, alternate which side runs first.
S=/tmp/claude-1000/-home-user-box3d/413a2160-2fc7-46df-b3a3-9f951ac9be05/scratchpad
OUT=$S/results-win; mkdir -p $OUT/raw
WCWD=/mnt/e/src/box3d-bench/results/cwd; mkdir -p $WCWD
CSV=$OUT/results.csv
echo "round,scene,side,order,threads,run_ms,win_cpu_pct_round_start" > $CSV
ROUNDS=${1:-10}; MAXCPU=15
SCENES="convex_pile joint_grid junkyard large_pyramid large_world many_pyramids mesh_drop post_field rain sleep sleeping_grid spinner trees100 trees50 trees25 village washer"
wincpu() { powershell.exe -NoProfile -Command "[int](Get-CimInstance Win32_Processor | Measure-Object -Property LoadPercentage -Average).Average" 2>/dev/null | tr -d '\r\n '; }
run() { # round scene side order
  local r=$1 sc=$2 side=$3 ord=$4
  local raw=$OUT/raw/r${r}_${sc}_${side}.txt
  (cd $WCWD && cmd.exe /c "E:\\src\\box3d-bench\\$side\\build\\bin\\Release\\benchmark.exe -b=$sc -t=8 -r=1" 2>&1 | tr -d '\r' > $raw)
  awk -v r=$r -v sc=$sc -v side=$side -v o=$ord -v l=$CPU '
    /^thread count:/ {t=$3}
    /^run [0-9]+ :/ {print r","sc","side","o","t","$4","l}' $raw >> $CSV
}
for r in $(seq 1 $ROUNDS); do
  w=0; CPU=$(wincpu); while [ "${CPU:-100}" -gt $MAXCPU ] && [ $w -lt 60 ]; do sleep 30; w=$((w+1)); CPU=$(wincpu); done
  echo "round $r start $(date +%T) win_cpu=${CPU}% waited=${w}x30s" >> $OUT/progress.log
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
