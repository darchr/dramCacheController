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

/**
 * @file
 * HBMCtrl declaration
 */

#ifndef __HBM_CTRL_HH__
#define __HBM_CTRL_HH__

#include <deque>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mem/simple_mem_ctrl.hh"
#include "params/HBMCtrl.hh"

namespace gem5
{

namespace memory
{

class MemInterface;
class DRAMInterface;


/**
 * HBM2 is divided into two pseudo channels which have independent data buses
 * but share a command bus. Thedrefore, the HBM memory controller should be
 * able to control both pseudo channels.
 * This HBM memory controller inherits from gem5's default
 * memory controller (pseudo channel 0) and manages the additional HBM pseudo
 * channel (pseudo channel 1).
 */
class HBMCtrl : public SimpleMemCtrl
{

  protected:

    bool respQEmpty()
    {
      return (respQueue.empty() && respQueueCh1.empty());
    };

  private:

    /**
     * Remember if we have to retry a request for second pseudo channel.
     */
    bool retryRdReqCh1;
    bool retryWrReqCh1;

  public:
    HBMCtrl(const HBMCtrlParams &p);


    void pruneRowBurstTick();
    void pruneColBurstTick();

    /**
     * Check for command bus contention for single cycle command.
     * If there is contention, shift command to next burst.
     * Check verifies that the commands issued per burst is less
     * than a defined max number, maxCommandsPerWindow.
     * Therefore, contention per cycle is not verified and instead
     * is done based on a burst window.
     *
     * @param cmd_tick Initial tick of command, to be verified
     * @param max_cmds_per_burst Number of commands that can issue
     *                           in a burst window
     * @return tick for command issue without contention
     */
    Tick verifySingleCmd(Tick cmd_tick, Tick max_cmds_per_burst, bool row_cmd);

    /**
     * Check for command bus contention for multi-cycle (2 currently)
     * command. If there is contention, shift command(s) to next burst.
     * Check verifies that the commands issued per burst is less
     * than a defined max number, maxCommandsPerWindow.
     * Therefore, contention per cycle is not verified and instead
     * is done based on a burst window.
     *
     * @param cmd_tick Initial tick of command, to be verified
     * @param max_multi_cmd_split Maximum delay between commands
     * @param max_cmds_per_burst Number of commands that can issue
     *                           in a burst window
     * @return tick for command issue without contention
     */
    Tick verifyMultiCmd(Tick cmd_tick, Tick max_cmds_per_burst,
                        Tick max_multi_cmd_split = 0);

    /**
     * NextReq and Respond events for second pseudo channel
     *
     */
    void processNextReqEventCh1();
    EventFunctionWrapper nextReqEventCh1;

    void processRespondEventCh1();
    EventFunctionWrapper respondEventCh1;

    /**
     * Check if the read queue partition of both pseudo
     * channels has room for more entries. This is used when the HBM ctrl
     * is run with partitioned queues
     *
     * @param pkt_count The number of entries needed in the read queue
     * @return true if read queue partition is full, false otherwise
     */
    bool readQueueFullCh0(unsigned int pkt_count) const;
    bool readQueueFullCh1(unsigned int pkt_count) const;
    bool readQueueFull(unsigned int pkt_count);

    /**
     * Check if the write queue partition of both pseudo
     * channels has room for more entries. This is used when the HBM ctrl
     * is run with partitioned queues
     *
     * @param pkt_count The number of entries needed in the write queue
     * @return true if write queue is full, false otherwise
     */
    bool writeQueueFullCh0(unsigned int pkt_count) const;
    bool writeQueueFullCh1(unsigned int pkt_count) const;


    /**
     * Following counters are used to keep track of the entries in read/write
     * queue for each pseudo channel (useful when the partitioned queues are
     * used)
     */
    uint64_t readQueueSizeCh0 = 0;
    uint64_t readQueueSizeCh1 = 0;
    uint64_t writeQueueSizeCh0 = 0;
    uint64_t writeQueueSizeCh1 = 0;

    /**
     * The memory schduler/arbiter - picks which request needs to
     * go next, based on the specified policy such as FCFS or FR-FCFS
     * and moves it to the head of the queue.
     * Prioritizes accesses to the same rank as previous burst unless
     * controller is switching command type.
     *
     * @param queue Queued requests to consider
     * @param extra_col_delay Any extra delay due to a read/write switch
     * @return an iterator to the selected packet, else queue.end()
     */
    MemPacketQueue::iterator chooseNext(MemPacketQueue& queue,
        Tick extra_col_delay);

    /**
     * For FR-FCFS policy reorder the read/write queue depending on row buffer
     * hits and earliest bursts available in memory
     *
     * @param queue Queued requests to consider
     * @param extra_col_delay Any extra delay due to a read/write switch
     * @return an iterator to the selected packet, else queue.end()
     */
    MemPacketQueue::iterator chooseNextFRFCFS(MemPacketQueue& queue,
            Tick extra_col_delay);

    /**
     * Calculate burst window aligned tick
     *
     * @param cmd_tick Initial tick of command
     * @return burst window aligned tick
     */
    Tick getBurstWindow(Tick cmd_tick);

    /**
     * Used for debugging to observe the contents of the queues.
     */
    void printQs() const;

    /**
     * Burst-align an address.
     *
     * @param addr The potentially unaligned address
     * @param is_dram Does this packet access DRAM?
     *
     * @return An address aligned to a memory burst
     */
    //Addr burstAlign(Addr addr, bool is_dram) const;
    Addr burstAlign(Addr addr) const;

    /**
     * The controller's main read and write queues,
     * with support for QoS reordering
     */
    std::vector<MemPacketQueue> readQueue;
    std::vector<MemPacketQueue> writeQueue;

    /**
     * To avoid iterating over the write queue to check for
     * overlapping transactions, maintain a set of burst addresses
     * that are currently queued. Since we merge writes to the same
     * location we never have more than one address to the same burst
     * address.
     */
    std::unordered_set<Addr> isInWriteQueue;

    /**
     * Response queue where read packets wait after we're done working
     * with them, but it's not time to send the response yet. The
     * responses are stored separately mostly to keep the code clean
     * and help with events scheduling. For all logical purposes such
     * as sizing the read queue, this and the main read queue need to
     * be added together.
     */
    std::deque<MemPacket*> respQueueCh1;

    /**
     * Holds count of commands issued in burst window starting at
     * defined Tick. This is used to ensure that the row command bandwidth
     * does not exceed the allowable media constraints.
     */
    std::unordered_multiset<Tick> rowBurstTicks;

    /**
     * This is used to ensure that the row command bandwidth
     * does not exceed the allowable media constraints. HBM2 has separate
     * command bus for row and column commands
     */
    std::unordered_multiset<Tick> colBurstTicks;


    /**
     * Pointers to interfaces of the two pseudo channels
     */
    DRAMInterface* ch0_int;
    DRAMInterface* ch1_int;

    /**
     * This indicates if the R/W queues will be partitioned among
     * pseudo channels
     */
    bool partitionedQ;

  public:

    /**
     * Is there a respondEvent for pseudo channel 1 scheduled?
     *
     * @return true if event is scheduled
     */
    bool respondEventCh1Scheduled() const
              { return respondEventCh1.scheduled(); }

    /**
     * Is there a read/write burst Event scheduled?
     *
     * @return true if event is scheduled
     */
    bool requestEventCh1Scheduled() const
              { return nextReqEventCh1.scheduled(); }

    /**
     * restart the controller scheduler for pseudo channel 1 packets
     *
     * @param Tick to schedule next event
     */
    void restartSchedulerCh1(Tick tick) { schedule(nextReqEventCh1, tick); }


    virtual void init() override;
    virtual void startup() override;
    virtual void drainResume() override;


  protected:
    Tick recvAtomic(PacketPtr pkt) override;
    Tick recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &backdoor) override;
    void recvFunctional(PacketPtr pkt) override;
    bool recvTimingReq(PacketPtr pkt) override;

};

} // namespace memory
} // namespace gem5

#endif //__HBM_CTRL_HH__
