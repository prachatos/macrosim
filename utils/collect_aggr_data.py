import os
import subprocess
import csv
import statistics
import sys
base_directory = sys.argv[1]
script_path = "utils/get_stall_cycles.py"
command_template = "python3 {} 1 1 {}"
output_file = os.path.join(base_directory, "sum.csv")

def run_and_collect(folder, filename_base):
    try:
        filepath = os.path.join(folder, f"{filename_base}.csv")
        command = command_template.format(script_path, os.path.join(folder, filename_base))
        print(f"Running: {command}")
        result = subprocess.run(command, shell=True, text=True, capture_output=True, check=True)

        processed = recv_prec = recv_diff = None
        for line in result.stdout.splitlines():
            if "received in 1.0 ms" in line:
                recv_prec = line.split()[0]
            elif "processed in 1.0 ms" in line:
                processed = line.split()[0]
            elif "received from diff precinct in" in line:
                recv_diff = line.split()[0]

        return processed, recv_prec, recv_diff
    except Exception as e:
        print(f"Error processing {filename_base}: {e}")
        return None, None

def compute_batch_stats(folder, filename_base):
    nw_file_path = os.path.join(folder, f"{filename_base}_nw_file.csv")
    if not os.path.isfile(nw_file_path):
        return None, None, None

    try:
        sizes = []
        with open(nw_file_path, newline='') as csvfile:
            reader = csv.reader(csvfile)
            for row in reader:
                if len(row) >= 2:
                    try:
                        sizes.append(int(row[1]))
                    except ValueError:
                        continue
        if sizes:
            avg = sum(sizes) / len(sizes)
            med = statistics.median(sizes)
            total = sum(sizes)
            return avg, med, total
    except Exception as e:
        print(f"Error reading {nw_file_path}: {e}")
    return None, None, None


# Collect and process data
collected_data = []

for root, dirs, files in os.walk(base_directory):
    for file in files:
        if file.endswith(".csv") and not file.endswith("file.csv"):
            filename_base = file[:-4]
            processed, sent_prec , recv_diff= run_and_collect(root, filename_base)
            avg_batch, med_batch, total_msgs = compute_batch_stats(root, filename_base)


            if processed or sent_prec:
                collected_data.append((
                    os.path.join(root, file),
                    processed,
                    sent_prec,
                    recv_diff,
                    avg_batch,
                    med_batch,
                    total_msgs
                ))

# Write the final CSV
with open(output_file, "w") as f:
    f.write("File,Processed,RecvTot,RecvDiff,AvgBatchSize,MedianBatchSize,TotalSent\n")
    for file_path, processed, sent_prec, recv_diff, avg_batch, med_batch, total_msgs in collected_data:
        f.write(f"{file_path},{processed},{sent_prec},{recv_diff},{avg_batch},{med_batch},{total_msgs}\n")

print(f"Summary written to {output_file}")
