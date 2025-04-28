import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.topology import *
from sst.merlin.interface import *
from sst.merlin.router import *

import argparse
import configparser
import json
import os
import random
import sys


# Configurable Simulation Parameters
# Full Scale FORZA - 4608 Precincts
# shape="24,24:24,24:8"
# Small system

parser = argparse.ArgumentParser(add_help=True)

parser.add_argument("--shape", help="Precinct NOC configuration", default="1,1:2", type=str)
parser.add_argument("--zones", help="Number of zones per precinct", default=2, type=int)
parser.add_argument("--zaps", help="Number of ZAPs per zone", default=2, type=int)
parser.add_argument("--pes", help="Number of PEs per ZAP", default=512, type=int)
parser.add_argument("--size_min", help="Minimum packet size", default=16, type=int)
parser.add_argument("--size_max", help="Maximum packet size", default=16, type=int)
parser.add_argument("--send_ins_min", help="Minimum instructions in send handler", default=250, type=int)
parser.add_argument("--send_ins_max", help="Maximum instructions in send handler", default=300, type=int)
parser.add_argument("--recv_ins_min", help="Minimum instructions in recv handler", default=250, type=int)
parser.add_argument("--recv_ins_max", help="Maximum instructions in recv handler", default=300, type=int)
parser.add_argument("--send_mem_min", help="Minimum memory ops in send handler", default=100, type=int)
parser.add_argument("--send_mem_max", help="Maximum memory ops  in send handler", default=200, type=int)
parser.add_argument("--recv_mem_min", help="Minimum memory ops in recv handler", default=100, type=int)
parser.add_argument("--recv_mem_max", help="Maximum memory ops  in recv handler", default=200, type=int)
parser.add_argument("--mem_ops_per_cycle", help="Maximum memory ops per cycle", default=32, type=int)
parser.add_argument("--read_slots", help="Total read slots", default=16, type=int)
parser.add_argument("--num_to_send", help="Number of packets to send", default=10, type=int)
parser.add_argument("--send_dist_file", help="Send distribution file", default="", type=str)
parser.add_argument("--size_file", help="Message size distribution file", default="", type=str)
parser.add_argument("--recv_ins_file", help="Receive instructions distribution file", default="", type=str)
parser.add_argument("--send_ins_file", help="Send instructions distribution file", default="", type=str)
parser.add_argument("--recv_mem_file", help="Receive memory ops distribution file", default="", type=str)
parser.add_argument("--tgt_gen_file", help="Tgt distribution file", default="", type=str)
parser.add_argument("--tgt_gen_max", help="Tgt distribution max", default="", type=str)
parser.add_argument("--send_mem_file", help="Send memory ops distribution file", default="", type=str)
parser.add_argument("--app", help="Select folder for application with distributions", default="", type=str)
parser.add_argument("--apps_path", help="Path to application Distributions", default="apps", type=str)
parser.add_argument("--traversed_edges", help="Traversed edges per precinct", default=1, type=int)
parser.add_argument("--nw_header_bits", help="Network header in bits", default=72, type=int)
parser.add_argument("--zop_header_bits", help="ZOP header in bits", default=128, type=int)
parser.add_argument("--link_bw", help="Precinct Link BW", default=1600, type=int)
parser.add_argument("--max_aggr_size", help="Maximum number of packets to aggregate (set to 0 to disable)", default=0, type=int)
parser.add_argument("--max_aggr_timeout", help="Maximum number of packets to aggregate (set to 0 to disable)", default=0, type=int)
parser.add_argument("--only_max", help="Maximum number of packets to aggregate (set to 0 to disable)", default=0, type=int)
parser.add_argument("--verbose", help="Output verbosity", default=0, type=int)
parser.add_argument("--aggr_timeout_cycles", help="Aggregation timeout in cycles", default=0, type=int)
parser.add_argument("--enableStatistics", help="Enable Statistics", default=False, type=bool)
parser.add_argument("--stat_path", help="Stat Path", default="StatisticOutput.csv", type=str)
parser.add_argument("--use_static_sends", help="Use static sends instead of dynamic", default=0, type=int)
parser.add_argument("--precinct_queue_size", help="Precinct Queue Size", default=1000, type=int)
parser.add_argument("--static_send_num", help="Number of static packets to send", default=10, type=int)
parser.add_argument("--pe_stats", help="Enable PE stats (max 8)", default=0, type=int)
parser.add_argument("--old_scale", help="Manual scaling - old scale", default=14, type=int)
parser.add_argument("--new_scale", help="Manual scaling - new scale", default=14, type=int)
parser.add_argument("--scaling_method", help="Manual scaling - scaling method", default="linear", type=str)
parser.add_argument("--pe_queue_size", help="PE queue depth", default=64, type=int)
parser.add_argument("--event_driven_send", help="Turn on event driven send", default=0, type=int)
parser.add_argument("--event_driven_recv", help="Turn on event driven recv", default=0, type=int)
parser.add_argument("--demo_mode", help="Turn on demo mode", default=0, type=int)
parser.add_argument("--clock_speed", help="Turn on event driven recv", default="2000MHz", type=str)
parser.add_argument("--zero_load_latency", help="Turn on event driven recv", default=520, type=int)
parser.add_argument("--dyn_v1", help="Dynamic scheduler 1", default=0, type=int)
parser.add_argument("--dyn_v2", help="Dynamic scheduler 2", default=0, type=int)
parser.add_argument("--nw_file_name", help="Path to network file", default="", type=str)
parser.add_argument("--lat_file_name", help="Path to latency file", default="", type=str)


args = parser.parse_args()

static_val = args.num_to_send

shape = args.shape
host_link_latency = None
link_latency = "100ns"
rtr_prefix="cornelis_"

num_pes_per_zone = args.pes
num_zaps = args.zaps
num_zones = args.zones

pe_zap_latency = "1ps"
zap_zone_latency = "45ns"
zone_precinct_latency = "90ns"

max_num_sends = 9000
max_send_pe = 0


all_sends = []

topo_params = {
    "shape":shape,

    "routing_alg":"deterministic",
    "adaptive_threshold":0.5
}

router_params = {
    "flit_size" : "8B",
    "input_buf_size" : "16kB",
    "input_latency" : "45ns",
    "link_bw" : "%dGb/s" % args.link_bw,
    "num_vns" : "1",
    "output_buf_size" : "16kB",
    "output_latency" : "45ns",
    "xbar_bw" : "%dGb/s" % args.link_bw,
}

precinct_params = {
    "eg_queue_size": 0,
    "new_scale": 11,
    "old_scale": 11,
    "verbose": args.verbose,
    "nw_header_bits": args.nw_header_bits,
    "zop_header_bits": args.zop_header_bits,
    "max_aggr_size": args.max_aggr_size,
    "max_aggr_timeout": args.max_aggr_timeout,
    "only_max": args.only_max,
    "aggr_timeout_cycles": args.aggr_timeout_cycles,
    "pe_queue_size": args.pe_queue_size,
    "demo_mode": args.demo_mode,
    "nw_file_name": args.nw_file_name
}

pe_static_params = {
    "size_min": args.size_min,
    "size_max": args.size_max,
    "send_ins_min": args.send_ins_min,
    "send_ins_max": args.send_ins_max,
    "recv_ins_min": args.recv_ins_min,
    "recv_ins_max": args.recv_ins_max,
    "send_mem_min": args.send_mem_min,
    "send_mem_max": args.send_mem_max,
    "recv_mem_min": args.recv_mem_min,
    "recv_mem_max": args.recv_mem_max,
    "use_static_sends": args.use_static_sends,
    "static_send_num": args.static_send_num,
    "lat_file_name": args.lat_file_name
}

pe_file_params = {
    "size_file": args.size_file,
    "ins_gen_recv_file": args.recv_ins_file,
    "ins_gen_send_file": args.send_ins_file,
    "mem_ins_gen_recv_file": args.recv_mem_file,
    "tgt_gen_file": args.tgt_gen_file,
    "mem_ins_gen_send_file": args.send_mem_file,

}

queue_size_params = {
    "precinct_queue_size": args.precinct_queue_size
}

pe_mode_params = {
    "event_driven_send": args.event_driven_send,
    "event_driven_recv": args.event_driven_recv,
    "dyn_v1": args.dyn_v1,
    "dyn_v2": args.dyn_v2
}

pe_clock_params = {
    "clock_speed": args.clock_speed
}

if args.app != "":
    pe_file_params = {
        "size_file": os.path.join(args.apps_path, args.app, "size_dist.csv") if os.path.exists(os.path.join(args.apps_path, args.app, "size_dist.csv")) else "",
        "ins_gen_recv_file": os.path.join(args.apps_path, args.app, "recv_ins.csv") if os.path.exists(os.path.join(args.apps_path, args.app, "recv_ins.csv")) else "",
        "ins_gen_send_file": os.path.join(args.apps_path, args.app, "send_ins.csv") if os.path.exists(os.path.join(args.apps_path, args.app, "send_ins.csv")) else "",
        "mem_ins_gen_recv_file": os.path.join(args.apps_path, args.app, "recv_mem.csv") if os.path.exists(os.path.join(args.apps_path, args.app, "recv_mem.csv")) else "",
        "mem_ins_gen_send_file": os.path.join(args.apps_path, args.app, "send_mem.csv") if os.path.exists(os.path.join(args.apps_path, args.app, "send_mem.csv")) else "",
        "tgt_gen_file": os.path.join(args.apps_path, args.app, "recv_dist.csv") if os.path.exists(os.path.join(args.apps_path, args.app, "recv_dist.csv")) else "",

    }
    send_dist_file = os.path.join(args.apps_path, args.app, "send_dist.json")
else:
    send_dist_file = args.send_dist_file

if len(pe_file_params["tgt_gen_file"]) > 0:
    pe_file_params["tgt_gen_max"] = len(open(pe_file_params["tgt_gen_file"], 'r').readlines())
    print("TGT GEN MAX is", pe_file_params["tgt_gen_max"], "size")

if send_dist_file:
    with open(send_dist_file, 'r') as f:
        send_res = json.load(f)
else:
    send_res = []

networkLinkControl_params = {
    "input_buf_size" : "14kB",
    "job_id" : "0",
    "job_size" : "2",
    "link_bw" : "1600Gb/s",
    "output_buf_size" : "14kB",
    "use_nid_remap" : "False",
}

mem_params = {
    "ops_per_cycle": args.mem_ops_per_cycle,
    "num_read": args.read_slots,
    "zero_load_latency": args.zero_load_latency
}

zone_id = 0
zap_id = 0
pe_id = 0
ups=[]
downs=[]
routers_per_level=[]
groups_per_level=[]
start_ids = []
total_hosts = 1

sst.addGlobalParams("router_params", router_params)
sst.addGlobalParams("precinct_params", precinct_params)
sst.addGlobalParams("networkLinkControl_params",networkLinkControl_params)

def getRouterNameForId(rtr_id):
    num_levels = len(start_ids)

    # Check to make sure the index is in range
    level = num_levels - 1
    if rtr_id >= (start_ids[level] + routers_per_level[level]) or rtr_id < 0:
        print("ERROR: topoFattree.getRouterNameForId: rtr_id not found: %d"%rtr_id)
        sst.exit()

    # Find the level
    for x in range(num_levels-1,0,-1):
        if rtr_id >= start_ids[x]:
            break
        level = level - 1

    # Find the group
    remainder = rtr_id - start_ids[level]
    routers_per_group = routers_per_level[level] // groups_per_level[level]
    group = remainder // routers_per_group
    router = remainder % routers_per_group
    return getRouterNameForLocation((level,group,router))

def getRouterNameForLocation(location):
    return "%srtr_l%s_g%d_r%d"%(rtr_prefix,location[0],location[1],location[2])

def make_zaps(zone, num_zaps, num_pes, precinct_id=0, zone_id=0):
  global zap_id
  global pe_id
  for z in range(num_zaps):
    #print(z)
    zap = sst.Component("ZAP" + str(zap_id), "forzasim_macro.ZAP_PE")
    zap.addGlobalParamSet("precinct_params")
    link = sst.Link("zap" + str(zap_id) + "_back_link")
    link.connect( (zone, "zap_port_" + str(z), zap_zone_latency), (zap, "zone_port", zap_zone_latency) )
    zap.addParams({"precinct_id": precinct_id, "zone_id": zone_id})
    zap.addParams({"msg_count": all_sends[0], "total_pe_count": int(num_pes_per_zone * num_zaps * num_zones * total_hosts),
                  "num_precincts": total_hosts,
                  "num_zones": num_zones,
                  "num_zaps": num_zaps,
                  "num_pes": num_pes_per_zone,
                  "static_dist": True,
                  "global_pe_id": pe_id
                  })
    zap.addParams(pe_static_params)
    zap.addParams(pe_file_params)
    zap.addParams(queue_size_params)
    zap.addParams(pe_mode_params)
    zap.addParams(pe_clock_params)
    to_send_arr = []
    for i in range(num_pes_per_zone):
        to_send = all_sends[pe_id]
        to_send_arr.append(to_send)
        if to_send == max_send_pe:
            zap.addParams({"max_send": True, "max_send_pe": i})
        pe_id += 1
    zap.addGlobalParamSet("precinct_params")
    zap.addParams({"to_send_arr": to_send_arr})
    zap.addParams({"precinct_id": precinct_id, "zone_id": zone_id, "zap_id": z})
    zap_id += 1

def make_zone(precinct, num_zones, num_zaps, num_pes, precinct_id=0):
  global zone_id
  for z in range(num_zones):
    zone = sst.Component("Zone" + str(zone_id), "forzasim_macro.Zone")
    mem_comp = zone.setSubComponent("memory", "forzasim_macro.ZAPBasicMemCtrlSimple")
    #mem_comp.addGlobalParamSet("precinct_params")
    mem_comp.addParams(mem_params)
    zone.addGlobalParamSet("precinct_params")
    zone.addParams({"precinct_id": precinct_id, "num_zaps":num_zaps})
    link = sst.Link("zone" + str(zone_id) + "_back_link")
    link.connect( (precinct, "zone_port_" + str(z), zone_precinct_latency), (zone, "precinct_port", zone_precinct_latency) )
    make_zaps(zone, num_zaps, num_pes, precinct_id, z)
    zone_id += 1

# Construct HR_Router SST Object
def instanciateRouter(radix, rtr_id):
    rtr = sst.Component(getRouterNameForId(rtr_id), "merlin.hr_router")
    rtr.addGlobalParamSet("router_params")
    rtr.addParam("num_ports",radix)
    rtr.addParam("id",rtr_id)
    return rtr

# Construct Endpoints
def instanciateEndpoint(id, addionalParams={}):
    precinct = sst.Component("precinct"+str(id), "forzasim_macro.Precinct")
    precinct.addGlobalParamSet("precinct_params")
    precinct.addParams({"id" : id, "num_zones": num_zones, "num_precincts": total_hosts, "traversed_edges": args.traversed_edges,
                        "num_pes": num_pes_per_zone, "num_zaps": num_zaps})
    precinct_lc = precinct.setSubComponent("rtrLink", "merlin.linkcontrol")
    precinct_lc.addGlobalParamSet("networkLinkControl_params")

    make_zone(precinct, num_zones, num_zaps, num_pes_per_zone, id)

    return precinct, precinct_lc

def findRouterByLocation(location):
    return sst.findComponentByName(getRouterNameForLocation(location))

# Process the shape
def calculateTopo():
    global shape
    global ups
    global downs
    global routers_per_level
    global groups_per_level
    global start_ids
    global total_hosts
    levels = shape.split(":")

    for l in levels:
        links = l.split(",")
        downs.append(int(links[0]))
        if len(links) > 1:
            ups.append(int(links[1]))

    for i in downs:
        total_hosts *= i
    print("Total precincts:", total_hosts)
    routers_per_level = [0] * len(downs)
    routers_per_level[0] = total_hosts // downs[0]
    for i in range(1,len(downs)):
        routers_per_level[i] = routers_per_level[i-1] * ups[i-1] // downs[i]

    start_ids = [0] * len(downs)
    for i in range(1,len(downs)):
        start_ids[i] = start_ids[i-1] + routers_per_level[i-1]

    groups_per_level = [1] * len(downs)
    if ups: # if ups is empty, then this is a single level and the following line will fail
        groups_per_level[0] = total_hosts // downs[0]

    for i in range(1,len(downs)-1):
        groups_per_level[i] = groups_per_level[i-1] // downs[i]

def buildTopo():
    global host_link_latency
    all_precincts = {}
    if not host_link_latency:
        host_link_latency = link_latency

    #Recursive function to build levels
    def fattree_rb(level, group, links):
        id = start_ids[level] + group * (routers_per_level[level]//groups_per_level[level])

        host_links = []
        if level == 0:
            # create all the nodes
            for i in range(downs[0]):
                node_id = id * downs[0] + i
                (ep, ep_lc) = instanciateEndpoint(node_id)
                all_precincts[node_id] = ep
                if ep:
                    plink = sst.Link("precinctlink_%d"%node_id)
                    ep_lc.addLink(plink, "rtr_port", host_link_latency)
                    host_links.append(plink)

            # Create the edge router
            rtr_id = id
            rtr = instanciateRouter(ups[0] + downs[0], rtr_id)
            # TODO Workout how to set the topology "subcomponent" through Base SST
            topology = rtr.setSubComponent("topology","merlin.fattree")
            topology.addParams(topo_params)
            # Add links
            for l in range(len(host_links)):
                rtr.addLink(host_links[l],"port%d"%l, link_latency)
            for l in range(len(links)):
                rtr.addLink(links[l],"port%d"%(l+downs[0]), link_latency)
            return

        rtrs_in_group = routers_per_level[level] // groups_per_level[level]
        # Create the down links for the routers
        rtr_links = [ [] for index in range(rtrs_in_group) ]
        for i in range(rtrs_in_group):
            for j in range(downs[level]):
                rtr_links[i].append(sst.Link("link_l%d_g%d_r%d_p%d"%(level,group,i,j)));

        # Now create group links to pass to lower level groups from router down links
        group_links = [ [] for index in range(downs[level]) ]
        for i in range(downs[level]):
            for j in range(rtrs_in_group):
                group_links[i].append(rtr_links[j][i])

        for i in range(downs[level]):
            fattree_rb(level-1,group*downs[level]+i,group_links[i])

        # Create the routers in this level.
        # Start by adding up links to rtr_links
        for i in range(len(links)):
            rtr_links[i % rtrs_in_group].append(links[i])

        for i in range(rtrs_in_group):
            rtr_id = id + i
            rtr = instanciateRouter(ups[level] + downs[level], rtr_id)
            topology = rtr.setSubComponent("topology","merlin.fattree")
            topology.addParams(topo_params)
            # Add links
            for l in range(len(rtr_links[i])):
                rtr.addLink(rtr_links[i][l],"port%d"%l, link_latency)
    #  End recursive function

    level = len(ups)
    if ups: # True for all cases except for single level
        #  Create the router links
        rtrs_in_group = routers_per_level[level] // groups_per_level[level]

        # Create the down links for the routers
        rtr_links = [ [] for index in range(rtrs_in_group) ]
        for i in range(rtrs_in_group):
            for j in range(downs[level]):
                rtr_links[i].append(sst.Link("link_l%d_g0_r%d_p%d"%(level,i,j)));

        # Now create group links to pass to lower level groups from router down links
        group_links = [ [] for index in range(downs[level]) ]
        for i in range(downs[level]):
            for j in range(rtrs_in_group):
                group_links[i].append(rtr_links[j][i])


        for i in range(downs[len(ups)]):
            fattree_rb(level-1,i,group_links[i])

        # Create the routers in this level
        radix = downs[level]
        for i in range(routers_per_level[level]):
            rtr_id = start_ids[len(ups)] + i
            rtr = instanciateRouter(radix,rtr_id);

            topology = rtr.setSubComponent("topology","merlin.fattree")
            topology.addParams(topo_params)

            for l in range(len(rtr_links[i])):
                rtr.addLink(rtr_links[i][l], "port%d"%l, link_latency)
    else: # Single level case
        # create all the nodes
        for i in range(downs[0]):
            node_id = i
    num_pc = len(all_precincts)
    for k,v in all_precincts.items():
        for i in range(k + 1, num_pc):
            src_name = "precinct_meta_port_" + str(k) + "_" + str(i)
            dest_name = "precinct_meta_port_" + str(i) + "_" + str(k)
            link = sst.Link(src_name + "_link")
            link.connect( (v, src_name, "1ns"), (all_precincts[i], dest_name, "1ns") )
            #print(src_name, dest_name)
    rtr_id = 0

# def get_max_msg():
#   global send_res
#   global precinct_params
#   N = len(send_res)
#   K = num_pes_per_zone * num_zaps * num_zones * total_hosts
#   if precinct_params["new_scale"] > params["old_scale"]:
#     scale_factor = (1 << (precinct_params["new_scale"] - precinct_params["old_scale"])*2)
#   else:
#     scale_factor =  1.0/(1 << (precinct_params["old_scale"] - precinct_params["new_scale"])*2)
#   return round(sum(send_res)) * scale_factor
#
cur_send = 0
deterministic = False
def sample_send_dist(K, scaling_method, new_scale, old_scale, scale_pes=False, static=-1):
  global cur_send
  global deterministic
  global send_res
  global params
  if deterministic:
      to_send = send_res[cur_send]
      cur_send += 1
      #print("Send for", cur_send, ":", to_send)
      return to_send
  if static > 0:
      return static
  N = len(send_res)
  if scaling_method == "linear":
    if new_scale > old_scale:
        scale_factor = (1 << (new_scale - old_scale))
    else:
        scale_factor = 1.0/(1 << (old_scale - new_scale))
  elif scaling_method == "power":
    scale_factor = (1 << (new_scale - old_scale)) ** 2
  #print(scale_factor)
  if not scale_pes:
    return round(random.choice(send_res)) * scale_factor
  return round(random.choice(send_res) / K * N) * scale_factor

calculateTopo()
max_num_sends = 0
for i in range(num_pes_per_zone * num_zaps * num_zones * total_hosts):
    if len(send_res) > 0:
        to_send = sample_send_dist(num_pes_per_zone * num_zaps * num_zones * total_hosts, args.scaling_method.lower(), args.new_scale, args.old_scale,
                               scale_pes=False)
    else:
        to_send = sample_send_dist(num_pes_per_zone * num_zaps * num_zones * total_hosts, "linear", 14, 14,
                               scale_pes=False, static=static_val)
    max_num_sends += to_send
    max_send_pe = max(max_send_pe, to_send)
    all_sends.append(to_send)
print("Total sends", max_num_sends, ", max sends from a PE", max_send_pe)
buildTopo()

if args.enableStatistics == True:
    print("Stats enabled")
    sst.setStatisticLoadLevel(1)
    sst.setStatisticOutput("sst.statOutputCSV", {"filepath": args.stat_path})
    sst.enableStatisticForComponentType("forzasim_macro.Precinct", 'TotRecvPrecinct', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.Precinct", 'TotSentPrecinct', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'TotalRecv', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'TotalRecvInternal', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'TotalSent', {"type": "sst.AccumulatorStatistic"})
    if args.pe_stats == True:
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE0Recv', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE0Sent', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE1Recv', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE1Sent', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE2Recv', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE2Sent', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE3Recv', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE3Sent', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE4Recv', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE4Sent', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE5Recv', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE5Sent', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE6Recv', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE6Sent', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE7Recv', {"type": "sst.AccumulatorStatistic"})
        sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'PE7Sent', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'Cycles', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'LastSentCycle', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'LastProcCycle', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.Zone", 'TotalSentZone', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'TotalProc', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'StallCycleBP', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'MemStalls', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.Precinct", 'TotRecvPrecinctInc', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'IdleCycles', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'InactiveCycles', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'TotalRecvIns', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAP_PE", 'TotalSendIns', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAPBasicMemCtrlSimple", 'StalledOps', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAPBasicMemCtrlSimple", 'ProcessTime', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticForComponentType("forzasim_macro.ZAPBasicMemCtrlSimple", 'TotalNumReads', {"type": "sst.AccumulatorStatistic"})
    sst.enableStatisticsForComponentType("merlin.hr_router","send_bit_count")
    sst.enableStatisticsForComponentType("merlin.linkcontrol","send_bit_count")

    #sst.enableStatisticForComponentType("forzasim_macro.Precinct", 'AvgAggrSize', {"type": "sst.HistogramStatistic", "minvalue": 0, "binwidth": 1, "numbins": 17, "IncludeOutOfBounds": 1})
    #sst.enableStatisticForComponentType("forzasim_macro.ZAPBasicMemCtrlSimple",
    #        'TotalNumReads', {"type": "sst.AccumulatorStatistic"})
    #sst.enableStatisticForComponentType("forzasim_macro.Precinct", 'QS', {"type": "sst.HistogramStatistic",
    #                                    "minvalue" : "0",
    #                                    "binwidth" : "5",
    #                                    "IncludeOutOfBounds" : "1"})
    #sst.enableStatisticForComponentType("forzasim_macro.PE", 'EventQueueSize', {"type": "sst.HistogramStatistic"})
    #sst.enableStatisticForComponentType("forzasim_macro.PE", 'SendActorOccupancy', {"type": "sst.AccumulatorStatistic"})
    #sst.enableStatisticForComponentType("forzasim_macro.PE", 'RecvActorOccupancy', {"type": "sst.AccumulatorStatistic"})
    #sst.enableStatisticForComponentType("forzasim_macro.PE", 'TotRecv', {"type": "sst.AccumulatorStatistic"})
    #sst.enableStatisticForComponentType("forzasim_macro.PE", 'TotSent', {"type": "sst.AccumulatorStatistic"})
    #sst.enableStatisticForComponentType("forzasim_macro.PE", 'ProcessStartTS', {"type": "sst.AccumulatorStatistic"})
    #sst.enableStatisticForComponentType("forzasim_macro.ZAPBasicMemCtrlSimple", 'QueueSize', {"type": "sst.HistogramStatistic"})
    #sst.enableStatisticForComponentType("forzasim_macro.ZAPBasicMemCtrlSimple", 'ProcessStartTS', {"type": "sst.AccumulatorStatistic"})
