import os
import subprocess
# bfs 8 pc, else 16 pc
# Parameters
app = "final/tc/scaled/2048"
aggr_values = [10, 25, 50, 100, 250, 500]
zap_shape = "1,1:16"
pes = 1
zaps = 4
zones = 32
read_slots = 520
clock_speed = "2GHz"
stat_base_path = "final_res_csv/aggr_2ms/tc/"

os.makedirs(stat_base_path, exist_ok=True)

base_command = (
    "sst --stop-at=2ms --print-timing-info=true "
    "test-scripts/macrosim-FatTree-ZAP.py -- "
    f"--shape=\"{zap_shape}\" --pes={pes} --zaps={zaps} --zones={zones} --app={app} "
    f"--enableStatistics 1 --demo_mode=0 --event_driven_send=1 --event_driven_recv=1 "
    f"--read_slots={read_slots} --clock_speed={clock_speed} --max_aggr_size={{}} --stat_path={{}} "
    f"--nw_file_name={{}} --lat_file_name={{}}"
)

tasks = []
baseline_config = {
    "only_max": 0,
    "timeout": 0,
    "suffix": "baseline"
}
tasks.append((0, baseline_config))

for aggr in aggr_values:
    configs = [
        {"only_max": 1, "timeout": 10000, "suffix": "max_inf_10k"},
        {"only_max": 1, "timeout": 5, "suffix": "max_5"},
        {"only_max": 1, "timeout": 25, "suffix": "max_25"},
        {"only_max": 0, "timeout": 0, "suffix": "no_max"},
    ]
    for config in configs:
        tasks.append((aggr, config))

processes = []
for aggr, config in tasks:
    stat_path = f"{stat_base_path}/aggr_{aggr}/{config['suffix']}.csv"
    nw_file = f"{stat_base_path}/aggr_{aggr}/{config['suffix']}_nw_file.csv"
    lat_file = f"{stat_base_path}/aggr_{aggr}/{config['suffix']}_lat_file.csv"
    os.makedirs(os.path.dirname(stat_path), exist_ok=True)

    command = base_command.format(aggr, stat_path, nw_file, lat_file)
    command += f" --only_max={config['only_max']}"
    if config["timeout"]:
        command += f" --max_aggr_timeout={config['timeout']}"

    print(f"Launching: {command}")
    proc = subprocess.Popen(command, shell=True)
    processes.append(proc)

for proc in processes:
    proc.wait()
