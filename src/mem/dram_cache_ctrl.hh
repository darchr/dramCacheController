/*
 * Copyright (c) 2012-2020 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2013 Amin Farmahini-Farahani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * DCacheCtrl declaration
 */

#ifndef __DCACHE_CTRL_HH__
#define __DCACHE_CTRL_HH__

#include "mem/simple_mem_ctrl.hh"
#include "params/DCacheCtrl.hh"
#include <queue>

namespace gem5
{

namespace memory
{
class DCacheCtrl : public SimpleMemCtrl
{
  private:

    class RequestPortDCache : public RequestPort
    {
      public:

        RequestPortDCache(const std::string& name, DCacheCtrl& _ctrl)
            : RequestPort(name, &_ctrl), ctrl(_ctrl)
        { }

      protected:

        void recvReqRetry() { ctrl.recvReqRetry(); }

        bool recvTimingResp(PacketPtr pkt)
        { return ctrl.recvTimingResp(pkt); }

        void recvTimingSnoopReq(PacketPtr pkt) { }

        void recvFunctionalSnoop(PacketPtr pkt) { }

        Tick recvAtomicSnoop(PacketPtr pkt) { return 0; }

      private:

        DCacheCtrl& ctrl;

    };

    /**
     * Outcoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    //RequestPortDCache reqPort;

    // A pointer to the far memory interface.
    // Mainly used for accessAndResponse only.
    MemInterface* farMemory;

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
    unsigned farMemWriteQueueMaxSize;

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
      locMemRead, locMemWrite,
      farMemRead, farMemWrite
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

        // pointer to the dram cache controller (dcc) packet
        MemPacket* dccPkt;

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
        Tick locWrEntered;
        Tick locWrIssued;
        Tick farRdEntered;
        Tick farRdIssued;
        Tick farRdRecvd;

        reqBufferEntry(
          bool _validEntry, Tick _arrivalTick,
          Addr _tagDC, Addr _indexDC,
          PacketPtr _owPkt, MemPacket* _dccPkt,
          reqState _state,  bool _issued,
          bool _isHit, bool _conflict,
          Addr _dirtyLineAddr, bool _handleDirtyLine,
          Tick _locRdEntered, Tick _locRdIssued,
          Tick _locWrEntered, Tick _locWrIssued,
          Tick _farRdEntered, Tick _farRdIssued, Tick _farRdRecvd)
        :
        validEntry(_validEntry), arrivalTick(_arrivalTick),
        tagDC(_tagDC), indexDC(_indexDC),
        owPkt( _owPkt), dccPkt(_dccPkt),
        state(_state), issued(_issued),
        isHit(_isHit), conflict(_conflict),
        dirtyLineAddr(_dirtyLineAddr), handleDirtyLine(_handleDirtyLine),
        locRdEntered(_locRdEntered), locRdIssued(_locRdIssued),
        locWrEntered(_locWrEntered), locWrIssued(_locWrIssued),
        farRdEntered(_farRdEntered), farRdIssued(_farRdIssued), farRdRecvd(_farRdRecvd)
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


    typedef std::pair<Tick, PacketPtr> confReqPair;
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
    std::vector<confReqPair> CRB;

    /**
     * This is a unified retry flag for both reads and writes.
     * It helps remember if we have to retry a request when available.
     */
    bool retry;

    // Counters and flags to keep track of read/write switchings
    // stallRds: A flag to stop processing reads and switching to writes
    bool stallRds;
    bool doLocWrite;
    bool doFarWrite;
    float locWrDrainPerc;
    unsigned minLocWrPerSwitch;
    unsigned minFarWrPerSwitch;
    unsigned locWrCounter;
    unsigned farWrCounter;

    /**
     * A queue for evicted dirty lines of DRAM cache,
     * to be written back to the backing memory.
     * These packets are not maintained in the ORB.
     */
    std::vector<MemPacketQueue> pktFarMemWrite;

    // Maintenance Queues
    std::vector<MemPacketQueue> pktLocMemRead;
    std::vector<MemPacketQueue> pktLocMemWrite;
    std::vector<MemPacketQueue> pktFarMemRead;

    std::deque <Addr> addrLocRdRespReady;
    std::deque <Addr> addrFarRdRespReady;

    // Maintenance variables
    unsigned maxConf, maxLocRdEvQ, maxLocRdRespEvQ,
    maxLocWrEvQ, maxFarRdEvQ, maxFarRdRespEvQ, maxFarWrEvQ;

    // needs be reimplemented
    bool recvTimingReq(PacketPtr pkt) override;

    // new functions for reqPort interactions with backing memory
    void recvReqRetry();
    bool recvTimingResp(PacketPtr pkt);

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

    void processOverallWriteEvent();
    EventFunctionWrapper overallWriteEvent;

    // management functions
    void handleRequestorPkt(PacketPtr pkt);
    void checkHitOrMiss(reqBufferEntry* orbEntry);
    bool checkDirty(Addr addr);
    void handleDirtyCacheLine(reqBufferEntry* orbEntry);
    bool checkConflictInDramCache(PacketPtr pkt);
    void checkConflictInCRB(reqBufferEntry* orbEntry);
    bool resumeConflictingReq(reqBufferEntry* orbEntry);
    void logStatsDcache(reqBufferEntry* orbEntry);

    Addr returnIndexDC(Addr pkt_addr, unsigned size);
    Addr returnTagDC(Addr pkt_addr, unsigned size);


  public:

    DCacheCtrl(const DCacheCtrlParams &p);

    void init() override;

    // TODO: write events
    bool requestEventScheduled() const override { return locMemReadEvent.scheduled(); }
    bool respondEventScheduled() const override { return locMemReadRespEvent.scheduled(); }
    void dummy() override { std::cout << "DC\n"; }

};

} // namespace memory
} // namespace gem5

#endif //__DCACHE_CTRL_HH__
