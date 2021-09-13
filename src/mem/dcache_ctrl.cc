
#include "mem/dcache_ctrl.hh"

#include "base/trace.hh"
#include "debug/DRAM.hh"
#include "debug/DcacheCtrl.hh"
#include "debug/Drain.hh"
#include "debug/NVM.hh"
#include "debug/QOS.hh"
#include "mem/dcmem_interface.hh"
#include "sim/system.hh"

DcacheCtrl::DcacheCtrl(const DcacheCtrlParams &p) :
    QoS::MemCtrl(p),
    port(name() + ".port", *this), isTimingMode(false),
    retryRdReq(false), retryWrReq(false),
    nextReqEvent([this]{ processNextReqEvent(); }, name()),
    respondEvent([this]{ processRespondEvent(); }, name()),
    nextOrbEvent([this]{ processNextOrbEvent(); }, name()),
    respOrbEvent([this]{ processRespOrbEvent(); }, name()),
    dram(p.dram), nvm(p.nvm),
    readBufferSize((dram ? dram->readBufferSize : 0) +
                   (nvm ? nvm->readBufferSize : 0)),
    writeBufferSize((dram ? dram->writeBufferSize : 0) +
                    (nvm ? nvm->writeBufferSize : 0)),
    writeHighThreshold(writeBufferSize * p.write_high_thresh_perc / 100.0),
    writeLowThreshold(writeBufferSize * p.write_low_thresh_perc / 100.0),
    minWritesPerSwitch(p.min_writes_per_switch),
    writesThisTime(0), readsThisTime(0),
    orbMaxSize(p.orb_max_size), orbSize(0),
    crbMaxSize(p.crb_max_size), crbSize(0),
    memSchedPolicy(p.mem_sched_policy),
    frontendLatency(p.static_frontend_latency),
    backendLatency(p.static_backend_latency),
    commandWindow(p.command_window),
    nextBurstAt(0), prevArrival(0),
    nextReqTime(0),
    stats(*this)
{
    DPRINTF(DcacheCtrl, "Setting up controller\n");

    //reqBuffer.resize(p.orb_max_size);
    //confReqBuffer.resize(p.crb_max_size);

    for (int i=0; i<orbMaxSize; i++) {
        reqBufferEntry* temp = new reqBufferEntry(
        false, MaxTick,
        0, false, false, 0,
        nullptr, nullptr, nullptr,
        idle, false, false);
        reqBuffer.push_back(temp);
    }

    readQueue.resize(p.qos_priorities);
    writeQueue.resize(p.qos_priorities);

    // Hook up interfaces to the controller
    if (dram)
        dram->setCtrl(this, commandWindow);
    if (nvm)
        nvm->setCtrl(this, commandWindow);

    fatal_if(!dram && !nvm, "Memory controller must have an interface");

    // perform a basic check of the write thresholds
    if (p.write_low_thresh_perc >= p.write_high_thresh_perc)
        fatal("Write buffer low threshold %d must be smaller than the "
              "high threshold %d\n", p.write_low_thresh_perc,
              p.write_high_thresh_perc);
}

void
DcacheCtrl::init()
{
   if (!port.isConnected()) {
        fatal("DcacheCtrl %s is unconnected!\n", name());
    } else {
        port.sendRangeChange();
    }
}

void
DcacheCtrl::startup()
{
    // remember the memory system mode of operation
    isTimingMode = system()->isTimingMode();

    if (isTimingMode) {
        // shift the bus busy time sufficiently far ahead that we never
        // have to worry about negative values when computing the time for
        // the next request, this will add an insignificant bubble at the
        // start of simulation
        nextBurstAt = curTick() + (dram ? dram->commandOffset() :
                                          nvm->commandOffset());
    }
}

Tick
DcacheCtrl::recvAtomic(PacketPtr pkt)
{
    DPRINTF(DcacheCtrl, "recvAtomic: %s 0x%x\n",
                     pkt->cmdString(), pkt->getAddr());

    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    Tick latency = 0;
    // do the actual memory access and turn the packet into a response
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        dram->access(pkt);

        if (pkt->hasData()) {
            // this value is not supposed to be accurate, just enough to
            // keep things going, mimic a closed page
            latency = dram->accessLatency();
        }
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
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

Tick
DcacheCtrl::recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    Tick latency = recvAtomic(pkt);
    if (dram) {
        dram->getBackdoor(backdoor);
    } else if (nvm) {
        nvm->getBackdoor(backdoor);
    }
    return latency;
}

void
DcacheCtrl::printORB()
{
    std::cout << curTick() << " // ORB :\n";
    for (int i=0; i<orbMaxSize; i++) {
        if (reqBuffer.at(i)->validEntry) {
            std::cout << "i: " << i << "\n"; 
            std::cout << "[" << i << "]: " <<
            reqBuffer.at(i)->arrivalTick << " // " <<
            reqBuffer.at(i)->dccPkt->getAddr() << " // " <<
            reqBuffer.at(i)->dccPkt->readyTime << "\n";
        }
    }
    std::cout << "-------\n";
}

void
DcacheCtrl::printCRB()
{
    std::cout << curTick() << " CRB size: " << confReqBuffer.size() << "\n";
    for (int i=0; i<confReqBuffer.size(); i++) {
            std::cout << "[" << i << "]: " <<
            confReqBuffer.at(i).second->getAddr() << "\n";
    }
    std::cout << "******\n";
}

inline Addr
DcacheCtrl::returnTag(Addr request_addr, unsigned size)
{
    int index_bits = ceilLog2(orbMaxSize);
    int block_bits = ceilLog2(size);
    return bits(request_addr, size-1, (index_bits+block_bits));
}

inline Addr
DcacheCtrl::returnIndex(Addr request_addr, unsigned size)
{
    return bits(request_addr, ceilLog2(size)+ceilLog2(orbMaxSize)-1,
                ceilLog2(size));
}

void
DcacheCtrl::handleRequestorPkt(PacketPtr pkt)
{
    unsigned index = returnIndex(pkt->getAddr(), pkt->getSize());

    dccPacket* dcc_pkt = dram->decodePacket(pkt, pkt->getAddr(),
                            pkt->getSize(), pkt->isRead(), true);
    dram->setupRank(dcc_pkt->rank, pkt->isRead());
    reqBuffer.at(index) = new reqBufferEntry(
        true, curTick(),
        returnTag(pkt->getAddr(), pkt->getSize()),
        true, pkt->isWrite(), pkt->getAddr(), pkt, dcc_pkt, nullptr,
        idle, false, false);

    // a read for tag and metadata check
    dram->access(pkt);
    checkHitOrMiss(reqBuffer.at(index));

    if (pkt->isRead()) {
        logRequest(MemCtrl::READ, pkt->requestorId(), pkt->qosValue(),
                   dcc_pkt->addr, 1);
    }
    else {
        accessAndRespond(pkt, frontendLatency, true);
        logRequest(MemCtrl::WRITE, pkt->requestorId(), pkt->qosValue(),
                   dcc_pkt->addr, 1);
    }
}

void
DcacheCtrl::checkHitOrMiss(reqBufferEntry* orbEntry)
{
    orbEntry->isHit = true;

    if (orbEntry->owPkt->isRead()) {
        orbEntry->state = dramRead;
    }
    else {
        orbEntry->state = dramWrite;
    }
}

void
DcacheCtrl::checkConflictInCRB(unsigned index)
{
    for (auto e = confReqBuffer.begin(); e != confReqBuffer.end(); ++e) {
        if (returnIndex(e->second->getAddr(),e->second->getSize()) == index) {
            reqBuffer.at(index)->conflict = true;
            break;
        }
    }
}

void
DcacheCtrl::resumeConflictingReq(unsigned index)
{
    bool conflictFound = false;
    for (auto e = confReqBuffer.begin(); e != confReqBuffer.end(); ++e) {
        auto entry = *e;
        if (returnIndex(entry.second->getAddr(), entry.second->getSize()) == index) {
                conflictFound = true;
                delete reqBuffer.at(index);              
                handleRequestorPkt(entry.second);
                confReqBuffer.erase(e);
                checkConflictInCRB(index);

                if (!nextOrbEvent.scheduled()) {
                    schedule(nextOrbEvent, curTick());
                }

                break;
            }

    }

    if (!conflictFound) {
        delete reqBuffer.at(index);
        reqBuffer.at(index) = new reqBufferEntry(
        false, MaxTick,
        0, false, false, 0,
        nullptr, nullptr, nullptr,
        idle, false, false);
    }
}

bool
DcacheCtrl::recvTimingReq(PacketPtr pkt)
{
    // This is where we enter from the outside world
    std::cout << "recvTimingReq: " << curTick() << " // " << pkt->getAddr() << "\n";
    DPRINTF(DcacheCtrl, "recvTimingReq: request %s addr %lld size %d\n",
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
    // We set a flag to make sure every single packet
    // checks DRAM first.
    bool is_dram = true;

    // Validate that pkt's address maps to the dram and nvm
    assert(nvm && nvm->getAddrRange().contains(pkt->getAddr()));
    assert(dram && dram->getAddrRange().contains(pkt->getAddr()));


    // Find out how many memory packets a pkt translates to
    // If the burst size is equal or larger than the pkt size, then a pkt
    // translates to only one memory packet. Otherwise, a pkt translates to
    // multiple memory packets
    unsigned pkt_size = pkt->getSize();
    const Addr base_addr = pkt->getAddr();
    Addr addr = base_addr;
    Addr burst_addr = burstAlign(addr, is_dram);
    uint32_t burst_size = dram->bytesPerBurst();
    unsigned size = std::min((addr | (burst_size - 1)) + 1,
                        base_addr + pkt->getSize()) - addr;

    // run the QoS scheduler and assign a QoS priority value to the packet
    // Ignored for now!
    //qosSchedule( { &reqBuffer, &confReqBuffer }, burst_size, pkt);

    // process merging for writes
    if (pkt->isWrite()) {
        assert(pkt_size != 0);
        // process merging
        bool merged = isInWriteQueue.find(burst_addr) !=
            isInWriteQueue.end();
        if (merged) {
            stats.mergedWrBursts++;
            accessAndRespond(pkt, frontendLatency, true);
            std::cout << pkt->getAddr() << " packet serviced by wr merging\n";
            // if (!nextOrbEvent.scheduled())
            //     schedule(nextOrbEvent, curTick()+1);
            return true;
        }
    }

    // process forwarding for reads
    bool foundInORB = false;
    bool foundInCRB = false;

    if (pkt->isRead()) {
        assert(pkt->isRead());
        assert(pkt_size != 0);
        if (isInWriteQueue.find(burst_addr) != isInWriteQueue.end()) {
            for (const auto& e : reqBuffer) {
                // check if the read is subsumed in the write queue
                // packet we are looking at
                if (e->owPkt->isWrite() &&
                    e->owPkt->getAddr() <= addr &&
                    ((addr + size) <=
                    (e->owPkt->getAddr() + e->owPkt->getSize()))) {
                    foundInORB = true;
                    stats.servicedByWrQ++;
                    stats.bytesReadWrQ += burst_size;
                    break;
                }
            }
            if (!foundInORB) {
                for (const auto& e : confReqBuffer) {
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
        }
        if (foundInORB || foundInCRB) {
            accessAndRespond(pkt, frontendLatency, true);
            std::cout << pkt->getAddr() << " packet serviced by FW\n";
            // if (!nextOrbEvent.scheduled())
            //     schedule(nextOrbEvent, curTick()+1);
            return true;
        }
    }

    // process conflicting requests
    // Ignored for now (because Dsize=Nsize): calculate dram address
    unsigned index = returnIndex(pkt->getAddr(), pkt->getSize());
    if (reqBuffer.at(index)->validEntry) {
        // if (confReqBuffer.size()>=crbMaxSize) {
        //     std::cout << " size()>=crbMax\n";
        //     return false;
        // }
        confReqBuffer.push_back(std::make_pair(curTick(), pkt));
        reqBuffer.at(index)->conflict = true;
        std::cout << pkt->getAddr() << " packet conflict!\n";
        // if (!nextOrbEvent.scheduled())
        //     schedule(nextOrbEvent, curTick()+1);
        return true;
    }

    // if none of the above cases happens,
    // then add the pkt to outstanding requests buffer
    handleRequestorPkt(pkt);

    if (!nextOrbEvent.scheduled()) {
        schedule(nextOrbEvent, curTick());
    }

    printORB();

    return true;
}

void
DcacheCtrl::processNextOrbEvent()
{
    std::cout << curTick() << " processNextOrbEvent\n";
    bool foundPkt = false;
    for (auto e = reqBuffer.begin(); e != reqBuffer.end(); ++e) {
        auto entry = *e;
        if (entry->validEntry &&
            entry->isHit &&
            entry->state != respReady) {
            
            foundPkt = true;

            if (entry->dccPkt->isRead()) {
                busState = READ;

                doBurstAccess(entry->dccPkt);

                // sanity check
                assert(entry->dccPkt->size <= (entry->dccPkt->isDram() ?
                                          dram->bytesPerBurst() :
                                          nvm->bytesPerBurst()) );
                assert(entry->dccPkt->readyTime >= curTick());

                logResponse(MemCtrl::READ, entry->dccPkt->requestorId(),
                        entry->dccPkt->qosValue(), entry->dccPkt->getAddr(), 1,
                        entry->dccPkt->readyTime - entry->dccPkt->entryTime);
                
                if (respIndices.empty()) {
                    assert(!respOrbEvent.scheduled());
                    schedule(respOrbEvent, entry->dccPkt->readyTime);
                }
                else {
                    assert(reqBuffer.at(respIndices.back())->dccPkt->readyTime
                                        <= entry->dccPkt->readyTime);
                    assert(respOrbEvent.scheduled());
                }

                respIndices.push_back(returnIndex(entry->owPkt->getAddr(),entry->owPkt->getSize()));
                entry->state = respReady;
                
            }
            else if (entry->dccPkt->isWrite()) {
                busState = WRITE;
                // sanity check
                assert(entry->dccPkt->size <= (entry->dccPkt->isDram() ?
                                        dram->bytesPerBurst() :
                                        nvm->bytesPerBurst()) );

                doBurstAccess(entry->dccPkt);

                logResponse(MemCtrl::WRITE, entry->dccPkt->requestorId(),
                        entry->dccPkt->qosValue(), entry->dccPkt->getAddr(), 1,
                        entry->dccPkt->readyTime - entry->dccPkt->entryTime);
                
                // delete the orb entry and bring in
                // the first conflicting req(s), if any
                unsigned index = returnIndex(entry->dccPkt->getAddr(),entry->dccPkt->getSize());
                isInWriteQueue.erase(burstAlign(entry->dccPkt->addr, entry->dccPkt->isDram()));
                resumeConflictingReq(index);
            }
            break;
            
        }
    }

    if (!foundPkt) {
            std::cout << "No pkt Found - exiting nextOrbEvent\n";
            return;
    }

    if (!nextOrbEvent.scheduled()) {
        schedule(nextOrbEvent, std::max(nextReqTime, curTick()));
    }

    // if (retryWrReq && totalWriteQueueSize < writeBufferSize) {
    //     retryWrReq = false;
    //     port.sendRetryReq();
    // }
}

void
DcacheCtrl::processRespOrbEvent()
{
    std::cout << curTick() << " processRespOrbEvent\n";

    unsigned i = respIndices.front();
    assert( reqBuffer.at(i)->validEntry &&
            reqBuffer.at(i)->dccPkt->isRead() &&
            reqBuffer.at(i)->state == respReady &&
            reqBuffer.at(i)->dccPkt->readyTime == curTick());
    
    if (reqBuffer.at(i)->dccPkt->isDram()) {
        // media specific checks and functions when read response is complete
        dram->respondEvent(reqBuffer.at(i)->dccPkt->rank);
    }
    accessAndRespond(reqBuffer.at(i)->owPkt, frontendLatency + backendLatency, true);
    

    respIndices.pop_front();
                
    if (!respIndices.empty()) {
        assert(reqBuffer.at(respIndices.front())->dccPkt->readyTime >= curTick());
        assert(!respOrbEvent.scheduled());
        schedule(respOrbEvent, reqBuffer.at(respIndices.front())->dccPkt->readyTime);
    } else {
        // if there is nothing left in any queue, signal a drain
        if (drainState() == DrainState::Draining &&
            !totalWriteQueueSize && !totalReadQueueSize &&
            allIntfDrained()) {

            DPRINTF(Drain, "Controller done draining\n");
            signalDrainDone();
        } else if (reqBuffer.at(i)->dccPkt->isDram()) {
            // check the refresh state and kick the refresh event loop
            // into action again if banks already closed and just waiting
            // for read to complete
            dram->checkRefreshState(reqBuffer.at(i)->dccPkt->rank);
        }
    }

    resumeConflictingReq(i);

    //if (retryRdReq) {
    //    retryRdReq = false;
    //    port.sendRetryReq();
    //}
}


void
DcacheCtrl::processRespondEvent()
{

}

dccPacketQueue::iterator
DcacheCtrl::chooseNext(dccPacketQueue& queue, Tick extra_col_delay)
{
    // This method does the arbitration between requests.

    dccPacketQueue::iterator ret = queue.end();

    if (!queue.empty()) {
        if (queue.size() == 1) {
            // available rank corresponds to state refresh idle
            dccPacket* dcc_pkt = *(queue.begin());
            if (packetReady(dcc_pkt)) {
                ret = queue.begin();
                DPRINTF(DcacheCtrl, "Single request, going to a free rank\n");
            } else {
                DPRINTF(DcacheCtrl, "Single request, going to a busy rank\n");
            }
        } else if (memSchedPolicy == Enums::fcfs) {
            // check if there is a packet going to a free rank
            for (auto i = queue.begin(); i != queue.end(); ++i) {
                dccPacket* dcc_pkt = *i;
                if (packetReady(dcc_pkt)) {
                    ret = i;
                    break;
                }
            }
        } else if (memSchedPolicy == Enums::frfcfs) {
            ret = chooseNextFRFCFS(queue, extra_col_delay);
        } else {
            panic("No scheduling policy chosen\n");
        }
    }
    return ret;
}

dccPacketQueue::iterator
DcacheCtrl::chooseNextFRFCFS(dccPacketQueue& queue, Tick extra_col_delay)
{
    auto selected_pkt_it = queue.end();
    Tick col_allowed_at = MaxTick;

    // time we need to issue a column command to be seamless
    const Tick min_col_at = std::max(nextBurstAt + extra_col_delay, curTick());

    // find optimal packet for each interface
    if (dram && nvm) {
        // create 2nd set of parameters for NVM
        auto nvm_pkt_it = queue.end();
        Tick nvm_col_at = MaxTick;

        // Select packet by default to give priority if both
        // can issue at the same time or seamlessly
        std::tie(selected_pkt_it, col_allowed_at) =
                 dram->chooseNextFRFCFS(queue, min_col_at);
        std::tie(nvm_pkt_it, nvm_col_at) =
                 nvm->chooseNextFRFCFS(queue, min_col_at);

        // Compare DRAM and NVM and select NVM if it can issue
        // earlier than the DRAM packet
        if (col_allowed_at > nvm_col_at) {
            selected_pkt_it = nvm_pkt_it;
        }
    } else if (dram) {
        std::tie(selected_pkt_it, col_allowed_at) =
                 dram->chooseNextFRFCFS(queue, min_col_at);
    } else if (nvm) {
        std::tie(selected_pkt_it, col_allowed_at) =
                 nvm->chooseNextFRFCFS(queue, min_col_at);
    }

    if (selected_pkt_it == queue.end()) {
        DPRINTF(DcacheCtrl, "%s no available packets found\n", __func__);
    }

    return selected_pkt_it;
}

void
DcacheCtrl::accessAndRespond(PacketPtr pkt, Tick static_latency, bool in_dram)
{
    DPRINTF(DcacheCtrl, "Responding to Address %lld.. \n",pkt->getAddr());

    bool needsResponse = pkt->needsResponse();
    // do the actual memory access which also turns the packet into a
    // response
    if (in_dram && dram && dram->getAddrRange().contains(pkt->getAddr())) {
        dram->access(pkt);
    } else if (!in_dram && nvm &&
                nvm->getAddrRange().contains(pkt->getAddr())) {
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

    DPRINTF(DcacheCtrl, "Done\n");

    return;
}

void
DcacheCtrl::pruneBurstTick()
{
    auto it = burstTicks.begin();
    while (it != burstTicks.end()) {
        auto current_it = it++;
        if (curTick() > *current_it) {
            DPRINTF(DcacheCtrl, "Removing burstTick for %d\n", *current_it);
            burstTicks.erase(current_it);
        }
    }
}

Tick
DcacheCtrl::getBurstWindow(Tick cmd_tick)
{
    // get tick aligned to burst window
    Tick burst_offset = cmd_tick % commandWindow;
    return (cmd_tick - burst_offset);
}

Tick
DcacheCtrl::verifySingleCmd(Tick cmd_tick, Tick max_cmds_per_burst)
{
    // start with assumption that there is no contention on command bus
    Tick cmd_at = cmd_tick;

    // get tick aligned to burst window
    Tick burst_tick = getBurstWindow(cmd_tick);

    // verify that we have command bandwidth to issue the command
    // if not, iterate over next window(s) until slot found
    while (burstTicks.count(burst_tick) >= max_cmds_per_burst) {
        DPRINTF(DcacheCtrl, "Contention found on command bus at %d\n",
                burst_tick);
        burst_tick += commandWindow;
        cmd_at = burst_tick;
    }

    // add command into burst window and return corresponding Tick
    burstTicks.insert(burst_tick);
    return cmd_at;
}

Tick
DcacheCtrl::verifyMultiCmd(Tick cmd_tick, Tick max_cmds_per_burst,
                         Tick max_multi_cmd_split)
{
    // start with assumption that there is no contention on command bus
    Tick cmd_at = cmd_tick;

    // get tick aligned to burst window
    Tick burst_tick = getBurstWindow(cmd_tick);

    // Command timing requirements are from 2nd command
    // Start with assumption that 2nd command will issue at cmd_at and
    // find prior slot for 1st command to issue
    // Given a maximum latency of max_multi_cmd_split between the commands,
    // find the burst at the maximum latency prior to cmd_at
    Tick burst_offset = 0;
    Tick first_cmd_offset = cmd_tick % commandWindow;
    while (max_multi_cmd_split > (first_cmd_offset + burst_offset)) {
        burst_offset += commandWindow;
    }
    // get the earliest burst aligned address for first command
    // ensure that the time does not go negative
    Tick first_cmd_tick = burst_tick - std::min(burst_offset, burst_tick);

    // Can required commands issue?
    bool first_can_issue = false;
    bool second_can_issue = false;
    // verify that we have command bandwidth to issue the command(s)
    while (!first_can_issue || !second_can_issue) {
        bool same_burst = (burst_tick == first_cmd_tick);
        auto first_cmd_count = burstTicks.count(first_cmd_tick);
        auto second_cmd_count = same_burst ? first_cmd_count + 1 :
                                   burstTicks.count(burst_tick);

        first_can_issue = first_cmd_count < max_cmds_per_burst;
        second_can_issue = second_cmd_count < max_cmds_per_burst;

        if (!second_can_issue) {
            DPRINTF(DcacheCtrl, "Contention (cmd2) found on "
                                "command bus at %d\n",
                    burst_tick);
            burst_tick += commandWindow;
            cmd_at = burst_tick;
        }

        // Verify max_multi_cmd_split isn't violated when command 2 is shifted
        // If commands initially were issued in same burst, they are
        // now in consecutive bursts and can still issue B2B
        bool gap_violated = !same_burst &&
             ((burst_tick - first_cmd_tick) > max_multi_cmd_split);

        if (!first_can_issue || (!second_can_issue && gap_violated)) {
            DPRINTF(DcacheCtrl, "Contention (cmd1) found on "
                                "command bus at %d\n",
                    first_cmd_tick);
            first_cmd_tick += commandWindow;
        }
    }

    // Add command to burstTicks
    burstTicks.insert(burst_tick);
    burstTicks.insert(first_cmd_tick);

    return cmd_at;
}

bool
DcacheCtrl::inReadBusState(bool next_state) const
{
    // check the bus state
    if (next_state) {
        // use busStateNext to get the state that will be used
        // for the next burst
        return (busStateNext == DcacheCtrl::READ);
    } else {
        return (busState == DcacheCtrl::READ);
    }
}

bool
DcacheCtrl::inWriteBusState(bool next_state) const
{
    // check the bus state
    if (next_state) {
        // use busStateNext to get the state that will be used
        // for the next burst
        return (busStateNext == DcacheCtrl::WRITE);
    } else {
        return (busState == DcacheCtrl::WRITE);
    }
}

void
DcacheCtrl::doBurstAccess(dccPacket* dcc_pkt)
{
    // first clean up the burstTick set, removing old entries
    // before adding new entries for next burst
    pruneBurstTick();

    // When was command issued?
    Tick cmd_at;

    // Issue the next burst and update bus state to reflect
    // when previous command was issued
    if (dcc_pkt->isDram()) {
        //std::vector<dccPacketQueue>& queue = selQueue(dcc_pkt->isRead());
        std::tie(cmd_at, nextBurstAt) =
                 dram->doBurstAccess(dcc_pkt, nextBurstAt);//, queue);

        // Update timing for NVM ranks if NVM is configured on this channel
        if (nvm)
            nvm->addRankToRankDelay(cmd_at);

    } else {
        std::tie(cmd_at, nextBurstAt) =
                 nvm->doBurstAccess(dcc_pkt, nextBurstAt);

        // Update timing for NVM ranks if NVM is configured on this channel
        if (dram)
            dram->addRankToRankDelay(cmd_at);

    }

    DPRINTF(DcacheCtrl, "Access to %lld, ready at %lld next burst at %lld.\n",
            dcc_pkt->addr, dcc_pkt->readyTime, nextBurstAt);

    // Update the minimum timing between the requests, this is a
    // conservative estimate of when we have to schedule the next
    // request to not introduce any unecessary bubbles. In most cases
    // we will wake up sooner than we have to.
    nextReqTime = nextBurstAt - (dram ? dram->commandOffset() :
                                        nvm->commandOffset());


    // Update the common bus stats
    if (dcc_pkt->isRead()) {
        ++readsThisTime;
        // Update latency stats
        stats.requestorReadTotalLat[dcc_pkt->requestorId()] +=
            dcc_pkt->readyTime - dcc_pkt->entryTime;
        stats.requestorReadBytes[dcc_pkt->requestorId()] += dcc_pkt->size;
    } else {
        ++writesThisTime;
        stats.requestorWriteBytes[dcc_pkt->requestorId()] += dcc_pkt->size;
        stats.requestorWriteTotalLat[dcc_pkt->requestorId()] +=
            dcc_pkt->readyTime - dcc_pkt->entryTime;
    }
}

void
DcacheCtrl::processNextReqEvent()
{

}

bool
DcacheCtrl::packetReady(dccPacket* pkt)
{
    return (pkt->isDram() ?
        dram->burstReady(pkt) : nvm->burstReady(pkt));
}

Tick
DcacheCtrl::minReadToWriteDataGap()
{
    Tick dram_min = dram ?  dram->minReadToWriteDataGap() : MaxTick;
    Tick nvm_min = nvm ?  nvm->minReadToWriteDataGap() : MaxTick;
    return std::min(dram_min, nvm_min);
}

Tick
DcacheCtrl::minWriteToReadDataGap()
{
    Tick dram_min = dram ? dram->minWriteToReadDataGap() : MaxTick;
    Tick nvm_min = nvm ?  nvm->minWriteToReadDataGap() : MaxTick;
    return std::min(dram_min, nvm_min);
}

Addr
DcacheCtrl::burstAlign(Addr addr, bool is_dram) const
{
    if (is_dram)
        return (addr & ~(Addr(dram->bytesPerBurst() - 1)));
    else
        return (addr & ~(Addr(nvm->bytesPerBurst() - 1)));
}

DcacheCtrl::CtrlStats::CtrlStats(DcacheCtrl &_ctrl)
    : Stats::Group(&_ctrl),
    ctrl(_ctrl),

    ADD_STAT(readReqs, "Number of read requests accepted"),
    ADD_STAT(writeReqs, "Number of write requests accepted"),

    ADD_STAT(readBursts,
             "Number of controller read bursts, "
             "including those serviced by the write queue"),
    ADD_STAT(writeBursts,
             "Number of controller write bursts, "
             "including those merged in the write queue"),
    ADD_STAT(servicedByWrQ,
             "Number of controller read bursts serviced by the write queue"),
    ADD_STAT(mergedWrBursts,
             "Number of controller write bursts merged with an existing one"),

    ADD_STAT(neitherReadNorWriteReqs,
             "Number of requests that are neither read nor write"),

    ADD_STAT(avgRdQLen, "Average read queue length when enqueuing"),
    ADD_STAT(avgWrQLen, "Average write queue length when enqueuing"),

    ADD_STAT(numRdRetry, "Number of times read queue was full causing retry"),
    ADD_STAT(numWrRetry, "Number of times write queue was full causing retry"),

    ADD_STAT(readPktSize, "Read request sizes (log2)"),
    ADD_STAT(writePktSize, "Write request sizes (log2)"),

    ADD_STAT(rdQLenPdf, "What read queue length does an incoming req see"),
    ADD_STAT(wrQLenPdf, "What write queue length does an incoming req see"),

    ADD_STAT(rdPerTurnAround,
             "Reads before turning the bus around for writes"),
    ADD_STAT(wrPerTurnAround,
             "Writes before turning the bus around for reads"),

    ADD_STAT(bytesReadWrQ, "Total number of bytes read from write queue"),
    ADD_STAT(bytesReadSys, "Total read bytes from the system interface side"),
    ADD_STAT(bytesWrittenSys,
             "Total written bytes from the system interface side"),

    ADD_STAT(avgRdBWSys, "Average system read bandwidth in MiByte/s"),
    ADD_STAT(avgWrBWSys, "Average system write bandwidth in MiByte/s"),

    ADD_STAT(totGap, "Total gap between requests"),
    ADD_STAT(avgGap, "Average gap between requests"),

    ADD_STAT(requestorReadBytes, "Per-requestor bytes read from memory"),
    ADD_STAT(requestorWriteBytes, "Per-requestor bytes write to memory"),
    ADD_STAT(requestorReadRate,
             "Per-requestor bytes read from memory rate (Bytes/sec)"),
    ADD_STAT(requestorWriteRate,
             "Per-requestor bytes write to memory rate (Bytes/sec)"),
    ADD_STAT(requestorReadAccesses,
             "Per-requestor read serviced memory accesses"),
    ADD_STAT(requestorWriteAccesses,
             "Per-requestor write serviced memory accesses"),
    ADD_STAT(requestorReadTotalLat,
             "Per-requestor read total memory access latency"),
    ADD_STAT(requestorWriteTotalLat,
             "Per-requestor write total memory access latency"),
    ADD_STAT(requestorReadAvgLat,
             "Per-requestor read average memory access latency"),
    ADD_STAT(requestorWriteAvgLat,
             "Per-requestor write average memory access latency")

{
}

void
DcacheCtrl::CtrlStats::regStats()
{
    using namespace Stats;

    assert(ctrl.system());
    const auto max_requestors = ctrl.system()->maxRequestors();

    avgRdQLen.precision(2);
    avgWrQLen.precision(2);

    readPktSize.init(ceilLog2(ctrl.system()->cacheLineSize()) + 1);
    writePktSize.init(ceilLog2(ctrl.system()->cacheLineSize()) + 1);

    rdQLenPdf.init(ctrl.readBufferSize);
    wrQLenPdf.init(ctrl.writeBufferSize);

    rdPerTurnAround
        .init(ctrl.readBufferSize)
        .flags(nozero);
    wrPerTurnAround
        .init(ctrl.writeBufferSize)
        .flags(nozero);

    avgRdBWSys.precision(2);
    avgWrBWSys.precision(2);
    avgGap.precision(2);

    // per-requestor bytes read and written to memory
    requestorReadBytes
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorWriteBytes
        .init(max_requestors)
        .flags(nozero | nonan);

    // per-requestor bytes read and written to memory rate
    requestorReadRate
        .flags(nozero | nonan)
        .precision(12);

    requestorReadAccesses
        .init(max_requestors)
        .flags(nozero);

    requestorWriteAccesses
        .init(max_requestors)
        .flags(nozero);

    requestorReadTotalLat
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorReadAvgLat
        .flags(nonan)
        .precision(2);

    requestorWriteRate
        .flags(nozero | nonan)
        .precision(12);

    requestorWriteTotalLat
        .init(max_requestors)
        .flags(nozero | nonan);

    requestorWriteAvgLat
        .flags(nonan)
        .precision(2);

    for (int i = 0; i < max_requestors; i++) {
        const std::string requestor = ctrl.system()->getRequestorName(i);
        requestorReadBytes.subname(i, requestor);
        requestorReadRate.subname(i, requestor);
        requestorWriteBytes.subname(i, requestor);
        requestorWriteRate.subname(i, requestor);
        requestorReadAccesses.subname(i, requestor);
        requestorWriteAccesses.subname(i, requestor);
        requestorReadTotalLat.subname(i, requestor);
        requestorReadAvgLat.subname(i, requestor);
        requestorWriteTotalLat.subname(i, requestor);
        requestorWriteAvgLat.subname(i, requestor);
    }

    // Formula stats
    avgRdBWSys = (bytesReadSys / 1000000) / simSeconds;
    avgWrBWSys = (bytesWrittenSys / 1000000) / simSeconds;

    avgGap = totGap / (readReqs + writeReqs);

    requestorReadRate = requestorReadBytes / simSeconds;
    requestorWriteRate = requestorWriteBytes / simSeconds;
    requestorReadAvgLat = requestorReadTotalLat / requestorReadAccesses;
    requestorWriteAvgLat = requestorWriteTotalLat / requestorWriteAccesses;
}

void
DcacheCtrl::recvFunctional(PacketPtr pkt)
{
    if (dram && dram->getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        dram->functionalAccess(pkt);
    } else if (nvm && nvm->getAddrRange().contains(pkt->getAddr())) {
        // rely on the abstract memory
        nvm->functionalAccess(pkt);
   } else {
        panic("Can't handle address range for packet %s\n",
              pkt->print());
   }
}

Port &
DcacheCtrl::getPort(const std::string &if_name, PortID idx)
{
    if (if_name != "port") {
        return QoS::MemCtrl::getPort(if_name, idx);
    } else {
        return port;
    }
}

bool
DcacheCtrl::allIntfDrained() const
{
   // ensure dram is in power down and refresh IDLE states
   bool dram_drained = !dram || dram->allRanksDrained();
   // No outstanding NVM writes
   // All other queues verified as needed with calling logic
   bool nvm_drained = !nvm || nvm->allRanksDrained();
   return (dram_drained && nvm_drained);
}

DrainState
DcacheCtrl::drain()
{
    // if there is anything in any of our internal queues, keep track
    // of that as well
    if (!(!totalWriteQueueSize && !totalReadQueueSize && respIndices.empty() &&
          allIntfDrained())) {

        DPRINTF(Drain, "Memory controller not drained, write: %d, read: %d,"
                " resp: %d\n", totalWriteQueueSize, totalReadQueueSize,
                respIndices.size());

        // the only queue that is not drained automatically over time
        // is the write queue, thus kick things into action if needed
        if (!totalWriteQueueSize && !nextReqEvent.scheduled()) {
            schedule(nextOrbEvent, curTick());
        }

        if (dram)
            dram->drainRanks();

        return DrainState::Draining;
    } else {
        return DrainState::Drained;
    }
}

void
DcacheCtrl::drainResume()
{
    if (!isTimingMode && system()->isTimingMode()) {
        // if we switched to timing mode, kick things into action,
        // and behave as if we restored from a checkpoint
        startup();
        dram->startup();
    } else if (isTimingMode && !system()->isTimingMode()) {
        // if we switch from timing mode, stop the refresh events to
        // not cause issues with KVM
        if (dram)
            dram->suspend();
    }

    // update the mode
    isTimingMode = system()->isTimingMode();
}

DcacheCtrl::MemoryPort::MemoryPort(const std::string& name, DcacheCtrl& _ctrl)
    : QueuedResponsePort(name, &_ctrl, queue), queue(_ctrl, *this, true),
      ctrl(_ctrl)
{ }

AddrRangeList
DcacheCtrl::MemoryPort::getAddrRanges() const
{
    AddrRangeList ranges;
    if (ctrl.dram) {
        DPRINTF(DRAM, "Pushing DRAM ranges to port\n");
        ranges.push_back(ctrl.dram->getAddrRange());
    }
    if (ctrl.nvm) {
        DPRINTF(NVM, "Pushing NVM ranges to port\n");
        ranges.push_back(ctrl.nvm->getAddrRange());
    }
    return ranges;
}

void
DcacheCtrl::MemoryPort::recvFunctional(PacketPtr pkt)
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
DcacheCtrl::MemoryPort::recvAtomic(PacketPtr pkt)
{
    return ctrl.recvAtomic(pkt);
}

Tick
DcacheCtrl::MemoryPort::recvAtomicBackdoor(
        PacketPtr pkt, MemBackdoorPtr &backdoor)
{
    return ctrl.recvAtomicBackdoor(pkt, backdoor);
}

bool
DcacheCtrl::MemoryPort::recvTimingReq(PacketPtr pkt)
{
    // pass it to the memory controller
    return ctrl.recvTimingReq(pkt);
}
