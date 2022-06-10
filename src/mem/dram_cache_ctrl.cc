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

#include "mem/dram_cache_ctrl.hh"

#include "base/trace.hh"
#include "debug/DCacheCtrl.hh"
#include "debug/DRAM.hh"
#include "debug/Drain.hh"
#include "debug/QOS.hh"
#include "mem/dram_interface.hh"
#include "mem/mem_interface.hh"
#include "sim/system.hh"

namespace gem5
{

namespace memory
{

DCacheCtrl::DCacheCtrl(const DCacheCtrlParams &p):
    SimpleMemCtrl(p),
    reqPort(name() + ".req_port", *this),
    farMemory(p.far_memory),
    dramCacheSize(p.dram_cache_size),
    blockSize(p.block_size),
    addrSize(p.addr_size),
    orbMaxSize(p.orb_max_size), orbSize(0),
    crbMaxSize(p.crb_max_size), crbSize(0),
    retry(false), retryFMW(false),
    stallRds(false), sendFarRdReq(true),
    waitingForRetryReqPort(false),
    rescheduleLocRead(false),
    rescheduleLocWrite(false),
    locWrCounter(0), farWrCounter(0),
    maxConf(0),
    maxLocRdEvQ(0), maxLocRdRespEvQ(0), maxLocWrEvQ(0),
    maxFarRdEvQ(0), maxFarRdRespEvQ(0), maxFarWrEvQ(0),
    locMemReadEvent([this]{ processLocMemReadEvent(); }, name()),
    locMemReadRespEvent([this]{ processLocMemReadRespEvent(); }, name()),
    locMemWriteEvent([this]{ processLocMemWriteEvent(); }, name()),
    farMemReadEvent([this]{ processFarMemReadEvent(); }, name()),
    farMemReadRespEvent([this]{ processFarMemReadRespEvent(); }, name()),
    farMemWriteEvent([this]{ processFarMemWriteEvent(); }, name())
{
    fatal_if(!dram, "DRAM cache controller must have a DRAM interface.\n");

    panic_if(orbMaxSize<4, "ORB maximum size must be at least 4.\n");

    // hook up interfaces to the controller
    dram->setCtrl(this, commandWindow);

    tagMetadataStore.resize(dramCacheSize/blockSize);
    dirtAdrGen();
    pktLocMemRead.resize(1);
    pktLocMemWrite.resize(1);
    // pktFarMemRead.resize(1);
    // pktFarMemWrite.resize(1);

    // perform a basic check of the write thresholds
    // if (p.write_low_thresh_perc >= p.write_high_thresh_perc)
    //     fatal("Write buffer low threshold %d must be smaller than the "
    //           "high threshold %d\n", p.write_low_thresh_perc,
    //           p.write_high_thresh_perc);

    // if (orbMaxSize>512) {
    //     locWrDrainPerc = 0.25;
    // }
    // else {
    //     locWrDrainPerc = 0.5;
    // }

    // This is actually a locWriteHighThreshold
    writeHighThreshold = 0.5 * orbMaxSize;

    minLocWrPerSwitch = 0.25 * orbMaxSize;

    // if (orbMaxSize == 1) {
    //     writeHighThreshold = 1;
    //     minLocWrPerSwitch = 0;
    // }

    // if (orbMaxSize == 2) {
    //     writeHighThreshold = 1;
    //     minLocWrPerSwitch = 1;
    // }



}

void
DCacheCtrl::init()
{
   if (!port.isConnected()) {
        fatal("DCacheCtrl %s is unconnected!\n", name());
    } else if (!reqPort.isConnected()) {
        fatal("DCacheCtrl %s is unconnected!\n", name());
    } else {
        port.sendRangeChange();
        //reqPort.recvRangeChange();
    }
}

void
DCacheCtrl::accessAndRespond(PacketPtr pkt, Tick static_latency,
                                            MemInterface* mem_intr)
{
    bool needsResponse = pkt->needsResponse();
    // do the actual memory access which also turns the packet into a
    // response
    panic_if(!mem_intr->getAddrRange().contains(pkt->getAddr()),
             "Can't handle address range for packet %s\n", pkt->print());
    mem_intr->access(pkt);
    // PacketPtr copyOwPkt = new Packet(pkt, false, pkt->isRead());
    // reqPort.sendFunctional(copyOwPkt);

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

    return;
}

bool
DCacheCtrl::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world
    // DPRINTF(DCacheCtrl, "dc: got %s %lld\n", pkt->cmdString(), pkt->getAddr());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller\n");

    // Calc avg gap between requests
    if (prevArrival != 0) {
        stats.totGap += curTick() - prevArrival;
    }
    prevArrival = curTick();

    // Validate that pkt's address maps to the dram
    assert(dram && dram->getAddrRange().contains(pkt->getAddr()));


    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets

    if (
        ((pktFarMemWrite.size() >= (orbMaxSize/2)) || (!pktFarMemWrite.empty() && pktFarMemRead.empty())) &&
        !waitingForRetryReqPort
       ) {
        if (!farMemWriteEvent.scheduled() && !farMemReadEvent.scheduled()) {
            sendFarRdReq = false;
            schedule(farMemWriteEvent, curTick()+1000);
        }
    }

    Addr addr = pkt->getAddr();

    unsigned burst_size = dram->bytesPerBurst();

    unsigned size = std::min((addr | (burst_size - 1)) + 1,
                             addr + pkt->getSize()) - addr;

    // check merging for writes
    if (pkt->isWrite()) {
        assert(pkt->getSize() != 0);

        bool merged = isInWriteQueue.find(pkt->getAddr()) !=
            isInWriteQueue.end();

        if (merged) {
            stats.writePktSize[ceilLog2(size)]++;
            stats.writeBursts++;
            stats.requestorWriteAccesses[pkt->requestorId()]++;
            stats.mergedWrBursts++;

            accessAndRespond(pkt, frontendLatency, farMemory);

            return true;
        }
    }

    // check forwarding for reads
    bool foundInORB = false;
    bool foundInCRB = false;
    bool foundInFarMemWrite = false;

    if (pkt->isRead()) {
        stats.readPktSize[ceilLog2(size)]++;
        stats.readBursts++;
        stats.requestorReadAccesses[pkt->requestorId()]++;

        assert(pkt->getSize() != 0);

        if (isInWriteQueue.find(pkt->getAddr()) != isInWriteQueue.end()) {

            if (!ORB.empty()) {
                for (const auto& e : ORB) {

                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e.second->validEntry &&
                        e.second->owPkt->isWrite() &&
                        e.second->owPkt->getAddr() <= addr &&
                        ((addr + size) <=
                        (e.second->owPkt->getAddr() +
                        e.second->owPkt->getSize()))) {

                        foundInORB = true;

                        stats.servicedByWrQ++;

                        stats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }

            if (!foundInORB && !CRB.empty()) {
                for (const auto& e : CRB) {

                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e.second->isWrite() &&
                        e.second->getAddr() <= addr &&
                        ((addr + size) <=
                        (e.second->getAddr() + e.second->getSize()))) {

                        foundInCRB = true;

                        stats.servicedByWrQ++;

                        stats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }

            if (!foundInORB && !foundInCRB && !pktFarMemWrite.empty()) {
                for (const auto& e : pktFarMemWrite) {
                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e->getAddr() <= addr &&
                        ((addr + size) <=
                        (e->getAddr() +
                         e->getSize()))) {

                        foundInFarMemWrite = true;

                        stats.servicedByWrQ++;

                        stats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }
        }

        if (foundInORB || foundInCRB || foundInFarMemWrite) {
            stats.writePktSize[ceilLog2(size)]++;
            stats.writeBursts++;
            stats.requestorWriteAccesses[pkt->requestorId()]++;

            accessAndRespond(pkt, frontendLatency, farMemory);

            return true;
        }
    }

    // process conflicting requests.
    // conflicts are checked only based on Index of DRAM cache
    if (checkConflictInDramCache(pkt)) {

        stats.totNumConf++;

        if (CRB.size()>=crbMaxSize) {

            stats.totNumConfBufFull++;

            retry = true;

            if (pkt->isRead()) {
                stats.numRdRetry++;
            }
            else {
                stats.numWrRetry++;
            }
            return false;
        }

        CRB.push_back(std::make_pair(curTick(), pkt));

        if (pkt->isWrite()) {
            isInWriteQueue.insert(pkt->getAddr());
        }

        if (CRB.size() > maxConf) {
            maxConf = CRB.size();
            stats.maxNumConf = CRB.size();
        }
        stats.writePktSize[ceilLog2(size)]++;
        stats.writeBursts++;
        stats.requestorWriteAccesses[pkt->requestorId()]++;
        return true;
    }
    // check if ORB or FMWB is full and set retry
    if (pktFarMemWrite.size()>= (orbMaxSize / 2)) {
        DPRINTF(DCacheCtrl, "FMWBfull: %lld\n", pkt->getAddr());
        retryFMW = true;

        if (pkt->isRead()) {
            stats.numRdRetry++;
        }
        else {
            stats.numWrRetry++;
        }
        return false;
    }

    if (ORB.size() >= orbMaxSize) {
        DPRINTF(DCacheCtrl, "ORBfull: addr %lld\n", pkt->getAddr());
        retry = true;

        if (pkt->isRead()) {
            stats.numRdRetry++;
        }
        else {
            stats.numWrRetry++;
        }
        return false;
    }

    // if none of the above cases happens,
    // then add the pkt to the outstanding requests buffer
    handleRequestorPkt(pkt);

    if (pkt->isWrite()) {
        isInWriteQueue.insert(pkt->getAddr());
    }

    // if (pktLocMemRead[0].empty() && !stallRds && !rescheduleLocRead) {
    //     assert(!locMemReadEvent.scheduled());
    //     schedule(locMemReadEvent, std::max(nextReqTime, curTick()));
    // }

    //else {
    //    assert(locMemReadEvent.scheduled() || stallRds || rescheduleLocRead); //dram->isBusy(false, false)
    //}

    pktLocMemRead[0].push_back(ORB.at(pkt->getAddr())->dccPkt);

    if (!stallRds && !rescheduleLocRead && !locMemReadEvent.scheduled()) {
        schedule(locMemReadEvent, std::max(nextReqTime, curTick()));
    }

    ORB.at(pkt->getAddr())->locRdEntered = curTick();

    if (pktLocMemRead[0].size() > maxLocRdEvQ) {
        maxLocRdEvQ = pktLocMemRead[0].size();
        stats.maxLocRdEvQ = pktLocMemRead[0].size();
    }

    DPRINTF(DCacheCtrl, "dc: acc %lld\n", pkt->getAddr());
    stats.writePktSize[ceilLog2(size)]++;
    stats.writeBursts++;
    stats.requestorWriteAccesses[pkt->requestorId()]++;
    return true;
}

void
DCacheCtrl::processLocMemReadEvent()
{
    if (stallRds || dram->isBusy(false, false) || rescheduleLocRead) {
        // it's possible that dram is busy and we return here before
        // reching to read_found check to set rescheduleLocRead
        if (dram->isBusy(false, false)) {
            rescheduleLocRead = true;
        }
        return;
    }

    assert(!pktLocMemRead[0].empty());

    MemPacketQueue::iterator to_read;

    bool read_found = false;

    bool switched_cmd_type = (busState == DCacheCtrl::WRITE);

    for (auto queue = pktLocMemRead.rbegin();
                 queue != pktLocMemRead.rend(); ++queue) {
        to_read = SimpleMemCtrl::chooseNext((*queue), switched_cmd_type ?
                                minWriteToReadDataGap() : 0, dram);
        if (to_read != queue->end()) {
            // candidate read found
            read_found = true;
            break;
        }
    }

    if (!read_found) {
        // Probably dram is refreshing.
        // Simply return, let the dram device
        // reschedule again once refresh is done.
        rescheduleLocRead = true;
        return;
    }

    auto orbEntry = ORB.at((*to_read)->getAddr());

    DPRINTF(DCacheCtrl, "LocRd: %lld\n", orbEntry->owPkt->getAddr());

    // sanity check for the chosen packet
    assert(orbEntry->validEntry);
    assert(orbEntry->dccPkt->isDram());
    assert(orbEntry->dccPkt->isRead());
    assert(orbEntry->state == locMemRead);
    assert(!orbEntry->issued);

    busState = DCacheCtrl::READ;

    if (switched_cmd_type) {
        stats.wrToRdTurnAround++;
    }

    SimpleMemCtrl::doBurstAccess(orbEntry->dccPkt, dram);

    // sanity check
    //assert(orbEntry->dccPkt->size <= dram->bytesPerBurst());
    assert(orbEntry->dccPkt->readyTime >= curTick());

    if (orbEntry->owPkt->isRead() && orbEntry->isHit) {
        logResponse(DCacheCtrl::READ,
                    orbEntry->dccPkt->requestorId(),
                    orbEntry->dccPkt->qosValue(),
                    orbEntry->owPkt->getAddr(), 1,
                    orbEntry->dccPkt->readyTime - orbEntry->dccPkt->entryTime);
    }

    if (addrLocRdRespReady.empty()) {
        assert(!locMemReadRespEvent.scheduled());
        schedule(locMemReadRespEvent, orbEntry->dccPkt->readyTime);
    }
    else {
        assert(ORB.at(addrLocRdRespReady.back())->dccPkt->readyTime
                            <= orbEntry->dccPkt->readyTime);

        assert(locMemReadRespEvent.scheduled());
    }

    addrLocRdRespReady.push_back(orbEntry->owPkt->getAddr());

    if (addrLocRdRespReady.size() > maxLocRdRespEvQ) {
        maxLocRdRespEvQ = addrLocRdRespReady.size();
        stats.maxLocRdRespEvQ = addrLocRdRespReady.size();
    }

    // keep the state as it is, no transition
    orbEntry->state = locMemRead;
    // mark the entry as issued (while in locMemRead)
    orbEntry->issued = true;
    // record the tick it was issued
    orbEntry->locRdIssued = curTick();

    pktLocMemRead[0].erase(to_read);

    unsigned rdsNum = pktLocMemRead[0].size();
    unsigned wrsNum = pktLocMemWrite[0].size();

    if ((rdsNum == 0 && wrsNum != 0) ||
        (wrsNum >= writeHighThreshold)) {
        stallRds = true;
        if (!locMemWriteEvent.scheduled()) {
            schedule(locMemWriteEvent, std::max(nextReqTime, curTick()));
        }
        return;
    }

    if (!pktLocMemRead[0].empty() && !locMemReadEvent.scheduled()) {
        //assert(!locMemReadEvent.scheduled());
        schedule(locMemReadEvent, std::max(nextReqTime, curTick()));
    }
}

void
DCacheCtrl::processLocMemReadRespEvent()
{
    assert(!addrLocRdRespReady.empty());

    reqBufferEntry* orbEntry = ORB.at(addrLocRdRespReady.front());

    DPRINTF(DCacheCtrl, "LocRdResp: %lld\n", orbEntry->owPkt->getAddr());

    // A series of sanity check
    assert(orbEntry->validEntry);
    assert(orbEntry->dccPkt->isDram());
    assert(orbEntry->dccPkt->isRead());
    assert(orbEntry->state == locMemRead);
    assert(orbEntry->issued);
    assert(orbEntry->dccPkt->readyTime == curTick());

    orbEntry->issued = false;

    if (orbEntry->handleDirtyLine) {
        handleDirtyCacheLine(orbEntry);
    }

    // A flag which is used for retrying read requests
    // in case one slot in ORB becomes available here
    // (happens only for read hits)
    bool canRetry = false;

    dram->respondEvent(orbEntry->dccPkt->rank);

    // Read Hit
    if (orbEntry->owPkt->isRead() &&
        orbEntry->dccPkt->isDram() &&
        orbEntry->isHit) {

            DPRINTF(DCacheCtrl, "Read Hit: %lld\n", orbEntry->owPkt->getAddr());


            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency,
                             farMemory);

            ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(orbEntry->validEntry,
                                                              orbEntry->arrivalTick,
                                                              orbEntry->tagDC,
                                                              orbEntry->indexDC,
                                                              copyOwPkt,
                                                              orbEntry->dccPkt,
                                                              orbEntry->state,
                                                              orbEntry->issued,
                                                              orbEntry->isHit,
                                                              orbEntry->conflict,
                                                              orbEntry->dirtyLineAddr,
                                                              orbEntry->handleDirtyLine,
                                                              orbEntry->locRdEntered,
                                                              orbEntry->locRdIssued,
                                                              orbEntry->locWrEntered,
                                                              orbEntry->locWrIssued,
                                                              orbEntry->farRdEntered,
                                                              orbEntry->farRdIssued,
                                                              orbEntry->farRdRecvd);

            delete orbEntry;

            orbEntry = ORB.at(addrLocRdRespReady.front());
    }

    // Write (Hit & Miss)
    if (orbEntry->owPkt->isWrite() &&
        orbEntry->dccPkt->isRead() &&
        orbEntry->dccPkt->isDram()) {
            // This is a write request which has done a tag check.
            // Delete its dcc packet which is read and create
            // a new one which is write.
            delete orbEntry->dccPkt;

            orbEntry->dccPkt = dram->decodePacket(orbEntry->owPkt,
                                                  orbEntry->owPkt->getAddr(),
                                                  orbEntry->owPkt->getSize(),
                                                  false);

            orbEntry->dccPkt->entryTime = orbEntry->arrivalTick;

            // pass the second argument "false" to
            // indicate a write access to DRAM
            dram->setupRank(orbEntry->dccPkt->rank, false);

            //** transition to locMemWrite
            orbEntry->state = locMemWrite;
            orbEntry->issued = false;
            orbEntry->locWrEntered = curTick();

            pktLocMemWrite[0].push_back(orbEntry->dccPkt);

            unsigned rdsNum = pktLocMemRead[0].size();
            unsigned wrsNum = pktLocMemWrite[0].size();

            if ((rdsNum == 0 && wrsNum != 0) ||
                (wrsNum >= writeHighThreshold)) {
                // stall reads, switch to writes
                stallRds = true;
                if (!locMemWriteEvent.scheduled() && !rescheduleLocWrite) {
                    schedule(locMemWriteEvent,
                             std::max(nextReqTime, curTick()));
                }
            }

            if (pktLocMemWrite[0].size() > maxLocWrEvQ) {
                maxLocWrEvQ = pktLocMemWrite[0].size();
                stats.maxLocWrEvQ = pktLocMemWrite[0].size();
            }
    }

    // Read Miss
    if (orbEntry->owPkt->isRead() &&
        orbEntry->dccPkt->isRead() &&
        orbEntry->dccPkt->isDram() &&
        !orbEntry->isHit) {
        DPRINTF(DCacheCtrl, "Read Miss: %lld\n", orbEntry->owPkt->getAddr());
        // initiate a read from far memory
        // delete the current dcc pkt which is for read from local memory
        delete orbEntry->dccPkt;

        // creating an nvm read dcc-pkt
        // orbEntry->dccPkt = farMemory->decodePacket(orbEntry->owPkt,
        //                                      orbEntry->owPkt->getAddr(),
        //                                      orbEntry->owPkt->getSize(),
        //                                      true);

        // orbEntry->dccPkt->entryTime = orbEntry->arrivalTick;
        // orbEntry->dccPkt->readyTime = MaxTick;
        //** transition to farMemRead
        orbEntry->state = farMemRead;
        orbEntry->issued = false;
        orbEntry->farRdEntered = curTick();

        // if (pktFarMemRead.empty() && sendFarRdReq) {
        //     assert(!farMemReadEvent.scheduled());
        //     schedule(farMemReadEvent, std::max(nextReqTime, curTick()));
        // } else {
        //     assert(farMemReadEvent.scheduled() || !sendFarRdReq || waitingForRetryReqPort);
        // }

        PacketPtr copyOwPkt_2 = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

        pktFarMemRead.push_back(copyOwPkt_2);

        if (!farMemReadEvent.scheduled() && sendFarRdReq && !waitingForRetryReqPort) {
            schedule(farMemReadEvent, std::max(nextReqTime, curTick()));
        }

        if (pktFarMemRead.size() > maxFarRdEvQ) {
            maxFarRdEvQ = pktFarMemRead.size();
            stats.maxFarRdEvQ = pktFarMemRead.size();
        }
    }

    // if (orbEntry->handleDirtyLine) {
    //     numDirtyLinesInDrRdRespQ--;
    // }

    addrLocRdRespReady.pop_front();

    if (!addrLocRdRespReady.empty()) {
        assert(ORB.at(addrLocRdRespReady.front())->dccPkt->readyTime
                >= curTick());
        assert(!locMemReadRespEvent.scheduled());
        schedule(locMemReadRespEvent,
                 ORB.at(addrLocRdRespReady.front())->dccPkt->readyTime);
    } else {

        unsigned rdsNum = pktLocMemRead[0].size();
        unsigned wrsNum = pktLocMemWrite[0].size();

        // if there is nothing left in any queue, signal a drain
        if (drainState() == DrainState::Draining &&
            !wrsNum && !rdsNum &&
            allIntfDrained()) {

            DPRINTF(Drain, "Controller done draining\n");
            signalDrainDone();
        } else if (orbEntry->owPkt->isRead() &&
                   orbEntry->dccPkt->isDram() &&
                   orbEntry->isHit) {
            // check the refresh state and kick the refresh event loop
            // into action again if banks already closed and just waiting
            // for read to complete
            dram->checkRefreshState(orbEntry->dccPkt->rank);
        }
    }

    if (orbEntry->owPkt->isRead() &&
        orbEntry->dccPkt->isDram() &&
        orbEntry->isHit) {
            DPRINTF(DCacheCtrl, "resu conf: %lld\n", orbEntry->owPkt->getAddr());
            // Remove the request from the ORB and
            // bring in a conflicting req waiting
            // in the CRB, if any.
            canRetry = !resumeConflictingReq(orbEntry);
    }

    if (retry && canRetry) {
        retry = false;
        port.sendRetryReq();
    }
}

void
DCacheCtrl::processLocMemWriteEvent()
{
    if (dram->isBusy(false, false) || rescheduleLocWrite) {
        // it's possible that dram is busy and we return here before
        // reching to read_found check to set rescheduleLocRead
        if (dram->isBusy(false, false)) {
            rescheduleLocWrite = true;
        }
        return;
    }

    assert(stallRds);

    assert(!pktLocMemWrite[0].empty());

    MemPacketQueue::iterator to_write;

    bool write_found = false;

    bool switched_cmd_type = (busState == DCacheCtrl::READ);

    if (switched_cmd_type) {
        stats.rdToWrTurnAround++;
    }

    for (auto queue = pktLocMemWrite.rbegin();
                queue != pktLocMemWrite.rend(); ++queue) {
        to_write = chooseNext((*queue), switched_cmd_type ?
                                minReadToWriteDataGap() : 0, dram);
        if (to_write != queue->end()) {
            // candidate write found
            write_found = true;
            break;
        }
    }

    if (!write_found) {
        // Probably dram is refreshing.
        // Simply return, let the dram device
        // reschedule again once refresh is done.
        rescheduleLocWrite = true;
        return;
    }

    auto orbEntry = ORB.at((*to_write)->getAddr());

    DPRINTF(DCacheCtrl, "LocWr: %lld\n", orbEntry->owPkt->getAddr());

    bool canRetry = false;

    assert(orbEntry->validEntry);

    if (orbEntry->owPkt->isRead()) {
        assert(!orbEntry->isHit);
    }
    assert(orbEntry->dccPkt->isDram());
    assert(orbEntry->state == locMemWrite);
    //assert(orbEntry->dccPkt->size <= dram->bytesPerBurst());

    busState = DCacheCtrl::WRITE;

    SimpleMemCtrl::doBurstAccess(orbEntry->dccPkt, dram);

    locWrCounter++;

    if (orbEntry->owPkt->isWrite()) {
        // log the response
        logResponse(DCacheCtrl::WRITE,
                    orbEntry->dccPkt->requestorId(),
                    orbEntry->dccPkt->qosValue(),
                    orbEntry->owPkt->getAddr(),
                    1,
                    orbEntry->dccPkt->readyTime - orbEntry->arrivalTick);
    }

    // Remove the request from the ORB and
    // bring in a conflicting req waiting
    // in the CRB, if any.
    canRetry = !resumeConflictingReq(orbEntry);

    pktLocMemWrite[0].erase(to_write);

    if (retry && canRetry) {
        retry = false;
        port.sendRetryReq();
    }


    if (locWrCounter < minLocWrPerSwitch && !pktLocMemWrite[0].empty()
        // && !pktLocMemRead[0].empty()
       ) {
        // assert(!locMemWriteEvent.scheduled());
        stallRds = true;
        if (!locMemWriteEvent.scheduled()) {
            schedule(locMemWriteEvent, std::max(nextReqTime, curTick()));
        }
        return;
    }
    else if (pktLocMemRead[0].empty() && !pktLocMemWrite[0].empty()) {
        // assert(!locMemWriteEvent.scheduled());
        stallRds = true;
        locWrCounter = 0;
        if (!locMemWriteEvent.scheduled()) {
            schedule(locMemWriteEvent, std::max(nextReqTime, curTick()));
        }
        return;
    }
    else if (!pktLocMemRead[0].empty() && (pktLocMemWrite[0].empty()||locWrCounter>=(minLocWrPerSwitch))) {
            // assert(!locMemReadEvent.scheduled());
            stallRds = false;
            locWrCounter = 0;
            if (!locMemReadEvent.scheduled()) {
                schedule(locMemReadEvent, std::max(nextReqTime, curTick()));
            }
            return;
    }
    else if (pktLocMemRead[0].empty() && pktLocMemWrite[0].empty()) {
        stallRds = false;
        locWrCounter = 0;
    }
}


void
DCacheCtrl::processFarMemReadEvent()
{
    if (!sendFarRdReq || waitingForRetryReqPort) {
        return;
    }

    assert(!pktFarMemRead.empty());

    auto rdPkt = pktFarMemRead.front();
    if (reqPort.sendTimingReq(rdPkt)) {
        pktFarMemRead.pop_front();
        stats.sentRdPort++;
    } else {
        waitingForRetryReqPort = true;
        stats.failedRdPort++;
        return;
    }

    if ((pktFarMemWrite.size() >= (orbMaxSize/2)) ||
        (!pktFarMemWrite.empty() && pktFarMemRead.empty())) {

        sendFarRdReq = false;
        if (!farMemWriteEvent.scheduled()) {
            schedule(farMemWriteEvent, curTick()+1000);
        }

        return;
    }

    if (!pktFarMemRead.empty() && !farMemReadEvent.scheduled()) {

        sendFarRdReq = true;

        schedule(farMemReadEvent, curTick()+1000);

        return;
    }
}

void
DCacheCtrl::processFarMemReadRespEvent()
{
    assert(!pktFarMemReadResp.empty());

    auto orbEntry = ORB.at(pktFarMemReadResp.front()->getAddr());
    
    // sanity check for the chosen packet
    assert(orbEntry->validEntry);
    assert(orbEntry->state == farMemRead);
    //assert(orbEntry->issued);
    assert(orbEntry->owPkt->isRead());
    assert(!orbEntry->isHit);

    // Read miss from dram cache, now is available
    // to send the response back to requestor
    if (orbEntry->owPkt->isRead() && !orbEntry->isHit) {

        logResponse(DCacheCtrl::READ,
                    orbEntry->owPkt->requestorId(),
                    orbEntry->owPkt->qosValue(),
                    orbEntry->owPkt->getAddr(), 1,
                    curTick() - orbEntry->arrivalTick);

        PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                         false,
                                         orbEntry->owPkt->isRead());
        accessAndRespond(orbEntry->owPkt,
                         frontendLatency + backendLatency,
                         farMemory);
        ORB.at(copyOwPkt->getAddr()) =  new reqBufferEntry(orbEntry->validEntry,
                                                          orbEntry->arrivalTick,
                                                          orbEntry->tagDC,
                                                          orbEntry->indexDC,
                                                          copyOwPkt,
                                                          orbEntry->dccPkt,
                                                          orbEntry->state,
                                                          orbEntry->issued,
                                                          orbEntry->isHit,
                                                          orbEntry->conflict,
                                                          orbEntry->dirtyLineAddr,
                                                          orbEntry->handleDirtyLine,
                                                          orbEntry->locRdEntered,
                                                          orbEntry->locRdIssued,
                                                          orbEntry->locWrEntered,
                                                          orbEntry->locWrIssued,
                                                          orbEntry->farRdEntered,
                                                          orbEntry->farRdIssued,
                                                          orbEntry->farRdRecvd);
        delete orbEntry;
        orbEntry = ORB.at(pktFarMemReadResp.front()->getAddr());

    }

    orbEntry->dccPkt = dram->decodePacket(pktFarMemReadResp.front(),
                                          pktFarMemReadResp.front()->getAddr(),
                                          pktFarMemReadResp.front()->getSize(),
                                          false);
    orbEntry->dccPkt->entryTime = orbEntry->arrivalTick;

    // pass the second argument "false" to
    // indicate a write access to DRAM
    dram->setupRank(orbEntry->dccPkt->rank, false);

    //** transition to locMemWrite
    orbEntry->state = locMemWrite;
    orbEntry->issued = false;
    orbEntry->locWrEntered = curTick();

    pktLocMemWrite[0].push_back(orbEntry->dccPkt);

    unsigned rdsNum = pktLocMemRead[0].size();
    unsigned wrsNum = pktLocMemWrite[0].size();

    if ((rdsNum == 0 && wrsNum != 0) ||
        (wrsNum >= writeHighThreshold)) {
        // stall reads, switch to writes
        stallRds = true;
        if (!locMemWriteEvent.scheduled() && !rescheduleLocWrite) {
            schedule(locMemWriteEvent,
                        std::max(nextReqTime, curTick()));
        }
    }

    if (pktLocMemWrite[0].size() > maxLocWrEvQ) {
        maxLocWrEvQ = pktLocMemWrite[0].size();
        stats.maxLocWrEvQ = pktLocMemWrite[0].size();
    }

    pktFarMemReadResp.pop_front();

    if (!pktFarMemReadResp.empty() && !farMemReadRespEvent.scheduled()) {
        schedule(farMemReadRespEvent, curTick());
    }
}

void
DCacheCtrl::processFarMemWriteEvent()
{
    assert(!pktFarMemWrite.empty());
    assert(!sendFarRdReq);
    assert(!waitingForRetryReqPort);
    auto wrPkt = pktFarMemWrite.front();
    if (reqPort.sendTimingReq(wrPkt)) {
        DPRINTF(DCacheCtrl, "FarWrSent:%lld\n", wrPkt->getAddr());
        pktFarMemWrite.pop_front();
        farWrCounter++;
        stats.sentWrPort++;
    } else {
        DPRINTF(DCacheCtrl, "FarWrRetry: %lld\n", pktFarMemWrite.front()->getAddr());
        waitingForRetryReqPort = true;
        stats.failedWrPort++;
        return;
    }

    if (retryFMW && pktFarMemWrite.size()< (orbMaxSize / 2)) {
        retryFMW = false;
        port.sendRetryReq();
    }

    if (!pktFarMemWrite.empty() &&
        (farWrCounter < (orbMaxSize/8) || pktFarMemRead.empty())) {

        sendFarRdReq = false;
        if (!farMemWriteEvent.scheduled()) {
            schedule(farMemWriteEvent, curTick()+1000);
        }

        return;
    }

    if (farWrCounter >= (orbMaxSize/8) && !pktFarMemRead.empty()) {

        sendFarRdReq = true;
        if (!farMemReadEvent.scheduled()) {
            schedule(farMemReadEvent, curTick()+1000);
        }

        return;
    }
}

void
DCacheCtrl::recvReqRetry()
{
    assert(waitingForRetryReqPort);
    waitingForRetryReqPort = false;

    if (sendFarRdReq) {
        if (!farMemReadEvent.scheduled()) {
            schedule(farMemReadEvent, curTick());
        }
        return;
    } else {
        if (!farMemWriteEvent.scheduled()) {
            schedule(farMemWriteEvent, curTick());
        }
        return;
    }
}

bool
DCacheCtrl::recvTimingResp(PacketPtr pkt) // This is equivalant of farMemReadRespEvent
{
    if (pkt->isRead()) {
        pktFarMemReadResp.push_back(pkt);

        if (!farMemReadRespEvent.scheduled()) {
            schedule(farMemReadRespEvent, curTick());
        }
    }

    return true;
}

bool
DCacheCtrl::requestEventScheduled() const
{
    return (locMemReadEvent.scheduled() || locMemWriteEvent.scheduled());
}

void
DCacheCtrl::restartScheduler(Tick tick)
{
    if (!stallRds) {
        //assert(rescheduleLocRead);
        rescheduleLocRead = false;
        if (!locMemReadEvent.scheduled() && !pktLocMemRead[0].empty()) {
            schedule(locMemReadEvent, tick);
        }
        return;
    } else {
        //assert(rescheduleLocWrite);
        rescheduleLocWrite = false;
        if (!locMemWriteEvent.scheduled() && !pktLocMemWrite[0].empty()) {
            schedule(locMemWriteEvent, tick);
        }
        return;
    }
    
}

Port &
DCacheCtrl::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (SimpleMemobj.py)
    if (if_name == "port") {
        return port;
    } else if (if_name == "req_port") {
        return reqPort;
    } else {
        // pass it along to our super class
        return qos::MemCtrl::getPort(if_name, idx);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////

bool
DCacheCtrl::checkConflictInDramCache(PacketPtr pkt)
{
    unsigned indexDC = returnIndexDC(pkt->getAddr(), pkt->getSize());

    for (auto e = ORB.begin(); e != ORB.end(); ++e) {
        if (indexDC == e->second->indexDC && e->second->validEntry) {

            e->second->conflict = true;

            return true;
        }
    }
    return false;
}

Addr
DCacheCtrl::returnIndexDC(Addr request_addr, unsigned size)
{
    int index_bits = ceilLog2(dramCacheSize/blockSize);
    int block_bits = ceilLog2(size);
    return bits(request_addr, block_bits + index_bits-1, block_bits);
}

Addr
DCacheCtrl::returnTagDC(Addr request_addr, unsigned size)
{
    int index_bits = ceilLog2(dramCacheSize/blockSize);
    int block_bits = ceilLog2(size);
    return bits(request_addr, addrSize-1, (index_bits+block_bits));
}

void
DCacheCtrl::checkHitOrMiss(reqBufferEntry* orbEntry)
{
    // access the tagMetadataStore data structure to
    // check if it's hit or miss

    // orbEntry->isHit =
    // tagMetadataStore.at(orbEntry->indexDC).validLine &&
    // (orbEntry->tagDC == tagMetadataStore.at(orbEntry->indexDC).tagDC);

    // if (!tagMetadataStore.at(orbEntry->indexDC).validLine &&
    //     !orbEntry->isHit) {
    //     stats.numColdMisses++;
    // } else if (tagMetadataStore.at(orbEntry->indexDC).validLine &&
    //          !orbEntry->isHit) {
    //     stats.numHotMisses++;
    // }

    //always hit
    // orbEntry->isHit = true;

    // always miss
    orbEntry->isHit = false;
}

bool
DCacheCtrl::checkDirty(Addr addr)
{
    // Addr index = returnIndexDC(addr, blockSize);
    // return (tagMetadataStore.at(index).validLine &&
    //         tagMetadataStore.at(index).dirtyLine);


    // always dirty
    return true;

    // always clean
    // return false;
}

void
DCacheCtrl::handleRequestorPkt(PacketPtr pkt)
{
    // Set is_read and is_dram to
    // "true", to do initial DRAM Read
    MemPacket* dcc_pkt = dram->decodePacket(pkt,
                                            pkt->getAddr(),
                                            pkt->getSize(),
                                            true);

    // pass the second argument "true", for
    // initial DRAM Read for all the received packets
    dram->setupRank(dcc_pkt->rank, true);

    reqBufferEntry* orbEntry = new reqBufferEntry(
                                true, curTick(),
                                returnTagDC(pkt->getAddr(), pkt->getSize()),
                                returnIndexDC(pkt->getAddr(), pkt->getSize()),
                                pkt, dcc_pkt,
                                locMemRead, false,
                                false, false,
                                -1, false,
                                curTick(), MaxTick,
                                MaxTick, MaxTick,
                                MaxTick, MaxTick, MaxTick
                            );

    ORB.emplace(pkt->getAddr(), orbEntry);

    if (pkt->isRead()) {
        logRequest(DCacheCtrl::READ, pkt->requestorId(), pkt->qosValue(),
                   pkt->getAddr(), 1);
    }
    else {
        //copying the packet
        PacketPtr copyOwPkt = new Packet(pkt, false, pkt->isRead());

        accessAndRespond(pkt, frontendLatency, farMemory);

        ORB.at(copyOwPkt->getAddr()) = new reqBufferEntry(orbEntry->validEntry,
                                                          orbEntry->arrivalTick,
                                                          orbEntry->tagDC,
                                                          orbEntry->indexDC,
                                                          copyOwPkt,
                                                          orbEntry->dccPkt,
                                                          orbEntry->state,
                                                          orbEntry->issued,
                                                          orbEntry->isHit,
                                                          orbEntry->conflict,
                                                          orbEntry->dirtyLineAddr,
                                                          orbEntry->handleDirtyLine,
                                                          orbEntry->locRdEntered,
                                                          orbEntry->locRdIssued,
                                                          orbEntry->locWrEntered,
                                                          orbEntry->locWrIssued,
                                                          orbEntry->farRdEntered,
                                                          orbEntry->farRdIssued,
                                                          orbEntry->farRdRecvd);

        delete orbEntry;

        orbEntry = ORB.at(copyOwPkt->getAddr());

        logRequest(DCacheCtrl::WRITE, copyOwPkt->requestorId(),
                   copyOwPkt->qosValue(),
                   copyOwPkt->getAddr(), 1);
    }

    checkHitOrMiss(orbEntry);

    if (checkDirty(orbEntry->owPkt->getAddr()) && !orbEntry->isHit) {
        orbEntry->dirtyLineAddr = tagMetadataStore.at(orbEntry->indexDC).farMemAddr;
        orbEntry->handleDirtyLine = true;
    }

    // Updating Tag & Metadata
    tagMetadataStore.at(orbEntry->indexDC).tagDC = orbEntry->tagDC;
    tagMetadataStore.at(orbEntry->indexDC).indexDC = orbEntry->indexDC;
    tagMetadataStore.at(orbEntry->indexDC).validLine = true;

    if (orbEntry->owPkt->isRead()) {
        if (orbEntry->isHit) {
            tagMetadataStore.at(orbEntry->indexDC).dirtyLine =
            tagMetadataStore.at(orbEntry->indexDC).dirtyLine;
        } else {
            tagMetadataStore.at(orbEntry->indexDC).dirtyLine = false;
        }
    } else {
        tagMetadataStore.at(orbEntry->indexDC).dirtyLine = true;
    }
    
    tagMetadataStore.at(orbEntry->indexDC).farMemAddr =
                        orbEntry->owPkt->getAddr();

    if (orbEntry->owPkt->isRead()) {
        stats.readReqs++;
    }
    else {
        stats.writeReqs++;
    }
}

bool
DCacheCtrl::resumeConflictingReq(reqBufferEntry* orbEntry)
{
    bool conflictFound = false;

    if (orbEntry->owPkt->isWrite()) {
        isInWriteQueue.erase(orbEntry->owPkt->getAddr());
    }

    logStatsDcache(orbEntry);

    for (auto e = CRB.begin(); e != CRB.end(); ++e) {

        auto entry = *e;

        if (returnIndexDC(entry.second->getAddr(), entry.second->getSize())
            == orbEntry->indexDC) {

                conflictFound = true;

                Addr confAddr = entry.second->getAddr();

                ORB.erase(orbEntry->owPkt->getAddr());

                delete orbEntry->owPkt;

                delete orbEntry->dccPkt;

                delete orbEntry;

                handleRequestorPkt(entry.second);

                ORB.at(confAddr)->arrivalTick = entry.first;

                CRB.erase(e);

                checkConflictInCRB(ORB.at(confAddr));

                // if (pktLocMemRead[0].empty() && !stallRds && !rescheduleLocRead) {
                //     assert(!locMemReadEvent.scheduled());
                //     schedule(locMemReadEvent, std::max(nextReqTime, curTick()));
                // } else {
                //     assert(locMemReadEvent.scheduled() || stallRds || rescheduleLocRead);
                // }

                pktLocMemRead[0].push_back(ORB.at(confAddr)->dccPkt);

                if (!stallRds && !rescheduleLocRead && !locMemReadEvent.scheduled()) {
                    schedule(locMemReadEvent, std::max(nextReqTime, curTick()));
                }

                if (pktLocMemRead[0].size() > maxLocRdEvQ) {
                    maxLocRdEvQ = pktLocMemRead[0].size();
                    stats.maxLocRdEvQ = pktLocMemRead[0].size();
                }

                break;
        }

    }

    if (!conflictFound) {

        ORB.erase(orbEntry->owPkt->getAddr());

        delete orbEntry->owPkt;

        delete orbEntry->dccPkt;

        delete orbEntry;
    }

    return conflictFound;
}

void
DCacheCtrl::checkConflictInCRB(reqBufferEntry* orbEntry)
{
    for (auto e = CRB.begin(); e != CRB.end(); ++e) {

        auto entry = *e;

        if (returnIndexDC(entry.second->getAddr(),entry.second->getSize())
            == orbEntry->indexDC) {
                orbEntry->conflict = true;
                break;
        }
    }
}

void
DCacheCtrl::logStatsDcache(reqBufferEntry* orbEntry)
{

}

void
DCacheCtrl::handleDirtyCacheLine(reqBufferEntry* orbEntry)
{
    assert(orbEntry->dirtyLineAddr != -1);

    // create a new request packet
    PacketPtr wbPkt = getPacket(orbEntry->dirtyLineAddr,
                                orbEntry->owPkt->getSize(),
                                MemCmd::WriteReq);

    pktFarMemWrite.push_back(wbPkt);

    if (
        ((pktFarMemWrite.size() >= (orbMaxSize/2)) || (!pktFarMemWrite.empty() && pktFarMemRead.empty())) &&
        !waitingForRetryReqPort
       ) {
        sendFarRdReq = false;
        if (!farMemWriteEvent.scheduled()) {
            schedule(farMemWriteEvent, curTick());
        }
    }

    if (pktFarMemWrite.size() > maxFarWrEvQ) {
        maxFarWrEvQ = pktFarMemWrite.size();
        stats.maxFarWrEvQ = pktFarMemWrite.size();
    }

    stats.numWrBacks++;
}

PacketPtr
DCacheCtrl::getPacket(Addr addr, unsigned size, const MemCmd& cmd,
                   Request::FlagsType flags)
{
    // Create new request
    RequestPtr req = std::make_shared<Request>(addr, size, flags,
                                               0);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr)0) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, cmd);

    uint8_t* pkt_data = new uint8_t[req->getSize()];
    pkt->dataDynamic(pkt_data);

    if (cmd.isWrite()) {
        std::fill_n(pkt_data, req->getSize(), (uint8_t)0);
    }

    return pkt;
}



void
DCacheCtrl::dirtAdrGen()
{
    for (int i=0; i<tagMetadataStore.size(); i++ ) {
        tagMetadataStore.at(i).farMemAddr = i*64+dramCacheSize;
    }
}


/* reqBufferEntry*
DCacheCtrl::makeOrbEntry(reqBufferEntry* orbEntry, PacketPtr copyOwPkt)
{
    return new reqBufferEntry( orbEntry->validEntry,
                               orbEntry->arrivalTick,
                               orbEntry->tagDC,
                               orbEntry->indexDC,
                               copyOwPkt,
                               orbEntry->dccPkt,
                               orbEntry->state,
                               orbEntry->issued,
                               orbEntry->isHit,
                               orbEntry->conflict,
                               orbEntry->dirtyLineAddr,
                               orbEntry->handleDirtyLine,
                               orbEntry->locRdEntered,
                               orbEntry->locRdIssued,
                               orbEntry->locWrEntered,
                               orbEntry->locWrIssued,
                               orbEntry->farRdEntered,
                               orbEntry->farRdIssued,
                               orbEntry->farRdRecvd);
} */

} // namespace memory
} // namespace gem5
