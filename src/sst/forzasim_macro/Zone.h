#ifndef ZONE_H_5CE0F319
#define ZONE_H_5CE0F319

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>
#include <sst/core/eli/elementinfo.h>
#include <sst/core/rng/mersenne.h>

#include "ZAP_memctrl_simple.h"
#include <cstdint>

namespace SST {
namespace forzasim_macro {

class Zone : public SST::Component {
 public:
  Zone(SST::ComponentId_t _id, SST::Params& _params);
  ~Zone() override;

  void setup() override;
  void finish() override;

  SST_ELI_REGISTER_COMPONENT(
      Zone,
      "forzasim_macro",
      "Zone",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "forzasim_macro Zone",
      COMPONENT_CATEGORY_UNCATEGORIZED)

  SST_ELI_DOCUMENT_PORTS(
    {"zap_port_%(portnum)d", "Links to Zaps.", {"forzasim_macro.Zap"}},
    {"precinct_port", "Link to precinct.", {"forzasim_macro.Precinct"}}
  )

  SST_ELI_DOCUMENT_PARAMS(
      {"num_zaps", "Number of zaps", "1"}
  )

  SST_ELI_DOCUMENT_STATISTICS(
  )

  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    {"memory", "Backend Zone memory infrastructure", "SST::forzasim_macro::ZAPMemCtrlSimple"}
  )

 private:
  Zone(const Zone&) = delete;
  void operator=(const Zone&) = delete;

  virtual void handleZapEvent(SST::Event* _event, int _port_num);
  virtual void handlePrecinctEvent(SST::Event* _event, int _port_num);
  bool memSendHandler(Cycle_t cycles);

  // Output
  SST::Output output_;
  TimeConverter* mem_send_tc;

  uint64_t num_zaps;
  uint64_t demo_mode;

  ZAPMemCtrlSimple* Ctrl;

  // Links
  std::vector<SST::Link*> links_;
  SST::Link* precinct_link_;
  std::vector<ZAPMemTargetSimple*> pending_tgts;
  uint64_t num_zaps_completed;

  // Random
  SST::RNG::MersenneRNG random_;
  Statistic<uint64_t>* TotalSentZone;
};

}  // namespace forzasim_macro
}  // namespace SST

#endif  /* end of include guard: ZONE_H_5CE0F319 */
