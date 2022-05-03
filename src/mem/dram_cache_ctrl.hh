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
 * MemCtrl declaration
 */

#ifndef __DCACHE_CTRL_HH__
#define __DCACHE_CTRL_HH__

#include "mem/simple_mem_ctrl.hh"
#include "params/DCacheCtrl.hh"

namespace gem5
{

namespace memory
{
class DCacheCtrl : public SimpleMemCtrl
{
  private:

    class DCacheReqPort : public RequestPort
    {
      public:

        DCacheReqPort(const std::string& _name, DCacheCtrl& _ctrl)
            : RequestPort(_name, &_ctrl), ctrl(_ctrl)
        { }

      protected:

        void recvReqRetry()
        { ctrl.recvReqRetry(); }

        bool recvTimingResp(PacketPtr pkt)
        { return ctrl.recvTimingResp(pkt); }

        void recvTimingSnoopReq(PacketPtr pkt)
        { panic("Not supported\n"); }

        void recvFunctionalSnoop(PacketPtr pkt)
        { panic("Not supported\n"); }

        Tick recvAtomicSnoop(PacketPtr pkt)
        { return 0; }

      private:

        DCacheCtrl& ctrl;

    };

    class DCacheRespPort : public ResponsePort
    {
      public:
        DCacheRespPort(const std::string& _name, DCacheCtrl& _ctrl)
            : ResponsePort(_name, &_ctrl), ctrl(_ctrl)
        { }

      protected:

        bool recvTimingReq(PacketPtr pkt) override
        { return ctrl.recvTimingReq(pkt); }

        bool recvTimingSnoopResp(PacketPtr pkt) override
        { return false; }

        void recvRespRetry() override
        { panic("Not supported\n"); }

        AddrRangeList getAddrRanges() const override;

        bool tryTiming(PacketPtr pkt) override
        { return false; }

        void recvFunctional(PacketPtr pkt) override
        { panic("Not supported\n"); }

        Tick recvAtomic(PacketPtr pkt) override
        { panic("Not supported\n"); }

      private:

        DCacheCtrl& ctrl;

    };

    /**
     * Incoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    DCacheRespPort respPort;

    /**
     * Outcoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    DCacheReqPort reqPort;

    /**
     * Remember if we have to retry a request when available.
     * This is a unified retry flag for both reads and writes.
     */
    bool retry;

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

    // new functions for reqPort interactions with backing memory
    void recvReqRetry();
    bool recvTimingResp(PacketPtr pkt);

    struct tagMetaStoreEntry
    {
      // DRAM cache related metadata
      Addr tagDC;
      Addr indexDC;
      // constant to indicate that the cache line is valid
      bool validLine = false;
      // constant to indicate that the cache line is dirty
      bool dirtyLine = false;
      Addr backingMemAddr;
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
      dCacheRead, dCacheWrite,
      backingMemRead, backingMemWrite
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
        bool isHit;
        bool conflict;
        bool issued;

        Addr dirtyLineAddr;
        bool handleDirtyLine;

        // recording the tick when the req transitions into a new stats.
        // The subtract between each two consecutive states entrance ticks,
        // is the number of ticks the req spent in the proceeded state.
        // The subtract between entrance and issuance ticks for each state,
        // is the number of ticks for waiting time in that state.
        Tick dcRdEntered;
        Tick dcRdIssued;
        Tick dcWrEntered;
        Tick dcWrIssued;
        Tick bmRdEntered;
        Tick bmRdIssued;
        Tick bmWrEntered;
        Tick bmWrIssued;

        reqBufferEntry(
          bool _validEntry, Tick _arrivalTick,
          Addr _tagDC, Addr _indexDC,
          PacketPtr _owPkt, MemPacket* _dccPkt,
          reqState _state,
          bool _isHit, bool _conflict, bool _issued,
          Addr _dirtyLineAddr, bool _handleDirtyLine,
          Tick _dcRdEntered, Tick _dcRdIssued,
          Tick _dcWrEntered, Tick _dcWrIssued,
          Tick _bmRdEntered, Tick _bmWrIssued)
        :
        validEntry(_validEntry), arrivalTick(_arrivalTick),
        tagDC(_tagDC), indexDC(_indexDC),
        owPkt( _owPkt), dccPkt(_dccPkt),
        state(_state),
        isHit(_isHit), conflict(_conflict), issued(_issued),
        dirtyLineAddr(_dirtyLineAddr), handleDirtyLine(_handleDirtyLine),
        dcRdEntered(_dcRdEntered), dcRdIssued(_dcRdIssued),
        dcWrEntered(_dcWrEntered), dcWrIssued(_dcWrIssued),
        bmRdEntered(_bmRdEntered), bmWrIssued(_bmWrIssued)
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

    // neede be reimplemented
    bool recvTimingReq(PacketPtr pkt) override;

  public:

    DCacheCtrl(const DCacheCtrlParams &p);

    void init() override;

};

} // namespace memory
} // namespace gem5

#endif //__DCACHE_CTRL_HH__
