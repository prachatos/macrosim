#ifndef ZAP_PE_H_2BD0D0B8
#define ZAP_PE_H_2BD0D0B8

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>
#include "sst/forzasim_macro/Precinct.h"
#include "sst/forzasim_macro/Generator.h"
#include "sst/core/shared/sharedMap.h"

namespace SST {
namespace forzasim_macro {

class ZAP_PE : public SST::Component {
 public:

  struct PEInternalEvent : public SST::Event {
    PEInternalEvent() = default;
    ~PEInternalEvent() override = default;
    uint64_t pe_id;
    typedef enum {
      SEND_NEW,
      SEND_MEM,
      SEND_COMP,
      RECV_NEW,
      RECV_MEM,
      RECV_COMP,
    } ev_type;
    ev_type type;
    uint64_t next_delay;

    PEInternalEvent(uint64_t pe_id, ev_type type) : pe_id(pe_id), type(type), next_delay(1) {}
    void serialize_order(SST::Core::Serialization::serializer& _serializer) {
      SST::Event::serialize_order(_serializer);
      _serializer & pe_id;
      _serializer & type;
      _serializer & next_delay;
    }

    ImplementSerializable(SST::forzasim_macro::ZAP_PE::PEInternalEvent);
  };

  ZAP_PE(SST::ComponentId_t _id, SST::Params& _params);
  ~ZAP_PE() override;

  void setup() override;
  void finish() override;

  void genNewSendTask(uint64_t pe_id);

  void nextSendMem(uint64_t pe_id);

  void completeSendTask(uint64_t pe_id, uint64_t next_delay);

  void handleSendEvent(SST::Event *_event);

  void genNewRecvTask(uint64_t pe_id);

  void nextRecvMem(uint64_t pe_id);

  void completeRecvTask(uint64_t pe_id);

  void handleRecvEvent(SST::Event *_event);

  SST_ELI_REGISTER_COMPONENT(
      ZAP_PE,
      "forzasim_macro",
      "ZAP_PE",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "forzasim_macro ZAP_PE",
      COMPONENT_CATEGORY_UNCATEGORIZED)

  SST_ELI_DOCUMENT_PORTS(
    {"zone_port", "Link to zone.", {"forzasim_macro.Zone"}},
    {"pe_send_port", "Link to send port.", {"forzasim_macro.ZAP_PE"}},
    {"pe_recv_port", "Link to recv port.", {"forzasim_macro.ZAP_PE"}},
  )

  SST_ELI_DOCUMENT_PARAMS(
    {"num_ZAP_PEs", "Number of ZAP_PEs", "1"}
  )

  SST_ELI_DOCUMENT_STATISTICS(
    {"TotalRecv", "Total msgs received so far", "unitless", 1},
    {"TotalSent", "Total msgs received so far", "unitless", 1},
    {"TotalProc", "Total msgs received so far", "unitless", 1},
    {"SendToReceive", "Total msgs received so far", "unitless", 1},
    {"NumToSend", "Total msgs received so far", "unitless", 1},
    {"StallCycleBP", "Stall cycle", "unitless", 1},
    {"InactiveCycles", "Stall cycle", "unitless", 1},
    {"Cycles", "Stall cycle", "unitless", 1},
    {"LastProcCycle", "Stall cycle", "unitless", 1},
  )


 private:
  ZAP_PE(const ZAP_PE&) = delete;
  void operator=(const ZAP_PE&) = delete;

  virtual void handleZoneEvent(SST::Event* _event, int _port_num);
  bool recvActor(Cycle_t cycles);
  bool sendActor(Cycle_t cycles);
  uint64_t send(uint64_t pe, uint64_t payload_id);
  void endSim(uint64_t pe);
  bool actor(Cycle_t cycles);
  DistGenerator* initSharedMap(std::string fname, std::string dist_name);
  std::map<uint64_t, double> generateMapFromCSV(const std::string& csvFilePath);
  std::map<uint64_t, double> generateMapFromCSVOld(const std::string& csvFilePath);
  uint64_t get_precinct_pe_id(uint64_t pe_id);

  // Output
  SST::Output output_;


  uint64_t num_pes;

  Shared::SharedMap<uint64_t, double> data;

  // Links
  SST::Link* zone_link_;
  SST::Link* pe_send_link_;
  SST::Link* pe_recv_link_;

  // Random
  SST::RNG::MersenneRNG random_;

  std::string string_format(const std::string fmt_str, ...);

  std::map<uint64_t, std::vector<uint64_t> > ev_queue;
  std::vector<bool> cur_event, cur_event_send;
  TimeConverter* recv_tc;
  TimeConverter* send_tc;
  std::vector<uint64_t> msg_count;
  std::vector<uint64_t> num_sent;
  bool is_max_send;
  uint64_t max_send_pe;
  uint64_t precinct_queue_size;
  uint64_t dynamic_proc_v1_enabled;
  uint64_t dynamic_proc_v2_enabled;
  uint64_t last_recv_pe_id;

  struct {
    uint64_t num_precincts;
    uint64_t num_zones;
    uint64_t num_zaps;
    uint64_t num_pes;
  } system_size_;

  SST::RNG::Random* rng;

  Clock::HandlerBase *send_handler;
  Clock::HandlerBase *recv_handler;

  DistGenerator* tgt_gen_real;
  DistGenerator* tgt_gen_scale;

  DistGenerator* ins_gen_recv;
  DistGenerator* ins_gen_send;
  DistGenerator* size_gen;
  DistGenerator* mem_ins_gen_recv;
  DistGenerator* mem_ins_gen_send;
  DistGenerator* mem_rand;
  DistGenerator* ipc_send;
  DistGenerator* ipc_recv;

  std::vector<uint64_t> tot_ins, tot_ins_send;
  std::vector<uint64_t> num_recv;
  std::vector<double> cur_ins, cur_ins_send, cur_ipc_send, cur_ipc;
  std::vector<uint64_t> mem_ins, mem_ins_send;
  std::vector<uint64_t> mem_ins_ratio, mem_ins_ratio_send;
  std::vector<int64_t> next_pe_tgt;
  std::vector<int64_t> next_pe_gen;
  std::vector<int64_t> next_to_send;
  std::vector<uint64_t> send_next_wakeup_cycle;
  std::vector<uint64_t> recv_next_wakeup_cycle;
  std::vector<uint64_t> pe_complete_counters;
  std::vector<uint64_t> last_proc_cycle;
  uint64_t total_pe_count;
  uint64_t complete_pe_count;
  uint64_t complete_send_pe_count;
  std::vector<bool> waiting_on_mem, waiting_on_mem_send;
  std::vector<bool> pe_send_done;
  std::string lat_file_name;
  uint64_t zap_id;
  uint64_t global_pe_id;

  double scale_factor;
  bool static_dist;

  std::string size_file;
  std::string ins_gen_recv_file;
  std::string ins_gen_send_file;
  std::string mem_ins_gen_send_file;
  std::string mem_ins_gen_recv_file;
  std::string tgt_gen_file;
  std::string ipc_send_file;
  std::string ipc_recv_file;

  uint64_t msgc;
  uint64_t precinct_id;
  uint64_t zone_id;
  uint64_t size_min, size_max;
  uint64_t send_ins_min, send_ins_max;
  uint64_t recv_ins_min, recv_ins_max;
  uint64_t recv_mem_min, recv_mem_max;
  uint64_t send_mem_min, send_mem_max;
  double ipc_send_min, ipc_send_max;
  double ipc_recv_min, ipc_recv_max;
  uint64_t tgt_gen_count;
  uint64_t all_bp_stalls;
  uint64_t event_driven_recv, event_driven_send;
  uint64_t demo_mode;

  double recv_tot_ratio, recv_lst_ratio;
  double send_tot_ratio, send_lst_ratio;

  bool use_static_sends;
  uint64_t num_static_sends;

  Statistic<uint64_t>* TotalSent;
  Statistic<uint64_t>* TotalRecv;
  Statistic<uint64_t>* TotalRecvInternal;
  Statistic<uint64_t>* TotalProc;
  Statistic<uint64_t>* SendToReceive;
  Statistic<uint64_t>* NumToSend;
  Statistic<uint64_t>* StallCycleBP;
  Statistic<uint64_t>* IdleCycles;
  Statistic<uint64_t>* InactiveCycles;
  Statistic<uint64_t>* Cycles;
  Statistic<uint64_t>* LastSentCycle;
  Statistic<uint64_t>* LastProcCycle;
  Statistic<uint64_t>* TotalSendIns;
  Statistic<uint64_t>* TotalRecvIns;
  Statistic<uint64_t>* MemStalls;
#ifdef PE_STATS
  Statistic<uint64_t>* PE0Sent;
  Statistic<uint64_t>* PE0Recv;
  Statistic<uint64_t>* PE1Sent;
  Statistic<uint64_t>* PE1Recv;
  Statistic<uint64_t>* PE2Sent;
  Statistic<uint64_t>* PE2Recv;
  Statistic<uint64_t>* PE3Sent;
  Statistic<uint64_t>* PE3Recv;
  Statistic<uint64_t>* PE4Sent;
  Statistic<uint64_t>* PE4Recv;
  Statistic<uint64_t>* PE5Sent;
  Statistic<uint64_t>* PE5Recv;
  Statistic<uint64_t>* PE6Sent;
  Statistic<uint64_t>* PE6Recv;
  Statistic<uint64_t>* PE7Sent;
  Statistic<uint64_t>* PE7Recv;
#endif
  //SST::Shared::SharedArray<uint64_t> eg_precinct_counters;
  std::vector<uint64_t> eg_precinct_counters;
  SST::Shared::SharedArray<uint64_t> pe_counters;

};

}  // namespace forzasim_macro
}  // namespace SST

#endif  // ZAP_PE_H_2BD0D0B8

