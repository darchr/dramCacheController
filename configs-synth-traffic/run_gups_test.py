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
from mem_info import MemInfo

sys.path.append("gem5")

from components_library.boards.test_board import TestBoard
from components_library.processors.gups_generator import GUPSGenerator
from components_library.cachehierarchies.private_l1_private_2_cache_hierarchy import PrivateL1PrivateL2CacheHierarchy



parser = argparse.ArgumentParser()
parser.add_argument("mem_type", type=str, help="type of memory to simulate")
parser.add_argument(
    "simulator", type=str, help="the simulator to use for memory"
)
parser.add_argument("update_limit", type=int, default=0)
args = parser.parse_args()

memory_class = MemInfo(args.simulator, args.mem_type)
memory = memory_class()

cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size="32KiB", l1i_size="32KiB", l2_size="1MiB"
)

mem_range = memory.get_memory_ranges()[0]
min_addr = mem_range.start
max_addr = mem_range.end
start_addr = min_addr
mem_size = str(int((max_addr - min_addr) / 32)) + "B"
print("start_addr", start_addr, "mem_size: ", mem_size)

generator = GUPSGenerator(
    start_addr=start_addr,
    mem_size=mem_size,
    update_limit=args.update_limit,
)

motherboard = TestBoard(
    clk_freq="4GHz",
    processor=generator,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)
motherboard.connect_things()

root = Root(full_system=False, system=motherboard)
m5.instantiate()
generator.start_traffic()
print("Beginning simulation!")
exit_event = m5.simulate()
print("Exiting @ tick %i because %s" % (m5.curTick(), exit_event.getCause()))
