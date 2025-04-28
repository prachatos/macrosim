//
// _zap_memctrl_cc_
//
//

#include "ZAP_memctrl_simple.h"

using namespace SST;
using namespace SST::forzasim_macro;
using namespace SST::Interfaces;

// ---------------------------------------------------------------
// ZAPBasicMemCtrlSimple
// ---------------------------------------------------------------

ZAPBasicMemCtrlSimple::~ZAPBasicMemCtrlSimple(){
}

void ZAPBasicMemCtrlSimple::init(unsigned int phase){
}

void ZAPBasicMemCtrlSimple::setup(){
}

void ZAPBasicMemCtrlSimple::finish(){
}

bool ZAPBasicMemCtrlSimple::sendWRITERequest(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target){
  //double t = calculateLatency(getCurrentSimTimeNano());
  //printf("Lat = %.2f\n", t);
  if( Size == 0 ){
    return true;
  }

  ZAPMemOpSimple *Op = new ZAPMemOpSimple(ZAPMemOpSimple::MemOp::ZAP_MemOpWRITE, Addr, Size, Target);
  rqstQ.push_back(Op);
  return true;
}

bool ZAPBasicMemCtrlSimple::sendREADRequest(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target){
  output.verbose(CALL_INFO, 3, 0, "Got Addr %lu Size %u\n", Addr, Size);
  if( Size == 0 ){
    return true;
  }

  ZAPMemOpSimple *Op = new ZAPMemOpSimple(ZAPMemOpSimple::MemOp::ZAP_MemOpREAD, Addr, Size, Target);
  Op->CreateTS = getCurrentSimTimeNano();
  rqstQ.push_back(Op);
  double old_lat = latency;
  /*calculateLatency(getCurrentSimTimeNano());
  if (old_lat != latency) {
    printf("Latency changed to %.2f at %lu ns\n", latency, getCurrentSimTimeNano());
  }*/
  return true;
}

uint64_t ZAPBasicMemCtrlSimple::sendREADRequestEventDriven(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target){
  ZAPMemOpSimple *Op = new ZAPMemOpSimple(ZAPMemOpSimple::MemOp::ZAP_MemOpREAD, Addr, Size, Target);
  uint64_t cur_ts = getCurrentSimTimeNano();
  Op->CreateTS = cur_ts;
  Op->InsertTS = cur_ts;
  if (pendingQ.size() != 0) {
    Op->InsertTS = pendingQ[0];
  }
  if (Op->InsertTS > 100000000) {
    //printf("Mem req at cycle %lu, insert at %lu, comp at %lu, outstanding req %u\n", cur_ts, Op->InsertTS, Op->InsertTS + nsLatency, OutstandingReads);
  }
  rqstQ.push_back(Op);
  //printf("Mem req at cycle %lu, insert at %lu, comp at %lu, outstanding req %u\n", cur_ts, Op->InsertTS, Op->InsertTS + nsLatency, OutstandingReads);
  // first drain the queue
  for ( unsigned i=0; i < pendingQ.size(); ++i) {
    if (cur_ts >= pendingQ[i]) {
      pendingQ.erase(pendingQ.begin() + i);
      ZAPMemOpSimple *op = requests[i];
      //printf("COMP: Mem req comp at cycle %lu, insert at %lu, outstanding req %u\n", cur_ts, op->InsertTS, OutstandingReads);
      delete op;
      requests.erase(requests.begin() + i);
      TotalNumReads->addData(1);
      OutstandingReads--;
    }
  }
  for( unsigned i=0; i<rqstQ.size(); i++ ) {
    ZAPMemOpSimple *op = rqstQ[i];
    if( cur_ts >= op->InsertTS && isMemOpAvail(op) ){
      // found a candidate operation to process
      OutstandingReads++;
      //printf("PROC: Mem req at cycle %lu, insert at %lu, comp at %lu, outstanding req %u\n", cur_ts, op->InsertTS, op->InsertTS + nsLatency, OutstandingReads);
      pendingQ.push_back(op->InsertTS + nsLatency);
      requests.push_back(op);
      rqstQ.erase(rqstQ.begin()+i);
    } else {
      // anything after is too late
      StalledOps->addData(1);
    }
  }
  ProcessTime->addData(Op->InsertTS + nsLatency - cur_ts);
  return Op->InsertTS + nsLatency - cur_ts;
}

bool ZAPBasicMemCtrlSimple::isOpenSlots(){
  if( (OutstandingReads == NumRead) &&
      (OutstandingWrites == NumWrite) ){
    return false;
  }
  return true;
}

bool ZAPBasicMemCtrlSimple::isMemOpAvail(ZAPMemOpSimple *Op ){
  switch(Op->getOp()){
  case ZAPMemOpSimple::MemOp::ZAP_MemOpREAD:
    if( OutstandingReads < NumRead ){
      return true;
    }
    break;
  case ZAPMemOpSimple::MemOp::ZAP_MemOpWRITE:
    if( OutstandingWrites < NumWrite ){
      return true;
    }
    break;
  default:
    output.fatal(CALL_INFO, -1, "Error : unknown memory operation type\n");
    return false;
    break;
  }
  return false;
}

double ZAPBasicMemCtrlSimple::calculateLatency( uint64_t cycle ) {
  TotalNumReads->addData(1);
  // Based on https://github.com/s5z/zsim/blob/master/src/mem_ctrls.cpp
  uint64_t phaseCycles = cycle - phaseStartCycle;
  output.verbose(CALL_INFO, 3, 0, "Latency %.2f,cycle, phs cycle %lu %lu\n", latency, cycle, phaseStartCycle);
  if (cycle - phaseStartCycle < 1000) {
    curPhaseAccesses++;
    ProcessTime->addData(latency);
    return latency;
  }
  //printf("cur phase access at cycle %lu, %lu. smoothed %.2f\n", cycle, curPhaseAccesses, smoothedPhaseAccesses);
  smoothedPhaseAccesses =  (curPhaseAccesses*0.5) + (smoothedPhaseAccesses*0.5);
  double requestsPerCycle = smoothedPhaseAccesses/((double)phaseCycles);
  double load = std::min(0.99, requestsPerCycle/maxRequestsPerNS);
  double latMultiplier = 1.0 + 0.5*load/(1.0 - load);
  latency = (latMultiplier*nsLatency);
  phaseStartCycle = cycle;
  curPhaseAccesses = 0;
  ProcessTime->addData(latency);
  return latency;
}

bool ZAPBasicMemCtrlSimple::processNextRqst( unsigned &t_max_ops, uint64_t cycle ){
  bool success = false;
  // retrieve the next candidate request
  for( unsigned i=0; i<rqstQ.size(); i++ ){
    ZAPMemOpSimple *op = rqstQ[i];
    /*if (op->ProcStartTime == 0) {
      op->ProcStartTime = cycle;
    }*/
    if( isMemOpAvail(op) ){
      output.verbose(CALL_INFO, 3, 0, "Process Addr %lu\n", op->getAddr());
      // found a candidate operation to process
      t_max_ops++;

      // update latency
      success = true;
      //printf("Latency %.2fns\n", calculateLatency(cycle));
      OutstandingReads++;

      // sent the request, remove it
      if( success ){
        pendingQ.push_back(cycle + round(zeroLoadLatency));
        output.verbose(CALL_INFO, 3, 0, "Will be done in %.2f\n", cycle + round(latency));
        requests.push_back(op);
        op->getTarget()->setDone();
        rqstQ.erase(rqstQ.begin()+i);
      }else{
        // an issue was encountered injecting the request,
        // max out our current outstanding requests
        t_max_ops = OpsPerCycle;
      }
    } else {
      StalledOps->addData(1);
      return false;
    }
  }

  return false;
}

bool ZAPBasicMemCtrlSimple::clock(Cycle_t cycle){
  for ( unsigned i=0; i < pendingQ.size(); ++i) {
    output.verbose(CALL_INFO, 3, 0, "pending val, cycle %lu %lu \n", pendingQ[i], cycle);
    if (cycle >= pendingQ[i]) {
      pendingQ.erase(pendingQ.begin() + i);
      ZAPMemOpSimple *op = requests[i];
      ProcessTime->addData(getCurrentSimTimeNano() - op->CreateTS);
      delete op;
      requests.erase(requests.begin() + i);
      TotalNumReads->addData(1);
      OutstandingReads--;
    }
  }
  if (cycle % 1000 == 0) {
    if (rqstQ.size() > 0)
    QueueSize->addData(rqstQ.size());
  }
  // check to see if there is anything to process
  if( rqstQ.size() == 0 ){
    return false;
  }

  // check to see if we've saturated the number of outstanding requests
  if( !isOpenSlots() ){
    return false;
  }

  // try and initiate an event process this cycle
  bool done = false;
  unsigned t_max_ops = 0;

  while( !done ){
    if( !processNextRqst(t_max_ops, cycle) ){
      done = true;
    }
    if( t_max_ops == OpsPerCycle ){
      done = true;
    }
  }

  // record the outstanding operation statistics
  OutReads->addData(OutstandingReads);
  //OutWrites->addData(OutstandingWrites);

  return false;
}

// EOF
