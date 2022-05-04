/*
 * Copyright (c) 2022 The Regents of the University of California
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
    retryRdReqCh1(false), retryWrReqCh1(false),
    nextReqEventCh1([this]{ processNextReqEventCh1(); }, name()),
    respondEventCh1([this]{ processRespondEventCh1(); }, name()),
    ch1_int(p.mem_2),
    partitionedQ(p.partitioned_q)
{
    DPRINTF(MemCtrl, "Setting up HBM controller\n");

    ch0_int = dynamic_cast<DRAMInterface*>(mem);

    assert(dynamic_cast<DRAMInterface*>(p.mem_2) != nullptr);

    readBufferSize = ch0_int->readBufferSize + ch1_int->readBufferSize;
    writeBufferSize = ch0_int->writeBufferSize + ch1_int->writeBufferSize;

    fatal_if(!ch0_int, "Memory controller must have ch0 interface");
    fatal_if(!ch1_int, "Memory controller must have ch1 interface");

    if (ch0_int) {
        ch0_int->setCtrl(this, commandWindow, 0);
    }
    if (ch1_int) {
        ch1_int->setCtrl(this, commandWindow, 1);
    }

    port.ctrl = this;

    if (partitionedQ) {
        writeHighThreshold = (writeBufferSize * (p.write_high_thresh_perc/2) / 100.0);
        writeLowThreshold = (writeBufferSize * (p.write_low_thresh_perc/2) / 100.0);
    } else {
        writeHighThreshold = (writeBufferSize * p.write_high_thresh_perc / 100.0);
        writeLowThreshold = (writeBufferSize * p.write_low_thresh_perc / 100.0);      
    }


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
            readBufferSize, readQueueSizeCh1 + respQueueCh1.size(),
            neededEntries);

    auto rdsize_new = readQueueSizeCh1 + respQueueCh1.size() + neededEntries;
    return rdsize_new > (readBufferSize/2);
}

bool
HBMCtrl::readQueueFull(unsigned int neededEntries)
{
    DPRINTF(MemCtrl,
            "HBMCtrl: Read queue limit %d, entries needed %d\n",
            readBufferSize, neededEntries);

    auto rdsize_new = totalReadQueueSize + respQueue.size() + respQueueCh1.size() + neededEntries;
    return rdsize_new > readBufferSize;
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

    // TODO: make the interleaving bit across pseudo channels a parameter
    if (bits(pkt->getAddr(), 6) == 0)
    {
        is_ch0 = true;
    }
    else
    {
        is_ch0 = false;
    }

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
                retryWrReqCh1 = true;
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
                                        HBMCtrl::readQueueFull(pkt_count))
            {
                DPRINTF(MemCtrl, "Read queue full, not accepting\n");
                // remember that we have to retry this port
                retryRdReqCh1 = true;
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
                                            HBMCtrl::readQueueFull(pkt_count))
            {
                DPRINTF(MemCtrl, "Read queue full, not accepting\n");
                // remember that we have to retry this port
                retryRdReqCh1 = true;
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
    MemCtrl::nextReqEventLogic(ch1_int, respQueueCh1, respondEventCh1, nextReqEventCh1);

    // If there is space available and we have writes waiting then let
    // them retry. This is done here to ensure that the retry does not
    // cause a nextReqEvent to be scheduled before we do so as part of
    // the next request processing
    if (retryWrReqCh1 && totalWriteQueueSize < writeBufferSize) {
        retryWrReqCh1 = false;
        MemCtrl::port.sendRetryReq();
    }
}

void
HBMCtrl::processRespondEventCh1()
{
    DPRINTF(MemCtrl,
            "processRespondEventCh1(): Some req has reached its readyTime\n");
    MemCtrl::respondEventLogic(ch1_int, respQueueCh1, respondEventCh1);
    // We have made a location in the queue available at this point,
    // so if there is a read that was forced to wait, retry now
    if (retryRdReqCh1) {
        retryRdReqCh1 = false;
        MemCtrl::port.sendRetryReq();
    }
}

void
HBMCtrl::pruneRowBurstTick()
{
    auto it = rowBurstTicks.begin();
    while (it != rowBurstTicks.end())
    {
        auto current_it = it++;
        if (MemCtrl::getBurstWindow(curTick()) > *current_it)
        {
            DPRINTF(MemCtrl, "Removing burstTick for %d\n", *current_it);
            rowBurstTicks.erase(current_it);
        }
    }
}

void
HBMCtrl::pruneColBurstTick()
{
    auto it = colBurstTicks.begin();
    while (it != colBurstTicks.end())
    {
        auto current_it = it++;
        if (MemCtrl::getBurstWindow(curTick()) > *current_it)
        {
            DPRINTF(MemCtrl, "Removing burstTick for %d\n", *current_it);
            colBurstTicks.erase(current_it);
        }
    }
}

Tick
HBMCtrl::verifySingleCmd(Tick cmd_tick, Tick max_cmds_per_burst, bool row_cmd)
{
    // start with assumption that there is no contention on command bus
    Tick cmd_at = cmd_tick;

    // get tick aligned to burst window
    Tick burst_tick = MemCtrl::getBurstWindow(cmd_tick);

    // verify that we have command bandwidth to issue the command
    // if not, iterate over next window(s) until slot found

    if (row_cmd) {
        while (rowBurstTicks.count(burst_tick) >= max_cmds_per_burst)
        {
            DPRINTF(MemCtrl, "Contention found on row command bus at %d\n",
                    burst_tick);
            burst_tick += commandWindow;
            cmd_at = burst_tick;
        }
        DPRINTF(MemCtrl, "Now can send a row cmd_at %d\n",
                    cmd_at);
        rowBurstTicks.insert(burst_tick);

    } else {

        //cmd_at = last_col_tick + commandWindow;
        //cmd_at  = std::max(cmd_at, curTick());
        //last_col_tick = cmd_at;

        while (colBurstTicks.count(burst_tick) >= max_cmds_per_burst)
        {
            DPRINTF(MemCtrl, "Contention found on col command bus at %d\n",
                    burst_tick);
            burst_tick += commandWindow;
            cmd_at = burst_tick;
        }
        DPRINTF(MemCtrl, "Now can send a col cmd_at %d\n",
                    cmd_at);
        colBurstTicks.insert(burst_tick);


    }
    return cmd_at;
}

Tick
HBMCtrl::verifyMultiCmd(Tick cmd_tick, Tick max_cmds_per_burst,
                        Tick max_multi_cmd_split)
{

    // start with assumption that there is no contention on command bus
    Tick cmd_at = cmd_tick;

    // get tick aligned to burst window
    Tick burst_tick = MemCtrl::getBurstWindow(cmd_tick);

    // Command timing requirements are from 2nd command
    // Start with assumption that 2nd command will issue at cmd_at and
    // find prior slot for 1st command to issue
    // Given a maximum latency of max_multi_cmd_split between the commands,
    // find the burst at the maximum latency prior to cmd_at
    Tick burst_offset = 0;
    Tick first_cmd_offset = cmd_tick % commandWindow;
    while (max_multi_cmd_split > (first_cmd_offset + burst_offset))
    {
        burst_offset += commandWindow;
    }
    // get the earliest burst aligned address for first command
    // ensure that the time does not go negative
    Tick first_cmd_tick = burst_tick - std::min(burst_offset, burst_tick);

    // Can required commands issue?
    bool first_can_issue = false;
    bool second_can_issue = false;
    // verify that we have command bandwidth to issue the command(s)
    while (!first_can_issue || !second_can_issue)
    {
        bool same_burst = (burst_tick == first_cmd_tick);
        auto first_cmd_count = rowBurstTicks.count(first_cmd_tick);
        auto second_cmd_count = same_burst ? first_cmd_count + 1 : rowBurstTicks.count(burst_tick);

        first_can_issue = first_cmd_count < max_cmds_per_burst;
        second_can_issue = second_cmd_count < max_cmds_per_burst;

        if (!second_can_issue)
        {
            DPRINTF(MemCtrl, "Contention (cmd2) found on command bus at %d\n",
                    burst_tick);
            burst_tick += commandWindow;
            cmd_at = burst_tick;
        }

        // Verify max_multi_cmd_split isn't violated when command 2 is shifted
        // If commands initially were issued in same burst, they are
        // now in consecutive bursts and can still issue B2B
        bool gap_violated = !same_burst &&
                            ((burst_tick - first_cmd_tick) > max_multi_cmd_split);

        if (!first_can_issue || (!second_can_issue && gap_violated))
        {
            DPRINTF(MemCtrl, "Contention (cmd1) found on command bus at %d\n",
                    first_cmd_tick);
            first_cmd_tick += commandWindow;
        }
    }

    // Add command to burstTicks
    rowBurstTicks.insert(burst_tick);
    rowBurstTicks.insert(first_cmd_tick);

    return cmd_at;
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

} // namespace memory
} // namespace gem5
