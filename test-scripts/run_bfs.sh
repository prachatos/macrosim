#!/bin/bash

mkdir -p logs

# --- Naive Scaled Runs (using real/128 as app) ---
for scale in 256 512 1024; do
  pes=$((scale / 64))
  sst --stop-at=2ms --print-timing-info=true \
  test-scripts/macrosim-FatTree-ZAP.py -- \
  --shape="1,1:2" --pes=$pes --zaps=4 --zones=8 \
  --app=final/bfs/real/128 \
  --stat_path=final_res_csv/bfs_naive_$scale.csv \
  --enableStatistics 1 --demo_mode=0 --event_driven_send=1 --event_driven_recv=1 \
  --read_slots=520 --clock_speed=2GHz --max_aggr_size=0 > logs/bfs_naive_${scale}.log 2>&1 &
done

# --- Real Runs ---
for scale in 64 128 256 512 1024; do
  pes=$((scale / 64))
  sst --stop-at=2ms --print-timing-info=true \
  test-scripts/macrosim-FatTree-ZAP.py -- \
  --shape="1,1:2" --pes=$pes --zaps=4 --zones=8 \
  --app=final/bfs/real/$scale \
  --stat_path=final_res_csv/bfs_real_$scale.csv \
  --enableStatistics 1 --demo_mode=0 --event_driven_send=1 --event_driven_recv=1 \
  --read_slots=520 --clock_speed=2GHz --max_aggr_size=0 > logs/bfs_real_${scale}.log 2>&1 &
done

# --- Scaled Runs ---
for scale in 256 512 1024; do
  pes=$((scale / 64))
  sst --stop-at=2ms --print-timing-info=true \
  test-scripts/macrosim-FatTree-ZAP.py -- \
  --shape="1,1:2" --pes=$pes --zaps=4 --zones=8 \
  --app=final/bfs/scaled/$scale \
  --stat_path=final_res_csv/bfs_scaled_$scale.csv \
  --enableStatistics 1 --demo_mode=0 --event_driven_send=1 --event_driven_recv=1 \
  --read_slots=520 --clock_speed=2GHz --max_aggr_size=0 > logs/bfs_scaled_${scale}.log 2>&1 &
done
