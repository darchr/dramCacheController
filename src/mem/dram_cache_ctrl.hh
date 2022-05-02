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
    class DCacheRespPort : public ResponsePort
    {

      public:

        DCacheRespPort(const std::string& name, SimpleMemCtrl& _ctrl);

      protected:

        Tick recvAtomic(PacketPtr pkt) override;
        Tick recvAtomicBackdoor(PacketPtr pkt,
                                MemBackdoorPtr &backdoor) override;

        void recvFunctional(PacketPtr pkt) override;

        bool recvTimingReq(PacketPtr) override;

        AddrRangeList getAddrRanges() const override;

      private:

        DCacheCtrl& ctrl;

    };

    class DCacheReqPort : public RequestPort
    {
      public:

        DCacheReqPort(const std::string& name, SimpleMemCtrl& _ctrl);

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
     * Incoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    DCacheRespPort respPort;

    /**
     * Outcoming port, for a multi-ported controller add a crossbar
     * in front of it
     */
    DCacheReqPort reqPort;

    void recvReqRetry();
    bool recvTimingResp(PacketPtr pkt);

    bool recvTimingReq(PacketPtr pkt) override;

  public:

    DCacheCtrl(const DCacheCtrlParams &p);

};

} // namespace memory
} // namespace gem5

#endif //__DCACHE_CTRL_HH__
