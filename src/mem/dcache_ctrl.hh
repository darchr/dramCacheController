
/**
 * @file
 * DcacheCtrl declaration
 */

#ifndef __DCACHE_CTRL_HH__
#define __DCACHE_CTRL_HH__

#include <deque>
#include <queue>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/callback.hh"
#include "base/statistics.hh"
#include "enums/MemSched.hh"
#include "mem/abstract_mem.hh"
#include "mem/qos/mem_ctrl.hh"
#include "mem/qport.hh"
#include "params/DcacheCtrl.hh"
#include "sim/eventq.hh"

class DRAMDCInterface;
class NVMDCInterface;

/**
 * A dram cache controller (dcc) packet stores packets along with
 * the timestamp of when the packet entered the queue, and also
 * the decoded address.
 */

class dccPacket
{
  public:

    /** When did request enter the controller */
    Tick entryTime;

    /** When will request leave the controller */
    Tick readyTime;

    /** This comes from the outside world */
    const PacketPtr pkt;

    /** RequestorID associated with the packet */
    const RequestorID _requestorId;

    const bool read;

    /** Does this packet access DRAM?*/
    const bool dram;

    /** Will be populated by address decoder */
    const uint8_t rank;
    const uint8_t bank;
    const uint32_t row;

    /**
     * Bank id is calculated considering banks in all the ranks
     * eg: 2 ranks each with 8 banks, then bankId = 0 --> rank0, bank0 and
     * bankId = 8 --> rank1, bank0
     */
    const uint16_t bankId;

    /**
     * The starting address of the packet.
     * This address could be unaligned to burst size boundaries. The
     * reason is to keep the address offset so we can accurately check
     * incoming read packets with packets in the write queue.
     */
    Addr addr;

    /**
     * The size of this dram packet in bytes
     * It is always equal or smaller than the burst size
     */
    unsigned int size;

    /**
     * QoS value of the encapsulated packet read at queuing time
     */
    uint8_t _qosValue;

    /**
     * Set the packet QoS value
     * (interface compatibility with Packet)
     */
    inline void qosValue(const uint8_t qv) { _qosValue = qv; }

    /**
     * Get the packet QoS value
     * (interface compatibility with Packet)
     */
    inline uint8_t qosValue() const { return _qosValue; }

    /**
     * Get the packet RequestorID
     * (interface compatibility with Packet)
     */
    inline RequestorID requestorId() const { return _requestorId; }

    /**
     * Get the packet size
     * (interface compatibility with Packet)
     */
    inline unsigned int getSize() const { return size; }

    /**
     * Get the packet address
     * (interface compatibility with Packet)
     */
    inline Addr getAddr() const { return addr; }

    /**
     * Return true if its a read packet
     * (interface compatibility with Packet)
     */
    inline bool isRead() const { return read; }

    /**
     * Return true if its a write packet
     * (interface compatibility with Packet)
     */
    inline bool isWrite() const { return !read; }

    /**
     * Return true if its a DRAM access
     */
    inline bool isDram() const { return dram; }

    dccPacket(PacketPtr _pkt, bool is_read, bool is_dram, uint8_t _rank,
              uint8_t _bank, uint32_t _row, uint16_t bank_id, Addr _addr,
              unsigned int _size)
        : entryTime(curTick()), readyTime(curTick()), pkt(_pkt),
          _requestorId((_pkt!=nullptr)?_pkt->requestorId():-1),
          read(is_read), dram(is_dram), rank(_rank), bank(_bank), row(_row),
          bankId(bank_id), addr(_addr), size(_size),
          _qosValue((_pkt!=nullptr)?_pkt->qosValue():-1)
    { }

};


// The memory packets are store in a multiple dequeue structure,
// based on their QoS priority
typedef std::deque<dccPacket*> dccPacketQueue;

class DcacheCtrl : public QoS::MemCtrl
{
  private:

    // For now, make use of a queued response port to avoid dealing with
    // flow control for the responses being sent back
    class MemoryPort : public QueuedResponsePort
    {

        RespPacketQueue queue;
        DcacheCtrl& ctrl;

      public:

        MemoryPort(const std::string& name, DcacheCtrl& _ctrl);

      protected:

        Tick recvAtomic(PacketPtr pkt) override;
        Tick recvAtomicBackdoor(
                PacketPtr pkt, MemBackdoorPtr &backdoor) override;

        void recvFunctional(PacketPtr pkt) override;

        bool recvTimingReq(PacketPtr) override;

        AddrRangeList getAddrRanges() const override;

    };

    /**
     * Our incoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    MemoryPort port;

    /**
     * Remember if the memory system is in timing mode
     */
    bool isTimingMode;

    /**
     * Remember if we have to retry a request when available.
     */
    bool retry;

    void printORB();
    void printCRB();
    void printAddrInitRead();
    void printAddrDramRespReady();
    void printNvmWritebackQueue();
    Addr returnTagDC(Addr pkt_addr, unsigned size);
    Addr returnIndexDC(Addr pkt_addr, unsigned size);

    /**
     * Bunch of things requires to setup "events" in gem5
     * When event "respondEvent" occurs for example, the method
     * processRespondEvent is called; no parameters are allowed
     * in these methods
     */
    void processNextReqEvent();
    EventFunctionWrapper nextReqEvent;

    void processRespondEvent();
    EventFunctionWrapper respondEvent;

    /**
     * processDramReadEvent() is an event handler which
     * schedules the initial DRAM read accesses for every
     * received packet by the DRAM Cache Controller.
    */
    void processDramReadEvent();
    EventFunctionWrapper dramReadEvent;

    /**
     * processDramWriteEvent() is an event handler which
     * handles DRAM write accesses in the DRAM Cache Controller.
    */
    void processDramWriteEvent();
    EventFunctionWrapper dramWriteEvent;

    /**
     * processRespDramReadEvent() is an event handler which
     * handles the responses of the initial DRAM read accesses
     * for the received packets by the DRAM Cache Controller.
    */
    void processRespDramReadEvent();
    EventFunctionWrapper respDramReadEvent;

    /**
     * processWaitingToIssueNvmReadEvent() is an event handler which
     * handles the satte in which the packets that missed in DRAM cache
     * will wait before being issued, if the NVM read has reached to the
     * maximum number allowed for pending reads.
    */
    void processWaitingToIssueNvmReadEvent();
    EventFunctionWrapper waitingToIssueNvmReadEvent;

    /**
     * processNvmReadEvent() is an event handler which
     * schedules the NVM read accesses in the DRAM Cache Controller.
    */
    void processNvmReadEvent();
    EventFunctionWrapper nvmReadEvent;

    /**
     * processRespNvmReadEvent() is an event handler which
     * handles the responses of the NVM read accesses in
     * the DRAM Cache Controller.
    */
    void processRespNvmReadEvent();
    EventFunctionWrapper respNvmReadEvent;

    /**
     * processNvmWriteEvent() is an event handler which
     * handles NVM write accesses in the DRAM Cache Controller.
    */
    void processNvmWriteEvent();
    EventFunctionWrapper nvmWriteEvent;

    /**
     * Actually do the burst based on media specific access function.
     * Update bus statistics when complete.
     *
     * @param mem_pkt The memory packet created from the outside world pkt
     */
    void doBurstAccess(dccPacket* mem_pkt);

    /**
     * When a packet reaches its "readyTime" in the response Q,
     * use the "access()" method in AbstractMemory to actually
     * create the response packet, and send it back to the outside
     * world requestor.
     *
     * @param pkt The packet from the outside world
     * @param static_latency Static latency to add before sending the packet
     */
    void accessAndRespond(PacketPtr pkt, Tick static_latency, bool in_dram);

    /**
     * Determine if there is a packet that can issue.
     *
     * @param pkt The packet to evaluate
     */
    bool packetReady(dccPacket* pkt);

    /**
     * Calculate burst window aligned tick
     *
     * @param cmd_tick Initial tick of command
     * @return burst window aligned tick
     */
    Tick getBurstWindow(Tick cmd_tick);

    /**
     * Burst-align an address.
     *
     * @param addr The potentially unaligned address
     * @param is_dram Does this packet access DRAM?
     *
     * @return An address aligned to a memory burst
     */
    Addr burstAlign(Addr addr, bool is_dram) const;

    /**
     * To avoid iterating over the outstanding requests buffer
     *  to check for overlapping transactions, maintain a set
     * of burst addresses that are currently queued.
     * Since we merge writes to the same location we never
     * have more than one address to the same burst address.
     */
    std::unordered_set<Addr> isInWriteQueue;

    struct tagMetaStoreEntry {
      // DRAM cache related metadata
      Addr tagDC;
      Addr indexDC;
      // constant to indicate that the cache line is valid
      bool validLine;
      // constant to indicate that the cache line is dirty
      bool dirtyLine;
      Addr nvmAddr;
    };

    /** A storage to keep the tag and metadata for the
     * DRAM Cache entries.
     */
    std::vector<tagMetaStoreEntry> tagMetadataStore;

    /** Different states a packet can transition from one
     * to the other while it's process in the DRAM Cache
     * Controller.
     */
    enum reqState { idle,
                    dramRead, dramWrite,
                    waitingToIssueNvmRead, nvmRead, nvmWrite};

    /**
     * A class for the entries of the
     * outstanding request buffer.
     */
    class reqBufferEntry {
      public:
      bool validEntry = false;
      Tick arrivalTick = MaxTick;

      // DRAM cache related metadata
      Addr tagDC = 0;
      Addr indexDC = 0;

      // pointer to the outside world (ow) packet received from llc
      const PacketPtr owPkt;
      // pointer to the dram cache controller (dcc) packet
      dccPacket* dccPkt;

      reqState state = idle;
      bool isHit = false;
      bool conflict = false;

      reqBufferEntry(
        bool _validEntry, Tick _arrivalTick,
        Addr _tagDC, Addr _indexDC,
        PacketPtr _owPkt, dccPacket* _dccPkt,
        reqState _state, bool _isHit, bool _conflict)
      :
      validEntry(_validEntry), arrivalTick(_arrivalTick),
      tagDC(_tagDC), indexDC(_indexDC),
      owPkt( _owPkt), dccPkt(_dccPkt),
      state(_state), isHit(_isHit), conflict(_conflict)
      { }
    };

    /**
     * This is the outstanding request buffer data
     * structure, the main DS within the DRAM Cache
     * Controller. The key is the address, for each key
     * the map returns a reqBufferEntry which maintains
     * the entire info related to that address while it's
     * been processed in the DRAM Cache controller.
     */
    std::map<Addr,reqBufferEntry*> reqBuffer;


    typedef std::pair<Tick, PacketPtr> confReqBufferPair;
    /**
     * This is the second important data structure
     * within the Dram Cache controller which hold
     * received packets that had conflict with some
     * other address(s) in the DRAM Cache that they
     * are still under process in the controller.
     * Once thoes addresses are finished processing,
     * confReqBufferPair is consulted to see if any
     * packet can be moved into outstanding request
     * buffer and start processing in the DRAM Cache
     * controller.
     */
    std::vector<confReqBufferPair> confReqBuffer;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for dramReadEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    std::deque <Addr> addrInitRead;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for respDramReadEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    std::deque <Addr> addrDramRespReady;

    //priority queue ordered by earliest tick
    typedef std::pair<Tick, Addr> addrNvmReadPair;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for nvmReadEvent handler, we maintain the
     * required addresses in a priority queue.
     */
    std::priority_queue<addrNvmReadPair, std::vector<addrNvmReadPair>,
            std::greater<addrNvmReadPair> > addrNvmRead;

    /**
     * To maintain the packets missed in DRAM cache and
     * now require to read NVM, this queue holds them in order,
     * incase they can't be issued due to reaching to the maximum
     * pending number of reads for NVM.
     */
    std::priority_queue<addrNvmReadPair, std::vector<addrNvmReadPair>,
            std::greater<addrNvmReadPair> > addrWaitingToIssueNvmRead;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for respNvmReadEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    std::deque <Addr> addrNvmRespReady;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for dramWriteEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    std::deque <Addr> addrDramFill;

    /**
     * To avoid iterating over the outstanding requests
     * buffer for nvmWriteEvent handler, we maintain the
     * required addresses in a fifo queue.
     */
    typedef std::pair<Tick, dccPacket*> nvmWritePair;
    std::priority_queue<nvmWritePair, std::vector<nvmWritePair>,
                        std::greater<nvmWritePair> > nvmWritebackQueue;

    void handleRequestorPkt(PacketPtr pkt);
    void checkHitOrMiss(reqBufferEntry* orbEntry);
    bool checkDirty(Addr addr);
    void handleDirtyCacheLine(reqBufferEntry* orbEntry);
    bool checkConflictInDramCache(PacketPtr pkt);
    void checkConflictInCRB(reqBufferEntry* orbEntry);
    bool resumeConflictingReq(reqBufferEntry* orbEntry);

    /**
     * Holds count of commands issued in burst window starting at
     * defined Tick. This is used to ensure that the command bandwidth
     * does not exceed the allowable media constraints.
     */
    std::unordered_multiset<Tick> burstTicks;

    /**
     * Create pointer to interface of the actual dram media when connected
     */
    DRAMDCInterface* const dram;

    /**
     * Create pointer to interface of the actual nvm media when connected
     */

    NVMDCInterface* const nvm;

    /**
     * The following are basic design parameters of the memory
     * controller, and are initialized based on parameter values.
     * The rowsPerBank is determined based on the capacity, number of
     * ranks and banks, the burst size, and the row buffer size.
     */
    uint64_t dramCacheSize;
    uint64_t blockSize;
    const uint32_t orbMaxSize;
    unsigned int orbSize;
    const uint32_t crbMaxSize;
    unsigned int crbSize;

    /**
     * Pipeline latency of the controller frontend. The frontend
     * contribution is added to writes (that complete when they are in
     * the write buffer) and reads that are serviced the write buffer.
     */
    const Tick frontendLatency;

    /**
     * Pipeline latency of the backend and PHY. Along with the
     * frontend contribution, this latency is added to reads serviced
     * by the memory.
     */
    const Tick backendLatency;

    /**
     * Length of a command window, used to check
     * command bandwidth
     */
    const Tick commandWindow;

    /**
     * Till when must we wait before issuing next RD/WR burst?
     */
    Tick nextBurstAt;

    Tick prevArrival;

    /**
     * The soonest you have to start thinking about the next request
     * is the longest access time that can occur before
     * nextBurstAt. Assuming you need to precharge, open a new row,
     * and access, it is tRP + tRCD + tCL.
     */
    Tick nextReqTime;

    struct CtrlStats : public Stats::Group
    {
        CtrlStats(DcacheCtrl &ctrl);

        void regStats() override;

        DcacheCtrl &ctrl;

        // All statistics that the model needs to capture
        Stats::Scalar readReqs;
        Stats::Scalar writeReqs;
        Stats::Scalar readBursts;
        Stats::Scalar writeBursts;
        Stats::Scalar servicedByWrQ;
        Stats::Scalar mergedWrBursts;
        Stats::Scalar neitherReadNorWriteReqs;
        // Average queue lengths
        Stats::Average avgRdQLen;
        Stats::Average avgWrQLen;

        Stats::Scalar numRdRetry;
        Stats::Scalar numWrRetry;
        Stats::Vector readPktSize;
        Stats::Vector writePktSize;
        Stats::Vector rdQLenPdf;
        Stats::Vector wrQLenPdf;
        Stats::Histogram rdPerTurnAround;
        Stats::Histogram wrPerTurnAround;

        Stats::Scalar bytesReadWrQ;
        Stats::Scalar bytesReadSys;
        Stats::Scalar bytesWrittenSys;
        // Average bandwidth
        Stats::Formula avgRdBWSys;
        Stats::Formula avgWrBWSys;

        Stats::Scalar totGap;
        Stats::Formula avgGap;

        // per-requestor bytes read and written to memory
        Stats::Vector requestorReadBytes;
        Stats::Vector requestorWriteBytes;

        // per-requestor bytes read and written to memory rate
        Stats::Formula requestorReadRate;
        Stats::Formula requestorWriteRate;

        // per-requestor read and write serviced memory accesses
        Stats::Vector requestorReadAccesses;
        Stats::Vector requestorWriteAccesses;

        // per-requestor read and write total memory access latency
        Stats::Vector requestorReadTotalLat;
        Stats::Vector requestorWriteTotalLat;

        // per-requestor raed and write average memory access latency
        Stats::Formula requestorReadAvgLat;
        Stats::Formula requestorWriteAvgLat;
    };

    CtrlStats stats;

    /**
     * Upstream caches need this packet until true is returned, so
     * hold it for deletion until a subsequent call
     */
    std::unique_ptr<Packet> pendingDelete;

    /**
     * Remove commands that have already issued from burstTicks
     */
    void pruneBurstTick();

  public:

    DcacheCtrl(const DcacheCtrlParams &p);


    /**
     * Ensure that all interfaced have drained commands
     *
     * @return bool flag, set once drain complete
     */
    bool allIntfDrained() const;

    DrainState drain() override;

    /**
     * Check for command bus contention for single cycle command.
     * If there is contention, shift command to next burst.
     * Check verifies that the commands issued per burst is less
     * than a defined max number, maxCommandsPerWindow.
     * Therefore, contention per cycle is not verified and instead
     * is done based on a burst window.
     *
     * @param cmd_tick Initial tick of command, to be verified
     * @param max_cmds_per_burst Number of commands that can issue
     *                           in a burst window
     * @return tick for command issue without contention
     */
    Tick verifySingleCmd(Tick cmd_tick, Tick max_cmds_per_burst);

    /**
     * Check for command bus contention for multi-cycle (2 currently)
     * command. If there is contention, shift command(s) to next burst.
     * Check verifies that the commands issued per burst is less
     * than a defined max number, maxCommandsPerWindow.
     * Therefore, contention per cycle is not verified and instead
     * is done based on a burst window.
     *
     * @param cmd_tick Initial tick of command, to be verified
     * @param max_multi_cmd_split Maximum delay between commands
     * @param max_cmds_per_burst Number of commands that can issue
     *                           in a burst window
     * @return tick for command issue without contention
     */
    Tick verifyMultiCmd(Tick cmd_tick, Tick max_cmds_per_burst,
                        Tick max_multi_cmd_split = 0);

    /**
     * Is there a respondEvent scheduled?
     *
     * @return true if event is scheduled
     */
    bool respondEventScheduled() const { return respondEvent.scheduled(); }

    /**
     * Is there a read/write burst Event scheduled?
     *
     * @return true if event is scheduled
     */
    bool requestEventScheduled() const { return nextReqEvent.scheduled(); }

    /**
     * restart the controller
     * This can be used by interfaces to restart the
     * scheduler after maintainence commands complete
     *
     * @param Tick to schedule next event
     */
    void restartScheduler(Tick tick) { schedule(nextReqEvent, tick); }

    /**
     * Check the current direction of the memory channel
     *
     * @param next_state Check either the current or next bus state
     * @return True when bus is currently in a read state
     */
    bool inReadBusState(bool next_state) const;

    /**
     * Check the current direction of the memory channel
     *
     * @param next_state Check either the current or next bus state
     * @return True when bus is currently in a write state
     */
    bool inWriteBusState(bool next_state) const;

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    virtual void init() override;
    virtual void startup() override;
    virtual void drainResume() override;

  protected:

    Tick recvAtomic(PacketPtr pkt);
    Tick recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor);
    void recvFunctional(PacketPtr pkt);
    bool recvTimingReq(PacketPtr pkt);

};

#endif //__DCACHE_CTRL_HH__
