#include "sst/forzasim_macro/Precinct.h"
#include "sst/forzasim_macro/Zone.h"
#include "sst/forzasim_macro/ZAP.h"
namespace SST {
namespace forzasim_macro {
//#define EVENT_DRIVEN_MEM
#define IMM_RESPONSE_MEM
//#define ACTUAL_DELAY
Zone::Zone(SST::ComponentId_t _id, SST::Params& _params)
    : SST::Component(_id) {
  uint64_t i = 0;
  // Configures output.
  int verbosity = _params.find<uint64_t>("verbose", 1);
  output_.init("[@t] forzasim_macro." + getName() + ": ", verbosity, 0,
               SST::Output::STDOUT);

  // Configures the links for all ports.
  num_zaps = _params.find<uint64_t>("num_zaps");
  for(i = 0; i < num_zaps; ++i)
  {
    std::string port_name = "zap_port_" + std::to_string(i);
    if (!isPortConnected(port_name)) {
      break;
    }
    SST::Link* link = configureLink(port_name, new SST::Event::Handler<Zone, int>(this, &Zone::handleZapEvent, i));
    if (!link) {
      output_.fatal(CALL_INFO, -1, "unable to configure link %" PRIu64 "\n", i);
    }
    links_.push_back(link);
  }
  //sst_assert(links_.size() > 0, CALL_INFO, -1, "ERROR\n");

  // TODO: Adjust name of port based on scheme chosen
  precinct_link_ = configureLink("precinct_port", new SST::Event::Handler<Zone, int>(this, &Zone::handlePrecinctEvent, i));
  if (!precinct_link_) {
    output_.fatal(CALL_INFO, -1, "Error: failed to initialize the memory subcomponent\n");
  }
  demo_mode = _params.find<uint64_t>("demo_mode", 0);
  if (!demo_mode) {
#ifndef IMM_RESPONSE_MEM
    mem_send_tc = registerClock("4GHz", new Clock::Handler<Zone>(this, &Zone::memSendHandler));
#endif
    Ctrl = loadUserSubComponent<ZAPMemCtrlSimple>("memory");
    if (!Ctrl) {
      output_.fatal(CALL_INFO, -1, "Error: failed to initialize the memory subcomponent\n");
    }
  }
  num_zaps_completed = 0;
  TotalSentZone = registerStatistic<uint64_t>("TotalSentZone");
  // Sets the time base.
  registerTimeBase("1ns");
}

Zone::~Zone() {}

void Zone::setup() {
  output_.verbose(CALL_INFO, 1, 0, "Setup() ns=%" PRIu64 "\n", getCurrentSimTimeNano());
  //TODO: Workout any setup
}

void Zone::finish() {
  output_.verbose(CALL_INFO, 1, 0, "Finish()\n");
}

void Zone::handleZapEvent(SST::Event* _event, int _port_num) {
  Precinct::PrecinctEvent* event = dynamic_cast<Precinct::PrecinctEvent*>(_event);
  if (event) {
    output_.verbose(CALL_INFO, 3, 0, "Received ZAP event on port %u ns=%" PRIu64 ".\n", _port_num, getCurrentSimTimeNano());
     if (event->is_mem) {
      output_.verbose(CALL_INFO, 3, 0, "Received mem op %u ns=%" PRIu64 ".\n",
                      _port_num, getCurrentSimTimeNano());
      ZAPMemTargetSimple *tgt = new ZAPMemTargetSimple();
      tgt->setIDS(event->getTarget().first, event->getTarget().second);
      output_.verbose(CALL_INFO, 3, 0, "src zap_id pe_id %" PRIu64 " %" PRIu64 "\n", event->getTarget().first, event->getTarget().second);
      tgt->setSrc(event->sz);
#ifdef EVENT_DRIVEN_MEM
      tgt->setDone();
      uint64_t time_complete = Ctrl->sendREADRequestEventDriven(1, 1, tgt);
      Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
        2, 0, 0,
        event->getTarget().first, event->getTarget().second,
        tgt->getSrc()
      );
      pe_event->mem_comp_ns = time_complete;
      output_.verbose(CALL_INFO, 3, 0, "Set src to %" PRIu64 "\n", tgt->getSrc());
      pe_event->is_mem = true;
      links_[event->getTarget().first]->send(pe_event);
#else
# ifdef IMM_RESPONSE_MEM
      tgt->setDone();
      Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
        2, 0, 0,
        event->getTarget().first, event->getTarget().second,
        tgt->getSrc()
      );
      output_.verbose(CALL_INFO, 3, 0, "Set src to %" PRIu64 "\n", tgt->getSrc());
      //printf("Sending resp\n");
      double lat = Ctrl->calculateLatency(getCurrentSimTimeNano());
      //printf("Latency calculated: %.2fns\n", lat);
      pe_event->is_mem = true;
      pe_event->mem_comp_ns = lat;
#ifdef ACTUAL_DELAY
      links_[event->getTarget().first]->send(lat, pe_event);
#else
      links_[event->getTarget().first]->send(pe_event);
#endif
# else
      Ctrl->sendREADRequest(1, 1, tgt);
      pending_tgts.push_back(tgt);
# endif
#endif
      delete event;
    } else if (event->payload == 1) {
      num_zaps_completed++;
      if (num_zaps_completed == num_zaps) {
        Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
          1, 0, 0,
          0, 0,
          0
        );
        precinct_link_->send(pe_event);
      }
    } else if (event->payload == 5) {
      // demo mode, respond back
      Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
        5, 0, 0,
        event->getTarget().first, event->getTarget().second,
        event->sz
      );
      links_[event->getTarget().first]->send(pe_event);
      delete event;
    } else {
      if (event->payload == 0) TotalSentZone->addData(1);
      precinct_link_->send(event);
    }
  } else {
    output_.fatal(CALL_INFO, -1, "Received bad event type on port %u.\n", _port_num);
  }
}

void Zone::handlePrecinctEvent(SST::Event* _event, int _port_num) {
    Precinct::PrecinctEvent* event = dynamic_cast<Precinct::PrecinctEvent*>(_event);
    if (event) {
      if (event->payload == 1) {
        // 1 = shutdown, 4 = send termination
        assert(false);
        return;
      } else if (event->payload == 3) {
        // broadcast
        for (int i = 0; i < num_zaps; ++i) {
          Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
            3, 0, 0,
            0, 0,
            0
          );
          pe_event->src_precinct = event->src_precinct;
          pe_event->src_precinct_q_size = event->src_precinct_q_size;
          links_[i]->send(pe_event);
        }
        delete event;
      } else {
        output_.verbose(CALL_INFO, 3, 0, "Received precinct event on port %u ns=%" PRIu64 ".\n", _port_num, getCurrentSimTimeNano());
        uint64_t tgt_zap = (uint64_t)event->tgt_zap;
        if (tgt_zap >= links_.size()) {
          output_.fatal(CALL_INFO, -1, "Received bad event target on port %u.\n", _port_num);
        }
        links_[tgt_zap]->send(event);
      }
    } else {
      output_.fatal(CALL_INFO, -1, "Received bad event type on port %u.\n", _port_num);
    }
}

bool Zone::memSendHandler(Cycle_t cycles) {
  for (unsigned i = 0; i < pending_tgts.size(); ++i) {
    if (pending_tgts[i]->isDone()) {
      output_.verbose(CALL_INFO, 3, 0, "tgt done\n");
      std::pair<uint64_t, uint64_t> ids = pending_tgts[i]->getIDS();

      //std::vector<uint64_t> tgt = pending_tgts[i]->getTarget();
      output_.verbose(CALL_INFO, 3, 0, "tgt zap_id pe_id %" PRIu64 " %" PRIu64 "\n", ids.first, ids.second);
      /*for (unsigned t = 0; t < tgt.size(); ++t) {
        output_.verbose(CALL_INFO, 1, 0, "tgt %u: %" PRIu64 "\n", t, tgt[t]);
      }*/
      Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
        2, 0, 0,
        ids.first, ids.second,
        pending_tgts[i]->getSrc()
      );
      output_.verbose(CALL_INFO, 3, 0, "Set src to %" PRIu64 "\n", pending_tgts[i]->getSrc());
      pe_event->is_mem = true;
      pe_event->mem_comp_ns = 0;
      links_[ids.first]->send(pe_event);
      output_.verbose(CALL_INFO, 3, 0, "tgt zap_id pe_id %" PRIu64 " %" PRIu64 "\n", ids.first, ids.second);
      delete pending_tgts[i];
      pending_tgts.erase(pending_tgts.begin() + i);
    }
  }
  return false;
}

}  // namespace forzasim_macro
}  // namespace SST
