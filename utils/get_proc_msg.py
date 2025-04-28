
import sys
tot_proc = 0

num_ranks = int(sys.argv[1])
if len(sys.argv) > 2:
    fname = sys.argv[2]
else:
    fname = "StatisticOutput_"
    if num_ranks == 1:
        fname = "StatisticOutput"
try:
    ext = sys.argv[4]
    if ext == "none":
        ext = ""
except Exception:
    ext = ".csv"

def parse(f):
    global tot_proc
    for l in f:
        r = l.split(',')
        try:
            if 'TotalProc' in r[1]:
                tot_proc += int(r[6]) # max
        except Exception: pass

if num_ranks > 1:
    for i in range(num_ranks):
        with open(fname + str(i) + ext, 'r') as f:
            parse(f)
else:
    with open(fname + ext, 'r') as f:
        parse(f)

print(tot_proc, "processed")