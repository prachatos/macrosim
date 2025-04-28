#include "sst/forzasim_macro/Precinct.h"
#include "sst/forzasim_macro/Zone.h"

#include <sst_config.h>

#include <sst/core/interfaces/simpleNetwork.h>


namespace SST {
namespace forzasim_macro {

void Precinct::PrecinctEvent::serialize_order(
    SST::Core::Serialization::serializer& _serializer) {
  SST::Event::serialize_order(_serializer);
  _serializer & payload;
  _serializer & tgt_precinct;
  _serializer & tgt_zone;
  _serializer & tgt_zap;
  _serializer & tgt_pe;
  _serializer & sz;
#ifdef PROC_TIME_TRACK
  _serializer & gen_time;
  _serializer & sent_time;
  _serializer & recv_time;
  _serializer & process_start_time;
#endif
  _serializer & zap_id;
  _serializer & pe_id;
  _serializer & zone_id;
  _serializer & is_mem;
  _serializer & src_pe_q_size;
  _serializer & src_precinct;
  _serializer & src_precinct_q_size;
  _serializer & mem_comp_ns;
};

void Precinct::PrecinctMetaEvent::serialize_order(
    SST::Core::Serialization::serializer& _serializer) {
  SST::Event::serialize_order(_serializer);
  _serializer & src_precinct;
  _serializer & src_precinct_q_size;
}

void Precinct::PrecinctEventList::serialize_order(
    SST::Core::Serialization::serializer& _serializer) {
  SST::Event::serialize_order(_serializer);
  _serializer & event_list;
};

Precinct::Precinct(SST::ComponentId_t _id, SST::Params& _params)
    : SST::Component(_id),
     initialized(false), initBroadcastSent(false),
     numDest(0)
{
  // Configures output.
  int verbosity = _params.find<uint64_t>("verbose", 1);
  output_.init("[@t] forzasim_macro." + getName() + ": ", verbosity, 0,
               SST::Output::STDOUT);

  id = _params.find<uint64_t>("id");
  printf("Precinct id %lu\n", id);
  // Configures the links for all ports.
  num_pes = _params.find<uint64_t>("num_pes");
  num_zaps = _params.find<uint64_t>("num_zaps");
  num_zones = _params.find<uint64_t>("num_zones");
  num_precincts = _params.find<uint64_t>("num_precincts");
  max_aggr_size = _params.find<uint64_t>("max_aggr_size", 0);
  max_aggr_timeout = _params.find<uint64_t>("max_aggr_timeout", 0);
  nw_header_bits = _params.find<uint64_t>("nw_header_bits", 72);
  zop_header_bits = _params.find<uint64_t>("zop_header_bits", 128);
  pe_queue_size = _params.find<uint64_t>("pe_queue_size", 64);
  traversedEdges = _params.find<uint64_t>("traversed_edges", 1);
  nw_file_name = _params.find<std::string>("nw_file_name", "");
  only_max = _params.find<uint64_t>("only_max", 0);
  demo_mode = _params.find<uint64_t>("demo_mode", 0);
  eg_queue_.resize(num_precincts);
  ing_precinct_counters.resize(num_precincts);
  //eg_precinct_counters.initialize(getName(), num_precincts, 0);
  eg_precinct_counters.resize(num_precincts);
  //pe_counters.initialize(getName() + "pe", num_zones * num_zaps * num_pes, 0);
  pe_counters.resize(num_zones * num_zaps * num_pes);
  pc_send_link_ = configureSelfLink("pc_send_port", new SST::Event::Handler<Precinct>(this, &Precinct::handleEgressEvent));
  for(uint64_t i = 0; i < num_zones; ++i)
  {
    std::string port_name = "zone_port_" + std::to_string(i);
    if (!isPortConnected(port_name)) {
      break;
    }
    SST::Link* link = configureLink(port_name, new SST::Event::Handler<Precinct, int>(this, &Precinct::handleZoneEvent, i));
    if (!link) {
      output_.fatal(CALL_INFO, -1, "unable to configure link %" PRIu64 "\n", i);
    }
    links_.push_back(link);

  }
  for(uint64_t i = 0; i < num_precincts; ++i)
  {
    if (i != id) {
      std::string port_name = "precinct_meta_port_" + std::to_string(id) + "_" + std::to_string(i);
      if (!isPortConnected(port_name)) {
        break;
      }
      SST::Link* link = configureLink(port_name, new SST::Event::Handler<Precinct, int>(this, &Precinct::handleMetaEvent, i));
      if (!link) {
        output_.fatal(CALL_INFO, -1, "unable to configure link %" PRIu64 "\n", i);
      }
      meta_links_.push_back(link);
    } else {
      meta_links_.push_back(nullptr);
    }
  }

  total_sends = 0;
  total_sent_pkt = 0;
  num_zones_completed = 0;
  num_precincts_completed = 0;
  if (!demo_mode) {
    tc = registerClock("1KHz", new Clock::Handler<Precinct>(this, &Precinct::drainQueue));
  }
  //AvgAggrSize = registerStatistic<double>("AvgAggrSize");
  TotRecvPrecinct = registerStatistic<uint64_t>("TotRecvPrecinct");
  TotSentPrecinct = registerStatistic<uint64_t>("TotSentPrecinct");
  TotRecvPrecinctInc = registerStatistic<uint64_t>("TotRecvPrecinctInc");


  //TODO: Configure link control to network
  m_linkControl = loadUserSubComponent<SST::Interfaces::SimpleNetwork>( "rtrLink", ComponentInfo::SHARE_PORTS, 1 );
  assert( m_linkControl );
  m_linkControl->setNotifyOnReceive( new SST::Interfaces::SimpleNetwork::Handler<Precinct>(this,&Precinct::handleNetworkEvent) );
  pes_in_precinct = num_pes * num_zaps * num_zones;
  ing_queue_.resize(pes_in_precinct);
  // m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );


  pc_send_link_->setDefaultTimeBase(getTimeConverter("0.1ns"));
  // Sets the time base.
  // registerTimeBase("1ns");
  aggr_timeout = 0;

  // Tells the simulator not to end without us.
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

Precinct::~Precinct() {}
void Precinct::init(unsigned int phase) {
    m_linkControl->init(phase);
    if( m_linkControl->isNetworkInitialized() ){
        if( !initBroadcastSent) {
            initBroadcastSent = true;
            SST::Interfaces::SimpleNetwork::Request * req = new SST::Interfaces::SimpleNetwork::Request();
            req->dest = SST::Interfaces::SimpleNetwork::INIT_BROADCAST_ADDR;
            req->src = m_linkControl->getEndpointID();
            m_linkControl->sendUntimedData(req);
            output_.verbose(CALL_INFO, 1, 0, "sent init message phase %d\n", phase);
        }
    }
    while( SST::Interfaces::SimpleNetwork::Request * req = m_linkControl->recvUntimedData() ) {
      numDest++;
      output_.verbose(CALL_INFO, 1, 0, "received init message phase %d\n", phase);
    }
}
void Precinct::setup() {
  output_.verbose(CALL_INFO, 1, 0, "Setup() ns=%" PRIu64 "\n", getCurrentSimTimeNano());
  m_linkControl->setup();
 //  SST::Interfaces::SimpleNetwork::Request* req = new SST::Interfaces::SimpleNetwork::Request();
 //  if(id == 0)
 //  {
 //      req->dest = 1;
 //  }
 //  else
 //  {
 //        req->dest =0;
 //  }
 // req->src = m_linkControl->getEndpointID();
 // req->vn = 0;
 // req->size_in_bits = 1;
 // PrecinctEvent p('a', 0, 0, 0, 0, 0);
 // req->givePayload(&p);
 // m_linkControl->send(req,req->vn);
}

void Precinct::handleEgressEvent(SST::Event* _event) {
  //printf("Arrived at %lu\n", getCurrentSimTimeNano());
  Precinct::PrecinctEvent* event = dynamic_cast<Precinct::PrecinctEvent*>(_event);
  drainQueuePrecinct((uint64_t)event->tgt_precinct);
}
void Precinct::complete(unsigned int phase) {
    m_linkControl->complete(phase);
}

uint64_t Precinct::get_precinct_pe_id(uint64_t zone_id, uint64_t zap_id, uint64_t pe_id) {
  return pe_id + zap_id * num_pes + zone_id * (num_pes * num_zaps);
}

void Precinct::finish() {
  output_.verbose(CALL_INFO, 1, 0, "Finish()\n");
  m_linkControl->finish();

  Units::registerBaseUnit("edges");
  UnitAlgebra timeTaken = getEndSimTime();
  UnitAlgebra teps = getEndSimTime().invert();
  teps *= traversedEdges;
  teps *= UnitAlgebra("1 edges");

  if(id == 0)
  {
    std::cout << "Edges: " << traversedEdges << " took " << timeTaken << " teps achieved: " << teps << std::endl;
    std::cout << "Edges: " << traversedEdges << " took " << timeTaken.toStringBestSI() << " teps achieved: " << teps.toStringBestSI() << std::endl;
  }
}

void Precinct::handleMetaEvent(SST::Event* _event, int i) {
  Precinct::PrecinctMetaEvent* event = dynamic_cast<Precinct::PrecinctMetaEvent*>(_event);
  if (event) {
    eg_precinct_counters[event->src_precinct].first = std::max(eg_queue_[event->src_precinct].size(), event->src_precinct_q_size);
    eg_precinct_counters[event->src_precinct].second = true;
  }
  delete event;
  return;
}

void Precinct::handleZoneEvent(SST::Event* _event, int _port_num) {
  Precinct::PrecinctEvent* event = dynamic_cast<Precinct::PrecinctEvent*>(_event);
  if (event) {
    output_.verbose(CALL_INFO, 3, 0, "Received event on port %u target %" PRIu64 " ns=%" PRIu64 ".\n",
                    _port_num, (uint64_t)event->tgt_precinct, getCurrentSimTimeNano());
    uint64_t tgt_precinct = (uint64_t)event->tgt_precinct;
    if (event->payload == 4) {
      // Explicit broadcast
      // Has to be serialized at precinct to guarantee in-order delivery between PE src, dest pairs
      for (int i = 0; i < num_precincts; ++i) {
        if (i == id) continue;
        Precinct::PrecinctEvent *pc_event = new Precinct::PrecinctEvent(
          4, 0, 0,
          0, 0,
          1
        );
        //m_linkControl->send(req, req->vn);
        eg_precinct_counters[i].first++;
        eg_precinct_counters[i].second = true;
        pc_event->src_precinct = id;
        eg_queue_[i].push_back(pc_event);
        //printf("Sent at %lu\n", getCurrentSimTimeNano());
        pc_send_link_->send(pc_event);
      }
      for (int zo = 0; zo < num_zones; ++zo) {
        for (int zp = 0; zp < num_zaps; ++zp) {
          for (int p = 0; p < num_pes; ++p) {
            Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
              4, id, zo,
              zp, p,
              1
            );
            pe_event->src_precinct = id;
            pe_event->setSrc(event->zone_id, event->zap_id, event->pe_id);
            //printf("Zone event, src: %lu\n", id);
            eg_precinct_counters[id].first++;
            eg_precinct_counters[id].second = true;
            ing_precinct_counters[id].first++;
            ing_precinct_counters[id].second = true;
            //printf("EG QUEUE %lu, ING QUEUE %lu\n", eg_precinct_counters[id], ing_precinct_counters[id]);

            uint64_t tgt_pe_id = get_precinct_pe_id(zo, zp, p);
            ing_queue_[tgt_pe_id].push_back(pe_event);
            processIngress(tgt_pe_id);
          }
        }
      }
      delete event;
      //primaryComponentOKToEndSim();
      return;
    } else if (event->payload == 5) {

    } else if (event->payload == 1) {
      num_zones_completed++;
      if (num_zones_completed == num_zones) {
        // do something
        printf("Precinct %lu completed\n", id);
        num_precincts_completed++;
        printf("Precint %lu->%lu, found %lu, total %lu\n", id, id, num_precincts_completed, num_precincts);
        primaryComponentOKToEndSim();
        /*for (int i = 0; i < num_precincts; ++i) {
          if (i == id) {
            continue;
          }
          Precinct::PrecinctMetaEvent* pe_event = new Precinct::PrecinctMetaEvent();
          pe_event->src_precinct = id;
          pe_event->src_precinct_q_size = 0;
          pe_event->term_event = true;
          meta_links_[i]->send(pe_event);
        }
        if (num_precincts_completed >= num_precincts) {
          primaryComponentOKToEndSim();
        }*/
      }
      return;
    }
    //pe_counters.write(get_precinct_pe_id(_port_num, event->zap_id, event->pe_id, num_zaps, num_pes), event->src_pe_q_size);
    if (event->src_pe_q_size < INT_MAX) {
      uint64_t tgt_pe_id = get_precinct_pe_id(_port_num, event->zap_id, event->pe_id);
      pe_counters[tgt_pe_id] = event->src_pe_q_size;
      if (pe_counters[tgt_pe_id] <= pe_queue_size) {
        processIngress(tgt_pe_id);
      }
    }
    if (event->payload == 3) {
      // just credit msg
      delete event;
      for (int i = 0; i < num_precincts; ++i) {
        Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
          3, 0, 0,
          0, 0,
          0
        );
        pe_event->src_precinct = i;
        pe_event->src_precinct_q_size = eg_precinct_counters[i].first;
        /*if (i != id) {
          pe_event->src_precinct_q_size = eg_precinct_counters[i];
        } else {
          pe_event->src_precinct_q_size = eg_queue_.size();
        }*/
        links_[_port_num]->send(pe_event);
      }
      return;
    }
    //if (event->src_pe_q_size != 0) printf("non zero at pc\n");
    event->src_precinct = id;
    //event->src_precinct_q_size = ing_precinct_counters[event->tgt_precinct];
    //printf("event->src_precinct_q_size: %lu, src %lu, tgt %lu\n", event->src_precinct_q_size, id, event->tgt_precinct);

    TotRecvPrecinctInc->addData(1);
    if (tgt_precinct != id) {
      output_.verbose(CALL_INFO, 3, 0, "NW route\n");
      eg_precinct_counters[tgt_precinct].first++;
      eg_precinct_counters[tgt_precinct].second = true;
      eg_queue_[tgt_precinct].push_back(event);
      pc_send_link_->send(event);
    } else {
      uint64_t tgt_zone = (uint64_t)event->tgt_zone;
      if (tgt_zone >= links_.size()) {
        output_.fatal(CALL_INFO, -1, "Received bad event target on port %u.\n", _port_num);
      }

      output_.verbose(CALL_INFO, 3, 0, "Self route\n");
      TotSentPrecinct->addData(1);
      TotRecvPrecinct->addData(1);
      event->src_precinct = id;
      eg_precinct_counters[id].first++;
      eg_precinct_counters[id].second = true;
      ing_precinct_counters[id].first++;
      ing_precinct_counters[id].second = true;
      //printf("EG QUEUE %lu, ING QUEUE %lu\n", eg_precinct_counters[id], ing_precinct_counters[id]);

      uint64_t tgt_pe_id = get_precinct_pe_id(event->tgt_zone, event->tgt_zap, event->tgt_pe);
      ing_queue_[tgt_pe_id].push_back(event);
      processIngress(tgt_pe_id);
    }
  } else {
    output_.fatal(CALL_INFO, -1, "Received bad event type on port %u.\n",
                  _port_num);
  }
}

void Precinct::drainQueuePrecinct(uint64_t tgt_precinct) {
  if (eg_queue_[tgt_precinct].size() <= 0) return;
  if (tgt_precinct == id) printf("WTF?\n");

  if (max_aggr_size > 0) {
    uint64_t sent_now = 0;
    while (eg_queue_[tgt_precinct].size() != 0) {
      if (aggr_timeout < max_aggr_timeout && (only_max != 0 && eg_queue_[tgt_precinct].size() < max_aggr_size)) {
        aggr_timeout++;
        return;
      }
      aggr_timeout = 0;
      SST::Interfaces::SimpleNetwork::Request* req = new SST::Interfaces::SimpleNetwork::Request();
      req->dest = tgt_precinct;
      req->src = id;
      req->vn = 0;
      Precinct::PrecinctEventList *pevent_list = new Precinct::PrecinctEventList();
      for (unsigned i=0; (i < max_aggr_size) & (eg_queue_[tgt_precinct].size() != 0); i++) {
        output_.verbose(CALL_INFO, 3, 0, "Tgt precinct %" PRIu64 "\n", tgt_precinct);
        Precinct::PrecinctEvent* event = eg_queue_[tgt_precinct].back();
        if (!event) { break; }
        // TODO: Sometimes this next part fails if PE=Zone=ZAP=1
        req->size_in_bits += zop_header_bits + event->sz * 8;
        pevent_list->event_list.push_back(event);
        eg_queue_[tgt_precinct].pop_back();
        TotSentPrecinct->addData(1);
        sent_now++;
      }
      req->size_in_bits += nw_header_bits;
      req->givePayload(pevent_list);
      //AvgAggrSize->addData(avg_size);
      if (m_linkControl->spaceToSend(0, req->size_in_bits)) {
        m_linkControl->send(req, req->vn);
        total_sends++;
        total_sent_pkt += pevent_list->event_list.size();
        if (nw_file_name.length() > 0) {
          FILE* fp = fopen(nw_file_name.c_str(), "a+");
          fprintf(fp, "%lu,%d\n", getCurrentSimTimeNano(), pevent_list->event_list.size());
          fclose(fp);
        }
        sent_now = 0;
        double avg_size = total_sent_pkt * 1.0 / total_sends;
        //printf("Sent this cycle is %lu, total overall %lu, total send %lu\n", pevent_list->event_list.size(), total_sent_pkt, total_sends);
        //printf("Avg size is %.2f\n", avg_size);
      } else {
        for (auto ev: pevent_list->event_list) {
          eg_queue_[tgt_precinct].push_back(ev);
        }
        return;
      }
      /*
      if (m_linkControl->spaceToSend(0, req->size_in_bits)) {
        m_linkControl->send(req, req->vn);
      } else {
        // drop the request and refill the queue if no space
        for (int i = 0; i < pevent_list->event_list.size(); ++i) {
          eg_queue_[tgt_precinct].push_back(pevent_list->event_list[i]);
        }
        //delete req;
      }
      */
    }
  } else {
    //printf("try to send\n");
    while (eg_queue_[tgt_precinct].size() != 0) {
      //printf("try to send in\n");
      SST::Interfaces::SimpleNetwork::Request* req = new SST::Interfaces::SimpleNetwork::Request();
      output_.verbose(CALL_INFO, 3, 0, "Tgt precinct %" PRIu64 "\n", tgt_precinct);
      Precinct::PrecinctEvent* event = eg_queue_[tgt_precinct].back();
      if (event) {
        //printf("try to send in 2\n");
        req->dest = tgt_precinct;
        req->src = id;
        req->vn = 0;
        // TODO: Sometimes this next part fails if PE=Zone=ZAP=1
        req->size_in_bits = nw_header_bits + zop_header_bits + event->sz * 8;
        if (event->payload == 4) {
          req->size_in_bits = 0;
        }
        if (m_linkControl->spaceToSend(0, req->size_in_bits)) {
          req->givePayload(event);
          m_linkControl->send(req, req->vn);
          TotSentPrecinct->addData(1);
          eg_queue_[tgt_precinct].pop_back();
          total_sent_pkt++;
          total_sends++;
        } else {
          if (event->payload == 4) {
            printf("Send FIN held back\n");
          }
          delete req;
          return;
        }
      }
    }
  }
}

void Precinct::processEgress(Cycle_t cycles) {
  for (uint64_t i = 0; i < num_precincts; ++i) {
    output_.verbose(CALL_INFO, 3, 0, "Calling drain queue at clock with size %" PRIu64 "\n", eg_queue_[i].size());
    drainQueuePrecinct(i);
  }
}

void Precinct::processIngress(uint64_t tgt_pe_id) {
  for (uint64_t i = 0; i < ing_queue_[tgt_pe_id].size(); ++i) {
    Precinct::PrecinctEvent* event = ing_queue_[tgt_pe_id][i];
#ifndef INF_QUEUE_PRECINCT
    uint64_t cur_ctr = pe_counters[tgt_pe_id];
    if (cur_ctr > pe_queue_size) {
      // No more space
      if (event->payload == 1) {
        printf("Send FIN held back at ing\n");
      }
      break;
    }
    //printf("Send FIN for %lu\n", tgt_pe_id);
    if (event->payload != 4) {
      pe_counters[tgt_pe_id]++;
    }
#endif
    if (event->src_precinct == id && eg_precinct_counters[id].first > 0) {
      eg_precinct_counters[id].first--;
      eg_precinct_counters[id].second = true;
    }
    if (ing_precinct_counters[event->src_precinct].first > 0) {
      ing_precinct_counters[event->src_precinct].first--;
      ing_precinct_counters[event->src_precinct].second = true;
    }
    /*if (event->payload == 4) {
      printf("Processing send exit for (%lu, %lu, %lu) to tgt %lu\n", event->zone_id, event->zap_id, event->pe_id, tgt_pe_id);
    }*/
    links_[event->tgt_zone]->send(event);
    ing_queue_[tgt_pe_id][i] = nullptr;  // set to null for deletion, do not delete the event
  }
  // cleanup
  ing_queue_[tgt_pe_id].erase(std::remove_if(begin(ing_queue_[tgt_pe_id]), end(ing_queue_[tgt_pe_id]),
                           [](auto x) { return x == nullptr; }),
                   end(ing_queue_[tgt_pe_id]));
}

void Precinct::processMeta(Cycle_t cycles) {
  if (cycles % 1 == 0) {
    for (uint64_t i = 0; i < num_precincts; ++i) {
      if (i == id) continue;
      if (!ing_precinct_counters[i].second) continue;
      Precinct::PrecinctMetaEvent* pe_event = new Precinct::PrecinctMetaEvent();
      pe_event->src_precinct = id;
      pe_event->src_precinct_q_size = ing_precinct_counters[i].first;
      ing_precinct_counters[i].second = false;
      meta_links_[i]->send(pe_event);
    }
  }
  if (cycles % 1 == 0) {
    for (int i = 0; i < num_precincts; ++i) {
      for (int port=0; port < num_zones; ++port) {
        if (!eg_precinct_counters[i].second) continue;
        Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
          3, 0, 0,
          0, 0,
          0
        );
        pe_event->src_precinct = i;
        pe_event->src_precinct_q_size = eg_precinct_counters[i].first;
        eg_precinct_counters[i].second = false;
        links_[port]->send(pe_event);
      }
    }
  }
}

bool Precinct::drainQueue(Cycle_t cycles){
  //processEgress(cycles);
  //processIngress(cycles);
  processMeta(cycles);
  return false;
}

bool Precinct::handleNetworkEvent(int i) {
  SST::Interfaces::SimpleNetwork::Request* req = m_linkControl->recv(0);
  uint64_t src_precinct = req->src;
  SST::Event* _event = req->takePayload();
  delete req;

  if (max_aggr_size > 0) {
    //printf("aggr\n");
    Precinct::PrecinctEventList* pevent_list = dynamic_cast<Precinct::PrecinctEventList*>(_event);
    if (pevent_list) {
      for (uint64_t i = 0; i < pevent_list->event_list.size(); ++i) {
        Precinct::PrecinctEvent* event = static_cast<Precinct::PrecinctEvent*>(pevent_list->event_list[i]);
        uint64_t tgt_zone = (uint64_t)event->tgt_zone;
        if (tgt_zone >= links_.size()) {
          output_.fatal(CALL_INFO, 1, 0, "Received bad event target %" PRIu64 "\n", tgt_zone);
          return true;
        } else {
          ing_precinct_counters[event->src_precinct].first++;
          ing_precinct_counters[event->src_precinct].second = true;
          TotRecvPrecinct->addData(1);
          uint64_t tgt_pe_id = get_precinct_pe_id(event->tgt_zone, event->tgt_zap, event->tgt_pe);
          ing_queue_[tgt_pe_id].push_back(event);
          processIngress(tgt_pe_id);
        }
      }
      return true;
    }
  }

  Precinct::PrecinctEvent* event = dynamic_cast<Precinct::PrecinctEvent*>(_event);
  if (event) {
    TotRecvPrecinct->addData(1);
    uint64_t tgt_zone = (uint64_t)event->tgt_zone;
    if (event->payload == 1) {
      assert(false);
      //primaryComponentOKToEndSim();
    } else if (event->payload == 4) {
      for (int zo = 0; zo < num_zones; ++zo) {
        for (int zp = 0; zp < num_zaps; ++zp) {
          for (int p = 0; p < num_pes; ++p) {
            Precinct::PrecinctEvent *pe_event = new Precinct::PrecinctEvent(
              event->payload, id, zo,
              zp, p,
              1
            );
            //printf("EG QUEUE %lu, ING QUEUE %lu\n", eg_precinct_counters[id], ing_precinct_counters[id]);
            pe_event->src_precinct = event->src_precinct;

            uint64_t tgt_pe_id = get_precinct_pe_id(zo, zp, p);
            //printf("Pc event for %lu, src: %lu\n", tgt_pe_id, event->src_precinct);
            ing_queue_[tgt_pe_id].push_back(pe_event);
            processIngress(tgt_pe_id);
          }
        }
      }
      delete event;
    } else {
      //eg_precinct_counters.write(event->src_precinct, event->src_precinct_q_size);
      //printf("%d %d\n", event->src_precinct, eg_precinct_counters.size());
      //eg_precinct_counters[event->src_precinct] = event->src_precinct_q_size;
      if (eg_precinct_counters[event->src_precinct].first > 0) {
        //printf("tgt counter > 0\n");
      }
      if (tgt_zone >= links_.size()) {
        output_.verbose(CALL_INFO, 1, 0, "Received bad event target %" PRIu64 "\n", tgt_zone);
      } else {
        //printf("ing_precinct_counters[event->src_precinct] : %lu\n", ing_precinct_counters[event->src_precinct]);
        ing_precinct_counters[event->src_precinct].first++;
        ing_precinct_counters[event->src_precinct].second = true;

        uint64_t tgt_pe_id = get_precinct_pe_id(event->tgt_zone, event->tgt_zap, event->tgt_pe);
        ing_queue_[tgt_pe_id].push_back(event);
        processIngress(tgt_pe_id);
      }
    }
    return true;
  }

  output_.verbose(CALL_INFO, 1, 0, "Received bad event\n");
  return true;
}

}  // namespace forzasim_macro
}  // namespace SST
