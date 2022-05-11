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
    //reqPort(name() + ".req_port", *this),
    farMemory(p.far_memory),
    dramCacheSize(p.dram_cache_size),
    blockSize(p.block_size),
    addrSize(p.addr_size),
    orbMaxSize(p.orb_max_size), orbSize(0),
    crbMaxSize(p.crb_max_size), crbSize(0),
    farMemWriteQueueMaxSize(p.far_mem_write_queue_max_size),
    retry(false),
    stallRds(false),
    doLocWrite(false),
    doFarWrite(false),
    locWrCounter(0),
    farWrCounter(0),
    maxConf(0),
    maxLocRdEvQ(0), maxLocRdRespEvQ(0), maxLocWrEvQ(0),
    maxFarRdEvQ(0), maxFarRdRespEvQ(0), maxFarWrEvQ(0),
    locMemReadEvent([this]{ processLocMemReadEvent(); }, name()),
    locMemReadRespEvent([this]{ processLocMemReadRespEvent(); }, name()),
    locMemWriteEvent([this]{ processLocMemWriteEvent(); }, name()),
    farMemReadEvent([this]{ processFarMemReadEvent(); }, name()),
    farMemReadRespEvent([this]{ processFarMemReadRespEvent(); }, name()),
    farMemWriteEvent([this]{ processFarMemWriteEvent(); }, name()),
    overallWriteEvent([this]{ processOverallWriteEvent(); }, name())
{
    DPRINTF(DCacheCtrl, "Setting up controller\n");

    fatal_if(!dram, "DRAM cache controller must have a DRAM interface.\n");

    // hook up interfaces to the controller
    dram->setCtrl(this, commandWindow);

    tagMetadataStore.resize(dramCacheSize/blockSize);
    pktLocMemRead.resize(1);
    pktLocMemWrite.resize(1);
    pktFarMemRead.resize(1);
    pktFarMemWrite.resize(1);

    // perform a basic check of the write thresholds
    if (p.write_low_thresh_perc >= p.write_high_thresh_perc)
        fatal("Write buffer low threshold %d must be smaller than the "
              "high threshold %d\n", p.write_low_thresh_perc,
              p.write_high_thresh_perc);

    if (orbMaxSize>512) {
        locWrDrainPerc = 0.25;
    }
    else {
        locWrDrainPerc = 0.5;
    }

    if (orbMaxSize == 1) {
        writeHighThreshold = 1;
    }

    minLocWrPerSwitch = 0.7 * minWritesPerSwitch;
    minFarWrPerSwitch = minWritesPerSwitch - minLocWrPerSwitch;



}

void
DCacheCtrl::init()
{
   if (!port.isConnected()) {
        fatal("DCacheCtrl %s is unconnected!\n", name());
    } /*else if (!reqPort.isConnected()) {
        fatal("DCacheCtrl %s is unconnected!\n", name());
    }*/ else {
        port.sendRangeChange();
    }
}


bool
DCacheCtrl::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world
    DPRINTF(DCacheCtrl, "recvTimingReq: request %s addr %lld size %d\n",
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

    // Validate that pkt's address maps to the dram
    assert(dram && dram->getAddrRange().contains(pkt->getAddr()));


    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets

    Addr addr = pkt->getAddr();

    unsigned burst_size = dram->bytesPerBurst();

    unsigned size = std::min((addr | (burst_size - 1)) + 1,
                             addr + pkt->getSize()) - addr;

    // check merging for writes
    if (pkt->isWrite()) {
        stats.writePktSize[ceilLog2(size)]++;
        stats.writeBursts++;
        stats.requestorWriteAccesses[pkt->requestorId()]++;

        assert(pkt->getSize() != 0);

        bool merged = isInWriteQueue.find(pkt->getAddr()) !=
            isInWriteQueue.end();

        if (merged) {

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

            if (!foundInORB && !foundInCRB && !pktFarMemWrite[0].empty()) {
                for (const auto& e : pktFarMemWrite[0]) {
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

            accessAndRespond(pkt, frontendLatency, farMemory);

            return true;
        }
    }

    // process conflicting requests
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

        return true;
    }

    // check if ORB is full and set retry
    if (ORB.size() >= orbMaxSize) {

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

    if (pktLocMemRead[0].empty() && !stallRds) {

        assert(!locMemReadEvent.scheduled());

        schedule(locMemReadEvent, std::max(nextReqTime, curTick()));

    } else {
        assert(locMemReadEvent.scheduled() || stallRds);
    }

    pktLocMemRead[0].push_back(ORB.at(pkt->getAddr())->dccPkt);

    ORB.at(pkt->getAddr())->locRdEntered = curTick();

    if (pktLocMemRead[0].size() > maxLocRdEvQ) {
        maxLocRdEvQ = pktLocMemRead[0].size();
        stats.maxLocRdEvQ = pktLocMemRead[0].size();
    }

    return true;
}

void
DCacheCtrl::processLocMemReadEvent()
{
    if (stallRds) {
        return;
    }

    assert(!pktLocMemRead[0].empty());

    MemPacketQueue::iterator to_read;

    bool read_found = false;

    bool switched_cmd_type = (busState == DCacheCtrl::WRITE);

    for (auto queue = pktLocMemRead.rbegin();
                 queue != pktLocMemRead.rend(); ++queue) {
        to_read = SimpleMemCtrl::chooseNext((*queue), switched_cmd_type ?
                                minWriteToReadDataGap() : 0);
        if (to_read != queue->end()) {
            // candidate read found
            read_found = true;
            break;
        }
    }

    if (!read_found) {
        schedule(locMemReadEvent,
                 std::max(nextReqTime, curTick() + dram->getTBurst()));
        return;
    }

    reqBufferEntry* orbEntry = ORB.at((*to_read)->getAddr());

    // sanity check for the chosen packet
    assert(orbEntry->validEntry);
    assert(orbEntry->dccPkt->isDram());
    assert(orbEntry->dccPkt->isRead());
    assert(orbEntry->state == locMemRead);
    assert(!orbEntry->issued);

    // if (orbEntry->handleDirtyLine) {
    //     if (pktFarMemWrite[0].size() >= nvm->getMaxPendingWrites()) {
    //         stallRds = true;
    //         drainNvmWrite = true;
    //         if (!overallWriteEvent.scheduled()) {
    //                 schedule(overallWriteEvent,
    //                          std::max(nextReqTime, curTick()));
    //         }
    //         return;
    //     }
    //     if (numDirtyLinesInDrRdRespQ >= nvm->getMaxPendingWrites()) {
    //         Tick schedTick = earliestDirtyLineInDrRdResp();
    //         assert(schedTick != MaxTick);
    //         schedule(dramReadEvent, std::max(nextReqTime, schedTick+1));
    //         return;
    //     }
    //     if (nvm->writeRespQueueFull()) {
    //         assert(!dramReadEvent.scheduled());
    //         schedule(dramReadEvent, std::max(nextReqTime,
    //                                 nvm->writeRespQueueFront()+2));
    //         return;
    //     }
    // }

    busState = DCacheCtrl::READ;

    if (switched_cmd_type) {
        stats.wrToRdTurnAround++;
    }

    assert(packetReady(orbEntry->dccPkt, dram));

    //Tick cmd_at = SimpleMemCtrl::doBurstAccess(orbEntry->dccPkt);
    SimpleMemCtrl::doBurstAccess(orbEntry->dccPkt);

    // sanity check
    assert(orbEntry->dccPkt->size <= dram->bytesPerBurst());
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

    // if (orbEntry->handleDirtyLine) {
    //     numDirtyLinesInDrRdRespQ++;
    // }

    // keep the state as it is, no transition
    orbEntry->state = locMemRead;
    // mark the entry as issued (while in locMemRead)
    orbEntry->issued = true;
    // record the tick it was issued
    orbEntry->locRdIssued = curTick();

    pktLocMemRead[0].erase(to_read);

    if (!pktLocMemRead[0].empty()) {
        assert(!locMemReadEvent.scheduled());
        schedule(locMemReadEvent, std::max(nextReqTime, curTick()));
    }

    unsigned rdsNum = pktLocMemRead[0].size() +
                        pktFarMemRead[0].size();
    unsigned wrsNum = pktLocMemWrite[0].size() +
                        pktFarMemWrite[0].size();

    if ((rdsNum == 0 && wrsNum != 0) ||
        (wrsNum >= writeHighThreshold)) {

        stallRds = true;

        if (!overallWriteEvent.scheduled()) {
            schedule(overallWriteEvent, std::max(nextReqTime, curTick()));
        }
    }
}

void
DCacheCtrl::processLocMemReadRespEvent()
{
    assert(!addrLocRdRespReady.empty());

    reqBufferEntry* orbEntry = ORB.at(addrLocRdRespReady.front());

    // A series of sanity check
    assert(orbEntry->validEntry);
    assert(orbEntry->dccPkt->isDram());
    assert(orbEntry->dccPkt->isRead());
    assert(orbEntry->state == locMemRead);
    assert(orbEntry->issued);
    assert(orbEntry->dccPkt->readyTime == curTick());

    orbEntry->issued = false;

    // if (orbEntry->handleDirtyLine) {
    //     handleDirtyCacheLine(orbEntry);
    // }

    // A flag which is used for retrying read requests
    // in case one slot in ORB becomes available here
    // (happens only for read hits)
    bool canRetry = false;

    dram->respondEvent(orbEntry->dccPkt->rank);

    // Read Hit
    if (orbEntry->owPkt->isRead() &&
        orbEntry->dccPkt->isDram() &&
        orbEntry->isHit) {
            PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
                                             false,
                                             orbEntry->owPkt->isRead());

            accessAndRespond(orbEntry->owPkt,
                             frontendLatency + backendLatency,
                             farMemory);

            ORB.at(copyOwPkt->getAddr()) =  new reqBufferEntry(
                                orbEntry->validEntry,
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
                                                  false, true);
            orbEntry->dccPkt->entryTime = orbEntry->arrivalTick;

            // pass the second argument "false" to
            // indicate a write access to DRAM
            dram->setupRank(orbEntry->dccPkt->rank, false);

            //** transition to locMemWrite
            orbEntry->state = locMemWrite;
            orbEntry->issued = false;
            orbEntry->locWrEntered = curTick();

            pktLocMemWrite[0].push_back(orbEntry->dccPkt);

            // if (pktLocMemWrite[0].size() >= (orbMaxSize*locWrDrainPerc)) {
            //     stallRds = true;
            //     doLocWrite = true;
            //     if (!overallWriteEvent.scheduled()) {
            //         schedule(overallWriteEvent,
            //                  std::max(nextReqTime, curTick()));
            //     }
            // }

            unsigned rdsNum = pktLocMemRead[0].size() +
                                pktFarMemRead[0].size();
            unsigned wrsNum = pktLocMemWrite[0].size() +
                                pktFarMemWrite[0].size();

            if ((pktLocMemWrite[0].size() >= (orbMaxSize*locWrDrainPerc)) ||
                (rdsNum == 0 && wrsNum != 0) ||
                (wrsNum >= writeHighThreshold)) {
                // stall reads, switch to writes
                stallRds = true;

                if (pktLocMemWrite[0].size() >= (orbMaxSize*locWrDrainPerc)) {
                    doLocWrite = true;
                }

                if (!overallWriteEvent.scheduled()) {
                    schedule(overallWriteEvent,
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
        // initiate a read from far memory
        // delete the current dcc pkt which is for read from local memory
        delete orbEntry->dccPkt;

        // creating an nvm read dcc-pkt
        orbEntry->dccPkt = farMemory->decodePacket(orbEntry->owPkt,
                                             orbEntry->owPkt->getAddr(),
                                             orbEntry->owPkt->getSize(),
                                             true, farMemory->isDramIntr);
        orbEntry->dccPkt->entryTime = orbEntry->arrivalTick;
        orbEntry->dccPkt->readyTime = MaxTick;
        //** transition to farMemRead
        orbEntry->state = farMemRead;
        orbEntry->issued = false;
        orbEntry->farRdEntered = curTick();     

        if (pktFarMemRead[0].empty() && !stallRds) {
            assert(!farMemReadEvent.scheduled());
            schedule(farMemReadEvent, std::max(nextReqTime, curTick()));
        }
        else {
            assert(farMemReadEvent.scheduled() || stallRds);
        }

        pktFarMemRead[0].push_back(orbEntry->dccPkt);

        if (pktFarMemRead[0].size() > maxFarRdEvQ) {
            maxFarRdEvQ = pktFarMemRead[0].size();
            stats.maxFarRdEvQ = pktFarMemRead[0].size();
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

        unsigned rdsNum = pktLocMemRead[0].size() +
                            pktFarMemRead[0].size();
        unsigned wrsNum = pktLocMemWrite[0].size() +
                            pktFarMemWrite[0].size();

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
            std::cout << "Dummy: " << requestEventScheduled() << "\n";
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

}
void
DCacheCtrl::processFarMemReadEvent()
{

}
void
DCacheCtrl::processFarMemReadRespEvent()
{

}
void
DCacheCtrl::processFarMemWriteEvent()
{

}
void
DCacheCtrl::processOverallWriteEvent()
{

}

void
DCacheCtrl::recvReqRetry()
{

}

bool
DCacheCtrl::recvTimingResp(PacketPtr pkt)
{
    return false;
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
    return bits(request_addr, ceilLog2(size) +
            ceilLog2(dramCacheSize/blockSize)-1, ceilLog2(size));
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
    /*orbEntry->isHit =
    tagMetadataStore.at(orbEntry->indexDC).validLine &&
    (orbEntry->tagDC == tagMetadataStore.at(orbEntry->indexDC).tagDC);

    if (!tagMetadataStore.at(orbEntry->indexDC).validLine &&
        !orbEntry->isHit) {
        stats.numColdMisses++;
    }
    else if (tagMetadataStore.at(orbEntry->indexDC).validLine &&
             !orbEntry->isHit) {
        stats.numHotMisses++;
    }*/

    // always hit
    orbEntry->isHit = true;

    // always miss
    // orbEntry->isHit = false;
}

bool
DCacheCtrl::checkDirty(Addr addr)
{
    /*Addr index = returnIndexDC(addr, blockSize);
    return (tagMetadataStore.at(index).validLine &&
            tagMetadataStore.at(index).dirtyLine);*/


    // always dirty
    //return true;

    // always clean
    return false;
}

void
DCacheCtrl::handleRequestorPkt(PacketPtr pkt)
{
    // Set is_read and is_dram to
    // "true", to do initial DRAM Read
    MemPacket* dcc_pkt = dram->decodePacket(pkt,
                                            pkt->getAddr(),
                                            pkt->getSize(),
                                            true,
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

        ORB.at(copyOwPkt->getAddr()) =  new reqBufferEntry(
                                orbEntry->validEntry,
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
                                orbEntry->farRdRecvd
                                );
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
        }
        else {
            tagMetadataStore.at(orbEntry->indexDC).dirtyLine = false;
        }
    }
    else {
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

                if (pktLocMemRead[0].empty() && !stallRds) {
                    assert(!locMemReadEvent.scheduled());
                    schedule(locMemReadEvent, std::max(nextReqTime, curTick()));
                } else {
                    assert(locMemReadEvent.scheduled() || stallRds);
                }

                pktLocMemRead[0].push_back(ORB.at(confAddr)->dccPkt);

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

} // namespace memory
} // namespace gem5
