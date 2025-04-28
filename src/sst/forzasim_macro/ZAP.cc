#include "sst/forzasim_macro/ZAP.h"
#include "sst/forzasim_macro/Zone.h"
#include "sst/forzasim_macro/PE.h"
#include "sst/forzasim_macro/Precinct.h"
namespace SST {
namespace forzasim_macro {
ZAP::ZAP(SST::ComponentId_t _id, SST::Params& _params)
    : SST::Component(_id) {
  uint64_t i = 0;
  // Configures output.
  output_.init("[@t] forzasim_macro." + getName() + ": ", 1, 0,
               SST::Output::STDOUT);

  num_pes = _params.find<uint64_t>("num_pes");
  for(i = 0; i < num_pes; ++i)
  {
    std::string port_name = "pe_port_" + std::to_string(i);
    if (!isPortConnected(port_name)) {
      break;
    }
    SST::Link* link = configureLink(port_name, new SST::Event::Handler<ZAP, int>(this, &ZAP::handlePEEvent, i));
    if (!link) {
      output_.fatal(CALL_INFO, -1, "unable to configure link %" PRIu64 "\n", i);
    }
    links_.push_back(link);
  }

  // TODO: Adjust name of port based on scheme chosen
  zone_link_ = configureLink("zone_port", new SST::Event::Handler<ZAP, int>(this, &ZAP::handleZoneEvent, i));

  // Sets the time base.
  registerTimeBase("1ns");
}

ZAP::~ZAP() {}

void ZAP::setup() {
  output_.verbose(CALL_INFO, 1, 0, "Setup() ns=%" PRIu64 "\n",
                  getCurrentSimTimeNano());
}

void ZAP::finish() {
  output_.verbose(CALL_INFO, 1, 0, "Finish()\n");
}


void ZAP::handlePEEvent(SST::Event* _event, int _port_num) {
  Precinct::PrecinctEvent* event = dynamic_cast<Precinct::PrecinctEvent*>(_event);
  if (event) {
    output_.verbose(CALL_INFO, 3, 0, "Received PE event on port %u ns=%" PRIu64 ".\n",
                    _port_num, getCurrentSimTimeNano());
    if (event->is_mem) {
      output_.verbose(CALL_INFO, 3, 0, "Received mem op %u ns=%" PRIu64 ".\n",
                      _port_num, getCurrentSimTimeNano());
    }
    output_.verbose(CALL_INFO, 3, 0, "event is_mem  %d %" PRIu64 "  %u %" PRIu64 "\n",
                    _port_num, getCurrentSimTimeNano(), event->is_mem, event->tgt_pe);
    zone_link_->send(event);
  } else {
    output_.fatal(CALL_INFO, -1, "Received bad event type on port %u.\n",
                  _port_num);
  }
}

void ZAP::handleZoneEvent(SST::Event* _event, int _port_num) {
  Precinct::PrecinctEvent* event = dynamic_cast<Precinct::PrecinctEvent*>(_event);
  if (event) {
    if (event->payload == 1) {
      //delete event;
      for (int i = 0; i < num_pes; ++i) {
          output_.verbose(CALL_INFO, 3, 0, "Payload tgt: (%" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 "), size: %" PRIu64 ", payload %d\n", event->tgt_precinct,
                    event->tgt_zone, event->tgt_zap, event->tgt_pe, event->sz, event->payload);
        Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
          1, 0, 0,
          0, 0,
          0
        );
        links_[i]->send(pe_event);
      }
      return;
    }
    output_.verbose(CALL_INFO, 3, 0, "Received zone event on port %u ns=%" PRIu64 ".\n",
                    _port_num, getCurrentSimTimeNano());
    uint64_t tgt_pe = (uint64_t)event->tgt_pe;
    if (tgt_pe >= links_.size()) {
      output_.fatal(CALL_INFO, -1, "Received bad event target on port %u.\n", _port_num);
    }
    links_[tgt_pe]->send(event);
  } else {
    output_.fatal(CALL_INFO, -1, "Received bad event type on port %u.\n",
                  _port_num);
  }
}

}  // namespace forzasim_macro
}  // namespace SST
