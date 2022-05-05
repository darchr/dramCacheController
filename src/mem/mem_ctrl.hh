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

#ifndef __MEM_CTRL_HH__
#define __MEM_CTRL_HH__

#include "mem/simple_mem_ctrl.hh"
#include "params/MemCtrl.hh"

namespace gem5
{

namespace memory
{
class MemCtrl : public SimpleMemCtrl
{
  private:

    NVMInterface* nvm;
    MemPacketQueue::iterator chooseNext(MemPacketQueue& queue,
                      Tick extra_col_delay, MemInterface* mem_int) override;
    MemPacketQueue::iterator nextFRFCFS(MemPacketQueue& queue,
                                              Tick extra_col_delay);
    Tick doBurstAccess(MemPacket* mem_pkt, MemInterface* mem_int) override;
    Tick minReadToWriteDataGap() override;
    Tick minWriteToReadDataGap() override;
    std::vector<MemInterface*> getMemInterface();

    void processRespondEvent() override;
    void processNextReqEvent() override;

  public:

    MemCtrl(const MemCtrlParams &p);

    bool allIntfDrained() const override;
    DrainState drain() override;
    void drainResume() override;

  protected:

    Tick recvAtomic(PacketPtr pkt) override;
    bool recvTimingReq(PacketPtr pkt) override;
    void recvFunctional(PacketPtr pkt) override;

};

} // namespace memory
} // namespace gem5

#endif //__MEM_CTRL_HH__
