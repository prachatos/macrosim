#ifndef PRECINCT_H_19435224
#define PRECINCT_H_19435224
#define PROC_TIME_TRACK
#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>
#include <sst/core/eli/elementinfo.h>
#include <sst/core/rng/mersenne.h>
#include <sst/core/interfaces/simpleNetwork.h>
#include <sst/core/shared/sharedArray.h>


#include <sst_config.h>

#include <algorithm>
#include <cstdint>

#include <limits.h>

#include "sst/forzasim_macro/ZAP.h"


namespace SST {
namespace forzasim_macro {

class Precinct : public SST::Component {
 public:
  struct PrecinctMetaEvent : public SST::Event {
    PrecinctMetaEvent() = default;
    ~PrecinctMetaEvent() override = default;
    uint64_t src_precinct;
    uint64_t src_precinct_q_size;
    bool term_event;
    void serialize_order(SST::Core::Serialization::serializer& _serializer)
        override;
    ImplementSerializable(SST::forzasim_macro::Precinct::PrecinctMetaEvent);
  };

  struct PrecinctEvent : public SST::Event {
    PrecinctEvent() = default;
    PrecinctEvent(char payload, uint64_t precinct, uint64_t zone, uint64_t zap, uint64_t pe, uint64_t sz)
      : payload(payload), tgt_precinct(precinct), tgt_zone(zone), tgt_zap(zap), tgt_pe(pe), sz(sz),
        pe_id(0), zap_id(0), is_mem(false), mem_comp_ns(0), src_pe_q_size(INT_MAX) {}
    ~PrecinctEvent() override = default;
    void setSrc(uint64_t zn_id, uint64_t zid, uint64_t pid) {
      zone_id = zn_id;
      zap_id = zid;
      pe_id = pid;
    }
    void setTarget(uint64_t zid, uint64_t pid) {
      zap_id = zid;
      pe_id = pid;
      is_mem = true;
    }
    std::pair<uint64_t, uint64_t> getTarget(void) {
      return std::make_pair(zap_id, pe_id);
    }
    char payload;
    // TODO: Use a struct for address
    uint64_t tgt_precinct;
    uint64_t tgt_zone;
    uint64_t tgt_zap;
    uint64_t tgt_pe;
    uint64_t sz;
  #ifdef PROC_TIME_TRACK
    uint64_t gen_time;
    uint64_t sent_time;
    uint64_t recv_time;
    uint64_t process_start_time;
  #endif
    uint64_t mem_comp_ns;
    bool is_mem;
    uint64_t zap_id, pe_id, zone_id;
    uint64_t src_precinct;
    uint64_t src_pe_q_size, src_precinct_q_size;
    void serialize_order(SST::Core::Serialization::serializer& _serializer)
        override;
    ImplementSerializable(SST::forzasim_macro::Precinct::PrecinctEvent);
  };

  struct PrecinctEventList : public SST::Event {
    PrecinctEventList() = default;
    ~PrecinctEventList() override = default;

    std::vector<PrecinctEvent*> event_list;
    void serialize_order(SST::Core::Serialization::serializer& _serializer)
        override;
    ImplementSerializable(SST::forzasim_macro::Precinct::PrecinctEventList);
  };
 public:
  Precinct(SST::ComponentId_t _id, SST::Params& _params);
  ~Precinct() override;

  void init(unsigned int phase) override;
  void setup() override;
  void complete(unsigned int phase) override;
  void finish() override;

  bool handleNetworkEvent(int i);
  void handleMetaEvent(SST::Event* _event, int i);


  SST_ELI_REGISTER_COMPONENT(
      Precinct,
      "forzasim_macro",
      "Precinct",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "forzasim_macro Precinct",
      COMPONENT_CATEGORY_UNCATEGORIZED)

  SST_ELI_DOCUMENT_PORTS(
      {"zone_port_%(portnum)d", "Links to Zones.", {"forzasim_macro.Zone"}},
      {"precinct_meta_port_%(portnumsrc)d_%(portnumdest)d", "Links to Precincts for Meta.", {"forzasim_macro.Precinct"}},
      {"rtrLink", "Links to Network.", {"merlin.linkcontrol"}}
  )

  SST_ELI_DOCUMENT_PARAMS(
      {"num_zones", "Number of zones", "1"},
      {"id", "Id of the Precinct"},
      {"traversed_edges", "Number of edges traversed", "1"},
      {"max_aggr_size", "Maximum number of packets to aggregate (set to 0 to disable)", "0"}
  )

  SST_ELI_DOCUMENT_STATISTICS(
  )

 private:
  Precinct(const Precinct&) = delete;
  void operator=(const Precinct&) = delete;

  void handleZoneEvent(SST::Event* _event, int _port_num);
  void handleEgressEvent(SST::Event* _event);
  void processIngress(uint64_t tgt_pe_id);
  void processEgress(Cycle_t cycles);
  void processMeta(Cycle_t cycles);
  bool drainQueue(Cycle_t cycles);
  void drainQueuePrecinct(uint64_t precinct_id);
  uint64_t get_precinct_pe_id(uint64_t zone_id, uint64_t zap_id, uint64_t pe_id);

  //uint64_t precinct_id;
  uint64_t max_aggr_size;
  uint64_t max_aggr_timeout;
  TimeConverter* tc;
  bool initialized;

  // Output
  SST::Output output_;

  uint64_t id;
  uint64_t num_pes;
  uint64_t num_zaps;
  uint64_t num_zones;
  uint64_t num_precincts;
  bool initBroadcastSent;
  uint64_t   numDest;
  uint64_t traversedEdges;
  uint64_t pes_in_precinct;
  uint64_t nw_header_bits;
  uint64_t zop_header_bits;
  uint64_t pe_queue_size;
  uint64_t total_sends, total_sent_pkt;
  uint64_t only_max;
    uint64_t aggr_timeout;
  uint64_t num_zones_completed;
  uint64_t num_precincts_completed;
  uint64_t demo_mode;
  SST::Interfaces::SimpleNetwork*     m_linkControl;

  SST::Link* pc_send_link_;
  // Links
  std::vector<SST::Link*> links_;
  std::vector<SST::Link*> meta_links_;

  std::vector<std::vector<PrecinctEvent*>> eg_queue_;
  //Statistic<double>* AvgAggrSize;
  Statistic<uint64_t>* TotRecvPrecinct;
  Statistic<uint64_t>* TotSentPrecinct;
  Statistic<uint64_t>* TotRecvPrecinctInc;
  std::vector<std::vector<PrecinctEvent*> > ing_queue_;
  std::vector<uint64_t> pe_counters;
  std::vector<std::pair<uint64_t, bool> > ing_precinct_counters;
  std::vector<std::pair<uint64_t, bool> >  eg_precinct_counters;
  std::string nw_file_name;

  // Random
  SST::RNG::MersenneRNG random_;
};

}  // namespace forzasim_macro
}  // namespace SST

#endif  /* end of include guard: PRECINCT_H_19435224 */
