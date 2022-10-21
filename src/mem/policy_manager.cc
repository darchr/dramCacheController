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
    port(name() + ".port", *this),
    locReqPort(name() + ".loc_req_port", *this),
    farReqPort(name() + ".far_req_port", *this),
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
    retryLLC(false), retryLLCFarMemWr(false),
    retryLocMemRead(false), retryFarMemRead(false),
    retryLocMemWrite(false), retryFarMemWrite(false),
    // stallRds(false), sendFarRdReq(true),
    // waitingForRetryReqPort(false),
    // rescheduleLocRead(false),
    // rescheduleLocWrite(false),
    // locWrCounter(0), farWrCounter(0),
    maxConf(0),
    maxLocRdEvQ(0), maxLocRdRespEvQ(0), maxLocWrEvQ(0),
    maxFarRdEvQ(0), maxFarRdRespEvQ(0), maxFarWrEvQ(0),
    locMemReadEvent([this]{ processLocMemReadEvent(); }, name()),
    // locMemReadRespEvent([this]{ processLocMemReadRespEvent(); }, name()),
    locMemWriteEvent([this]{ processLocMemWriteEvent(); }, name()),
    farMemReadEvent([this]{ processFarMemReadEvent(); }, name()),
    // farMemReadRespEvent([this]{ processFarMemReadRespEvent(); }, name()),
    farMemWriteEvent([this]{ processFarMemWriteEvent(); }, name()),
    polManStats(*this)
{
    panic_if(orbMaxSize<8, "ORB maximum size must be at least 8.\n");

    tagMetadataStore.resize(dramCacheSize/blockSize);

}

void
PolicyManager::init()
{
   if (!port.isConnected()) {
        fatal("Policy Manager %s is unconnected!\n", name());
    } else if (!locReqPort.isConnected()) {
        fatal("Policy Manager %s is unconnected!\n", name());
    } else if (!farReqPort.isConnected()) {
        fatal("Policy Manager %s is unconnected!\n", name());
    } else {
        port.sendRangeChange();
        //reqPort.recvRangeChange();
    }
}

bool
PolicyManager::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world
    DPRINTF(PolicyManager, "recvTimingReq: request %s addr %d size %d\n",
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

            // farMemCtrl->accessInterface(pkt);

            // sendRespondToRequestor(pkt, frontendLatency);

            // return true;
        }
    }

    // check forwarding for reads
    bool foundInORB = false;
    bool foundInCRB = false;
    //bool foundInFarMemWrite = false;

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

                        // foundInFarMemWrite = true;

                        polManStats.servicedByWrQ++;

                        polManStats.bytesReadWrQ += burst_size;

                        break;
                    }
                }
            }
        }

        // if (foundInORB || foundInCRB || foundInFarMemWrite) {
        //     polManStats.readPktSize[ceilLog2(size)]++;

        //     farMemCtrl->accessInterface(pkt);

        //     sendRespondToRequestor(pkt, frontendLatency);

        //     return true;
        // }
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
    if (pktFarMemWrite.size() >= (orbMaxSize / 2)) {
        DPRINTF(PolicyManager, "FMWBfull: %lld\n", pkt->getAddr());
        retryLLCFarMemWr = true;

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

    PacketPtr rdLocMemPkt = getPacket(pktLocMemRead.front(),
                                   blockSize,
                                   MemCmd::ReadReq);

    if (locReqPort.sendTimingReq(rdLocMemPkt)) {
        DPRINTF(PolicyManager, "loc mem read is sent : %lld\n", rdLocMemPkt->getAddr());
        orbEntry->state = waitingLocMemReadResp;
        orbEntry->issued = true;
        orbEntry->locRdIssued = curTick();
        pktLocMemRead.pop_front();
    } else {
        DPRINTF(PolicyManager, "loc mem read sending failed: %lld\n", rdLocMemPkt->getAddr());
        retryLocMemRead = true;
        delete rdLocMemPkt;
    }

    if (!pktLocMemRead.empty() && !locMemReadEvent.scheduled() && !retryLocMemRead) {
        schedule(locMemReadEvent, curTick()+1000);
    }
}

void
PolicyManager::processLocMemWriteEvent()
{
    // sanity check for the chosen packet
    auto orbEntry = ORB.at(pktLocMemWrite.front());
    assert(orbEntry->validEntry);
    assert(orbEntry->state == locMemWrite);
    assert(!orbEntry->issued);

    PacketPtr wrLocMemPkt = getPacket(pktLocMemWrite.front(),
                                   blockSize,
                                   MemCmd::WriteReq);

    if (locReqPort.sendTimingReq(wrLocMemPkt)) {
        DPRINTF(PolicyManager, "loc mem write is sent : %lld\n", wrLocMemPkt->getAddr());
        orbEntry->state = waitingLocMemWriteResp;
        orbEntry->issued = true;
        orbEntry->locWrIssued = curTick();
        pktLocMemWrite.pop_front();
    } else {
        DPRINTF(PolicyManager, "loc mem write sending failed: %lld\n", wrLocMemPkt->getAddr());
        retryLocMemWrite = true;
        delete wrLocMemPkt;
    }

    if (!pktLocMemWrite.empty() && !locMemWriteEvent.scheduled() && !retryLocMemWrite) {
        schedule(locMemWriteEvent, curTick()+1000);
    }
}

void
PolicyManager::processFarMemReadEvent()
{
    // sanity check for the chosen packet
    auto orbEntry = ORB.at(pktFarMemRead.front());
    assert(orbEntry->validEntry);
    assert(orbEntry->state == farMemRead);
    assert(!orbEntry->issued);

    PacketPtr rdFarMemPkt = getPacket(pktFarMemRead.front(),
                                      blockSize,
                                      MemCmd::ReadReq);

    if (farReqPort.sendTimingReq(rdFarMemPkt)) {
        DPRINTF(PolicyManager, "far mem read is sent : %lld\n", rdFarMemPkt->getAddr());
        orbEntry->state = waitingFarMemReadResp;
        orbEntry->issued = true;
        orbEntry->farRdIssued = curTick();
        pktFarMemRead.pop_front();
    } else {
        DPRINTF(PolicyManager, "far mem read sending failed: %lld\n", rdFarMemPkt->getAddr());
        retryFarMemRead = true;
        delete rdFarMemPkt;
    }

    if (!pktFarMemRead.empty() && !farMemReadEvent.scheduled() && !retryFarMemRead) {
        schedule(farMemReadEvent, curTick()+1000);
    }
}

void
PolicyManager::processFarMemWriteEvent()
{
    PacketPtr wrFarMemPkt = getPacket(pktFarMemWrite.front().second->getAddr(),
                                      blockSize,
                                      MemCmd::WriteReq);
    DPRINTF(PolicyManager, "FarMemWriteEvent: request %s addr %#x\n",
            wrFarMemPkt->cmdString(), wrFarMemPkt->getAddr());

    if (farReqPort.sendTimingReq(wrFarMemPkt)) {
        DPRINTF(PolicyManager, "far mem write is sent : %lld\n", wrFarMemPkt->getAddr());
        pktFarMemWrite.pop_front();
    } else {
        DPRINTF(PolicyManager, "far mem write sending failed: %lld\n", wrFarMemPkt->getAddr());
        retryFarMemWrite = true;
        delete wrFarMemPkt;
    }

    if (!pktFarMemWrite.empty() && !farMemWriteEvent.scheduled() && !retryFarMemWrite) {
        schedule(farMemWriteEvent, curTick()+1000);
    }

    if (retryLLCFarMemWr && pktFarMemWrite.size()< (orbMaxSize / 2)) {
        retryLLCFarMemWr = false;
        port.sendRetryReq();
    }
}

bool
PolicyManager::locMemRecvTimingResp(PacketPtr pkt)
{
    auto orbEntry = ORB.at(pkt->getAddr());

    if (pkt->isRead()) {
        assert(orbEntry->state == waitingLocMemReadResp);
        orbEntry->locRdExit = curTick();
    }
    else {
        assert(pkt->isWrite());
        assert(orbEntry->state == waitingLocMemWriteResp);
        orbEntry->locWrExit = curTick();
    }

    setNextState(orbEntry);

    handleNextState(orbEntry);

    delete pkt;

    return true;
}

bool
PolicyManager::farMemRecvTimingResp(PacketPtr pkt)
{
    if (pkt->isRead()) {
        auto orbEntry = ORB.at(pkt->getAddr());
        assert(orbEntry->state == waitingFarMemReadResp);
        orbEntry->farRdExit = curTick();
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
PolicyManager::locMemRecvReqRetry()
{
    // assert(retryLocMemRead || retryLocMemWrite);
    bool schedRd = false;
    bool schedWr = false;
    if (retryLocMemRead) {
        if (!locMemReadEvent.scheduled() && !pktLocMemRead.empty()) {
            schedule(locMemReadEvent, curTick());
        }
        retryLocMemRead = false;
        schedRd = true;
    }
    if (retryLocMemWrite) {
        if (!locMemWriteEvent.scheduled() && !pktLocMemWrite.empty()) {
            schedule(locMemWriteEvent, curTick());
        }
        retryLocMemWrite = false;
        schedWr = true;
    }
    if (!schedRd && !schedWr) {
            // panic("Wrong local mem retry event happend.\n");

            // TODO: there are cases where none of retryLocMemRead and retryLocMemWrite
            // are true, yet locMemRecvReqRetry() is called. I should fix this later.
            if (!locMemReadEvent.scheduled() && !pktLocMemRead.empty()) {
                schedule(locMemReadEvent, curTick());
            }
            if (!locMemWriteEvent.scheduled() && !pktLocMemWrite.empty()) {
                schedule(locMemWriteEvent, curTick());
            }
    }
}

void
PolicyManager::farMemRecvReqRetry()
{
    assert(retryFarMemRead || retryFarMemWrite);
    
    if (retryFarMemRead) {
        if (!farMemReadEvent.scheduled() && !pktFarMemRead.empty()) {
            schedule(farMemReadEvent, curTick());
        }
        retryFarMemRead = false;
    } else if (retryFarMemWrite) {
        if (!farMemWriteEvent.scheduled() && !pktFarMemWrite.empty()) {
            schedule(farMemWriteEvent, curTick());
        }
        retryFarMemWrite = false;
    } else {
        panic("Wrong far mem retry event happend.\n");
    }
}


void
PolicyManager::setNextState(reqBufferEntry* orbEntry)
{
    orbEntry->issued = false;

    // start --> read tag
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->state == start) {
            orbEntry->state = locMemRead;
            orbEntry->locRdEntered = curTick();
            return;
    }

    // tag ready && read && hit --> DONE
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        orbEntry->isHit) {
            // done
            // do nothing
            return;
    }

    // tag ready && write --> loc write
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isWrite() &&
        orbEntry->state == waitingLocMemReadResp) {
            // write it to the DRAM cache
            orbEntry->state = locMemWrite;
            orbEntry->locWrEntered = curTick();
            return;
    }

    // loc read resp ready && read && miss --> far read
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingLocMemReadResp &&
        !orbEntry->isHit) {
            orbEntry->state = farMemRead;
            orbEntry->farRdEntered = curTick();
            return;
    }

    // far read resp ready && read && miss --> loc write
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->owPkt->isRead() &&
        orbEntry->state == waitingFarMemReadResp &&
        !orbEntry->isHit) {
            sendRespondToRequestor(orbEntry->owPkt, frontendLatency + backendLatency);
            if (orbEntry->handleDirtyLine) {
                handleDirtyCacheLine(orbEntry);
            }
            orbEntry->state = locMemWrite;
            orbEntry->locWrEntered = curTick();
            return;
    }

    // loc write received
    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        // orbEntry->owPkt->isRead() &&
        // !orbEntry->isHit &&
        orbEntry->state == waitingLocMemWriteResp) {
            // done
            // do nothing
            return;
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
        orbEntry->state == farMemRead) {

            assert(!orbEntry->isHit);

            // do a read from far mem
            pktFarMemRead.push_back(orbEntry->owPkt->getAddr());

            if (!farMemReadEvent.scheduled()) {
                schedule(farMemReadEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        orbEntry->state == locMemWrite) {

            if (orbEntry->owPkt->isRead()) {
                assert(!orbEntry->isHit);
            }

            // do a read from far mem
            pktLocMemWrite.push_back(orbEntry->owPkt->getAddr());

            if (!locMemWriteEvent.scheduled()) {
                schedule(locMemWriteEvent, curTick());
            }
            return;

    }

    if (orbEntry->pol == enums::CascadeLakeNoPartWrs &&
        // orbEntry->owPkt->isRead() &&
        // !orbEntry->isHit &&
        orbEntry->state == waitingLocMemWriteResp) {
            // DONE
            // clear ORB
            resumeConflictingReq(orbEntry);

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
                                locMemPolicy, start,
                                false, false, false,
                                -1, false,
                                MaxTick, MaxTick, MaxTick,
                                MaxTick, MaxTick, MaxTick,
                                MaxTick, MaxTick, MaxTick
                            );

    ORB.emplace(pkt->getAddr(), orbEntry);

    if (pkt->isWrite()) {
        sendRespondToRequestor(pkt, frontendLatency);
    }

    // polManStats.avgORBLen = ORB.size();
    polManStats.avgORBLen = ORB.size();
    // polManStats.avgLocRdQLenStrt = countLocRdInORB();
    // polManStats.avgFarRdQLenStrt = countFarRdInORB();
    // polManStats.avgLocWrQLenStrt = countLocWrInORB();
    // polManStats.avgFarWrQLenStrt = countFarWr();

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

    bool currValid = tagMetadataStore.at(orbEntry->indexDC).validLine;
    bool currDirty = tagMetadataStore.at(orbEntry->indexDC).dirtyLine;

    orbEntry->isHit = currValid && (orbEntry->tagDC == tagMetadataStore.at(orbEntry->indexDC).tagDC);

    if (orbEntry->isHit) {

        polManStats.numTotHits++;

        if (orbEntry->owPkt->isRead()) {
            polManStats.numRdHit++;
            if (currDirty) {
                polManStats.numRdHitDirty++;
            } else {
                polManStats.numRdHitClean++;
            }
        } else {
            polManStats.numWrHit++;
            if (currDirty) {
                polManStats.numWrHitDirty++;
            } else {
                polManStats.numWrHitClean++;
            }
        }

    } else {

        polManStats.numTotMisses++;

        if (currValid) {
            polManStats.numHotMisses++;
        } else {
            polManStats.numColdMisses++;
        }

        if (orbEntry->owPkt->isRead()) {
            if (currDirty && currValid) {
                polManStats.numRdMissDirty++;
            } else {
                polManStats.numRdMissClean++;
            }
        } else {
            if (currDirty && currValid) {
                polManStats.numWrMissDirty++;
            } else {
                polManStats.numWrMissClean++;
            }
            
        }
    }

    // orbEntry->isHit = alwaysHit;
}

bool
PolicyManager::checkDirty(Addr addr)
{
    Addr index = returnIndexDC(addr, blockSize);
    return (tagMetadataStore.at(index).validLine &&
            tagMetadataStore.at(index).dirtyLine);

    // return alwaysDirty;
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

void
PolicyManager::sendRespondToRequestor(PacketPtr pkt, Tick static_latency)
{
    PacketPtr copyOwPkt = new Packet(pkt,
                                     false,
                                     pkt->isRead());
    copyOwPkt->makeResponse();
    
    Tick response_time = curTick() + static_latency + copyOwPkt->headerDelay + copyOwPkt->payloadDelay;
    // Here we reset the timing of the packet before sending it out.
    copyOwPkt->headerDelay = copyOwPkt->payloadDelay = 0;

    // queue the packet in the response queue to be sent out after
    // the static latency has passed
    port.schedTimingResp(copyOwPkt, response_time);

}

bool
PolicyManager::resumeConflictingReq(reqBufferEntry* orbEntry)
{
    // if ( locMemPolicy == enums::CascadeLakeNoPartWrs &&
    //      !(orbEntry->owPkt->isRead() && !orbEntry->isHit)) {
    //     PacketPtr copyOwPkt = new Packet(orbEntry->owPkt,
    //                                      false,
    //                                      orbEntry->owPkt->isRead());
    //     farMemCtrl->callMemIntfAccess(copyOwPkt);
    // }
    

    bool conflictFound = false;

    if (orbEntry->owPkt->isWrite()) {
        isInWriteQueue.erase(orbEntry->owPkt->getAddr());
    }

    logStatsPolMan(orbEntry);

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

        if (retryLLC) {
            retryLLC = false;
            port.sendRetryReq();
        }
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

Addr
PolicyManager::returnIndexDC(Addr request_addr, unsigned size)
{
    int index_bits = ceilLog2(dramCacheSize/blockSize);
    int block_bits = ceilLog2(size);
    return bits(request_addr, block_bits + index_bits-1, block_bits);
}

Addr
PolicyManager::returnTagDC(Addr request_addr, unsigned size)
{
    int index_bits = ceilLog2(dramCacheSize/blockSize);
    int block_bits = ceilLog2(size);
    return bits(request_addr, addrSize-1, (index_bits+block_bits));
}

void
PolicyManager::handleDirtyCacheLine(reqBufferEntry* orbEntry)
{
    assert(orbEntry->dirtyLineAddr != -1);

    // create a new request packet
    PacketPtr wbPkt = getPacket(orbEntry->dirtyLineAddr,
                                orbEntry->owPkt->getSize(),
                                MemCmd::WriteReq);

    pktFarMemWrite.push_back(std::make_pair(curTick(), wbPkt));

    polManStats.avgFarWrQLenEnq = pktFarMemWrite.size();

    if (
        ((pktFarMemWrite.size() >= (orbMaxSize/2)) || (!pktFarMemWrite.empty() && pktFarMemRead.empty()))
        // && !waitingForRetryReqPort
       ) {
        // sendFarRdReq = false;
        if (!farMemWriteEvent.scheduled()) {
            schedule(farMemWriteEvent, curTick());
        }
    }

    if (pktFarMemWrite.size() > maxFarWrEvQ) {
        maxFarWrEvQ = pktFarMemWrite.size();
        polManStats.maxFarWrEvQ = pktFarMemWrite.size();
    }

    polManStats.numWrBacks++;
}

void
PolicyManager::logStatsPolMan(reqBufferEntry* orbEntry)
{
    polManStats.totPktsServiceTime += ((curTick() - orbEntry->arrivalTick)/1000);
    polManStats.totPktsORBTime += ((curTick() - orbEntry->locRdEntered)/1000);
    polManStats.totTimeFarRdtoSend += ((orbEntry->farRdIssued - orbEntry->farRdEntered)/1000);
    polManStats.totTimeFarRdtoRecv += ((orbEntry->locRdExit - orbEntry->farRdIssued)/1000);
    polManStats.totTimeInLocRead += ((orbEntry->locRdExit - orbEntry->locRdEntered)/1000);
    polManStats.totTimeInLocWrite += ((orbEntry->locWrExit - orbEntry->locWrEntered)/1000);
    polManStats.totTimeInFarRead += ((orbEntry->farRdExit - orbEntry->farRdEntered)/1000);

}


PolicyManager::PolicyManagerStats::PolicyManagerStats(PolicyManager &_polMan)
    : statistics::Group(&_polMan),
    polMan(_polMan),

/////
    ADD_STAT(readReqs, statistics::units::Count::get(),
             "Number of read requests accepted"),
    ADD_STAT(writeReqs, statistics::units::Count::get(),
             "Number of write requests accepted"),

    ADD_STAT(readBursts, statistics::units::Count::get(),
             "Number of controller read bursts, including those serviced by "
             "the write queue"),
    ADD_STAT(writeBursts, statistics::units::Count::get(),
             "Number of controller write bursts, including those merged in "
             "the write queue"),
    ADD_STAT(servicedByWrQ, statistics::units::Count::get(),
             "Number of controller read bursts serviced by the write queue"),
    ADD_STAT(mergedWrBursts, statistics::units::Count::get(),
             "Number of controller write bursts merged with an existing one"),

    ADD_STAT(neitherReadNorWriteReqs, statistics::units::Count::get(),
             "Number of requests that are neither read nor write"),

    ADD_STAT(avgRdQLen, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average read queue length when enqueuing"),
    ADD_STAT(avgWrQLen, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average write queue length when enqueuing"),

    ADD_STAT(numRdRetry, statistics::units::Count::get(),
             "Number of times read queue was full causing retry"),
    ADD_STAT(numWrRetry, statistics::units::Count::get(),
             "Number of times write queue was full causing retry"),

    ADD_STAT(readPktSize, statistics::units::Count::get(),
             "Read request sizes (log2)"),
    ADD_STAT(writePktSize, statistics::units::Count::get(),
             "Write request sizes (log2)"),

    // ADD_STAT(rdQLenPdf, statistics::units::Count::get(),
    //          "What read queue length does an incoming req see"),
    // ADD_STAT(wrQLenPdf, statistics::units::Count::get(),
    //          "What write queue length does an incoming req see"),

    // ADD_STAT(rdPerTurnAround, statistics::units::Count::get(),
    //          "Reads before turning the bus around for writes"),
    // ADD_STAT(wrPerTurnAround, statistics::units::Count::get(),
    //          "Writes before turning the bus around for reads"),

    ADD_STAT(bytesReadWrQ, statistics::units::Byte::get(),
             "Total number of bytes read from write queue"),
    ADD_STAT(bytesReadSys, statistics::units::Byte::get(),
             "Total read bytes from the system interface side"),
    ADD_STAT(bytesWrittenSys, statistics::units::Byte::get(),
             "Total written bytes from the system interface side"),

    ADD_STAT(avgRdBWSys, statistics::units::Rate<
                statistics::units::Byte, statistics::units::Second>::get(),
             "Average system read bandwidth in Byte/s"),
    ADD_STAT(avgWrBWSys, statistics::units::Rate<
                statistics::units::Byte, statistics::units::Second>::get(),
             "Average system write bandwidth in Byte/s"),

    ADD_STAT(totGap, statistics::units::Tick::get(),
             "Total gap between requests"),
    ADD_STAT(avgGap, statistics::units::Rate<
                statistics::units::Tick, statistics::units::Count>::get(),
             "Average gap between requests"),

    // ADD_STAT(requestorReadBytes, statistics::units::Byte::get(),
    //          "Per-requestor bytes read from memory"),
    // ADD_STAT(requestorWriteBytes, statistics::units::Byte::get(),
    //          "Per-requestor bytes write to memory"),
    // ADD_STAT(requestorReadRate, statistics::units::Rate<
    //             statistics::units::Byte, statistics::units::Second>::get(),
    //          "Per-requestor bytes read from memory rate"),
    // ADD_STAT(requestorWriteRate, statistics::units::Rate<
    //             statistics::units::Byte, statistics::units::Second>::get(),
    //          "Per-requestor bytes write to memory rate"),
    // ADD_STAT(requestorReadAccesses, statistics::units::Count::get(),
    //          "Per-requestor read serviced memory accesses"),
    // ADD_STAT(requestorWriteAccesses, statistics::units::Count::get(),
    //          "Per-requestor write serviced memory accesses"),
    // ADD_STAT(requestorReadTotalLat, statistics::units::Tick::get(),
    //          "Per-requestor read total memory access latency"),
    // ADD_STAT(requestorWriteTotalLat, statistics::units::Tick::get(),
    //          "Per-requestor write total memory access latency"),
    // ADD_STAT(requestorReadAvgLat, statistics::units::Rate<
    //             statistics::units::Tick, statistics::units::Count>::get(),
    //          "Per-requestor read average memory access latency"),
    // ADD_STAT(requestorWriteAvgLat, statistics::units::Rate<
    //             statistics::units::Tick, statistics::units::Count>::get(),
    //          "Per-requestor write average memory access latency"),
////////

    ADD_STAT(avgORBLen, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average ORB length"),
    ADD_STAT(avgLocRdQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local read queue length"),
    ADD_STAT(avgLocWrQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local write queue length"),
    ADD_STAT(avgFarRdQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average far read queue length"),
    ADD_STAT(avgFarWrQLenStrt, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average far write queue length"),
    
    ADD_STAT(avgLocRdQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local read queue length when enqueuing"),
    ADD_STAT(avgLocWrQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average local write queue length when enqueuing"),
    ADD_STAT(avgFarRdQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average far read queue length when enqueuing"),
    ADD_STAT(avgFarWrQLenEnq, statistics::units::Rate<
                statistics::units::Count, statistics::units::Tick>::get(),
             "Average far write queue length when enqueuing"),

    ADD_STAT(numWrBacks,
            "Total number of write backs from DRAM cache to main memory"),
    ADD_STAT(totNumConf,
            "Total number of packets conflicted on DRAM cache"),
    ADD_STAT(totNumORBFull,
            "Total number of packets ORB full"),
    ADD_STAT(totNumConfBufFull,
            "Total number of packets conflicted yet couldn't "
            "enter confBuffer"),

    ADD_STAT(maxNumConf,
            "Maximum number of packets conflicted on DRAM cache"),
    ADD_STAT(maxLocRdEvQ,
            "Maximum number of packets in locMemReadEvent concurrently"),
    ADD_STAT(maxLocRdRespEvQ,
            "Maximum number of packets in locMemReadRespEvent concurrently"),
    ADD_STAT(maxLocWrEvQ,
            "Maximum number of packets in locMemRWriteEvent concurrently"),
    ADD_STAT(maxFarRdEvQ,
            "Maximum number of packets in farMemReadEvent concurrently"),
    ADD_STAT(maxFarRdRespEvQ,
            "Maximum number of packets in farMemReadRespEvent concurrently"),
    ADD_STAT(maxFarWrEvQ,
            "Maximum number of packets in farMemWriteEvent concurrently"),
    
    ADD_STAT(rdToWrTurnAround,
            "Maximum number of packets in farMemReadRespEvent concurrently"),
    ADD_STAT(wrToRdTurnAround,
            "Maximum number of packets in farMemWriteEvent concurrently"),

    ADD_STAT(sentRdPort,
            "Number of read packets successfully sent through the request port to the far memory."),
    ADD_STAT(failedRdPort,
            "Number of read packets failed to be sent through the request port to the far memory."),
    ADD_STAT(recvdRdPort,
            "Number of read packets resp recvd through the request port from the far memory."),
    ADD_STAT(sentWrPort,
            "Number of write packets successfully sent through the request port to the far memory."),
    ADD_STAT(failedWrPort,
            "Number of write packets failed to be sent through the request port to the far memory."),
    ADD_STAT(totPktsServiceTime,
            "stat"),
    ADD_STAT(totPktsORBTime,
            "stat"),
    ADD_STAT(totTimeFarRdtoSend,
            "stat"),
    ADD_STAT(totTimeFarRdtoRecv,
            "stat"),
    ADD_STAT(totTimeFarWrtoSend,
            "stat"),
    ADD_STAT(totTimeInLocRead,
            "stat"),
    ADD_STAT(totTimeInLocWrite,
            "stat"),
    ADD_STAT(totTimeInFarRead,
            "stat"),
    ADD_STAT(QTLocRd,
            "stat"),
    ADD_STAT(QTLocWr,
            "stat"),
    ADD_STAT(numTotHits,
            "stat"),
    ADD_STAT(numTotMisses,
            "stat"),
    ADD_STAT(numColdMisses,
            "stat"),
    ADD_STAT(numHotMisses,
            "stat"),
    ADD_STAT(numRdMissClean,
            "stat"),
    ADD_STAT(numRdMissDirty,
            "stat"),
    ADD_STAT(numRdHit,
            "stat"),
    ADD_STAT(numWrMissClean,
            "stat"),
    ADD_STAT(numWrMissDirty,
            "stat"),
    ADD_STAT(numWrHit,
            "stat"),
    ADD_STAT(numRdHitDirty,
            "stat"),
    ADD_STAT(numRdHitClean,
            "stat"),
    ADD_STAT(numWrHitDirty,
            "stat"),
    ADD_STAT(numWrHitClean,
            "stat")
    
{
}

void
PolicyManager::PolicyManagerStats::regStats()
{
    using namespace statistics;

    //assert(polMan.system());
    //const auto max_requestors = polMan.system()->maxRequestors();

    avgRdQLen.precision(2);
    avgWrQLen.precision(2);

    avgORBLen.precision(4);
    avgLocRdQLenStrt.precision(2);
    avgLocWrQLenStrt.precision(2);
    avgFarRdQLenStrt.precision(2);
    avgFarWrQLenStrt.precision(2);

    avgLocRdQLenEnq.precision(2);
    avgLocWrQLenEnq.precision(2);
    avgFarRdQLenEnq.precision(2);
    avgFarWrQLenEnq.precision(2);

    readPktSize.init(ceilLog2(polMan.blockSize) + 1);
    writePktSize.init(ceilLog2(polMan.blockSize) + 1);

    // rdQLenPdf.init(polMan.readBufferSize);
    // wrQLenPdf.init(polMan.writeBufferSize);

    // rdPerTurnAround
    //     .init(ctrl.readBufferSize)
    //     .flags(nozero);
    // wrPerTurnAround
    //     .init(ctrl.writeBufferSize)
    //     .flags(nozero);

    avgRdBWSys.precision(8);
    avgWrBWSys.precision(8);
    avgGap.precision(2);

    // per-requestor bytes read and written to memory
    // requestorReadBytes
    //     .init(max_requestors)
    //     .flags(nozero | nonan);

    // requestorWriteBytes
    //     .init(max_requestors)
    //     .flags(nozero | nonan);

    // per-requestor bytes read and written to memory rate
    // requestorReadRate
    //     .flags(nozero | nonan)
    //     .precision(12);

    // requestorReadAccesses
    //     .init(max_requestors)
    //     .flags(nozero);

    // requestorWriteAccesses
    //     .init(max_requestors)
    //     .flags(nozero);

    // requestorReadTotalLat
    //     .init(max_requestors)
    //     .flags(nozero | nonan);

    // requestorReadAvgLat
    //     .flags(nonan)
    //     .precision(2);

    // requestorWriteRate
    //     .flags(nozero | nonan)
    //     .precision(12);

    // requestorWriteTotalLat
    //     .init(max_requestors)
    //     .flags(nozero | nonan);

    // requestorWriteAvgLat
    //     .flags(nonan)
    //     .precision(2);

    // for (int i = 0; i < max_requestors; i++) {
    //     const std::string requestor = ctrl.system()->getRequestorName(i);
    //     requestorReadBytes.subname(i, requestor);
    //     requestorReadRate.subname(i, requestor);
    //     requestorWriteBytes.subname(i, requestor);
    //     requestorWriteRate.subname(i, requestor);
    //     requestorReadAccesses.subname(i, requestor);
    //     requestorWriteAccesses.subname(i, requestor);
    //     requestorReadTotalLat.subname(i, requestor);
    //     requestorReadAvgLat.subname(i, requestor);
    //     requestorWriteTotalLat.subname(i, requestor);
    //     requestorWriteAvgLat.subname(i, requestor);
    // }

    // Formula stats
    avgRdBWSys = (bytesReadSys) / simSeconds;
    avgWrBWSys = (bytesWrittenSys) / simSeconds;

    avgGap = totGap / (readReqs + writeReqs);

    // requestorReadRate = requestorReadBytes / simSeconds;
    // requestorWriteRate = requestorWriteBytes / simSeconds;
    // requestorReadAvgLat = requestorReadTotalLat / requestorReadAccesses;
    // requestorWriteAvgLat = requestorWriteTotalLat / requestorWriteAccesses;
}

Port &
PolicyManager::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (SimpleMemobj.py)
    if (if_name == "port") {
        return port;
    } else if (if_name == "loc_req_port") {
        return locReqPort;
    } else if (if_name == "far_req_port") {
        return farReqPort;
    } else {
        // pass it along to our super class
        panic("PORT NAME ERROR !!!!\n");
    }
}

} // namespace memory
} // namespace gem5