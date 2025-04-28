#ifndef ZAP_H_2BD0D0B8
#define ZAP_H_2BD0D0B8

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>
#include <sst/core/eli/elementinfo.h>
#include <sst/core/rng/mersenne.h>

#include <cstdint>


namespace SST {
namespace forzasim_macro {

class ZAP : public SST::Component {
 public:
  ZAP(SST::ComponentId_t _id, SST::Params& _params);
  ~ZAP() override;

  void setup() override;
  void finish() override;

  SST_ELI_REGISTER_COMPONENT(
      ZAP,
      "forzasim_macro",
      "ZAP",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "forzasim_macro ZAP",
      COMPONENT_CATEGORY_UNCATEGORIZED)

  SST_ELI_DOCUMENT_PORTS(
    {"pe_port_%(portnum)d", "Links to PE", {"forzasim_macro.PE"}},
    {"zone_port", "Link to zone.", {"forzasim_macro.Zone"}}
  )

  SST_ELI_DOCUMENT_PARAMS(
    {"num_zaps", "Number of zaps", "1"}
  )

  SST_ELI_DOCUMENT_STATISTICS(
  )


 private:
  ZAP(const ZAP&) = delete;
  void operator=(const ZAP&) = delete;

  virtual void handleZoneEvent(SST::Event* _event, int _port_num);
  virtual void handlePEEvent(SST::Event* _event, int _port_num);

  // Output
  SST::Output output_;


  uint64_t num_pes;

  // Links
  std::vector<SST::Link*> links_;
  SST::Link* zone_link_;

  // Random
  SST::RNG::MersenneRNG random_;
};

}  // namespace forzasim_macro
}  // namespace SST

#endif  // ZAP_H_2BD0D0B8

