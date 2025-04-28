//
// _zap_memctrl_h_
//
//

#ifndef _ZAP_MEMCTRL_H_
#define _ZAP_MEMCTRL_H_

// -- CXX Headers
#include <vector>
#include <algorithm>
#include <map>

// -- SST Headers
#include <sst/core/sst_config.h>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/interfaces/simpleNetwork.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/statapi/stataccumulator.h>
#include <sst/core/subcomponent.h>
#include <sst/core/timeConverter.h>
#include <sst/core/model/element_python.h>

namespace SST::forzasim_macro{

  using namespace SST::Interfaces;

  // target class so that we can keep track of when read and write requests finish
  class ZAPMemTargetSimple {
    public:
      ZAPMemTargetSimple() : done(false) {}
      ZAPMemTargetSimple(std::vector<uint64_t> target) : done(false), target(target) {}
      ~ZAPMemTargetSimple() {}

      bool isDone() { return done; }
      void setDone() { done = true; }

      std::vector<uint64_t> getTarget() { return target; };
      void setTarget(std::vector<uint64_t> t) { target = t; };

      void setIDS(uint64_t zid, uint64_t pid) { zap_id = zid; pe_id = pid; }
      void setSrc(uint64_t s) { src = s; }
      std::pair<uint64_t, uint64_t> getIDS() { return std::make_pair(zap_id, pe_id); }
      uint64_t getSrc() { return src; }

    private:
      bool done;
      std::vector<uint64_t> target;
      uint64_t zap_id;
      uint64_t pe_id;
      uint64_t src;
  };

  // ------------------------------------------------------------
  // ZAPMemOpSimple Class
  // ------------------------------------------------------------
  class ZAPMemOpSimple{
  public:
    enum MemOp{
      ZAP_MemOpREAD       = 0,
      ZAP_MemOpWRITE      = 1,
    };
    ZAPMemOpSimple(MemOp Op, ZAPMemTargetSimple* Target)
      : Op(Op), Addr(0), Size(0), Target(Target)/*, RecvTime(0), ProcStartTime(0)*/ {}

    /// ZAPMemOpSimple: constructor
    ZAPMemOpSimple(MemOp Op, uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target)
      : Op(Op), Addr(Addr), Size(Size), Target(Target)/*, RecvTime(0), ProcStartTime(0)*/ {}

    /// ZAPMemOpSimple: destructor
    ~ZAPMemOpSimple() = default;

    /// ZAPMemOpSimple: retrieve the operation type
    uint64_t getOp() const { return Op; }

    /// ZAPMemOp: retrieve the address
    uint64_t getAddr() const { return Addr; }

    /// ZAPMemOp: retrieve the size
    uint32_t getSize() const { return Size; }

    /// ZAPMemOp: retrieve the buffer
    std::vector<uint64_t> getBuf() const { return Target->getTarget(); }

    /// ZAPMemOp: retrieve the target
    ZAPMemTargetSimple* getTarget() const { return Target; }

    /// ZAPMemOpSimple: set the operation type
    void setOp(MemOp O) { Op = O; }

    /// ZAPMemOpSimple: set the address
    void setAddr(uint64_t A) { Addr = A; }

    /// ZAPMemOpSimple: set the size
    void setSize(uint32_t S) {Size = S; }
    uint64_t CreateTS;
    uint64_t InsertTS;

  private:
    MemOp Op;                       ///< ZAPMemOpSimple: target memory operation
    uint64_t Addr;                  ///< ZAPMemOpSimple: address
    uint32_t Size;                  ///< ZAPMemOpSimple: size of the request
    ZAPMemTargetSimple* Target;           ///< ZAPMemOpSimple: memory target
  };

  // ------------------------------------------------------------
  // ZAPMemCtrlSimple Base Subcomponent Class
  // ------------------------------------------------------------
  class ZAPMemCtrlSimple : public SST::SubComponent {
  public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::forzasim_macro::ZAPMemCtrlSimple)

    SST_ELI_DOCUMENT_PARAMS(
      { "verbose", "Set the verbosity of output for the memory controller", "0" }
    )

    /// ZAPMemCtrlSimple: constructor
    ZAPMemCtrlSimple(ComponentId_t id, const Params& params)
      : SubComponent(id) {
      const int Verbosity = params.find<int>("verbose", 0);
      output.init("ZAPMemCtrlSimple[" + getName() + ":@p:@t]: ",
                  1, 0, SST::Output::STDOUT);
      test = 2;
    }

    /// ZAPMemCtrlSimple: destructor
    virtual ~ZAPMemCtrlSimple() = default;

    /// ZAPMemCtrlSimple: initialization function
    virtual void init(unsigned int phase) = 0;

    /// ZAPMemCtrlSimple: setup function
    virtual void setup() = 0;

    /// ZAPMemCtrlSimple: finish function
    virtual void finish() = 0;

    /// ZAPMemCtrlSimple: send a read request
    virtual bool sendWRITERequest(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target) = 0;

    /// ZAPMemCtrlSimple: send a read request
    virtual bool sendREADRequest(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target) = 0;

    /// ZAPBasicMemCtrlSimple: build a StandardMem request for the current operation
    virtual double calculateLatency(uint64_t cycle) = 0;

    /// ZAPMemCtrlSimple: send a read request that immediately gives completion time
    virtual uint64_t sendREADRequestEventDriven(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target) = 0;
    uint64_t test = 6;

  protected:
    SST::Output output;       ///< ZAPMemCtrlSimple: sst output object

  private:

  };  // class ZAPMemCtrlSimple

  // ------------------------------------------------------------
  // ZAPBasicMemCtrlSimple Inherited Subcomponent Class
  // ------------------------------------------------------------
  class ZAPBasicMemCtrlSimple : public ZAPMemCtrlSimple{
  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
      ZAPBasicMemCtrlSimple,
      "forzasim_macro",
      "ZAPBasicMemCtrlSimple",
      SST_ELI_ELEMENT_VERSION(1, 0, 0),
      "forzasim-macro ZAP basic memory controller",
      SST::forzasim_macro::ZAPMemCtrlSimple
    )

    SST_ELI_DOCUMENT_PARAMS(
      { "verbose",        "Set the verbosity of output for the memory controller",    "0" },
      { "clock",          "Sets the clock frequency of the memory conroller",         "4Ghz" },
      { "num_read",       "Sets the number of outstanding reads",                     "16" },
      { "num_write",      "Sets the number of outstanding write",                     "16" },
      { "ops_per_cycle",  "Sets the number of ops per cycle",                         "32" }
    )


    SST_ELI_DOCUMENT_PORTS()

    SST_ELI_DOCUMENT_STATISTICS(
      {"TotalNumReads",     "Total number of read operations",        "count", 1},
      {"TotalNumWrites",    "Total number of write operations",       "count", 1},
      {"OutstandingReads",  "Number of oustanding write operations",  "count", 1},
      {"OutstandingWrites", "Number of outstanding write operations", "count", 1},
      {"StalledOps", "Number of outstanding write operations", "count", 1},
      {"ProcessTime", "Counts actual execution time", "unitless", 1},
      {"QueueSize", "Queue size", "unitless", 1},
    )

    // Public class members

    /// ZAPBasicMemCtrlSimple: constructor
    ZAPBasicMemCtrlSimple(ComponentId_t id, const Params& params)
    : ZAPMemCtrlSimple(id, params),
    LineSize(0), OutstandingReads(0), OutstandingWrites(0){

      int verbosity = params.find<int>("verbose", 0);
      output.init("ZAPBasicMemCtrlSimple["+getName()+":@p:@t]: ", 1, 0, SST::Output::STDOUT);

      // read all the parameters
      std::string ClockFreq = params.find<std::string>("clock", "4GHz");
      NumRead = params.find<unsigned>("num_read", 0);
      NumWrite = params.find<unsigned>("num_write", 0);
      OpsPerCycle = params.find<unsigned>("ops_per_cycle", 0);
      demo_mode = params.find<unsigned>("demo_mode", 0);
      zeroLoadLatency = params.find<unsigned>("zero_load_latency", 520);

      ProcessTime = registerStatistic<uint64_t>("ProcessTime");
      QueueSize = registerStatistic<uint64_t>("QueueSize");
      StalledOps = registerStatistic<uint64_t>("StalledOps");
      // initialize the memory interface
      phaseStartCycle = 0;
      curPhaseAccesses = 0;
      nsLatency = (uint64_t)(zeroLoadLatency * 0.25);
      maxRequestsPerNS = 4 * NumRead * 1.0 / nsLatency;
      //printf("Max req %.2f\n", maxRequestsPerNS);
      latency = zeroLoadLatency;
      test2 = 1;

      // register the statistics
      TotalNumReads = registerStatistic<uint64_t>("TotalNumReads");
      // TotalWrites = registerStatistic<uint64_t>("TotalWrites");
      OutReads = registerStatistic<uint64_t>("OutstandingReads");
      // OutWrites = registerStatistic<uint64_t>("OutstandingWrites");

      // register the clock

      if (!demo_mode) {
        //registerClock(ClockFreq,
        //              new Clock::Handler<ZAPBasicMemCtrlSimple>(this,
        //                                                &ZAPBasicMemCtrlSimple::clock));
      }

    }

    /// ZAPBasicMemCtrlSimple: destructor
    virtual ~ZAPBasicMemCtrlSimple();

    /// ZAPBasicMemCtrlSimple: initialization function
    virtual void init(unsigned int phase) override;

    /// ZAPBasicMemCtrlSimple: setup function
    virtual void setup() override;

    /// ZAPBasicMemCtrlSimple: finish function
    virtual void finish() override;

    /// ZAPBasicMemCtrlSimple: clock function
    bool clock(Cycle_t cycle);

    /// ZAPMemCtrlSimple: send a read request
    virtual bool sendWRITERequest(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target) override;

    /// ZAPMemCtrlSimple: send a read request
    virtual bool sendREADRequest(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target) override;

    /// ZAPMemCtrlSimple: send a read request that immediately gives completion time
    virtual uint64_t sendREADRequestEventDriven(uint64_t Addr, uint32_t Size, ZAPMemTargetSimple* Target) override;
    virtual double calculateLatency(uint64_t cycle) override;
    uint64_t test2 = 5;

  private:

    // Private methods

    /// ZAPBasicMemCtrlSimple: process the next memory request
    bool processNextRqst( unsigned &t_max_ops, Cycle_t cycle );

    /// ZAPBasicMemCtrlSimple: determine if there available injection slots
    bool isOpenSlots();

    /// ZAPBasicMemCtrlSimple: is the target memory op available to send?
    bool isMemOpAvail(ZAPMemOpSimple *op);

    /// ZAPBasicMemCtrlSimple: build a StandardMem request for the current operation

    // Private data
    unsigned LineSize;                        ///< Cache line size
    unsigned NumRead;                         ///< maximum number of outstanding reads
    unsigned NumWrite;                        ///< maximum number of outstanding writes
    unsigned OpsPerCycle;                     ///< number of ops per cycle to dispatch

    unsigned OutstandingReads;                ///< number of outstanding reads
    unsigned OutstandingWrites;               ///< number of outstanding writes

    uint64_t phaseStartCycle, curPhaseAccesses;
    double maxRequestsPerNS;
    uint64_t demo_mode;

    double latency, zeroLoadLatency;
    double smoothedPhaseAccesses;
    uint64_t nsLatency;

    std::vector<ZAPMemOpSimple *> requests; ///< outstanding StandardMem requests
    std::vector<ZAPMemOpSimple *> rqstQ;                    ///< queued memory requests
    std::vector<Cycle_t> pendingQ;                    ///< queued memory requests
    std::map<StandardMem::Request::id_t, ZAPMemOpSimple *> outstanding;    ///< map of outstanding requests

    // Statistics
    Statistic<uint64_t>* TotalNumReads;          ///< total number of reads
    Statistic<uint64_t>* TotalWrites;         ///< total number of writes
    Statistic<uint64_t>* OutReads;            ///< number of outstanding reads
    Statistic<uint64_t>* OutWrites;           ///< number of outstanding writes
    Statistic<uint64_t>* ProcessTime;
    Statistic<uint64_t>* QueueSize;
    Statistic<uint64_t>* StalledOps;

  }; // class ZAPBasicMemCtrlSimple
} // namespace SST::ZAP

#endif

// EOF
