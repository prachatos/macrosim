import pandas as pd
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser(description="Plot RecvDiff normalized to baseline.")
parser.add_argument("csv_file", type=str, help="Path to input CSV file")
parser.add_argument("output_file", type=str, help="Path to output graph (e.g., plot.pdf or plot.png)")
args = parser.parse_args()

plt.rcParams.update({
    'axes.labelsize': 24,
    'xtick.labelsize': 24,
    'ytick.labelsize': 18,
    'legend.fontsize': 20
})
df = pd.read_csv(args.csv_file)

df["Approach"] = df["File"].apply(lambda x: "/".join(x.split("/")[-3:]))

# Extract aggr and policy labels
def parse_approach(row):
    parts = row["Approach"].split("/")
    aggr = parts[1].replace("aggr_", "")
    suffix = parts[2].replace(".csv", "")
    if suffix == "no_max":
        label = "Send-Immediately"
    elif "max_inf" in suffix:
        label = "Size-Only"
    elif "max_" in suffix:
        label = f"Time-Or-Size ({suffix.split('_')[-1]})"
    elif suffix == "baseline":
        label = "Baseline"
    else:
        label = suffix
    return pd.Series([int(aggr), label])

df[["Agg", "Policy"]] = df.apply(parse_approach, axis=1)

# Normalize to baseline
baseline = df[df["Policy"] == "Baseline"]["RecvDiff"].values[0]
df["RecvDiff_orig"] = df["RecvDiff"] / baseline  # original normalized values
df["RecvDiff_to_baseline"] = df["RecvDiff_orig"].clip(lower=0.5)

# Filter out baseline
filtered_df = df[df["Agg"] < 500]
filtered_df = filtered_df[filtered_df["Agg"] > 0]

marina_colors = ['#DDA0DD', '#8FBC8F', '#FAA460', '#CD5C5C', '#9370DB', '#6495ED', '#66CDAA']
markers = ['o', 's', 'D', '^', 'v', '<', '>', 'p', '*']
linestyles = ['-', '--', '-.', ':']

# Plot
fig, ax = plt.subplots(figsize=(8, 5), constrained_layout=True)

for idx, (policy, group) in enumerate(filtered_df.groupby("Policy")):
    group = group.sort_values("Agg")
    ax.plot(
        group["Agg"], group["RecvDiff_to_baseline"],
        marker=markers[idx % len(markers)],
        linestyle=linestyles[idx % len(linestyles)],
        color=marina_colors[idx % len(marina_colors)],
        label=policy,
        linewidth=4
    )

    for _, row in group.iterrows():
        if row["RecvDiff_orig"] < 0.5:
            ax.text(row["Agg"], row["RecvDiff_to_baseline"] + 0.02,
                    f'{row["RecvDiff_orig"]:.2f}',
                    fontsize=10, ha='center', va='bottom')

import math

ax.axhline(1.0, color='gray', linestyle='--', label='Baseline', linewidth=4)
ax.set_xscale("log")

agg_ticks = sorted(filtered_df["Agg"].unique())
ax.set_xticks(agg_ticks)
ax.set_xticklabels([str(x) for x in agg_ticks])
ax.set_xlabel("Aggregation Degree")
ax.set_ylabel("Norm. External\nMsgs Processed")

ymax = max(filtered_df["RecvDiff_to_baseline"].max(), 1.0)
next_ytick = math.ceil(ymax / 0.2) * 0.2
yticks = list(ax.get_yticks())
if next_ytick > yticks[-1]:
    yticks.append(next_ytick)
    ax.set_yticks(yticks)

ax.legend()
ax.grid(True, which='both', axis='x')

plt.savefig(args.output_file, bbox_inches="tight")
