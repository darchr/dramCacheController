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

#include "mem/mem_ctrl.hh"

#include "base/trace.hh"
#include "debug/DRAM.hh"
#include "debug/Drain.hh"
#include "debug/MemCtrl.hh"
#include "debug/NVM.hh"
#include "debug/QOS.hh"
#include "mem/dram_interface.hh"
#include "mem/mem_interface.hh"
#include "mem/nvm_interface.hh"
#include "sim/system.hh"

namespace gem5
{

namespace memory
{

MemCtrl::MemCtrl(const MemCtrlParams &p) :
    SimpleMemCtrl(p)
{
    DPRINTF(MemCtrl, "Setting up controller\n");

    nvm = p.nvm;

    fatal_if(!dram || !nvm, "Memory controller must have two interfaces");

    // hook up interfaces to the controller
    dram->setCtrl(this, commandWindow);
    nvm->setCtrl(this, commandWindow);

    readBufferSize = dram->readBufferSize + nvm->readBufferSize;
    writeBufferSize = dram->writeBufferSize + nvm->writeBufferSize;

    readQueue.resize(p.qos_priorities);
    writeQueue.resize(p.qos_priorities);

    // perform a basic check of the write thresholds
    if (p.write_low_thresh_perc >= p.write_high_thresh_perc)
        fatal("Write buffer low threshold %d must be smaller than the "
              "high threshold %d\n", p.write_low_thresh_perc,
              p.write_high_thresh_perc);
}

Tick
MemCtrl::recvAtomic(PacketPtr pkt)
{
    DPRINTF(MemCtrl, "recvAtomic: %s 0x%x\n",
                     pkt->cmdString(), pkt->getAddr());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    Tick latency = 0;
    // do the actual memory access and turn the packet into a response
    if (dram->getAddrRange().contains(pkt->getAddr())) {
        dram->access(pkt);

        if (pkt->hasData()) {
            // this value is not supposed to be accurate, just enough to
            // keep things going, mimic a closed page
            latency = dram->accessLatency();
        }
    } else if (nvm->getAddrRange().contains(pkt->getAddr())) {
        nvm->access(pkt);

        if (pkt->hasData()) {
            // this value is not supposed to be accurate, just enough to
            // keep things going, mimic a closed page
            latency = nvm->accessLatency();
        }
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    return latency;
}

bool
MemCtrl::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world
    DPRINTF(MemCtrl, "recvTimingReq: request %s addr %#x size %d\n",
            pkt->cmdString(), pkt->getAddr(), pkt->getSize());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller\n");

    // Calc avg gap between requests
    if (prevArrival != 0) {
        stats.totGap += curTick() - prevArrival;
    }
    prevArrival = curTick();

    // What type of media does this packet access?
    bool is_dram;
    if (dram->getAddrRange().contains(pkt->getAddr())) {
        is_dram = true;
    } else if (nvm->getAddrRange().contains(pkt->getAddr())) {
        is_dram = false;
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets
    unsigned size = pkt->getSize();
    uint32_t burst_size = is_dram ? dram->bytesPerBurst() :
                                    nvm->bytesPerBurst();
    unsigned offset = pkt->getAddr() & (burst_size - 1);
    unsigned int pkt_count = divCeil(offset + size, burst_size);

    // run the QoS scheduler and assign a QoS priority value to the packet
    qosSchedule( { &readQueue, &writeQueue }, burst_size, pkt);

    // check local buffers and do not accept if full
    if (pkt->isWrite()) {
        assert(size != 0);
        if (writeQueueFull(pkt_count)) {
            DPRINTF(MemCtrl, "Write queue full, not accepting\n");
            // remember that we have to retry this port
            retryWrReq = true;
            stats.numWrRetry++;
            return false;
        } else {
            addToWriteQueue(pkt, pkt_count, is_dram ? dram : nvm);
            stats.writeReqs++;
            stats.bytesWrittenSys += size;
        }
    } else {
        assert(pkt->isRead());
        assert(size != 0);
        if (readQueueFull(pkt_count)) {
            DPRINTF(MemCtrl, "Read queue full, not accepting\n");
            // remember that we have to retry this port
            retryRdReq = true;
            stats.numRdRetry++;
            return false;
        } else {
            addToReadQueue(pkt, pkt_count, is_dram ? dram : nvm);
            stats.readReqs++;
            stats.bytesReadSys += size;
        }
    }

    return true;
}

void
MemCtrl::processRespondEvent()
{
    DPRINTF(MemCtrl,
            "processRespondEvent(): Some req has reached its readyTime\n");

    MemPacket* mem_pkt = respQueue.front();

    if (mem_pkt->isDram()) {
        // media specific checks and functions when read response is complete
        dram->respondEvent(mem_pkt->rank);
    }

    if (mem_pkt->burstHelper) {
        // it is a split packet
        mem_pkt->burstHelper->burstsServiced++;
        if (mem_pkt->burstHelper->burstsServiced ==
            mem_pkt->burstHelper->burstCount) {
            // we have now serviced all children packets of a system packet
            // so we can now respond to the requestor
            // @todo we probably want to have a different front end and back
            // end latency for split packets
            accessAndRespond(mem_pkt->pkt, frontendLatency + backendLatency);
            delete mem_pkt->burstHelper;
            mem_pkt->burstHelper = NULL;
        }
    } else {
        // it is not a split packet
        accessAndRespond(mem_pkt->pkt, frontendLatency + backendLatency);
    }

    respQueue.pop_front();

    if (!respQueue.empty()) {
        assert(respQueue.front()->readyTime >= curTick());
        assert(!respondEvent.scheduled());
        schedule(respondEvent, respQueue.front()->readyTime);
    } else {
        // if there is nothing left in any queue, signal a drain
        if (drainState() == DrainState::Draining &&
            !totalWriteQueueSize && !totalReadQueueSize &&
            allIntfDrained()) {

            DPRINTF(Drain, "Controller done draining\n");
            signalDrainDone();
        } else {
            // check the refresh state and kick the refresh event loop
            // into action again if banks already closed and just waiting
            // for read to complete
            // DRAM only
            dram->checkRefreshState(mem_pkt->rank);
        }
    }

    delete mem_pkt;

    // We have made a location in the queue available at this point,
    // so if there is a read that was forced to wait, retry now
    if (retryRdReq) {
        retryRdReq = false;
        port.sendRetryReq();
    }
}

MemPacketQueue::iterator
MemCtrl::chooseNext(MemPacketQueue& queue, Tick extra_col_delay)
{
    // This method does the arbitration between requests.

    MemPacketQueue::iterator ret = queue.end();

    if (!queue.empty()) {
        if (queue.size() == 1) {
            // available rank corresponds to state refresh idle
            MemPacket* mem_pkt = *(queue.begin());
            if (packetReady(mem_pkt, mem_pkt->isDram()? dram : nvm)) {
                ret = queue.begin();
                DPRINTF(MemCtrl,
                "Single request, going to a free rank\n");
            } else {
                DPRINTF(MemCtrl,
                "Single request, going to a busy rank\n");
            }
        } else if (memSchedPolicy == enums::fcfs) {
            // check if there is a packet going to a free rank
            for (auto i = queue.begin(); i != queue.end(); ++i) {
                MemPacket* mem_pkt = *i;
                if (packetReady(mem_pkt, mem_pkt->isDram()? dram : nvm)) {
                    ret = i;
                    break;
                }
            }
        } else if (memSchedPolicy == enums::frfcfs) {
            ret = chooseNextFRFCFS(queue, extra_col_delay);
        } else {
            panic("No scheduling policy chosen\n");
        }
    }
    return ret;
}

MemPacketQueue::iterator
MemCtrl::chooseNextFRFCFS(MemPacketQueue& queue, Tick extra_col_delay)
{
    auto selected_pkt_it = queue.end();
    auto nvm_pkt_it = queue.end();
    Tick col_allowed_at = MaxTick;
    Tick nvm_col_allowed_at = MaxTick;

    // time we need to issue a column command to be seamless
    const Tick min_col_at = std::max(nextBurstAt + extra_col_delay, curTick());

    assert (dram && nvm);

    // find optimal packet for each interface
    // Select packet by default to give priority if both
    // can issue at the same time or seamlessly
    std::tie(selected_pkt_it, col_allowed_at) =
                dram->chooseNextFRFCFS(queue, min_col_at);
    std::tie(nvm_pkt_it, nvm_col_allowed_at) =
                nvm->chooseNextFRFCFS(queue, min_col_at);

    // Compare DRAM and NVM and select NVM if it can issue
    // earlier than the DRAM packet
    if (col_allowed_at > nvm_col_allowed_at) {
        selected_pkt_it = nvm_pkt_it;
    }

    if (selected_pkt_it == queue.end()) {
        DPRINTF(MemCtrl, "%s no available packets found\n", __func__);
    }

    return selected_pkt_it;
}

void
MemCtrl::accessAndRespond(PacketPtr pkt, Tick static_latency)
{
    DPRINTF(MemCtrl, "Responding to Address %#x.. \n", pkt->getAddr());

    bool needsResponse = pkt->needsResponse();
    // do the actual memory access which also turns the packet into a
    // response
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        dram->access(pkt);
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        nvm->access(pkt);
    } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
    }

    // turn packet around to go back to requestor if response expected
    if (needsResponse) {
        // access already turned the packet into a response
        assert(pkt->isResponse());
        // response_time consumes the static latency and is charged also
        // with headerDelay that takes into account the delay provided by
        // the xbar and also the payloadDelay that takes into account the
        // number of data beats.
        Tick response_time = curTick() + static_latency + pkt->headerDelay +
                             pkt->payloadDelay;
        // Here we reset the timing of the packet before sending it out.
        pkt->headerDelay = pkt->payloadDelay = 0;

        // queue the packet in the response queue to be sent out after
        // the static latency has passed
        port.schedTimingResp(pkt, response_time);
    } else {
        // @todo the packet is going to be deleted, and the MemPacket
        // is still having a pointer to it
        pendingDelete.reset(pkt);
    }

    DPRINTF(MemCtrl, "Done\n");

    return;
}

void
MemCtrl::doBurstAccess(MemPacket* mem_pkt)
{
    // first clean up the burstTick set, removing old entries
    // before adding new entries for next burst
    pruneBurstTick();

    // When was command issued?
    Tick cmd_at;

    // Issue the next burst and update bus state to reflect
    // when previous command was issued

    std::vector<MemPacketQueue>& queue = selQueue(mem_pkt->isRead());

    if (dram && dram->getAddrRange().contains(mem_pkt->getAddr())) {
        std::tie(cmd_at, nextBurstAt) =
                 dram->doBurstAccess(mem_pkt, nextBurstAt, queue);

        // Update timing for NVM ranks if NVM is configured on this channel
        nvm->addRankToRankDelay(cmd_at);

    } else if (nvm && nvm->getAddrRange().contains(mem_pkt->getAddr())) {
        std::tie(cmd_at, nextBurstAt) =
                 nvm->doBurstAccess(mem_pkt, nextBurstAt, queue);

        // Update timing for NVM ranks if NVM is configured on this channel
        dram->addRankToRankDelay(cmd_at);
    } else {
        panic("Can't handle address range for packet in doBurstAccess\n");
    }

    DPRINTF(MemCtrl, "Access to %#x, ready at %lld next burst at %lld.\n",
            mem_pkt->addr, mem_pkt->readyTime, nextBurstAt);

    // Update the minimum timing between the requests, this is a
    // conservative estimate of when we have to schedule the next
    // request to not introduce any unecessary bubbles. In most cases
    // we will wake up sooner than we have to.
    nextReqTime = nextBurstAt - dram->commandOffset();


    // Update the common bus stats
    if (mem_pkt->isRead()) {
        ++readsThisTime;
        // Update latency stats
        stats.requestorReadTotalLat[mem_pkt->requestorId()] +=
            mem_pkt->readyTime - mem_pkt->entryTime;
        stats.requestorReadBytes[mem_pkt->requestorId()] += mem_pkt->size;
    } else {
        ++writesThisTime;
        stats.requestorWriteBytes[mem_pkt->requestorId()] += mem_pkt->size;
        stats.requestorWriteTotalLat[mem_pkt->requestorId()] +=
            mem_pkt->readyTime - mem_pkt->entryTime;
    }
}

void
MemCtrl::processNextReqEvent()
{
    // transition is handled by QoS algorithm if enabled
    if (turnPolicy) {
        // select bus state - only done if QoS algorithms are in use
        busStateNext = selectNextBusState();
    }

    // detect bus state change
    bool switched_cmd_type = (busState != busStateNext);
    // record stats
    recordTurnaroundStats();

    DPRINTF(MemCtrl, "QoS Turnarounds selected state %s %s\n",
            (busState==MemCtrl::READ)?"READ":"WRITE",
            switched_cmd_type?"[turnaround triggered]":"");

    if (switched_cmd_type) {
        if (busState == MemCtrl::READ) {
            DPRINTF(MemCtrl,
                    "Switching to writes after %d reads with %d reads "
                    "waiting\n", readsThisTime, totalReadQueueSize);
            stats.rdPerTurnAround.sample(readsThisTime);
            readsThisTime = 0;
        } else {
            DPRINTF(MemCtrl,
                    "Switching to reads after %d writes with %d writes "
                    "waiting\n", writesThisTime, totalWriteQueueSize);
            stats.wrPerTurnAround.sample(writesThisTime);
            writesThisTime = 0;
        }
    }

    // updates current state
    busState = busStateNext;

    for (auto queue = readQueue.rbegin();
            queue != readQueue.rend(); ++queue) {
            // select non-deterministic NVM read to issue
            // assume that we have the command bandwidth to issue this along
            // with additional RD/WR burst with needed bank operations
            if (nvm->readsWaitingToIssue()) {
                // select non-deterministic NVM read to issue
                nvm->chooseRead(*queue);
            }
    }

    // check ranks for refresh/wakeup - uses busStateNext, so done after
    // turnaround decisions
    // Default to busy status and update based on interface specifics
    bool dram_busy, nvm_busy = true;

    dram_busy = dram->isBusy(false, false);
    nvm_busy = nvm->isBusy(totalReadQueueSize == 0,
                    nvm->numWritesQueued == totalWriteQueueSize);

    // Default state of unused interface is 'true'
    // Simply AND the busy signals to determine if system is busy
    if (dram_busy && nvm_busy) {
        // if all ranks are refreshing wait for them to finish
        // and stall this state machine without taking any further
        // action, and do not schedule a new nextReqEvent
        return;
    }

    // when we get here it is either a read or a write
    if (busState == READ) {

        // track if we should switch or not
        bool switch_to_writes = false;

        if (totalReadQueueSize == 0) {
            // In the case there is no read request to go next,
            // trigger writes if we have passed the low threshold (or
            // if we are draining)
            if (!(totalWriteQueueSize == 0) &&
                (drainState() == DrainState::Draining ||
                 totalWriteQueueSize > writeLowThreshold)) {

                DPRINTF(MemCtrl,
                        "Switching to writes due to read queue empty\n");
                switch_to_writes = true;
            } else {
                // check if we are drained
                // not done draining until in PWR_IDLE state
                // ensuring all banks are closed and
                // have exited low power states
                if (drainState() == DrainState::Draining &&
                    respQueue.empty() && allIntfDrained()) {

                    DPRINTF(Drain, "MemCtrl controller done draining\n");
                    signalDrainDone();
                }

                // nothing to do, not even any point in scheduling an
                // event for the next request
                return;
            }
        } else {

            bool read_found = false;
            MemPacketQueue::iterator to_read;
            uint8_t prio = numPriorities();

            for (auto queue = readQueue.rbegin();
                 queue != readQueue.rend(); ++queue) {

                prio--;

                DPRINTF(QOS,
                        "Checking READ queue [%d] priority [%d elements]\n",
                        prio, queue->size());

                // Figure out which read request goes next
                // If we are changing command type, incorporate the minimum
                // bus turnaround delay which will be rank to rank delay
                to_read = chooseNext((*queue), switched_cmd_type ?
                                               minWriteToReadDataGap() : 0);

                if (to_read != queue->end()) {
                    // candidate read found
                    read_found = true;
                    break;
                }
            }

            // if no read to an available rank is found then return
            // at this point. There could be writes to the available ranks
            // which are above the required threshold. However, to
            // avoid adding more complexity to the code, return and wait
            // for a refresh event to kick things into action again.
            if (!read_found) {
                DPRINTF(MemCtrl, "No Reads Found - exiting\n");
                return;
            }

            auto mem_pkt = *to_read;

            doBurstAccess(mem_pkt);

            // sanity check
            assert(mem_pkt->size <= (mem_pkt->isDram() ?
                                        dram->bytesPerBurst() :
                                        nvm->bytesPerBurst()));
            assert(mem_pkt->readyTime >= curTick());

            // log the response
            logResponse(MemCtrl::READ, (*to_read)->requestorId(),
                        mem_pkt->qosValue(), mem_pkt->getAddr(), 1,
                        mem_pkt->readyTime - mem_pkt->entryTime);


            // Insert into response queue. It will be sent back to the
            // requestor at its readyTime
            if (respQueue.empty()) {
                assert(!respondEvent.scheduled());
                schedule(respondEvent, mem_pkt->readyTime);
            } else {
                assert(respQueue.back()->readyTime <= mem_pkt->readyTime);
                assert(respondEvent.scheduled());
            }

            respQueue.push_back(mem_pkt);

            // we have so many writes that we have to transition
            // don't transition if the writeRespQueue is full and
            // there are no other writes that can issue
            if ((totalWriteQueueSize > writeHighThreshold) &&
               !(nvm->numWritesQueued == totalWriteQueueSize &&
                 nvm->writeRespQueueFull())) {
                switch_to_writes = true;
            }

            // remove the request from the queue
            // the iterator is no longer valid .
            readQueue[mem_pkt->qosValue()].erase(to_read);
        }

        // switching to writes, either because the read queue is empty
        // and the writes have passed the low threshold (or we are
        // draining), or because the writes hit the hight threshold
        if (switch_to_writes) {
            // transition to writing
            busStateNext = WRITE;
        }
    } else {

        bool write_found = false;
        MemPacketQueue::iterator to_write;
        uint8_t prio = numPriorities();

        for (auto queue = writeQueue.rbegin();
             queue != writeQueue.rend(); ++queue) {

            prio--;

            DPRINTF(QOS,
                    "Checking WRITE queue [%d] priority [%d elements]\n",
                    prio, queue->size());

            // If we are changing command type, incorporate the minimum
            // bus turnaround delay
            to_write = chooseNext((*queue),
                     switched_cmd_type ? minReadToWriteDataGap() : 0);

            if (to_write != queue->end()) {
                write_found = true;
                break;
            }
        }

        // if there are no writes to a rank that is available to service
        // requests (i.e. rank is in refresh idle state) are found then
        // return. There could be reads to the available ranks. However, to
        // avoid adding more complexity to the code, return at this point and
        // wait for a refresh event to kick things into action again.
        if (!write_found) {
            DPRINTF(MemCtrl, "No Writes Found - exiting\n");
            return;
        }

        auto mem_pkt = *to_write;

        // sanity check
        assert(mem_pkt->size <= (mem_pkt->isDram() ?
                                    dram->bytesPerBurst() :
                                    nvm->bytesPerBurst()));

        doBurstAccess(mem_pkt);

        isInWriteQueue.erase(burstAlign(mem_pkt->addr,
                    mem_pkt->isDram() ? dram : nvm));

        // log the response
        logResponse(MemCtrl::WRITE, mem_pkt->requestorId(),
                    mem_pkt->qosValue(), mem_pkt->getAddr(), 1,
                    mem_pkt->readyTime - mem_pkt->entryTime);


        // remove the request from the queue - the iterator is no longer valid
        writeQueue[mem_pkt->qosValue()].erase(to_write);

        delete mem_pkt;

        // If we emptied the write queue, or got sufficiently below the
        // threshold (using the minWritesPerSwitch as the hysteresis) and
        // are not draining, or we have reads waiting and have done enough
        // writes, then switch to reads.
        // If we are interfacing to NVM and have filled the writeRespQueue,
        // with only NVM writes in Q, then switch to reads
        bool below_threshold =
            totalWriteQueueSize + minWritesPerSwitch < writeLowThreshold;

        if (totalWriteQueueSize == 0 ||
            (below_threshold && drainState() != DrainState::Draining) ||
            (totalReadQueueSize && writesThisTime >= minWritesPerSwitch) ||
            (totalReadQueueSize && nvm->writeRespQueueFull() &&
             nvm->numWritesQueued == totalWriteQueueSize)) {

            // turn the bus back around for reads again
            busStateNext = MemCtrl::READ;

            // note that the we switch back to reads also in the idle
            // case, which eventually will check for any draining and
            // also pause any further scheduling if there is really
            // nothing to do
        }
    }
    // It is possible that a refresh to another rank kicks things back into
    // action before reaching this point.
    if (!nextReqEvent.scheduled())
        schedule(nextReqEvent, std::max(nextReqTime, curTick()));

    // If there is space available and we have writes waiting then let
    // them retry. This is done here to ensure that the retry does not
    // cause a nextReqEvent to be scheduled before we do so as part of
    // the next request processing
    if (retryWrReq && totalWriteQueueSize < writeBufferSize) {
        retryWrReq = false;
        port.sendRetryReq();
    }
}

Tick
MemCtrl::minReadToWriteDataGap()
{
    Tick dram_min  = dram ? dram->minReadToWriteDataGap()  : MaxTick;
    Tick nvm_min = nvm ? nvm->minReadToWriteDataGap() : MaxTick;
    return std::min(dram_min, nvm_min);
}

Tick
MemCtrl::minWriteToReadDataGap()
{
    Tick dram_min  = dram ? dram->minWriteToReadDataGap()  : MaxTick;
    Tick nvm_min = nvm ? nvm->minWriteToReadDataGap() : MaxTick;
    return std::min(dram_min, nvm_min);
}

void
MemCtrl::recvFunctional(PacketPtr pkt)
{
    if (dram->getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        dram->functionalAccess(pkt);
    } else if (nvm->getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        nvm->functionalAccess(pkt);
   } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
   }
}

bool
MemCtrl::allIntfDrained() const
{
    // ensure dram is in power down and refresh IDLE states
    bool dram_drained = dram->allRanksDrained();
    // No outstanding NVM writes
    // All other queues verified as needed with calling logic
    bool nvm_drained = nvm->allRanksDrained();
    return (dram_drained && nvm_drained);
}

DrainState
MemCtrl::drain()
{
    // if there is anything in any of our internal queues, keep track
    // of that as well
    if (!(!totalWriteQueueSize && !totalReadQueueSize && respQueue.empty() &&
          allIntfDrained())) {

        DPRINTF(Drain, "Memory controller not drained, write: %d, read: %d,"
                " resp: %d\n", totalWriteQueueSize, totalReadQueueSize,
                respQueue.size());

        // the only queue that is not drained automatically over time
        // is the write queue, thus kick things into action if needed
        if (!totalWriteQueueSize && !nextReqEvent.scheduled()) {
            schedule(nextReqEvent, curTick());
        }

        dram->drainRanks();

        return DrainState::Draining;
    } else {
        return DrainState::Drained;
    }
}

void
MemCtrl::drainResume()
{
    if (!isTimingMode && system()->isTimingMode()) {
        // if we switched to timing mode, kick things into action,
        // and behave as if we restored from a checkpoint
        startup();
        dram->startup();
    } else if (isTimingMode && !system()->isTimingMode()) {
        // if we switch from timing mode, stop the refresh events to
        // not cause issues with KVM
        dram->suspend();
    }

    // update the mode
    isTimingMode = system()->isTimingMode();
}

std::vector<MemInterface*>
MemCtrl::getMemInterface()
{
    std::vector<MemInterface*> intrfc;
    intrfc.push_back(dram);
    intrfc.push_back(nvm);
    return intrfc;
}

} // namespace memory
} // namespace gem5
