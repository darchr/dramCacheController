/**
 * @file
 * DCacheCtrl declaration
 */

#ifndef __POLICY_MANAGER_HH__
#define __POLICY_MANAGER_HH__

#include <queue>
#include <deque>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/callback.hh"
#include "base/statistics.hh"
#include "enums/MemSched.hh"
#include "mem/qport.hh"
#include "sim/eventq.hh"

namespace gem5
{

namespace memory
{
class PolicyManager
{
  private:

    class RespPortPolManager : public QueuedResponsePort
    {
      public:

        RespPortPolManager(const std::string& name, PolicyManager& _polMan)
            : QueuedResponsePort(name, &_polMan),
              queue(_ctrl, *this, true),
              polMan(_polMan)
        { }

      protected:

        bool recvTimingReq(PacketPtr) override { return polMan.recvTimingReq(pkt); }

        AddrRangeList getAddrRanges() override { return polMan.getAddrRanges(); }

      private:

        PolicyManager& polMan;
        RespPacketQueue queue;

    };
    
    class ReqPortPolManager : public RequestPort
    {
      public:

        ReqPortPolManager(const std::string& name, PolicyManager& _polMan)
            : RequestPort(name, &_polMan), polMan(_polMan)
        { }

      protected:

        void recvReqRetry()
        { if (this.name == "locMemPort") { polMan.locMemRecvReqRetry(); }
          if (this.name == "farMemPort") { polMan.farMemRecvReqRetry(); }
        }

        bool recvTimingResp(PacketPtr pkt)
        { if (this.name == "locMemPort") { return polMan.locMemRecvTimingResp(pkt); }
          if (this.name == "farMemPort") { return polMan.farMemRecvTimingResp(pkt); }
        }

      private:

        PolicyManager& polMan;

    };

    RespPortPolManager respPort;
    ReqPortPolManager locMemPort;
    ReqPortPolManager farMemPort;

    MemCtrl& locMemCtrl;
    MemCtrl& farMemCtrl;

    /**
     * The following are basic design parameters of the unified
     * DRAM cache controller, and are initialized based on parameter values.
     * The rowsPerBank is determined based on the capacity, number of
     * ranks and banks, the burst size, and the row buffer size.
     */

    unsigned long long dramCacheSize;
    unsigned blockSize;
    unsigned addrSize;
    unsigned orbMaxSize;
    unsigned orbSize;
    unsigned crbMaxSize;
    unsigned crbSize;
    bool alwaysHit;
    bool alwaysDirty;

    Tick prevArrival;

    std::unordered_set<Addr> isInWriteQueue;

    struct tagMetaStoreEntry
    {
      // DRAM cache related metadata
      Addr tagDC;
      Addr indexDC;
      // constant to indicate that the cache line is valid
      bool validLine = false;
      // constant to indicate that the cache line is dirty
      bool dirtyLine = false;
      Addr farMemAddr;
    };

    /** A storage to keep the tag and metadata for the
     * DRAM Cache entries.
     */
    std::vector<tagMetaStoreEntry> tagMetadataStore;

    /** Different states a packet can transition from one
     * to the other while it's process in the DRAM Cache
     * Controller.
     */
    enum reqState
    {
      locMemRead, waitingLocMemReadResp,
      locMemWrite,
      farMemRead, waitingFarMemReadResp
      farMemWrite
    };

    enum Policy
    {
      MainMemory,
      CascadeLake, CascadeLakeNoPartWrs, CascadeLakeHBM2,
      AlloyCache, AlloyCacheHBM2
    };

    /**
     * A class for the entries of the
     * outstanding request buffer (ORB).
     */
    class reqBufferEntry
    {
      public:

        bool validEntry;
        Tick arrivalTick;

        // DRAM cache related metadata
        Addr tagDC;
        Addr indexDC;

        // pointer to the outside world (ow) packet received from llc
        const PacketPtr owPkt;

        Policy pol;
        reqState state;
        
        bool issued;
        bool isHit;
        bool conflict;

        Addr dirtyLineAddr;
        bool handleDirtyLine;

        // recording the tick when the req transitions into a new stats.
        // The subtract between each two consecutive states entrance ticks,
        // is the number of ticks the req spent in the proceeded state.
        // The subtract between entrance and issuance ticks for each state,
        // is the number of ticks for waiting time in that state.
        Tick locRdEntered;
        Tick locRdIssued;
        Tick locRdExit;
        Tick locWrEntered;
        Tick locWrExit;
        Tick farRdEntered;
        Tick farRdIssued;
        Tick farRdRecvd;
        Tick farRdExit;

        reqBufferEntry(
          bool _validEntry, Tick _arrivalTick,
          Addr _tagDC, Addr _indexDC,
          PacketPtr _owPkt
          Policy _pol, reqState _state,  
          bool _issued, bool _isHit, bool _conflict,
          Addr _dirtyLineAddr, bool _handleDirtyLine,
          Tick _locRdEntered, Tick _locRdIssued, Tick _locRdExit,
          Tick _locWrEntered, Tick _locWrExit,
          Tick _farRdEntered, Tick _farRdIssued, Tick _farRdRecvd, Tick _farRdExit)
        :
        validEntry(_validEntry), arrivalTick(_arrivalTick),
        tagDC(_tagDC), indexDC(_indexDC),
        owPkt( _owPkt), dccPkt(_dccPkt),
        pol(_pol), state(_state),
        issued(_issued), isHit(_isHit), conflict(_conflict),
        dirtyLineAddr(_dirtyLineAddr), handleDirtyLine(_handleDirtyLine),
        locRdEntered(_locRdEntered), locRdIssued(_locRdIssued), locRdExit(_locRdExit),
        locWrEntered(_locWrEntered), locWrExit(_locWrExit),
        farRdEntered(_farRdEntered), farRdIssued(_farRdIssued), farRdRecvd(_farRdRecvd), farRdExit(_farRdExit)
        { }
    };

    /**
     * This is the outstanding request buffer (ORB) data
     * structure, the main DS within the DRAM Cache
     * Controller. The key is the address, for each key
     * the map returns a reqBufferEntry which maintains
     * the entire info related to that address while it's
     * been processed in the DRAM Cache controller.
     */
    std::map<Addr,reqBufferEntry*> ORB;

    typedef std::pair<Tick, PacketPtr> timeReqPair;
    /**
     * This is the second important data structure
     * within the DRAM cache controller which holds
     * received packets that had conflict with some
     * other address(s) in the DRAM Cache that they
     * are still under process in the controller.
     * Once thoes addresses are finished processing,
     * Conflicting Requets Buffre (CRB) is consulted
     * to see if any packet can be moved into the
     * outstanding request buffer and start being
     * processed in the DRAM cache controller.
     */
    std::vector<timeReqPair> CRB;

    /**
     * This is a unified retry flag for both reads and writes.
     * It helps remember if we have to retry a request when available.
     */
    bool retryLLC;
    bool retryLocMem;
    bool retryFarMem;

    // Counters and flags to keep track of read/write switchings
    // stallRds: A flag to stop processing reads and switching to writes
    bool stallRds;
    bool sendFarRdReq;
    bool waitingForRetryReqPort;
    bool rescheduleLocRead;
    bool rescheduleLocWrite;
    float locWrDrainPerc;
    unsigned writeHighThreshold;
    unsigned minLocWrPerSwitch;
    unsigned minFarWrPerSwitch;
    unsigned locWrCounter;
    unsigned farWrCounter;

    /**
     * A queue for evicted dirty lines of DRAM cache,
     * to be written back to the backing memory.
     * These packets are not maintained in the ORB.
     */
    std::deque <timeReqPair> pktFarMemWrite; 

    // Maintenance Queues
    std::vector <PacketPtr> pktLocMemRead;
    std::vector <PacketPtr> pktLocMemWrite;
    std::deque <PacketPtr> pktFarMemRead;
    std::deque <PacketPtr> pktFarMemReadResp;

    std::deque <Addr> addrLocRdRespReady;
    //std::deque <Addr> addrFarRdRespReady;

    // Maintenance variables
    unsigned maxConf, maxLocRdEvQ, maxLocRdRespEvQ,
    maxLocWrEvQ, maxFarRdEvQ, maxFarRdRespEvQ, maxFarWrEvQ;

    // needs be reimplemented
    bool recvTimingReq(PacketPtr pkt) override;

    void accessAndRespond(PacketPtr pkt, Tick static_latency,
                                                MemInterface* mem_intr) override;

    // events
    void processLocMemReadEvent();
    EventFunctionWrapper locMemReadEvent;

    void processLocMemReadRespEvent();
    EventFunctionWrapper locMemReadRespEvent;

    void processLocMemWriteEvent();
    EventFunctionWrapper locMemWriteEvent;

    void processFarMemReadEvent();
    EventFunctionWrapper farMemReadEvent;

    void processFarMemReadRespEvent();
    EventFunctionWrapper farMemReadRespEvent;

    void processFarMemWriteEvent();
    EventFunctionWrapper farMemWriteEvent;

    // management functions
    void setNextState(reqBufferEntry* orbEntry);
    bool handleTagRead(PacketPtr pkt);
    void handleNextState(reqBufferEntry* orbEntry);
    void sendRespondToRequestor(PacketPtr pkt);
    void printQSizes();
    void handleRequestorPkt(PacketPtr pkt);
    void checkHitOrMiss(reqBufferEntry* orbEntry);
    bool checkDirty(Addr addr);
    void handleDirtyCacheLine(reqBufferEntry* orbEntry);
    bool checkConflictInDramCache(PacketPtr pkt);
    void checkConflictInCRB(reqBufferEntry* orbEntry);
    bool resumeConflictingReq(reqBufferEntry* orbEntry);
    void logStatsDcache(reqBufferEntry* orbEntry);
    //reqBufferEntry* makeOrbEntry(reqBufferEntry* orbEntry, PacketPtr copyOwPkt);
    PacketPtr getPacket(Addr addr, unsigned size, const MemCmd& cmd, Request::FlagsType flags = 0);
    void dirtAdrGen();

    unsigned countLocRdInORB();
    unsigned countFarRdInORB();
    unsigned countLocWrInORB();
    unsigned countFarWr();

    Addr returnIndexDC(Addr pkt_addr, unsigned size);
    Addr returnTagDC(Addr pkt_addr, unsigned size);

    // port management
    void locMemRecvReqRetry();
    void farMemRecvReqRetry();

    void locMemRetryReq();
    void farMemRetryReq();

    bool locMemRecvTimingResp(PacketPtr pkt);
    bool farMemRecvTimingResp(PacketPtr pkt);
    struct PolicyManagerStats : public statistics::Group
    {
      PolicyManagerStats(PolicyManager &polMan);

      void regStats() override;

      PolicyManager &polMan;

        // All statistics that the model needs to capture
        statistics::Scalar readReqs;
        statistics::Scalar writeReqs;
        statistics::Scalar readBursts;
        statistics::Scalar writeBursts;
        statistics::Scalar servicedByWrQ;
        statistics::Scalar mergedWrBursts;
        statistics::Scalar neitherReadNorWriteReqs;
        // Average queue lengths
        statistics::Average avgRdQLen;
        statistics::Average avgWrQLen;

        statistics::Scalar numRdRetry;
        statistics::Scalar numWrRetry;
        statistics::Vector readPktSize;
        statistics::Vector writePktSize;
        statistics::Vector rdQLenPdf;
        statistics::Vector wrQLenPdf;
        statistics::Histogram rdPerTurnAround;
        statistics::Histogram wrPerTurnAround;

        statistics::Scalar bytesReadWrQ;
        statistics::Scalar bytesReadSys;
        statistics::Scalar bytesWrittenSys;
        // Average bandwidth
        statistics::Formula avgRdBWSys;
        statistics::Formula avgWrBWSys;

        statistics::Scalar totGap;
        statistics::Formula avgGap;

        // per-requestor bytes read and written to memory
        statistics::Vector requestorReadBytes;
        statistics::Vector requestorWriteBytes;

        // per-requestor bytes read and written to memory rate
        statistics::Formula requestorReadRate;
        statistics::Formula requestorWriteRate;

        // per-requestor read and write serviced memory accesses
        statistics::Vector requestorReadAccesses;
        statistics::Vector requestorWriteAccesses;

        // per-requestor read and write total memory access latency
        statistics::Vector requestorReadTotalLat;
        statistics::Vector requestorWriteTotalLat;

        // per-requestor raed and write average memory access latency
        statistics::Formula requestorReadAvgLat;
        statistics::Formula requestorWriteAvgLat;

      statistics::Average avgORBLen;
      statistics::Average avgLocRdQLenStrt;
      statistics::Average avgLocWrQLenStrt;
      statistics::Average avgFarRdQLenStrt;
      statistics::Average avgFarWrQLenStrt;

      statistics::Average avgLocRdQLenEnq;
      statistics::Average avgLocWrQLenEnq;
      statistics::Average avgFarRdQLenEnq;
      statistics::Average avgFarWrQLenEnq;

      
      Stats::Scalar numWrBacks;
      Stats::Scalar totNumConf;
      Stats::Scalar totNumORBFull;
      Stats::Scalar totNumConfBufFull;

      Stats::Scalar maxNumConf;
      Stats::Scalar maxLocRdEvQ;
      Stats::Scalar maxLocRdRespEvQ;
      Stats::Scalar maxLocWrEvQ;
      Stats::Scalar maxFarRdEvQ;
      Stats::Scalar maxFarRdRespEvQ;
      Stats::Scalar maxFarWrEvQ;

      Stats::Scalar rdToWrTurnAround;
      Stats::Scalar wrToRdTurnAround;

      Stats::Scalar sentRdPort;
      Stats::Scalar failedRdPort;
      Stats::Scalar recvdRdPort;
      Stats::Scalar sentWrPort;
      Stats::Scalar failedWrPort;

      Stats::Scalar totPktsServiceTime;
      Stats::Scalar totPktsORBTime;
      Stats::Scalar totTimeFarRdtoSend;
      Stats::Scalar totTimeFarRdtoRecv;
      Stats::Scalar totTimeFarWrtoSend;
      Stats::Scalar totTimeInLocRead;
      Stats::Scalar totTimeInLocWrite;
      Stats::Scalar totTimeInFarRead;
      Stats::Scalar QTLocRd;
      Stats::Scalar QTLocWr;
    };

    PolicyManagerStats polManStats;

  public:

    PolicyManager(const PolicyManagerParams &p);

    void init() override;

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    // bool requestEventScheduled(uint8_t pseudo_channel = 0) const override;
    // void restartScheduler(Tick tick,  uint8_t pseudo_channel = 0) override;
    // bool respondEventScheduled() const override { return locMemReadRespEvent.scheduled(); }

};

} // namespace memory
} // namespace gem5

#endif //__POLICY_MANAGER_HH__
