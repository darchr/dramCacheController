# Copyright (c) 2022 The Regents of the University of California
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

""" DRAM Cache based memory system
    Uses Policy Manager and two other memory systems
"""

from .memory import ChanneledMemory
from .abstract_memory_system import AbstractMemorySystem
from ..boards.abstract_board import AbstractBoard
from math import log
from ...utils.override import overrides
from m5.objects import AddrRange, DRAMInterface, MemCtrl, Port, PolicyManager, L2XBar
from typing import Type, Optional, Union, Sequence, Tuple, List
from .memory import _try_convert
#from .dram_interfaces.hbm import HBM_2000_4H_1x64
from .dram_interfaces.ddr4 import DDR4_2400_8x8
from .multi_channel import DualChannelDDR4_2400
from .single_channel import SingleChannelDDR4_2400

class DCacheSystem(AbstractMemorySystem):
    """
    This class extends ChanneledMemory and can be used to create HBM based
    memory system where a single physical channel contains two pseudo channels.
    This is supposed to be used with the HBMCtrl and two dram (HBM2) interfaces
    per channel.
    """

    def __init__(
        self,
        loc_mem: Type[ChanneledMemory],
        far_mem: Type[ChanneledMemory],
        loc_mem_policy: [str] = None,
        size: [str] = None,
    ) -> None:
        """
        :param loc_mem_policy: DRAM cache policy to be used
        :param size: Optionally specify the size of the DRAM controller's
            address space. By default, it starts at 0 and ends at the size of
            the DRAM device specified
        """
        super().__init__()

        #self.loc_mem = _try_convert(loc_mem, ChanneledMemory)
        self.loc_mem = loc_mem()
        self.far_mem = far_mem()
        print(type(self.loc_mem))
        self._size = size

        self.loc_mem._dram[0].in_addr_map = False
        self.far_mem._dram[0].in_addr_map = False

        self.loc_mem._dram[0].range = AddrRange('1GiB')
        self.far_mem._dram[0].range = AddrRange('1GiB')

        self.loc_mem._dram[0].null = True
        self.far_mem._dram[0].null = True

       #_num_channels = _try_convert(num_channels, int)

        print("whatever")

        self.policy_manager = PolicyManager(range=AddrRange('1GiB'))
        self.policy_manager.loc_mem_policy = loc_mem_policy
        self.policy_manager.bypass_dcache = False
        self.policy_manager.tRP = '14.16ns'
        self.policy_manager.tRCD_RD = '14.16ns'

        self.policy_manager.tRL = '14.16ns'

        self.policy_manager.always_hit = False
        self.policy_manager.always_dirty = True

        self.policy_manager.dram_cache_size = "64MiB"


        #self._farMemXBar = L2XBar(width=64)

        #self._nearMemXBar = L2XBar(width=64)

        #self.policy_manager.far_req_port = self._farMemXBar.cpu_side_ports

        #self.policy_manager.loc_req_port = self._nearMemXBar.cpu_side_ports

        self.policy_manager.loc_req_port = self.loc_mem.get_memory_controllers()[0].port

        self.policy_manager.far_req_port = self.far_mem.get_memory_controllers()[0].port

        #print(self.loc_mem.get_size())
        print(self.loc_mem.get_memory_controllers())

        #self._nearMemXBar.mem_side_ports = self.loc_mem.get_memory_controllers()[0].port

        #self._farMemXBar.mem_side_ports = self.far_mem.get_memory_controllers()[0].port

    @overrides(AbstractMemorySystem)
    def get_size(self) -> int:
        return self._size

    @overrides(AbstractMemorySystem)
    def set_memory_range(self, ranges: List[AddrRange]) -> None:
        """Need to add support for non-contiguous non overlapping ranges in
        the future.

        if len(ranges) != 1 or ranges[0].size() != self._size:
            raise Exception(
                "Multi channel memory controller requires a single range "
                "which matches the memory's size.\n"
                f"The range size: {range[0].size()}\n"
                f"This memory's size: {self._size}"
            )
        """
        self._mem_range = ranges[0]
        #self._interleave_addresses()

    @overrides(AbstractMemorySystem)
    def incorporate_memory(self, board: AbstractBoard) -> None:
        pass

    @overrides(AbstractMemorySystem)
    def get_memory_controllers(self):
        return [self.policy_manager,]
        #return [self.loc_mem.get_memory_controllers()[0], self.far_mem.get_memory_controllers()[0]]

def DualChannelDDR4_2400(
    size: Optional[str] = None,
) -> AbstractMemorySystem:
    """
    A dual channel memory system using DDR4_2400_8x8 based DIMM
    """
    return ChanneledMemory(
        DDR4_2400_8x8,
        2,
        64,
        size=size,
    )

def BaseDCache(
    size: Optional[str] = "4GiB",
) -> AbstractMemorySystem:
    return DCacheSystem(SingleChannelDDR4_2400, SingleChannelDDR4_2400, 'CascadeLakeNoPartWrs', size='128MiB')
