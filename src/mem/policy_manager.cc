#include "mem/policy_manager.hh"

#include "base/trace.hh"
#include "debug/PolicyManager.hh"
#include "sim/system.hh"

namespace gem5
{

namespace memory
{

PolicyManager::PolicyManager(const PolicyManagerParams &p):
    ClockedObject(p),
    respPort(name() + ".resp_port", *this),
    locMemPort(name() + ".loc_req_port", *this),
    farMemPort(name() + ".far_req_port", *this),
    locMemCtrl(p.loc_mem_ctrl),
    farMemCtrl(p.far_mem_ctrl),
    locMemPolicy(p.loc_mem_policy),
    dramCacheSize(p.dram_cache_size),
    blockSize(p.block_size),
    addrSize(p.addr_size),
    orbMaxSize(p.orb_max_size), orbSize(0),
    crbMaxSize(p.crb_max_size), crbSize(0),
    alwaysHit(p.always_hit), alwaysDirty(p.always_dirty),
    frontendLatency(p.static_frontend_latency),
    backendLatency(p.static_backend_latency),
    retryLLC(false), retryLocMem(false), retryFarMem(false),
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
    farMemWriteEvent([this]{ processFarMemWriteEvent(); }, name()),
    polManStats(*this)
{
    panic_if(orbMaxSize<8, "ORB maximum size must be at least 8.\n");

    tagMetadataStore.resize(dramCacheSize/blockSize);

}

bool
PolicyManager::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world
    DPRINTF(PolicyManager, "recvTimingReq: request %s addr %#x size %d\n",
            pkt->cmdString(), pkt->getAddr(), pkt->getSize());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller\n");
    assert(pkt->getSize() != 0);

    // Calc avg gap between requests
    if (prevArrival != 0) {
        polManStats.totGap += curTick() - prevArrival;
    }
    prevArrival = curTick();

    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets

    const Addr base_addr = pkt->getAddr();
    Addr addr = base_addr;
    uint32_t burst_size = locMemCtrl->bytesPerBurst();
    unsigned size = std::min((addr | (burst_size - 1)) + 1,
                    base_addr + pkt->getSize()) - addr;

    // check merging for writes
    if (pkt->isWrite()) {

        polManStats.writePktSize[ceilLog2(size)]++;

        bool merged = isInWriteQueue.find(locMemCtrl->burstAlign(addr)) !=
            isInWriteQueue.end();

        if (merged) {

            polManStats.mergedWrBursts++;

            sendRespondToRequestor(pkt, frontendLatency);

            return true;
        }
    }

    // check forwarding for reads
    bool foundInORB = false;
    bool foundInCRB = false;
    bool foundInFarMemWrite = false;

    if (pkt->isRead()) {

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

                        polManStats.servicedByWrQ++;

                        polManStats.bytesReadWrQ += burst_size;

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

                        polManStats.servicedByWrQ++;

                        polManStats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }

            if (!foundInORB && !foundInCRB && !pktFarMemWrite.empty()) {
                for (const auto& e : pktFarMemWrite) {
                    // check if the read is subsumed in the write queue
                    // packet we are looking at
                    if (e.second->getAddr() <= addr &&
                        ((addr + size) <=
                        (e.second->getAddr() +
                         e.second->getSize()))) {

                        foundInFarMemWrite = true;

                        polManStats.servicedByWrQ++;

                        polManStats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }
        }

        if (foundInORB || foundInCRB || foundInFarMemWrite) {
            polManStats.readPktSize[ceilLog2(size)]++;

            sendRespondToRequestor(pkt, frontendLatency);

            return true;
        }
    }

    // process conflicting requests.
    // conflicts are checked only based on Index of DRAM cache
    if (checkConflictInDramCache(pkt)) {

        polManStats.totNumConf++;

        if (CRB.size()>=crbMaxSize) {

            polManStats.totNumConfBufFull++;

            retryLLC = true;

            if (pkt->isRead()) {
                polManStats.numRdRetry++;
            }
            else {
                polManStats.numWrRetry++;
            }
            return false;
        }

        CRB.push_back(std::make_pair(curTick(), pkt));

        if (pkt->isWrite()) {
            isInWriteQueue.insert(pkt->getAddr());
        }

        if (CRB.size() > maxConf) {
            maxConf = CRB.size();
            polManStats.maxNumConf = CRB.size();
        }

        return true;
    }
    // check if ORB or FMWB is full and set retry
    if (pktFarMemWrite.size()>= (orbMaxSize / 2)) {
        DPRINTF(PolicyManager, "FMWBfull: %lld\n", pkt->getAddr());
        retryFarMem = true;

        if (pkt->isRead()) {
            polManStats.numRdRetry++;
        }
        else {
            polManStats.numWrRetry++;
        }
        return false;
    }

    if (ORB.size() >= orbMaxSize) {
        DPRINTF(PolicyManager, "ORBfull: addr %lld\n", pkt->getAddr());
        polManStats.totNumORBFull++;
        retryLLC = true;

        if (pkt->isRead()) {
            polManStats.numRdRetry++;
        }
        else {
            polManStats.numWrRetry++;
        }
        return false;
    }

    // if none of the above cases happens,
    // add ir to the ORB
    handleRequestorPkt(pkt);

    if (pkt->isWrite()) {
        isInWriteQueue.insert(pkt->getAddr());
    }

    pktLocMemRead.push_back(pkt->getAddr());

    polManStats.avgLocRdQLenEnq = pktLocMemRead.size();

    setNextState(ORB.at(pkt->getAddr()));

    handleNextState(ORB.at(pkt->getAddr()));

    DPRINTF(PolicyManager, "Policy manager accepted packet %lld\n", pkt->getAddr());

    return true;
}

void
PolicyManager::processLocMemReadEvent()
{
    // sanity check for the chosen packet
    auto orbEntry = ORB.at(pktLocMemRead.front());
    assert(orbEntry->validEntry);
    assert(orbEntry->state == locMemRead);
    assert(!orbEntry->issued);

    PacketPtr rdTagPkt = getPacket(pktLocMemRead.front(),
                                   blockSize,
                                   MemCmd::ReadReq);

    if (locMemPort.sendTimingReq(rdTagPkt)) {
        DPRINTF(PolicyManager, "loc mem read is sent : %lld\n", rdTagPkt->getAddr());
        orbEntry->state = waitingFarMemReadResp;
        orbEntry->issued = true;
        pktLocMemRead.pop_front();
    } else {
        DPRINTF(PolicyManager, "loc mem read sending failed: %lld\n", rdTagPkt->getAddr());
        delete rdTagPkt;
    }

    if (!pktLocMemRead.empty() && !locMemReadEvent.scheduled()) {
        schedule(locMemReadEvent, curTick()+1000);
        return;
    }
}

void
PolicyManager::handleRequestorPkt(PacketPtr pkt)
{
    reqBufferEntry* orbEntry = new reqBufferEntry(
                                true, curTick(),
                                returnTagDC(pkt->getAddr(), pkt->getSize()),
                                returnIndexDC(pkt->getAddr(), pkt->getSize()),
                                pkt,
                                locMemPolicy, Init,
                                false, false, false,
                                -1, false,
                                curTick(), MaxTick, MaxTick,
                                MaxTick, MaxTick,
                                MaxTick, MaxTick, MaxTick, MaxTick
                            );

    ORB.emplace(pkt->getAddr(), orbEntry);

    // polManStats.avgORBLen = ORB.size();
    polManStats.avgORBLen = ORB.size();
    polManStats.avgLocRdQLenStrt = countLocRdInORB();
    polManStats.avgFarRdQLenStrt = countFarRdInORB();
    polManStats.avgLocWrQLenStrt = countLocWrInORB();
    polManStats.avgFarWrQLenStrt = countFarWr();

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

    
    Addr addr = pkt->getAddr();

    unsigned burst_size = locMemCtrl->bytesPerBurst();

    unsigned size = std::min((addr | (burst_size - 1)) + 1,
                             addr + pkt->getSize()) - addr;

    if (pkt->isRead()) {
        polManStats.readPktSize[ceilLog2(size)]++;
        polManStats.readReqs++;
    } else {
        polManStats.writePktSize[ceilLog2(size)]++;
        polManStats.writeReqs++;
    }

}

bool
PolicyManager::checkConflictInDramCache(PacketPtr pkt)
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

void
PolicyManager::checkHitOrMiss(reqBufferEntry* orbEntry)
{
    // access the tagMetadataStore data structure to
    // check if it's hit or miss

    // orbEntry->isHit =
    // tagMetadataStore.at(orbEntry->indexDC).validLine &&
    // (orbEntry->tagDC == tagMetadataStore.at(orbEntry->indexDC).tagDC);

    // if (!tagMetadataStore.at(orbEntry->indexDC).validLine &&
    //     !orbEntry->isHit) {
    //     polManStats.numColdMisses++;
    // } else if (tagMetadataStore.at(orbEntry->indexDC).validLine &&
    //          !orbEntry->isHit) {
    //     polManStats.numHotMisses++;
    // }

    // always hit
    // orbEntry->isHit = true;

    // always miss
    // orbEntry->isHit = false;

    orbEntry->isHit = alwaysHit;
}

bool
PolicyManager::checkDirty(Addr addr)
{
    // Addr index = returnIndexDC(addr, blockSize);
    // return (tagMetadataStore.at(index).validLine &&
    //         tagMetadataStore.at(index).dirtyLine);

    // always dirty
    // return true;

    // always clean
    // return false;

    return alwaysDirty;
}

PacketPtr
PolicyManager::getPacket(Addr addr, unsigned size, const MemCmd& cmd,
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

bool
PolicyManager::locMemRecvTimingResp(PacketPtr pkt)
{
    if (pkt->isRead()) {
        auto orbEntry = ORB.at(pkt->getAddr());
        setNextState(orbEntry);
        handleNextState(orbEntry);
        delete pkt;
    }
    else {
        assert(pkt->isWrite());

        delete pkt;

    }

    return true;
}

void
PolicyManager::setNextState(reqBufferEntry* orbEntry)
{
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->state == Init &&
        orbEntry->isHit) {
            orbEntry->state = locMemRead;
    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        orbEntry->isHit) {
            // done
            // do nothing
            return;
    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        !orbEntry->isHit) {
            orbEntry->state = farMemRead;
    }
}

void
PolicyManager::handleNextState(reqBufferEntry* orbEntry)
{
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->state == locMemRead) {

        assert(!pktLocMemRead.empty());
            
        if (!locMemReadEvent.scheduled()) {
            schedule(locMemReadEvent, curTick());
        }

        return;            
    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        orbEntry->isHit) {
            // DONE
            // send the respond to the requestor
            sendRespondToRequestor(orbEntry->owPkt, frontendLatency + backendLatency);

            // clear ORB
            resumeConflictingReq(orbEntry);

            return;            
    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        !orbEntry->isHit) {
            // do a read from far mem
    }
}

void
PolicyManager::sendRespondToRequestor(PacketPtr pkt, Tick static_latency)
{
    PacketPtr copyOwPkt = new Packet(pkt,
                                     false,
                                     pkt->isRead());
    
    Tick response_time = curTick() + static_latency + copyOwPkt->headerDelay +
                         copyOwPkt->payloadDelay;
    // Here we reset the timing of the packet before sending it out.
    copyOwPkt->headerDelay = copyOwPkt->payloadDelay = 0;

    // queue the packet in the response queue to be sent out after
    // the static latency has passed
    respPort.schedTimingResp(copyOwPkt, response_time);

}

bool
PolicyManager::resumeConflictingReq(reqBufferEntry* orbEntry)
{
    polManStats.totPktsServiceTime += ((orbEntry->locWrExit - orbEntry->arrivalTick)/1000);
    polManStats.totPktsORBTime += ((curTick() - orbEntry->arrivalTick)/1000);
    polManStats.totTimeFarRdtoSend += ((orbEntry->farRdIssued - orbEntry->farRdEntered)/1000);
    polManStats.totTimeFarRdtoRecv += ((orbEntry->farRdRecvd - orbEntry->farRdIssued)/1000);
    polManStats.totTimeInLocRead += ((orbEntry->locRdExit - orbEntry->locRdEntered)/1000);
    polManStats.totTimeInLocWrite += ((orbEntry->locWrExit - orbEntry->locWrEntered)/1000);
    polManStats.totTimeInFarRead += ((orbEntry->farRdExit - orbEntry->farRdEntered)/1000);

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

                delete orbEntry;

                handleRequestorPkt(entry.second);

                ORB.at(confAddr)->arrivalTick = entry.first;

                CRB.erase(e);

                checkConflictInCRB(ORB.at(confAddr));

                pktLocMemRead.push_back(confAddr);

                polManStats.avgLocRdQLenEnq = pktLocMemRead.size();

                setNextState(ORB.at(confAddr));

                handleNextState(ORB.at(confAddr));

                break;
        }

    }

    if (!conflictFound) {

        ORB.erase(orbEntry->owPkt->getAddr());

        delete orbEntry->owPkt;

        delete orbEntry;
    }

    return conflictFound;
}

void
PolicyManager::checkConflictInCRB(reqBufferEntry* orbEntry)
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

AddrRangeList
PolicyManager::getAddrRanges()
{
    // AddrRangeList ranges;
    AddrRangeList locRanges = locMemCtrl->getAddrRanges();
    AddrRangeList farRanges = farMemCtrl->getAddrRanges();

    locRanges.merge(farRanges);
    
    // for (auto range = locRanges.rbegin();
    //         range != locRanges.rend(); ++range) {
    //     ranges.push_back(range);
    // }

    // for (auto range = farRanges.rbegin();
    //         range != farRanges.rend(); ++range) {
    //     ranges.push_back(range);
    // }
    
    // return ranges;
    return locRanges;
}

} // namespace memory
} // namespace gem5