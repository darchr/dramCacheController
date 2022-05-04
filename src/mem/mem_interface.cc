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

#include "mem/mem_interface.hh"

#include "base/bitfield.hh"
#include "base/cprintf.hh"
#include "base/trace.hh"
#include "debug/MemIntr.hh"
#include "sim/system.hh"

namespace gem5
{

namespace memory
{

MemInterface::MemInterface(const MemInterfaceParams &_p)
    : AbstractMemory(_p),
      addrMapping(_p.addr_mapping),
      burstSize((_p.devices_per_rank * _p.burst_length *
                 _p.device_bus_width) / 8),
      deviceSize(_p.device_size),
      deviceRowBufferSize(_p.device_rowbuffer_size),
      devicesPerRank(_p.devices_per_rank),
      rowBufferSize(devicesPerRank * deviceRowBufferSize),
      burstsPerRowBuffer(rowBufferSize / burstSize),
      burstsPerStripe(range.interleaved() ?
                      range.granularity() / burstSize : 1),
      ranksPerChannel(_p.ranks_per_channel),
      banksPerRank(_p.banks_per_rank), rowsPerBank(0),
      tCK(_p.tCK), tCS(_p.tCS), tBURST(_p.tBURST),
      tRTW(_p.tRTW),
      tWTR(_p.tWTR),
      readBufferSize(_p.read_buffer_size),
      writeBufferSize(_p.write_buffer_size)
{}

void
MemInterface::setCtrl(MemCtrl* _ctrl, unsigned int command_window)
{
    ctrl = _ctrl;
    maxCommandsPerWindow = command_window / tCK;
}

MemPacket*
MemInterface::decodePacket(const PacketPtr pkt, Addr pkt_addr,
                       unsigned size, bool is_read, bool is_dram)
{
    // decode the address based on the address mapping scheme, with
    // Ro, Ra, Co, Ba and Ch denoting row, rank, column, bank and
    // channel, respectively
    uint8_t rank;
    uint8_t bank;
    // use a 64-bit unsigned during the computations as the row is
    // always the top bits, and check before creating the packet
    uint64_t row;

    // Get packed address, starting at 0
    Addr addr = getCtrlAddr(pkt_addr);

    // truncate the address to a memory burst, which makes it unique to
    // a specific buffer, row, bank, rank and channel
    addr = addr / burstSize;

    // we have removed the lowest order address bits that denote the
    // position within the column
    if (addrMapping == enums::RoRaBaChCo || addrMapping == enums::RoRaBaCoCh) {
        // the lowest order bits denote the column to ensure that
        // sequential cache lines occupy the same row
        addr = addr / burstsPerRowBuffer;

        // after the channel bits, get the bank bits to interleave
        // over the banks
        bank = addr % banksPerRank;
        addr = addr / banksPerRank;

        // after the bank, we get the rank bits which thus interleaves
        // over the ranks
        rank = addr % ranksPerChannel;
        addr = addr / ranksPerChannel;

        // lastly, get the row bits, no need to remove them from addr
        row = addr % rowsPerBank;
    } else if (addrMapping == enums::RoCoRaBaCh) {
        // with emerging technologies, could have small page size with
        // interleaving granularity greater than row buffer
        if (burstsPerStripe > burstsPerRowBuffer) {
            // remove column bits which are a subset of burstsPerStripe
            addr = addr / burstsPerRowBuffer;
        } else {
            // remove lower column bits below channel bits
            addr = addr / burstsPerStripe;
        }

        // start with the bank bits, as this provides the maximum
        // opportunity for parallelism between requests
        bank = addr % banksPerRank;
        addr = addr / banksPerRank;

        // next get the rank bits
        rank = addr % ranksPerChannel;
        addr = addr / ranksPerChannel;

        // next, the higher-order column bites
        if (burstsPerStripe < burstsPerRowBuffer) {
            addr = addr / (burstsPerRowBuffer / burstsPerStripe);
        }

        // lastly, get the row bits, no need to remove them from addr
        row = addr % rowsPerBank;
    } else
        panic("Unknown address mapping policy chosen!");

    assert(rank < ranksPerChannel);
    assert(bank < banksPerRank);
    assert(row < rowsPerBank);
    assert(row < Bank::NO_ROW);

    DPRINTF(MemIntr, "Address: %#x Rank %d Bank %d Row %d\n",
            pkt_addr, rank, bank, row);

    // create the corresponding memory packet with the entry time and
    // ready time set to the current tick, the latter will be updated
    // later
    uint16_t bank_id = banksPerRank * rank + bank;

    return new MemPacket(pkt, is_read, is_dram, rank, bank, row, bank_id,
                   pkt_addr, size);
}

} // namespace memory
} // namespace gem5
