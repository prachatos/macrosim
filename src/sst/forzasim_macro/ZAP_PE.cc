#include "sst/core/rng/marsaglia.h"
#include "sst/forzasim_macro/ZAP_PE.h"
#include "sst/forzasim_macro/Zone.h"
#include "sst/forzasim_macro/Precinct.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include "ZAP_PE.h"
//#define END_SEMANTICS
#define PROC_TIME_TRACK
//#define DISABLE_MEM_SEND
//#define DISABLE_MEM_RECV
namespace SST {
namespace forzasim_macro {
ZAP_PE::ZAP_PE(SST::ComponentId_t _id, SST::Params& _params)
    : SST::Component(_id) {
  uint64_t i = 0;
  // Configures output.
  int verbosity = _params.find<uint64_t>("verbose", 1);
  output_.init("[@t] forzasim_macro." + getName() + ": ", verbosity, 0,
               SST::Output::STDOUT);

  num_pes = _params.find<uint64_t>("num_pes");

  // TODO: Adjust name of port based on scheme chosen
  zone_link_ = configureLink("zone_port", new SST::Event::Handler<ZAP_PE, int>(this, &ZAP_PE::handleZoneEvent, i));
  pe_send_link_ = configureSelfLink("pe_send_port", new SST::Event::Handler<ZAP_PE>(this, &ZAP_PE::handleSendEvent));
  pe_recv_link_ = configureSelfLink("pe_recv_port", new SST::Event::Handler<ZAP_PE>(this, &ZAP_PE::handleRecvEvent));
  // FIXME: Fix
  msgc = _params.find<uint64_t>("msg_count", 10000000);

  total_pe_count = _params.find<uint64_t>("total_pe_count", 1);
  zap_id = _params.find<uint64_t>("zap_id");
  zone_id = _params.find<uint64_t>("zone_id");
  global_pe_id = _params.find<uint64_t>("global_pe_id", 0);
  is_max_send = _params.find<bool>("max_send", false);
  max_send_pe = _params.find<uint64_t>("max_send_pe", 0);
  static_dist = _params.find<bool>("static_dist", false);
  output_.verbose(CALL_INFO, 3, 0, "PE ID: %lu\n", global_pe_id);
  //printf("PE ID: %lu\n", global_pe_id);
  SendToReceive = registerStatistic<uint64_t>("SendToReceive");
  StallCycleBP = registerStatistic<uint64_t>("StallCycleBP");
  MemStalls = registerStatistic<uint64_t>("MemStalls");
  InactiveCycles = registerStatistic<uint64_t>("InactiveCycles");
  IdleCycles = registerStatistic<uint64_t>("IdleCycles");
  TotalRecvIns = registerStatistic<uint64_t>("TotalRecvIns");
  TotalSendIns = registerStatistic<uint64_t>("TotalSendIns");
#ifdef PE_STATS
  PE0Sent = registerStatistic<uint64_t>("PE0Sent");
  PE0Recv = registerStatistic<uint64_t>("PE0Recv");
  PE1Sent = registerStatistic<uint64_t>("PE1Sent");
  PE1Recv = registerStatistic<uint64_t>("PE1Recv");
  PE2Sent = registerStatistic<uint64_t>("PE2Sent");
  PE2Recv = registerStatistic<uint64_t>("PE2Recv");
  PE3Sent = registerStatistic<uint64_t>("PE3Sent");
  PE3Recv = registerStatistic<uint64_t>("PE3Recv");
  PE4Sent = registerStatistic<uint64_t>("PE4Sent");
  PE4Recv = registerStatistic<uint64_t>("PE4Recv");
  PE5Sent = registerStatistic<uint64_t>("PE5Sent");
  PE5Recv = registerStatistic<uint64_t>("PE5Recv");
  PE6Sent = registerStatistic<uint64_t>("PE6Sent");
  PE6Recv = registerStatistic<uint64_t>("PE6Recv");
  PE7Sent = registerStatistic<uint64_t>("PE7Sent");
  PE7Recv = registerStatistic<uint64_t>("PE7Recv");
#endif
  precinct_id = _params.find<uint64_t>("precinct_id", 0);
  //eg_precinct_counters.initialize("precinct" + std::to_string(precinct_id));
  //eg_precinct_counters.publish();
  scale_factor = 1;
  all_bp_stalls = 0;
  complete_pe_count = 0;
  complete_send_pe_count = 0;
  system_size_ = {
    _params.find<uint64_t>("num_precincts", 1),
    _params.find<uint64_t>("num_zones", 1),
    _params.find<uint64_t>("num_zaps", 1),
    _params.find<uint64_t>("num_pes", 1)
  };
  eg_precinct_counters.resize(system_size_.num_precincts);
  TotalRecv = registerStatistic<uint64_t>("TotalRecv");
  TotalRecvInternal = registerStatistic<uint64_t>("TotalRecvInternal");
  TotalSent = registerStatistic<uint64_t>("TotalSent");
  TotalProc = registerStatistic<uint64_t>("TotalProc");
  Cycles = registerStatistic<uint64_t>("Cycles");
  LastProcCycle = registerStatistic<uint64_t>("LastProcCycle");
  LastSentCycle = registerStatistic<uint64_t>("LastSentCycle");
  // Synthetic distribution params
  size_min = _params.find<uint64_t>("size_min", 16);
  size_max = _params.find<uint64_t>("size_max", 16);
  send_ins_min = _params.find<uint64_t>("send_ins_min", 250);
  send_ins_max = _params.find<uint64_t>("send_ins_max", 450);
  recv_ins_min = _params.find<uint64_t>("recv_ins_min", 250);
  recv_ins_max = _params.find<uint64_t>("recv_ins_max", 450);
  send_mem_min = _params.find<uint64_t>("send_mem_min", 100);
  send_mem_max = _params.find<uint64_t>("send_mem_max", 150);
  recv_mem_min = _params.find<uint64_t>("recv_mem_min", 100);
  recv_mem_max = _params.find<uint64_t>("recv_mem_max", 150);
  ipc_send_min = _params.find<double>("ipc_send_min", 2);
  ipc_send_max = _params.find<double>("ipc_send_max", 2);
  ipc_recv_min = _params.find<double>("ipc_recv_min", 2);
  ipc_recv_max = _params.find<double>("ipc_recv_max", 2);
  send_lst_ratio = _params.find<double>("send_lst_ratio", 0.07);
  send_tot_ratio = _params.find<double>("send_tot_ratio", 0.65);
  recv_tot_ratio = _params.find<double>("recv_tot_ratio", 1.2);
  recv_lst_ratio = _params.find<double>("recv_lst_ratio", 0.6);

  use_static_sends = _params.find<uint64_t>("use_static_sends", false);
  if (use_static_sends) {
    num_static_sends = _params.find<uint64_t>("static_send_num", 0);
    msg_count.assign(num_pes, num_static_sends);
  } else {
    _params.find_array<uint64_t>("to_send_arr", msg_count);
  }

  precinct_queue_size = _params.find<uint64_t>("precinct_queue_size", 10000);

  dynamic_proc_v1_enabled = _params.find<uint64_t>("dyn_v1", 0);
  dynamic_proc_v2_enabled = _params.find<uint64_t>("dyn_v2", 0);
  event_driven_send = _params.find<uint64_t>("event_driven_send", 0);
  event_driven_recv = _params.find<uint64_t>("event_driven_recv", 0);
  demo_mode = _params.find<uint64_t>("demo_mode", 0);
  // File based distribution params
  ins_gen_recv_file = _params.find<std::string>("ins_gen_recv_file", "");
  ins_gen_send_file = _params.find<std::string>("ins_gen_send_file", "");
  size_file = _params.find<std::string>("size_file", "");
  mem_ins_gen_send_file = _params.find<std::string>("mem_ins_gen_send_file", "");
  mem_ins_gen_recv_file = _params.find<std::string>("mem_ins_gen_recv_file", "");
  tgt_gen_file = _params.find<std::string>("tgt_gen_file", "");
  lat_file_name = _params.find<std::string>("lat_file_name", "");
  //printf("ZAP ID: %lu\n", zap_id);
  rng = new SST::RNG::MarsagliaRNG();

  //tgt_gen_real     = new ExponentialDist(1.0/total_pe_count, rng);
  if (tgt_gen_file.length() == 0) {
    output_.fatal(CALL_INFO, -1, "Did not get target distribution");
    //tgt_gen_real     = new ExponentialDist(1.0/total_pe_count, rng);
    tgt_gen_real     = new UniformDistV2(0, total_pe_count, rng);
    tgt_gen_scale    = new UniformDist(1, 1, rng);
  } else {
    uint64_t spec_pe = _params.find<uint64_t>("tgt_gen_max", total_pe_count);
    //printf("Tgt gen scale: %lu %lu %.2f\n", spec_pe, total_pe_count, total_pe_count * 1.0/spec_pe);
    tgt_gen_real     = initSharedMap(tgt_gen_file, "tgt_gen_file");
    tgt_gen_scale    = new UniformDist(1, total_pe_count * 1.0/spec_pe, rng);
  }
  //tgt_gen_real     = new UniformDistV2(0, total_pe_count, rng);
  if (ins_gen_recv_file.length() == 0) {
    output_.fatal(CALL_INFO, -1, "Did not get recv instr distribution");
    ins_gen_recv     = new UniformDist(recv_ins_min, recv_ins_max, rng);
  } else {
    ins_gen_recv     = initSharedMap(ins_gen_recv_file, "ins_gen_recv");
  }
  if (ins_gen_send_file.length() == 0) {
    output_.fatal(CALL_INFO, -1, "Did not get send instr distribution");
    ins_gen_send     = new UniformDist(send_ins_min, send_ins_max, rng);
  } else {
    ins_gen_send     = initSharedMap(ins_gen_send_file, "ins_gen_send");
  }
  if (size_file.length() == 0) {
    size_gen       = new UniformDist(size_min, size_max, rng);
  } else {
    size_gen       = initSharedMap(size_file, "size_gen");
  }
  if (mem_ins_gen_send_file.length() == 0) {
    output_.fatal(CALL_INFO, -1, "Did not get send mem distribution");
    mem_ins_gen_send       = new UniformDist(send_mem_min, send_mem_max, rng);
  } else {
    mem_ins_gen_send       = initSharedMap(mem_ins_gen_send_file, "mem_ins_gen_send");
  }
  if (mem_ins_gen_recv_file.length() == 0) {
    output_.fatal(CALL_INFO, -1, "Did not get recv mem distribution");
    mem_ins_gen_recv       = new UniformDist(recv_mem_min, recv_mem_max, rng);
  } else {
    mem_ins_gen_recv       = initSharedMap(mem_ins_gen_recv_file, "mem_ins_gen_recv");
  }
  if (ipc_send_file.length() == 0) {
    ipc_send               =  new UniformDist(ipc_send_min, ipc_send_max, rng);
  } else {
    ipc_send               = initSharedMap(ipc_send_file, "ipc_send");
  }
  if (ipc_recv_file.length() == 0) {
    ipc_recv               =  new UniformDist(ipc_recv_min, ipc_recv_max, rng);
  } else {
    ipc_recv               = initSharedMap(ipc_recv_file, "ipc_recv");
  }
  mem_rand         = new UniformDist(0, 100, rng);

  waiting_on_mem.resize(num_pes, false);
  waiting_on_mem_send.resize(num_pes, false);
  tot_ins.resize(num_pes, 0);
  last_proc_cycle.resize(num_pes, 0);
  tot_ins_send.resize(num_pes, 0);
  num_recv.resize(num_pes, 0);
  cur_ins.resize(num_pes, 0);
  cur_ins_send.resize(num_pes, 0);
  cur_ipc.resize(num_pes, 0);
  cur_ipc_send.resize(num_pes, 0);
  mem_ins.resize(num_pes, 0);
  mem_ins_send.resize(num_pes, 0);
  mem_ins_ratio.resize(num_pes, 0);
  mem_ins_ratio_send.resize(num_pes, 0);
  cur_event.resize(num_pes, false);
  cur_event_send.resize(num_pes, false);
  num_sent.resize(num_pes, 0);
  next_pe_tgt.resize(num_pes, -1);
  next_pe_gen.resize(num_pes, 0);
  next_to_send.resize(num_pes, 0);
  send_next_wakeup_cycle.resize(num_pes, 0);
  recv_next_wakeup_cycle.resize(num_pes, 0);
  pe_complete_counters.resize(num_pes, 0);
  pe_send_done.resize(num_pes, false);
  for (int i = 0; i < num_pes; ++i) {
    next_to_send[i] = get_precinct_pe_id(i) + num_pes * system_size_.num_zaps * system_size_.num_zones;
    //printf("next_to_send[%lu]: %lu\n", i, next_to_send[i]);
  }
  send_handler = new Clock::Handler<ZAP_PE>(this, &ZAP_PE::sendActor);
  recv_handler = new Clock::Handler<ZAP_PE>(this, &ZAP_PE::recvActor);
  if (!demo_mode) {
    if (event_driven_recv != 0) {
      recv_tc = registerClock("1Hz", recv_handler);
    } else {
      recv_tc = registerClock(_params.find<std::string>("clock_speed", "2000MHz"), recv_handler);
    }
    if (event_driven_send != 0) {
      send_tc = registerClock("1Hz", send_handler);
    } else {
      send_tc = registerClock(_params.find<std::string>("clock_speed", "2000MHz"), send_handler);
    }
  }
  last_recv_pe_id = num_pes - 1;
  pe_send_link_->setDefaultTimeBase(getTimeConverter("0.5ns"));
  pe_recv_link_->setDefaultTimeBase(getTimeConverter("0.5ns"));

  //printf("ins gen %.2f\n", ins_gen_recv->generate_objects_single());
  // Sets the time base.
  registerTimeBase("1ps");
}

ZAP_PE::~ZAP_PE() {
  delete rng;

  delete tgt_gen_real;

  delete ins_gen_recv;
  delete ins_gen_send;
  delete size_gen;
  delete mem_ins_gen_recv;
  delete mem_ins_gen_send;
  delete mem_rand;
}

uint64_t ZAP_PE::get_precinct_pe_id(uint64_t pe_id) {
  return pe_id + zap_id * system_size_.num_pes + zone_id * (num_pes * system_size_.num_zaps) + precinct_id * (num_pes * system_size_.num_zaps * system_size_.num_zones);
}

void ZAP_PE::setup() {
  output_.verbose(CALL_INFO, 1, 0, "Setup() ns=%" PRIu64 "\n",
                  getCurrentSimTimeNano());
  if (demo_mode) {
    for (uint64_t i = 0; i < 100; ++i) {
      Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
        5, 0, 0,
        0, 0,
        num_sent[0]
      );
      printf("Sending SEQ id %lu\n", num_sent[0]);
      pe_event->setTarget(zap_id, 0);
      pe_event->is_mem = false;
      num_sent[0]++;
      zone_link_->send(pe_event);
    }
  }
  if (event_driven_send != 0) {
    for (uint64_t i = 0; i < num_pes; ++i) {
      ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(i, ZAP_PE::PEInternalEvent::SEND_NEW);
      pe_send_link_->send(i, pe_event);
    }
  }
}

void ZAP_PE::finish() {
  output_.verbose(CALL_INFO, 1, 0, "Finish()\n");
}

void ZAP_PE::endSim(uint64_t pe) {
  //printf("End sim from (%lu, %lu, %lu, %lu), complete_count: %lu\n", precinct_id, zone_id, zap_id, pe, complete_pe_count);
  if (complete_pe_count == num_pes) {
    //printf("ZAP %lu exiting\n", zap_id);
    Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
      1, 0, 0,
      0, 0,
      0
    );
    zone_link_->send(pe_event);
  }
  /*if (!cur_event[pe] && is_max_send && max_send_pe == pe) {
    Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
      1, 0, 0,
      0, 0,
      0
    );
    zone_link_->send(pe_event);
  }
  primaryComponentOKToEndSim();*/
}

 DistGenerator* ZAP_PE::initSharedMap(std::string fname, std::string dist_name) {
  std::map<uint64_t, double> dist;
  if (global_pe_id == 0) {
    printf("Found global PE id 0, reading %s for distribution %s\n", fname.c_str(), dist_name.c_str());
    dist = generateMapFromCSV(fname);
  }
  return new DiscreteDist(global_pe_id, dist_name, dist, rng);
}

std::map<uint64_t, double> ZAP_PE::generateMapFromCSV(const std::string& csvFilePath) {
  //printf("Path: %s\n", csvFilePath.c_str());
  std::map<uint64_t, double> dist;
  std::ifstream file(csvFilePath);

  if (!file.is_open()) {
    output_.verbose(CALL_INFO, 1, 0, "Error opening file %s\n", csvFilePath.c_str());
    return dist;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    uint64_t key;
    std::string keyValue;

    if (std::getline(ss, keyValue, ',')) {
      std::stringstream((keyValue)) >> key;
      std::string value;
      if (std::getline(ss, value, ',')) {
        double doubleValue = std::stod((value));
        dist[key] = doubleValue;
      }
    }
  }
  /*for (auto k: dist) {
    printf("%d %.2f\n", k.first, k.second);
  }*/
  file.close();
  return dist;
}

bool ZAP_PE::actor(Cycle_t cycles) {
  if (cycles % 2) {
    recvActor(cycles);
  } else {
    sendActor(cycles);
  }
  Cycles->addData(1);
  return false;
}

bool ZAP_PE::sendActor(Cycle_t cycles){
  uint64_t pe_id = cycles % num_pes; //(cycles/2) % num_pes;
  //printf("Time on send: %lu\n", getCurrentSimTimeNano());
  if (event_driven_send != 0) {
    return false;
  }
  //printf("Send cycle %lu, timer %lu\n", cycles, getCurrentSimTimeNano());
  if (cur_event_send[pe_id]) {
    if (cur_ins_send[pe_id] >= tot_ins_send[pe_id] && !waiting_on_mem_send[pe_id]) {
      uint64_t res = send(pe_id, 0);
      if (res == 0 || res == 1) {
        // success
        cur_event_send[pe_id] = false;
        cur_ins_send[pe_id] = 0;
        mem_ins_send[pe_id] = 0;
        tot_ins_send[pe_id] = 0;
      } else {
        // failed, trying next cycle
      }
    } else {
      #ifndef DISABLE_MEM_SEND
      if (waiting_on_mem_send[pe_id]) {
        MemStalls->addData(1);
        //if (pe_id > 1) output_.verbose(CALL_INFO, 1, 0, "pe_id %" PRIu64 ", cycle %" PRIu64 " wait\n", pe_id, cycles);
      }  else if (mem_ins_send[pe_id] > 0) {
        Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
          0, 0, 0,
          0, 0,
          1
        );
        pe_event->setTarget(zap_id, pe_id);
        zone_link_->send(pe_event);
        waiting_on_mem_send[pe_id] = true;
        output_.verbose(CALL_INFO, 3, 0, "pe_id %" PRIu64 ", cycle %" PRIu64 " wait\n", pe_id, cycles);
        mem_ins_send[pe_id]--;
        cur_ins_send[pe_id] += 1;
      } else
      #endif
      {
        //if (zap_id == 1 && pe_id == 0) {
        //output_.verbose(CALL_INFO, 1, 0, "pe_id %" PRIu64 ", cycle %" PRIu64 " wait\n", pe_id, cycles);

        //}
        cur_ins_send[pe_id] += cur_ipc_send[pe_id];
        TotalSendIns->addData(1);
      }
    }
  } else if (num_sent[pe_id] < msg_count[pe_id]) {
    if (pe_id == 0) {
     //printf("%lu sending %lu\n", pe_id, num_sent[pe_id]);
    }
    cur_event_send[pe_id] = true;
    cur_ins_send[pe_id] = 0;
    mem_ins_send[pe_id] = mem_ins_gen_send->generate_objects_single();
    cur_ipc_send[pe_id] = ipc_send->generate_objects_single();

    tot_ins_send[pe_id] = ins_gen_send->generate_objects_single();
    if (mem_ins_send[pe_id] > tot_ins_send[pe_id]) {
      mem_ins_send[pe_id] = tot_ins_send[pe_id];
    }
    int r = 0;
    while (tot_ins_send[pe_id] == 0) {
      tot_ins_send[pe_id] = ins_gen_send->generate_objects_single();
      ++r;
      if (r > 5) {
        tot_ins_send[pe_id] = 5;
        break;
      }
    }
    waiting_on_mem_send[pe_id] = false;
    mem_ins_ratio_send[pe_id] = mem_ins_send[pe_id] * 100.0 / tot_ins_send[pe_id];
  } else {
    cur_event_send[pe_id] = false;

    if (pe_id == 0) {
      //printf("%lu sending %lu done!\n", pe_id, num_sent[pe_id]);
    }
#ifdef END_SEMANTICS
    if (!pe_send_done[pe_id]) {
      //printf("Precinct id %lu, Zone id %lu, Zap id %lu, PE %lu sent %lu, target %lu at cycle %lu, time %lu ns\n", precinct_id, zone_id, zap_id, pe_id, num_sent[pe_id], msg_count[pe_id], cycles, getCurrentSimTimeNano());
      // Send side termination
      Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
        4, 0, 0,
        0, 0,
        1
      );
      pe_event->setSrc(zone_id, zap_id, pe_id);
      pe_send_done[pe_id] = true;
      zone_link_->send(pe_event);
      complete_send_pe_count++;
      num_sent[pe_id]++;
      printf("(%lu, %lu, %lu, %lu): FIN at %lu\n", precinct_id, zone_id, zap_id, pe_id, getCurrentSimTimeNano());
    }
#endif
    LastSentCycle->addData(cycles);
    InactiveCycles->addData(1);
    //endSim(pe_id);
  }
  return false;
}

uint64_t ZAP_PE::send(uint64_t pe, uint64_t payload_id) {
  uint64_t gen_time;
  bool gen_time_pulled = false;
  //if (precinct_id == 1 && zone_id == 1 && zap_id == 1 && pe == 1) {
  //  printf("ZAP %lu PE %lu sent %lu, target %lu\n", zap_id, pe, num_sent[pe], msg_count[pe]);
  //}
  if (num_sent[pe] >= msg_count[pe]) {
    return 0;
  }
  uint64_t total_harts_per_precinct = total_pe_count / system_size_.num_precincts;
  uint64_t tgt_pe_id;
  double tgt_pe_base, tgt_pe_scale;
  if (next_pe_tgt[pe] < 0) {
    #ifdef ROUNDROBIN
      tgt_pe_id = next_to_send[pe] % total_pe_count;
      next_to_send[pe] = (next_to_send[pe] + 1) % total_pe_count;
    #endif
    #ifndef ROUNDROBIN
    tgt_pe_base = ((tgt_gen_real->generate_objects_single()));
    tgt_pe_scale = (tgt_gen_scale->generate_objects_single() );
    //printf("PE ID base: %.2f, scale %.2f\n", tgt_pe_base, tgt_pe_scale);
    tgt_pe_id = (uint64_t)((tgt_pe_base + 1)* tgt_pe_scale);
    //printf("PE ID final: %lu, max id %lu\n", tgt_pe_id, total_pe_count);
    #endif
    //printf("Tgt id %lu\n", tgt_pe_id);
    tgt_pe_id = std::min(tgt_pe_id - 1, total_pe_count - 1);
    gen_time = getCurrentSimTimeNano();
    //output_.verbose(CALL_INFO, 3, 0, "Payload tgt: (%" PRIu64 ")\n", tgt_pe_id);
  } else {
    tgt_pe_id = next_pe_tgt[pe];
    gen_time = next_pe_gen[pe];
    gen_time_pulled = true;
    next_pe_tgt[pe] = -1;
  }
  //printf("PE glob %lu\n", tgt_pe_id);
  uint64_t tgt_pe_mod = tgt_pe_id;
  uint64_t tgt_precinct = tgt_pe_id / total_harts_per_precinct;
  tgt_pe_mod %= total_harts_per_precinct;

  uint64_t tgt_zone_id = (tgt_pe_mod) /  (system_size_.num_zaps * system_size_.num_pes);
  tgt_pe_mod %= (system_size_.num_zaps * system_size_.num_pes);
  uint64_t tgt_zap_id = (tgt_pe_mod) / system_size_.num_pes;
  tgt_pe_mod %= system_size_.num_pes;
  //printf("Tgt id %lu %lu %lu %lu\n", tgt_precinct, tgt_zone_id, tgt_zap_id, tgt_pe_mod);
  /*if (eg_precinct_counters[tgt_precinct] > 0) {
    printf("tgt counter > 0\n");
  }*/
#ifndef INF_QUEUE_PE
  if (eg_precinct_counters[tgt_precinct] >= precinct_queue_size) { // TODO: Custom threshold
    // don't allow
    //printf("Not allowing from %lu to %lu! Size is %lu\n", precinct_id, tgt_precinct, eg_precinct_counters[tgt_precinct]);
    StallCycleBP->addData(1);
    next_pe_tgt[pe] = tgt_pe_id;
    // only update gen time iff its current time, i.e. it wasn't read back
    if (!gen_time_pulled) {
      next_pe_gen[pe] = getCurrentSimTimeNano();
    }
    return 2;
  }
#endif
  eg_precinct_counters[tgt_precinct]++;
  Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
    payload_id, tgt_precinct, tgt_zone_id,
    tgt_zap_id, tgt_pe_mod,
    size_gen->generate_objects_single()
  );

#ifdef PROC_TIME_TRACK
  pe_event->gen_time = gen_time;
  pe_event->sent_time = getCurrentSimTimeNano();
#endif
  pe_event->is_mem = false;
  pe_event->setSrc(zone_id, zap_id, pe);
  pe_event->src_pe_q_size = ev_queue[pe].size();
  if (false && pe_event->src_pe_q_size != 0) printf("Precinct ID %lu, PE ID %lu: %lu\n", precinct_id, get_precinct_pe_id(pe), pe_event->src_pe_q_size);

  if (payload_id == 0) {
    TotalSent->addData(1);
  }
#ifdef PE_STATS
  if (pe == 0) {
    PE0Sent->addData(1);
  } else if (pe == 1) {
    PE1Sent->addData(1);
  } else if (pe == 2) {
    PE2Sent->addData(1);
  } else if (pe == 3) {
    PE3Sent->addData(1);
  } else if (pe == 4) {
    PE4Sent->addData(1);
  } else if (pe == 5) {
    PE5Sent->addData(1);
  } else if (pe == 6) {
    PE6Sent->addData(1);
  } else if (pe == 7) {
    PE7Sent->addData(1);
  }
#endif
  zone_link_->send(pe_event);
  num_sent[pe]++;
  return 1;
}

bool ZAP_PE::recvActor(Cycle_t cycles){
  if (event_driven_recv != 0) {
    return false;
  }
  uint64_t pe_id;
  if (dynamic_proc_v1_enabled) {
    pe_id = (last_recv_pe_id + 1) % num_pes;
    // Pick first PE with currently running code, then pick
    while ((!cur_event[pe_id] && ev_queue[pe_id].empty()) || waiting_on_mem[pe_id]) {
      pe_id = (pe_id + 1) % num_pes;
      if (pe_id == (last_recv_pe_id + 1) % num_pes) {
        // Global inactive cycle
        break;
      }
    }
    //if (zone_id == 0 && precinct_id == 0) printf("Selected PE id %lu\n", pe_id);
  } else if (dynamic_proc_v2_enabled) {
    pe_id = (last_recv_pe_id + 1) % num_pes;
    // Pick first PE with currently running code, then pick
    uint64_t worst_case_pe = pe_id;
    bool worst_case_pe_set = false;
    while (!cur_event[pe_id]) {
      if (ev_queue[pe_id].size() < 5 && !worst_case_pe_set) {
        worst_case_pe = pe_id;
        worst_case_pe_set = true;
      } else if (ev_queue[pe_id].size() > 5) { break; }
      pe_id = (pe_id + 1) % num_pes;
      if (pe_id == (last_recv_pe_id + 1) % num_pes) {
        // Global inactive cycle
        break;
      }
    }
    if (worst_case_pe_set) {
      pe_id = worst_case_pe;
    }
    //if (zone_id == 0 && precinct_id == 0) printf("Selected PE id %lu\n", pe_id);
  } else {
    pe_id = cycles % num_pes; //((cycles - 1)/2) % num_pes;
  }
  last_recv_pe_id = pe_id;
  if (cur_event[pe_id]) {
    if (cur_ins[pe_id] == tot_ins[pe_id] && !waiting_on_mem[pe_id]) {
      cur_event[pe_id] = false;
      cur_ins[pe_id] = 0;
      mem_ins[pe_id] = 0;
      tot_ins[pe_id] = 0;

      TotalProc->addData(1);
      LastProcCycle->addData(cycles);
      // Update counters to precinct
      if (true) {
        Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
          3, 0, 0,
          0, 0,
          0
        );
#ifdef PROC_TIME_TRACK
        pe_event->sent_time = getCurrentSimTimeNano();
#endif
        pe_event->is_mem = false;
        pe_event->setSrc(zone_id, zap_id, pe_id);
        pe_event->src_pe_q_size = ev_queue[pe_id].size();
        zone_link_->send(pe_event);
      }
    } else {
      //printf("Test\n");
      #ifndef DISABLE_MEM_RECV
      //printf("Test\n");
      if (waiting_on_mem[pe_id]) {
        // nothing
      } else if (mem_ins[pe_id] > 0) {
        output_.verbose(CALL_INFO, 3, 0, "Mem op!\n");
        Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
          0, 0, 0,
          0, 0,
          2
        );
        pe_event->setTarget(zap_id, pe_id);
        zone_link_->send(pe_event);
        waiting_on_mem[pe_id] = true;
        mem_ins[pe_id]--;
        cur_ins[pe_id] += 1;
      } else
      #endif
      {
        cur_ins[pe_id] += cur_ipc[pe_id];
        TotalRecvIns->addData(1);
      }
    }
  } else if (!ev_queue[pe_id].empty()) {
    /*if (num_sent[pe_id] >= msg_count[pe_id]) {
      cur_event[pe_id] = false;
      endSim(pe_id);
      return false;
    }*/
    cur_event[pe_id] = true;
    cur_ins[pe_id] = 0;
    mem_ins[pe_id] = 0;
    tot_ins[pe_id] = ev_queue[pe_id].back();
    cur_ipc[pe_id] = ipc_recv->generate_objects_single();
    mem_ins[pe_id] = mem_ins_gen_recv->generate_objects_single();
    if (mem_ins[pe_id] > tot_ins[pe_id]) {
      mem_ins[pe_id] = tot_ins[pe_id];
    }
    mem_ins_ratio[pe_id] = mem_ins[pe_id] * 100.0 / tot_ins[pe_id];
    ev_queue[pe_id].pop_back();
  } else {
    if (pe_complete_counters[pe_id] == total_pe_count) {
      //printf("(%lu, %lu, %lu, %lu): %lu, tgt, %lu\n", precinct_id, zone_id, zap_id, tgt_pe, pe_complete_counters[tgt_pe], total_pe_count);
      complete_pe_count++;
      pe_complete_counters[pe_id]++;  // so that this condition is only true once
      endSim(pe_id);
    }
    IdleCycles->addData(1);
  }
  return false;
}

void ZAP_PE::genNewSendTask(uint64_t pe_id) {
  //printf("New send task\n");
  if (num_sent[pe_id] == msg_count[pe_id]) {
    // Send side termination
#ifdef END_SEMANTICS
    Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
      4, 0, 0,
      0, 0,
      1
    );
    pe_event->setSrc(zone_id, zap_id, pe_id);
    zone_link_->send(pe_event);
    num_sent[pe_id]++;
    complete_send_pe_count++;
#endif
    return;
  }
  if (num_sent[pe_id] > msg_count[pe_id]) {
    assert(false);
  }
  cur_event_send[pe_id] = true;
  mem_ins_send[pe_id] = std::ceil(mem_ins_gen_send->generate_objects_single() * send_lst_ratio);
  cur_ipc_send[pe_id] = ipc_send->generate_objects_single();

  tot_ins_send[pe_id] = std::ceil(ins_gen_send->generate_objects_single() * send_tot_ratio);
  int r = 0;
  while (tot_ins_send[pe_id] == 0 && r < 5) {
    uint64_t base_send = ins_gen_send->generate_objects_single();
    //printf("Base send, prod, scale %lu %lu %.2f", base_send, base_send * send_tot_ratio, send_tot_ratio);
    if (base_send > 0 && std::ceil(base_send * send_tot_ratio) < 0) {
      tot_ins_send[pe_id] = 1;
    } else {
      tot_ins_send[pe_id] = std::ceil(base_send * send_tot_ratio);
    }
    ++r;
  }
  if (r == 5) {
    tot_ins_send[pe_id] = 1;
  }
  if (mem_ins_send[pe_id] > tot_ins_send[pe_id]) {
    mem_ins_send[pe_id] = tot_ins_send[pe_id];
  }
  waiting_on_mem_send[pe_id] = false;
  mem_ins_ratio_send[pe_id] = mem_ins_send[pe_id] * 100.0 / tot_ins_send[pe_id];
#ifdef DISABLE_MEM_SEND
  mem_ins_ratio_send[pe_id] = mem_ins_send[pe_id] = 0;
#endif
  cur_ins_send[pe_id] = 0;
  TotalSendIns->addData(tot_ins_send[pe_id]);
  //printf("Tot, mem, ipc: %lu, %lu, %lu\n", tot_ins_send[pe_id], mem_ins_send[pe_id], cur_ipc_send[pe_id]);
  // schedule event for mem events
  // for now just complete
  cur_ins_send[pe_id] = tot_ins_send[pe_id] - mem_ins_send[pe_id];
  ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(pe_id,  ZAP_PE::PEInternalEvent::SEND_MEM);
  pe_send_link_->send(num_pes * (tot_ins_send[pe_id] - mem_ins_send[pe_id] - 1)*1.0/cur_ipc_send[pe_id], pe_event);
  //pe_send_link_->send(num_pes, pe_event);
}

void ZAP_PE::nextSendMem(uint64_t pe_id) {
  ///printf("Mem ins -> %lu, cur_ins -> %.2f, delta -> %.2f\n", mem_ins_send[pe_id], cur_ins_send[pe_id], tot_ins_send[pe_id] - cur_ins_send[pe_id]);
  if (mem_ins_send[pe_id] == 0) {
    ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(pe_id,  ZAP_PE::PEInternalEvent::SEND_COMP);
    //printf("Send fwd by %lu ins\n", tot_ins_send[pe_id] - cur_ins_send[pe_id]);
    pe_send_link_->send(num_pes * (tot_ins_send[pe_id] - cur_ins_send[pe_id]), pe_event);
  } else {
    Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
      0, 0, 0,
      0, 0,
      1
    );
    pe_event->setTarget(zap_id, pe_id);
    zone_link_->send(pe_event);
    waiting_on_mem_send[pe_id] = true;
    mem_ins_send[pe_id]--;
    cur_ins_send[pe_id] += 1;
  }
}

void ZAP_PE::completeSendTask(uint64_t pe_id, uint64_t next_delay) {
  if (num_sent[pe_id] >= msg_count[pe_id]) {
    assert(false);;
  }
  uint64_t res = send(pe_id, 0);
  if (res == 1) {
    // success
    cur_event_send[pe_id] = false;
    cur_ins_send[pe_id] = 0;
    mem_ins_send[pe_id] = 0;
    tot_ins_send[pe_id] = 0;
    waiting_on_mem_send[pe_id] = false;
    ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(pe_id, ZAP_PE::PEInternalEvent::SEND_NEW);
    pe_send_link_->send(num_pes, pe_event);
  } else if (res == 0) {
    return;
  } else {
    // No space to send, retry
    ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(pe_id, ZAP_PE::PEInternalEvent::SEND_COMP);
    pe_event->next_delay = next_delay * 6;
    pe_event->next_delay = std::min((uint64_t)1000, pe_event->next_delay);
    pe_send_link_->send(num_pes*pe_event->next_delay, pe_event);
  }
}


void ZAP_PE::genNewRecvTask(uint64_t pe_id) {
  if (cur_event[pe_id] || ev_queue[pe_id].size() == 0) {
    return;
  }

  FILE* fp = fopen("pe_idle.csv", "a+");
  fprintf(fp, "%lu, %lu\n", get_precinct_pe_id(pe_id), getCurrentSimTimeNano() - last_proc_cycle[pe_id]);
  last_proc_cycle[pe_id] = -1;
  fclose(fp);
  cur_event[pe_id] = true;
  mem_ins[pe_id] = std::ceil(mem_ins_gen_recv->generate_objects_single() * recv_lst_ratio);
  cur_ipc[pe_id] = ipc_recv->generate_objects_single();
  tot_ins[pe_id] = ev_queue[pe_id].back();
  if (mem_ins[pe_id] > tot_ins[pe_id]) {
    mem_ins[pe_id] = tot_ins[pe_id];
  }
  waiting_on_mem[pe_id] = false;
  mem_ins_ratio[pe_id] = mem_ins[pe_id] * 100.0 / tot_ins[pe_id];
  ev_queue[pe_id].pop_back();
  TotalRecvIns->addData(tot_ins[pe_id]);
  // for now just complete
#ifdef DISABLE_MEM_RECV
  mem_ins_ratio[pe_id] = mem_ins[pe_id] = 0;
#endif
  cur_ins[pe_id] = 0;
  // schedule event for mem events
  // for now just complete
  ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(pe_id,  ZAP_PE::PEInternalEvent::RECV_MEM);
  //pe_recv_link_->send(num_pes, pe_event);
  cur_ins[pe_id] = tot_ins[pe_id] - mem_ins[pe_id];
  //printf("Recv Mem ins -> %lu, cur_ins -> %.2f, delta -> %.2f, tot -> %.2f\n", mem_ins[pe_id], cur_ins[pe_id], tot_ins[pe_id] - cur_ins[pe_id], tot_ins[pe_id]);
  pe_recv_link_->send(num_pes*(tot_ins[pe_id] - mem_ins[pe_id] - 1), pe_event);
}

void ZAP_PE::nextRecvMem(uint64_t pe_id) {
  //printf("Recv Mem ins -> %lu, cur_ins -> %.2f, delta -> %.2f\n", mem_ins[pe_id], cur_ins[pe_id], tot_ins[pe_id] - cur_ins[pe_id]);
  if (mem_ins[pe_id] == 0) {
    ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(pe_id,  ZAP_PE::PEInternalEvent::RECV_COMP);
    pe_recv_link_->send(num_pes * (tot_ins[pe_id] - cur_ins[pe_id]), pe_event);
  } else {
    Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
      0, 0, 0,
      0, 0,
      2
    );
    pe_event->setTarget(zap_id, pe_id);
    zone_link_->send(pe_event);
    waiting_on_mem[pe_id] = true;
    mem_ins[pe_id]--;
    cur_ins[pe_id] += 1;
  }
}

void ZAP_PE::completeRecvTask(uint64_t pe_id) {

  cur_event[pe_id] = false;
  mem_ins[pe_id] = 0;
  mem_ins_ratio[pe_id] = 0;
  waiting_on_mem[pe_id] = false;
  TotalProc->addData(1);

  Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
    3, 0, 0,
    0, 0,
    0
  );
#ifdef PROC_TIME_TRACK
  pe_event->sent_time = getCurrentSimTimeNano();
#endif
  pe_event->is_mem = false;
  pe_event->setSrc(zone_id, zap_id, pe_id);
  pe_event->src_pe_q_size = ev_queue[pe_id].size();
  zone_link_->send(pe_event);
  if (ev_queue[pe_id].size() != 0) {
    // No space to send, retry
    ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(pe_id, ZAP_PE::PEInternalEvent::RECV_NEW);
    pe_recv_link_->send(num_pes - 1, pe_event);
    last_proc_cycle[pe_id] = getCurrentSimTimeNano();
  } else if (pe_complete_counters[pe_id] == total_pe_count) {
    //printf("(%lu, %lu, %lu, %lu): %lu, tgt, %lu\n", precinct_id, zone_id, zap_id, tgt_pe, pe_complete_counters[tgt_pe], total_pe_count);
    complete_pe_count++;
    pe_complete_counters[pe_id]++;  // so that this condition is only true once
    printf("Precinct %lu, Zone %lu, ZAP %lu, PE %lu exiting at %lu\n", precinct_id, zone_id, zap_id, pe_id, getCurrentSimTimeNano());
    endSim(pe_id);
  }
}

void ZAP_PE::handleSendEvent(SST::Event* _event) {
  ZAP_PE::PEInternalEvent* event = dynamic_cast<ZAP_PE::PEInternalEvent*>(_event);
  //printf("Handler PE id: %lu\n", event->pe_id);
  //printf("Time on send: %lu, task type: %lu\n", getCurrentSimTimeNano(), event->type);
  if (event->type == ZAP_PE::PEInternalEvent::SEND_NEW) {
    genNewSendTask(event->pe_id);
  } else if (event->type == ZAP_PE::PEInternalEvent::SEND_MEM) {
    nextSendMem(event->pe_id);
  } else if (event->type == ZAP_PE::PEInternalEvent::SEND_COMP) {
    completeSendTask(event->pe_id, event->next_delay);
  } else {
    printf("Error\n");
  }
  delete event;
}

void ZAP_PE::handleRecvEvent(SST::Event* _event) {
 //printf("Time on recv: %lu\n", getCurrentSimTimeNano());
  ZAP_PE::PEInternalEvent* event = dynamic_cast<ZAP_PE::PEInternalEvent*>(_event);
  if (event->type == ZAP_PE::PEInternalEvent::RECV_NEW) {
    genNewRecvTask(event->pe_id);
  } else if (event->type == ZAP_PE::PEInternalEvent::RECV_MEM) {
    //if (event->next_delay != 1) printf("Recv at %lu, lat %lu\n", getCurrentSimTimeNano(), event->next_delay);
    nextRecvMem(event->pe_id);
  }  else if (event->type == ZAP_PE::PEInternalEvent::RECV_COMP) {
    completeRecvTask(event->pe_id);
  } else {
    printf("Error\n");
  }
  delete event;

}

void ZAP_PE::handleZoneEvent(SST::Event* _event, int _port_num) {
  Precinct::PrecinctEvent* event = dynamic_cast<Precinct::PrecinctEvent*>(_event);
  if (event) {
    if (event->payload == 1) {
      delete event;
      endSim(0);
      return;
    }
    output_.verbose(CALL_INFO, 3, 0, "Received zone event on port %u ns=%" PRIu64 ".\n",
                    _port_num, getCurrentSimTimeNano());
    uint64_t tgt_pe = (uint64_t)event->tgt_pe;
    if (tgt_pe >= num_pes) {
      output_.fatal(CALL_INFO, -1, "Received bad event target on port %u.\n", _port_num);
    }
    if (event->payload == 2) {
      output_.verbose(CALL_INFO, 3, 0, "tgt_pe %lu, size %lu\n", tgt_pe, event->sz);
      if (event->sz == 2) {
        waiting_on_mem[tgt_pe] = false;
        if (event_driven_recv != 0) {
          ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(tgt_pe, ZAP_PE::PEInternalEvent::RECV_MEM);
          pe_event->next_delay = event->mem_comp_ns;
          //printf("Sending at %lu, lat %lu\n", getCurrentSimTimeNano(), event->mem_comp_ns);
          pe_recv_link_->send((event->mem_comp_ns)*2, pe_event);
        }
        delete event;
      } else {
        waiting_on_mem_send[tgt_pe] = false;
        output_.verbose(CALL_INFO, 3, 0, "tgt_pe %lu, size %lu no wait, cur_event %u, cur_ins %lu, tot_ins %lu\n", tgt_pe, event->sz,
                        (unsigned)cur_event_send[tgt_pe], cur_ins_send[tgt_pe], tot_ins_send[tgt_pe]);
        if (event_driven_send != 0) {
          ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(tgt_pe, ZAP_PE::PEInternalEvent::SEND_MEM);
          pe_send_link_->send((event->mem_comp_ns)*2, pe_event);
        }
        delete event;
      }
      return;
    } else if (event->payload == 3) {
      eg_precinct_counters[event->src_precinct] = event->src_precinct_q_size;
      delete event;
    } else if (event->payload == 4) {
      pe_complete_counters[tgt_pe]++;
      //printf("PE: %lu finack, set to %lu\n", tgt_pe, pe_complete_counters[tgt_pe]);
      if (event_driven_recv != 0) {
        if (pe_complete_counters[tgt_pe] == total_pe_count && ev_queue[tgt_pe].empty()) {
          // Empty queue and all completes received - new call
          complete_pe_count++;
          pe_complete_counters[tgt_pe]++;  // so that this condition is only true once
          endSim(tgt_pe);
        }
      }
      delete event;
    } else if (event->payload == 5) {
      if (demo_mode) {
        printf("Receiving SEQ no %lu, total handled %lu\n", event->sz, num_recv[tgt_pe]);
        num_recv[tgt_pe]++;
        if (num_recv[tgt_pe] == 1000) {
          complete_pe_count++;
          pe_complete_counters[tgt_pe]++;  // so that this condition is only true once
          endSim(tgt_pe);
        }
      } else {
        assert(false);
      }
    } else {
#ifdef PROC_TIME_TRACK
      event->recv_time = getCurrentSimTimeNano();
      if (lat_file_name.length() > 0 && event->src_precinct != precinct_id) {
        FILE* fp = fopen(lat_file_name.c_str(), "a+");
        fprintf(fp, "%lu, %d\n", getCurrentSimTimeNano(), event->recv_time - event->sent_time);
        fclose(fp);
      }
      SendToReceive->addData(event->recv_time - event->sent_time);
#endif
      uint64_t ins_num = ins_gen_recv->generate_objects_single();
      int r =  0;
      while (ins_num == 0) {
        ins_num = ins_gen_recv->generate_objects_single();
        ++r;
        if (r > 5) {
          ins_num = 1;
          break;
        }
      }
      ev_queue[tgt_pe].push_back(std::max(1.0,std::ceil(ins_num * recv_tot_ratio)));
      if (event_driven_recv != 0) {
        ZAP_PE::PEInternalEvent *pe_event = new ZAP_PE::PEInternalEvent(tgt_pe, ZAP_PE::PEInternalEvent::RECV_NEW);
        pe_recv_link_->send(pe_event);
      }
      TotalRecv->addData(1);
      if (event->src_precinct == precinct_id) {
        TotalRecvInternal->addData(1);
      }
#ifdef PE_STATS
      if (tgt_pe == 0) {
        PE0Recv->addData(1);
      } else if (tgt_pe == 1) {
        PE1Recv->addData(1);
      } else if (tgt_pe == 2) {
        PE2Recv->addData(1);
      } else if (tgt_pe == 3) {
        PE3Recv->addData(1);
      } else if (tgt_pe == 4) {
        PE4Recv->addData(1);
      } else if (tgt_pe == 5) {
        PE5Recv->addData(1);
      } else if (tgt_pe == 6) {
        PE6Recv->addData(1);
      } else if (tgt_pe == 7) {
        PE7Recv->addData(1);
      }
  #endif
      delete event;
    }
  } else {
    output_.fatal(CALL_INFO, -1, "Received bad event type on port %u.\n",
                  _port_num);
  }
}

}  // namespace forzasim_macro
}  // namespace SST
