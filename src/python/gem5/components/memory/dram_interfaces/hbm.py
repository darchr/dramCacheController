# Copyright (c) 2012-2021 Arm Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2013 Amin Farmahini-Farahani
# Copyright (c) 2015 University of Kaiserslautern
# Copyright (c) 2015 The University of Bologna
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Interfaces for LPDDR5 memory devices

These memory "interfaces" contain the timing,energy,etc parameters for each
memory type and are usually based on datasheets for the memory devices.

You can use these interfaces in the MemCtrl object as the `dram` timing
interface.
"""

from m5.objects import DRAMInterface

class HBM_2000_4H_1x128(DRAMInterface):
    """
    A single HBM2 x128 interface (one command and address bus)
    """

    # 128-bit interface legacy mode
    device_bus_width = 128

    # HBM supports BL4 and BL2 (legacy mode only)
    burst_length = 4

    # size of channel in bytes, 4H stack of 8Gb dies is 4GiB per stack;
    # with 8 channels, 1024MiB per channel
    device_size = "1024MiB"

    device_rowbuffer_size = "2KiB"

    # 1x128 configuration
    devices_per_rank = 1

    ranks_per_channel = 2

    # HBM has 8 or 16 banks depending on capacity
    # 2Gb dies have 8 banks
    banks_per_rank = 16

    bank_groups_per_rank = 4

    # 1000 MHz for 2Gbps DDR data rate
    tCK = "1ns"

    tRP = "14ns"

    # tCCD_L = 3ns (RBus), tCCD_L = 4ns (FGDRAM paper)
    tCCD_L = "3ns"
    # tCCD_S = 2ns (RBus), tCCD_S = 2ns (FGDRAM paper)
    # this is set same as tBURST, no separate param

    #tRCDRD = 12ns, tRCDWR = 6ns (RBus)
    #tRCD = 16ns (FGDRAM paper)
    tRCD = "12ns"
    #tCL from FGDRAM paper
    tCL = "16ns"
    #tRAS = 28ns (RBus) / 29ns (FGDRAM paper)
    tRAS = "28ns"

    # Only BL4 supported
    # DDR @ 1000 MHz means 4 * 1ns / 2 = 2ns
    tBURST = "2ns"

    #tRFC from RBus
    tRFC = "220ns"

    #tREFI from RBus
    tREFI = "3.9us"

    #tWR = 14ns (RBus) / 16ns (FGDRAM paper)
    tWR = "14ns"
    #tRTP = 5ns (RBus)
    tRTP = "5ns"
    #tWTR = 9ns, 4ns (RBus) / 8ns, 3ns (FGDRAM paper)
    tWTR = "9ns"

    #tRTW from RBus
    tRTW = "18ns"

    # not available anywhere
    tCS = "0ns"

    # tRRD = 2ns (FGDRAM paper)
    tRRD = "2ns"

    # tRRDL = 6ns, tRRDS = 4ns (RBus)
    tRRD_L = "6ns"
    # no param for tRRD_S

    # tFAW = 16ns (RBus), tFAW = 14ns (FGDRAM paper)
    tXAW = "16ns"
    # activates in tFAW = 8 (from FGDRAM paper)
    activation_limit = 8

    # tXP from RBus
    tXP = "8ns"

    # tXS from RBus
    tXS = "216ns"

class HBM_1000_4H_1x128(DRAMInterface):
    """
    A single HBM x128 interface (one command and address bus), with
    default timings based on data publically released
    ("HBM: Memory Solution for High Performance Processors", MemCon, 2014),
    IDD measurement values, and by extrapolating data from other classes.
    Architecture values based on published HBM spec
    A 4H stack is defined, 2Gb per die for a total of 1GiB of memory.

    **IMPORTANT**
    HBM gen1 supports up to 8 128-bit physical channels
    Configuration defines a single channel, with the capacity
    set to (full_ stack_capacity / 8) based on 2Gb dies
    To use all 8 channels, set 'channels' parameter to 8 in
    system configuration
    """

    # 128-bit interface legacy mode
    device_bus_width = 128

    # HBM supports BL4 and BL2 (legacy mode only)
    burst_length = 4

    # size of channel in bytes, 4H stack of 2Gb dies is 1GiB per stack;
    # with 8 channels, 128MiB per channel
    device_size = "128MiB"

    device_rowbuffer_size = "2KiB"

    # 1x128 configuration
    devices_per_rank = 1

    # HBM does not have a CS pin; set rank to 1
    ranks_per_channel = 1

    # HBM has 8 or 16 banks depending on capacity
    # 2Gb dies have 8 banks
    banks_per_rank = 8

    # depending on frequency, bank groups may be required
    # will always have 4 bank groups when enabled
    # current specifications do not define the minimum frequency for
    # bank group architecture
    # setting bank_groups_per_rank to 0 to disable until range is defined
    bank_groups_per_rank = 0

    # 500 MHz for 1Gbps DDR data rate
    tCK = "2ns"

    # use values from IDD measurement in JEDEC spec
    # use tRP value for tRCD and tCL similar to other classes
    tRP = "15ns"
    tRCD = "15ns"
    tCL = "15ns"
    tRAS = "33ns"

    # BL2 and BL4 supported, default to BL4
    # DDR @ 500 MHz means 4 * 2ns / 2 = 4ns
    tBURST = "4ns"

    # value for 2Gb device from JEDEC spec
    tRFC = "160ns"

    # value for 2Gb device from JEDEC spec
    tREFI = "3.9us"

    # extrapolate the following from LPDDR configs, using ns values
    # to minimize burst length, prefetch differences
    tWR = "18ns"
    tRTP = "7.5ns"
    tWTR = "10ns"

    # start with 2 cycles turnaround, similar to other memory classes
    # could be more with variations across the stack
    tRTW = "4ns"

    # single rank device, set to 0
    tCS = "0ns"

    # from MemCon example, tRRD is 4ns with 2ns tCK
    tRRD = "4ns"

    # from MemCon example, tFAW is 30ns with 2ns tCK
    tXAW = "30ns"
    activation_limit = 4

    # 4tCK
    tXP = "8ns"

    # start with tRFC + tXP -> 160ns + 8ns = 168ns
    tXS = "168ns"


class HBM_1000_4H_1x64(HBM_1000_4H_1x128):
    """
    A single HBM x64 interface (one command and address bus), with
    default timings based on HBM gen1 and data publically released
    A 4H stack is defined, 8Gb per die for a total of 4GiB of memory.
    Note: This defines a pseudo-channel with a unique controller
    instantiated per pseudo-channel
    Stay at same IO rate (1Gbps) to maintain timing relationship with
    HBM gen1 class (HBM_1000_4H_x128) where possible

    **IMPORTANT**
    For HBM gen2 with pseudo-channel mode, configure 2X channels.
    Configuration defines a single pseudo channel, with the capacity
    set to (full_ stack_capacity / 16) based on 8Gb dies
    To use all 16 pseudo channels, set 'channels' parameter to 16 in
    system configuration
    """

    # 64-bit pseudo-channel interface
    device_bus_width = 64

    # HBM pseudo-channel only supports BL4
    burst_length = 4

    # size of channel in bytes, 4H stack of 8Gb dies is 4GiB per stack;
    # with 16 channels, 256MiB per channel
    device_size = "256MiB"

    # page size is halved with pseudo-channel; maintaining the same same number
    # of rows per pseudo-channel with 2X banks across 2 channels
    device_rowbuffer_size = "1KiB"

    # HBM has 8 or 16 banks depending on capacity
    # Starting with 4Gb dies, 16 banks are defined
    banks_per_rank = 16

    # reset tRFC for larger, 8Gb device
    # use HBM1 4Gb value as a starting point
    tRFC = "260ns"

    # start with tRFC + tXP -> 160ns + 8ns = 168ns
    tXS = "268ns"
    # Default different rank bus delay to 2 CK, @1000 MHz = 2 ns
    tCS = "2ns"
    tREFI = "3.9us"

    # active powerdown and precharge powerdown exit time
    tXP = "10ns"

    # self refresh exit time
    tXS = "65ns"

    read_buffer_size = 64
    write_buffer_size = 64

# This is a hypothetical HBM interface based on DDR3
# Increases the clock of DDR3 by 10x
# Decreases burst length (and increaes device
# bus width) by 2x
class HBM_FROM_DDR3(DRAMInterface):
    # size of device in bytes
    device_size = '512MiB'

    device_bus_width = 16

    # Using a burst length of 4
    burst_length = 4

    # Each device has a page (row buffer) size of 1 Kbyte (1K columns x8)
    device_rowbuffer_size = '1KiB'

    # 8x8 configuration, so 8 devices
    devices_per_rank = 8

    # Use two ranks
    ranks_per_channel = 2

    # DDR3 has 8 banks in all configurations
    banks_per_rank = 8

    # 8000 MHz
    tCK = '0.125ns'

    # 4 beats across an x64 interface translates to 2 clocks @ 8000 MHz
    tBURST = '0.25ns'

    # Keeping the other times same as DDR3
    # DDR3-1600 11-11-11
    tRCD = '13.75ns'
    tCL = '13.75ns'
    tRP = '13.75ns'
    tRAS = '35ns'
    tRRD = '6ns'
    tXAW = '30ns'
    activation_limit = 4
    tRFC = '260ns'

    tWR = '15ns'

    # Greater of 4 CK or 7.5 ns
    tWTR = '7.5ns'

    # Greater of 4 CK or 7.5 ns
    tRTP = '7.5ns'

    # Default same rank rd-to-wr bus turnaround to 2 CK, @800 MHz = 2.5 ns
    tRTW = '2.5ns'

    # Default different rank bus delay to 2 CK, @800 MHz = 2.5 ns
    tCS = '2.5ns'

    # <=85C, half for >85C
    tREFI = '7.8us'

    # active powerdown and precharge powerdown exit time
    tXP = '6ns'

    # self refresh exit time
    tXS = '270ns'

    # Current values from datasheet Die Rev E,J
    IDD0 = '55mA'
    IDD2N = '32mA'
    IDD3N = '38mA'
    IDD4W = '125mA'
    IDD4R = '157mA'
    IDD5 = '235mA'
    IDD3P1 = '38mA'
    IDD2P1 = '32mA'
    IDD6 = '20mA'
    VDD = '1.5V'

    read_buffer_size = 1024
    write_buffer_size = 1024

# This is a hypothetical HBM interface based on DDR3
# Increases the clock of DDR3 by 10x
# Decreases burst length (and increaes device
# bus width) by 2x
class HBM_FROM_DDR3_v2(DRAMInterface):
    # size of device in bytes
    device_size = '512MiB'

    device_bus_width = 16

    # Using a burst length of 4
    burst_length = 4

    # Each device has a page (row buffer) size of 1 Kbyte (1K columns x8)
    device_rowbuffer_size = '1KiB'

    # 8x8 configuration, so 8 devices
    devices_per_rank = 8

    # Use two ranks
    ranks_per_channel = 2

    # DDR3 has 8 banks in all configurations
    banks_per_rank = 8

    # 8000 MHz
    tCK = '0.125ns'

    # 4 beats across an x64 interface translates to 2 clocks @ 8000 MHz
    tBURST = '0.25ns'

    # Keeping the other times same as DDR3
    # DDR3-1600 11-11-11
    tRCD = '1.375ns'
    tCL = '1.375ns'
    tRP = '1.375ns'
    tRAS = '3.5ns'
    tRRD = '0.6ns'
    tXAW = '3ns'
    activation_limit = 4
    tRFC = '26ns'

    tWR = '1.5ns'

    # Greater of 4 CK or 7.5 ns
    tWTR = '0.75ns'

    # Greater of 4 CK or 7.5 ns
    tRTP = '0.75ns'

    # Default same rank rd-to-wr bus turnaround to 2 CK, @800 MHz = 2.5 ns
    tRTW = '0.25ns'

    # Default different rank bus delay to 2 CK, @800 MHz = 2.5 ns
    tCS = '0.25ns'

    # <=85C, half for >85C
    tREFI = '0.78us'

    # active powerdown and precharge powerdown exit time
    tXP = '0.6ns'

    # self refresh exit time
    tXS = '27ns'

    # Current values from datasheet Die Rev E,J
    IDD0 = '55mA'
    IDD2N = '32mA'
    IDD3N = '38mA'
    IDD4W = '125mA'
    IDD4R = '157mA'
    IDD5 = '235mA'
    IDD3P1 = '38mA'
    IDD2P1 = '32mA'
    IDD6 = '20mA'
    VDD = '1.5V'

    read_buffer_size = 1024
    write_buffer_size = 1024

class HBM_1000_4H_1x64_pseudo(HBM_1000_4H_1x64):
    """
    This is an approximation of HBM_1000_4H_1x64
    single channel (with two pseudo channels)
    decreases the burst length by 2x -->
    increases the bus width to maintain an atom
    size of 64 bytes
    increases clock frequency by 2x (is
    that needed?)
    """

    device_bus_width = 256
    burst_length = 2

    tCK = "1ns"
    tBURST = "2ns"

    read_buffer_size = 128
    write_buffer_size = 128

class HBM_1000_4H_1x64_pseudo_v2(HBM_1000_4H_1x128):
    """
    This is an approximation of HBM_1000_4H_1x64
    single channel (with two pseudo channels)
    decreases the burst length by 2x -->
    increases the bus width to maintain an atom
    size of 64 bytes
    increases clock frequency by 2x (is
    that needed?)
    """

    device_bus_width = 256
    burst_length = 2

    tCK = "1ns"
    tBURST = "2ns"

    tRFC = "130ns"

    tXS = "134ns"
    tCS = "1ns"
    tREFI = "1.95us"
    tXP = "5ns"
    tRP = "7.5ns"
    tRCD = "7.5ns"
    tCL = "7.5ns"
    tRAS = "16.5ns"
    tWR = "9ns"
    tRTP = "3.75ns"
    tWTR = "5ns"
    tRTW = "2ns"
    tRRD = "2ns"
    tXAW = "15ns"

    read_buffer_size = 128
    write_buffer_size = 128