# Copyright (c) 2021 The Regents of the University of California
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


import m5
import sys
import argparse

from m5.util import fatal
from m5.objects import Root
from m5.util.convert import toMemorySize

sys.path.append("gem5")
from components_library.boards.test_board import TestBoard

from components_library.cachehierarchies.classic.mesi_two_level_cache_hierarchy import (
    MESITwoLevelCacheHierarchy,
)
from components_library.memory.single_channel import SingleChannelDDR3_1600
from components_library.memory.single_channel import SingleChannelDDR4_2400
from components_library.memory.single_channel import SingleChannelLPDDR3_1600
from components_library.memory.single_channel import SingleChannelHBM

from components_library.processors.complex_generator import ComplexGenerator

parser = argparse.ArgumentParser()
parser.add_argument("mem_type", type=str, help="type of memory to simulate")
parser.add_argument("data_limit", type=str, help="The amount of data to read")

args = parser.parse_args()

data_limit = toMemorySize(args.data_limit)

cache_hierarchy = MESITwoLevelCacheHierarchy(
    l1i_size="64KiB",
    l1i_assoc="8",
    l1d_size="64KiB",
    l1d_assoc="8",
    l2_size="512KiB",
    l2_assoc="4",
    num_l2_banks=1,
)

if args.mem_type == "lpddr3":
    memory = SingleChannelLPDDR3_1600()
elif args.mem_type == "ddr3":
    memory = SingleChannelDDR3_1600()
elif args.mem_type == "ddr4":
    memory = SingleChannelDDR4_2400()
elif args.mem_type == "hbm":
    memory = SingleChannelHBM()
else:
    fatal("Memory type not supported")

generator = ComplexGenerator(num_cores=1)

motherboard = TestBoard(
    clk_freq="4GHz",
    processor=generator,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)
motherboard.connect_things()
mem_range = memory.get_memory_ranges()[0]
mem_size = mem_range.end - mem_range.start

root = Root(full_system=False, system=motherboard)
m5.instantiate()
generator.add_linear(
    duration = "1ms", rate="100GB/s", min_addr=0, max_addr=mem_size, data_limit=0x80000
)
generator.add_linear(
    duration = "1ms", rate="100GB/s", min_addr=0, max_addr=mem_size, data_limit=0x10000
)
generator.add_linear(
    duration = "1ms", rate="1000GB/s", min_addr=0, max_addr=mem_size, data_limit=data_limit
)

generator.start_traffic()
print("Warming up the L2 cache!")
exit_event = m5.simulate()
print("Exiting @ tick %i because %s" % (m5.curTick(), exit_event.getCause()))
generator.start_traffic()
print("Warming up the L1 cache")
exit_event = m5.simulate()
print("Exiting @ tick %i because %s" % (m5.curTick(), exit_event.getCause()))
generator.start_traffic()
m5.stats.reset()
print("Running the actual simulation")
exit_event = m5.simulate()
print("Exiting @ tick %i because %s" % (m5.curTick(), exit_event.getCause()))
