
import sys
tot_recv = 0
tot_sent = 0
tot_proc = 0
tot_recv_prec = 0
tot_sent_prec= 0
tot_sent_zone = 0
tot_recv_prec_inc = 0
num_ranks = int(sys.argv[1])
time_in_ms = float(sys.argv[2])
tot_send_ins = 0
tot_recv_ins = 0
mem_stall = 0
mem_ops = 0
proc_time = 0
max_proc_time = 0
min_proc_time = 9999
tot_recv = 0
tot_recv_int = 0

num_pes = 0
tot_inac = 0
tot_idl = 0
max_last_proc = 0
max_last_sent = 0
tot_sent = 0
tot_stalls = 0
pe_mem_stalls = 0


num_ranks = int(sys.argv[1])
time_in_ms = float(sys.argv[2])
if len(sys.argv) > 3:
    fname = sys.argv[3]
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
    global tot_recv, num_pes, tot_idl, tot_inac, max_last_proc, max_last_sent
    global tot_send_ins, tot_recv_ins, tot_proc, tot_recv_prec, tot_sent_prec
    global tot_sent_zone, tot_recv_prec_inc, mem_stall, mem_ops, proc_time
    global min_proc_time, max_proc_time
    global tot_sent, tot_recv, tot_stalls
    global tot_recv_int
    for l in f:
        r = l.split(',')
        try:
            if 'StallCycleBP' in r[1]:
                tot_stalls += int(r[6]) # max
                num_pes += 1
            elif 'IdleCycles' in r[1]:
                tot_idl += int(r[6]) # max
            elif 'InactiveCycles' in r[1]:
                tot_inac += int(r[6]) # max
            elif 'LastProcCycle' in r[1]:
                max_last_proc = max(max_last_proc, int(r[10]))
            elif 'LastSentCycle' in r[1]:
                max_last_sent = max(max_last_sent, int(r[10]))
            elif 'TotalSendIns' in r[1]:
                tot_send_ins += int(r[6])
            elif 'TotalRecvIns' in r[1]:
                tot_recv_ins += int(r[6])
            elif 'TotalRecvInternal' in r[1]:
                tot_recv_int += int(r[6])
            elif 'TotalProc' in r[1]:
                tot_proc += int(r[6]) # max
            elif 'TotRecvPrecinct' in r[1] and 'Inc' not in r[1]:
                tot_recv_prec += int(r[6]) # max
            elif 'TotSentPrecinct' in r[1]:
                tot_sent_prec += int(r[6]) # max
            elif 'TotalSentZone' in r[1]:
                tot_sent_zone += int(r[6]) # max
            elif 'TotRecvPrecinctInc' in r[1]:
                tot_recv_prec_inc += int(r[6])
            elif 'MemStalls' in r[1]:
                pe_mem_stalls += int(r[6])
            elif 'StalledOps' in r[1]:
                mem_stall += int(r[6])
            elif 'TotalNumReads' in r[1]:
                mem_ops += int(r[6])
            elif 'ProcessTime' in r[1]:
                proc_time += int(r[6])
                min_proc_time = min(int(r[-2]), min_proc_time)
                max_proc_time = max(int(r[-1]), max_proc_time)
            elif 'TotalRecv' in r[1]:
                tot_recv += int(r[6])
            elif 'TotalSent' in r[1] and 'Zone' not in r[1]:
                tot_sent += int(r[6])
        except Exception: pass

print(num_ranks)
if num_ranks > 1:
    for i in range(num_ranks):
        with open(fname + str(i) + ext, 'r') as f:
            parse(f)
else:
    print("Parse", fname, ext)
    with open(fname + ext, 'r') as f:
        parse(f)

print(tot_stalls, "stalls in", num_pes, "ZAPs")
print(max_last_proc, "was the last processed cycle")
print(max_last_sent, "was the last sent cycle")


print(tot_sent, "sent in", time_in_ms, "ms")
print(tot_recv_prec, "received in precinct in", time_in_ms, "ms")
print(tot_sent_prec, "sent in precinct in", time_in_ms, "ms")
print(tot_recv, "received in", time_in_ms, "ms")
print(tot_recv_int, "received from same precinct in", time_in_ms, "ms")
print(tot_recv - tot_recv_int, "received from diff precinct in", time_in_ms, "ms")
print(tot_proc, "processed in", time_in_ms, "ms")
if tot_sent != 0:
    print(tot_send_ins, "send instructions in", time_in_ms, "ms. Average handler length: %.2f" % (tot_send_ins/tot_sent))
else:
    print(tot_send_ins, "send instructions in", time_in_ms, "ms.")
if tot_proc != 0:
    print(tot_recv_ins, "receive instructions in", time_in_ms, "ms. Average handler length: %.2f" % (tot_recv_ins/tot_proc))
else:
    print(tot_recv_ins, "send instructions in", time_in_ms, "ms.")
print(mem_stall, "memory internal stalls in", time_in_ms, "ms")
print(mem_ops, "total mem ops")
if mem_ops > 0:
    print("Avg svc time in ns:", proc_time * 1.0/mem_ops, ",range:", min_proc_time, "->", max_proc_time, "ns")