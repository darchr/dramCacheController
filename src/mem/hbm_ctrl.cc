/*
 * Copyright (c) 2010-2020 ARM Limited
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

#include "mem/hbm_ctrl.hh"

#include "base/trace.hh"
#include "debug/DRAM.hh"
#include "debug/Drain.hh"
#include "debug/MemCtrl.hh"
#include "debug/NVM.hh"
#include "debug/QOS.hh"
#include "mem/mem_interface.hh"
#include "sim/system.hh"

namespace gem5
{

namespace memory
{

HBMCtrl::HBMCtrl(const HBMCtrlParams &p) :
    MemCtrl(p),
    port(name() + ".port", *this),
    retryRdReq2(false), retryWrReq2(false),
    nextReqEventCh1([this]{ processNextReqEventCh1(); }, name()),
    respondEventCh1([this]{ processRespondEventCh1(); }, name()),
    ch1_int(p.mem_2),
    partitionedQ(p.partitioned_q)
{
    DPRINTF(MemCtrl, "Setting up controller\n");

    if (dynamic_cast<DRAMInterface*>(p.mem) != nullptr) {
        ch0_int = dynamic_cast<DRAMInterface*>(p.mem);
    }

    assert(dynamic_cast<DRAMInterface*>(p.mem_2) != nullptr);

    readBufferSize = ch0_int->readBufferSize + ch1_int->readBufferSize;
    writeBufferSize = ch0_int->writeBufferSize + ch1_int->writeBufferSize;

    if (ch0_int) {
        ch0_int->setCtrl(this, commandWindow, 0);
    }
    if (ch1_int) {
        ch1_int->setCtrl(this, commandWindow, 1);
    }
    fatal_if(!ch0_int, "Memory controller must have ch0 interface");
    fatal_if(!ch1_int, "Memory controller must have ch1 interface");
}

void
HBMCtrl::init()
{
    MemCtrl::init();
}


void
HBMCtrl::startup()
{
    MemCtrl::startup();

    isTimingMode = system()->isTimingMode();
    if (isTimingMode) {
        // shift the bus busy time sufficiently far ahead that we never
        // have to worry about negative values when computing the time for
        // the next request, this will add an insignificant bubble at the
        // start of simulation
        // nextBurstAt = curTick() + (dram ? dram->commandOffset() :
        //                                   nvm->commandOffset());
        ch1_int->nextBurstAt = curTick() + ch1_int->commandOffset();
    }
}

Tick
HBMCtrl::recvAtomic(PacketPtr pkt)
{
    DPRINTF(MemCtrl, "HBMCtrl : recvAtomic: %s 0x%x\n",
                     pkt->cmdString(), pkt->getAddr());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    Tick latency = 0;
    // do the actual memory access and turn the packet into a response

    if (ch0_int && ch0_int->getAddrRange().contains(pkt->getAddr())) {
         ch0_int->access(pkt);

         if (pkt->hasData()) {
             // this value is not supposed to be accurate, just enough to
             // keep things going, mimic a closed page
             latency = ch0_int->accessLatency();
         }
    } else if (ch1_int && ch1_int->getAddrRange().contains(pkt->getAddr())) {
         ch1_int->access(pkt);

         if (pkt->hasData()) {
             // this value is not supposed to be accurate, just enough to
             // keep things going, mimic a closed page
             latency = ch1_int->accessLatency();
         }
    }
    else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    return latency;
}

void
HBMCtrl::recvFunctional(PacketPtr pkt)
{
    MemCtrl::recvFunctional(pkt);    
}


Tick
HBMCtrl::recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    Tick latency = recvAtomic(pkt);

    if (ch0_int && ch0_int->getAddrRange().contains(pkt->getAddr())) {
        ch0_int->getBackdoor(backdoor);
    } else if (ch1_int && ch1_int->getAddrRange().contains(pkt->getAddr())) {
        ch1_int->getBackdoor(backdoor);
    }
    else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }
    return latency;
}


bool
HBMCtrl::writeQueueFullCh0(unsigned int neededEntries) const
{
    DPRINTF(MemCtrl,
            "Write queue limit %d, ch0 size %d, entries needed %d\n",
            writeBufferSize, writeQueueSizeCh0, neededEntries);

    auto wrsize_new = (writeQueueSizeCh0 + neededEntries);
    return wrsize_new > (writeBufferSize/2);
}

bool
HBMCtrl::writeQueueFullCh1(unsigned int neededEntries) const
{
    DPRINTF(MemCtrl,
            "Write queue limit %d, ch1 size %d, entries needed %d\n",
            writeBufferSize, writeQueueSizeCh1, neededEntries);

    auto wrsize_new = (writeQueueSizeCh1 + neededEntries);
    return wrsize_new > (writeBufferSize/2);
}

bool
HBMCtrl::readQueueFullCh0(unsigned int neededEntries) const
{
    DPRINTF(MemCtrl,
            "Read queue limit %d, ch0 size %d, entries needed %d\n",
            readBufferSize, readQueueSizeCh0 + respQueue.size(),
            neededEntries);

    auto rdsize_new = readQueueSizeCh0 + respQueue.size() + neededEntries;
    return rdsize_new > (readBufferSize/2);
}

bool
HBMCtrl::readQueueFullCh1(unsigned int neededEntries) const
{
    DPRINTF(MemCtrl,
            "Read queue limit %d, ch1 size %d, entries needed %d\n",
            readBufferSize, readQueueSizeCh1 + respQueue2.size(),
            neededEntries);

    auto rdsize_new = readQueueSizeCh1 + respQueue2.size() + neededEntries;
    return rdsize_new > (readBufferSize/2);
}

bool
HBMCtrl::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world
    DPRINTF(MemCtrl, "recvTimingReq: request %s addr %#x size %d\n",
            pkt->cmdString(), pkt->getAddr(), pkt->getSize());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
                                        "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
                "Should only see read and writes at memory controller\n");

    // Calc avg gap between requests
    if (prevArrival != 0)
    {
        stats.totGap += curTick() - prevArrival;
    }
    prevArrival = curTick();

    // What type of media does this packet access?
    bool is_ch0;
    /*
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        is_dram = true;
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        is_dram = false;
    } else {
        panic("Can't handle address range for packet %s\n",
        pkt->print());
    }*/

    if (bits(pkt->getAddr(), 6) == 0)
    {
        is_ch0 = true;
    }
    else
    {
        is_ch0 = false;
    }

    // AYAZ:  temporarily updating the address of the pkt
    //pkt->setAddr(pkt->getAddr() | 0x40);

    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets
    unsigned size = pkt->getSize();
    uint32_t burst_size = ch0_int->bytesPerBurst();
    unsigned offset = pkt->getAddr() & (burst_size - 1);
    unsigned int pkt_count = divCeil(offset + size, burst_size);

    // run the QoS scheduler and assign a QoS priority value to the packet
    qosSchedule({&readQueue, &writeQueue}, burst_size, pkt);

    // check local buffers and do not accept if full
    if (pkt->isWrite())
    {
        if (is_ch0) {
            if (partitionedQ ? writeQueueFullCh0(pkt_count) :
                                        writeQueueFull(pkt_count))
            {
                DPRINTF(MemCtrl, "Write queue full, not accepting\n");
                // remember that we have to retry this port
                MemCtrl::retryWrReq = true;
                stats.numWrRetry++;
                return false;
            }
            else
            {
                addToWriteQueue(pkt, pkt_count, ch0_int);
                stats.writeReqs++;
                stats.bytesWrittenSys += size;
            }
        } else {
            if (partitionedQ ? writeQueueFullCh1(pkt_count) :
                                        writeQueueFull(pkt_count))
            {
                DPRINTF(MemCtrl, "Write queue full, not accepting\n");
                // remember that we have to retry this port
                retryWrReq2 = true;
                stats.numWrRetry++;
                return false;
            }
            else
            {
                addToWriteQueue(pkt, pkt_count, ch1_int);
                stats.writeReqs++;
                stats.bytesWrittenSys += size;
            }
        }
    }
    else
    {
        assert(pkt->isRead());
        assert(size != 0);

        if (is_ch0) {
            if (partitionedQ ? readQueueFullCh0(pkt_count) :
                                        readQueueFull(pkt_count))
            {
                DPRINTF(MemCtrl, "Read queue full, not accepting\n");
                // remember that we have to retry this port
                retryRdReq2 = true;
                stats.numRdRetry++;
                return false;
            }
            else
            {
                if (!addToReadQueue(pkt, pkt_count, ch0_int)) {

                    if (!nextReqEvent.scheduled()) {
                        DPRINTF(MemCtrl, "Request scheduled immediately\n");
                        schedule(nextReqEvent, curTick());
                    }
                }

                stats.readReqs++;
                stats.bytesReadSys += size;
            }
        } else {
            if (partitionedQ ? readQueueFullCh1(pkt_count) :
                                            readQueueFull(pkt_count))
            {
                DPRINTF(MemCtrl, "Read queue full, not accepting\n");
                // remember that we have to retry this port
                retryRdReq2 = true;
                stats.numRdRetry++;
                return false;
            }
            else
            {
                if (!addToReadQueue(pkt, pkt_count, ch1_int)) {

                    if (!nextReqEventCh1.scheduled()) {
                        DPRINTF(MemCtrl, "Request scheduled immediately\n");
                        schedule(nextReqEventCh1, curTick());
                    }
                }
                stats.readReqs++;
                stats.bytesReadSys += size;
            }
        }

    }

    return true;
}

void
HBMCtrl::processNextReqEventCh1()
{
    MemCtrl::nextReqEventLogic(ch1_int);
    // It is possible that a refresh to another rank kicks things back into
    // action before reaching this point.
    if (!nextReqEventCh1.scheduled())
        schedule(nextReqEventCh1, std::max(ch1_int->nextReqTime, curTick()));

    // If there is space available and we have writes waiting then let
    // them retry. This is done here to ensure that the retry does not
    // cause a nextReqEvent to be scheduled before we do so as part of
    // the next request processing
    if (retryWrReq2 && totalWriteQueueSize < writeBufferSize) {
        retryWrReq2 = false;
        port.sendRetryReq();
    }    
}

void
HBMCtrl::processRespondEventCh1()
{
    MemCtrl::respondEventLogic(ch1_int, respQueue2);
}

void
HBMCtrl::drainResume()
{

    MemCtrl::drainResume();

    if (!isTimingMode && system()->isTimingMode()) {
        // if we switched to timing mode, kick things into action,
        // and behave as if we restored from a checkpoint
        startup();
        ch1_int->startup();
    } else if (isTimingMode && !system()->isTimingMode()) {
        // if we switch from timing mode, stop the refresh events to
        // not cause issues with KVM
        if (ch1_int)
            ch1_int->drainRanks();
    }

    // update the mode
    isTimingMode = system()->isTimingMode();
}


HBMCtrl::MemoryPort::MemoryPort(const std::string& name, HBMCtrl& _ctrl)
    : QueuedResponsePort(name, &_ctrl, queue), queue(_ctrl, *this, true),
      ctrl(_ctrl)
{ }

AddrRangeList
HBMCtrl::MemoryPort::getAddrRanges() const
{
    AddrRangeList ranges;

    if (ctrl.ch0_int) {
        ranges.push_back(ctrl.ch0_int->getAddrRange());
    }
    if (ctrl.ch1_int) {
        ranges.push_back(ctrl.ch1_int->getAddrRange());
    }
    return ranges;
}

void
HBMCtrl::MemoryPort::recvFunctional(PacketPtr pkt)
{
    pkt->pushLabel(ctrl.name());

    if (!queue.trySatisfyFunctional(pkt)) {
        // Default implementation of SimpleTimingPort::recvFunctional()
        // calls recvAtomic() and throws away the latency; we can save a
        // little here by just not calculating the latency.
        ctrl.recvFunctional(pkt);
    }

    pkt->popLabel();
}

Tick
HBMCtrl::MemoryPort::recvAtomic(PacketPtr pkt)
{
    return ctrl.recvAtomic(pkt);
}

Tick
HBMCtrl::MemoryPort::recvAtomicBackdoor(
        PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    return ctrl.recvAtomicBackdoor(pkt, backdoor);
}

bool
HBMCtrl::MemoryPort::recvTimingReq(PacketPtr pkt)
{
    // pass it to the memory controller
    return ctrl.recvTimingReq(pkt);
}

} // namespace memory
} // namespace gem5
