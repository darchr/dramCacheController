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

"""Single channel "generic" DDR memory controllers
"""

from ..boards.abstract_board import AbstractBoard
from .abstract_memory_system import AbstractMemorySystem
from ..utils.override import overrides

from m5.objects import (AddrRange, DRAMInterface, DRAMDCInterface,
                        NVMDCInterface, MemCtrl, DcacheCtrl, Port)

from typing import List, Sequence, Tuple, Type, Optional

class SingleChannelDCMemory(AbstractMemorySystem):
    """A simple implementation of a single channel memory system which
        also uses a DRAM cache

    This class can take a DRAM Interface as a parameter to model many different
    DDR memory systems as a cache for a NVM main memory.
    """

    def __init__(
        self,
        dram_interface_class: Type[DRAMDCInterface],
        nvm_interface_class: Type[NVMDCInterface],
        size_dcache: Optional[str] = None,
        size_nvm: Optional[str] = None,
    ):
        """
        :param dram_interface_class: The DRAM interface type (used as cache)
         to create with this memory controller
        :param nvm_interface_class: The NVM interface type (used as main
         memory) to create with this memory controller
        :param size_dcache: Optionally specify the size of the DRAM
            controller's address space. By default, it starts at 0 and ends
            at the size of the DRAM device specified
        :param size_nvm: Optionally specify the size of the DRAM controller's
            address space. By default, it starts at 0 and ends at the size of
            the DRAM device specified
        """
        super().__init__()

        self._dram = dram_interface_class()
        self._nvm = nvm_interface_class()

        if size_dcache:
            self._dram.range = size
        else:
            self._dram.range = AddrRange(self.get_size_dcache(self._dram))

        if size_nvm:
            self._nvm.range = size
        else:
            self._nvm.range = AddrRange(self.get_size_nvm(self._nvm))

        self._dram.in_addr_map = False

        self.mem_ctrl = DcacheCtrl(dram = self._dram, nvm = self._nvm)

        self.mem_ctrl.dram.tREFI = "1000"
        self.mem_ctrl.orb_max_size = "500"
        self.mem_ctrl.crb_max_size = "32"



    def get_size_dcache(self, dram: DRAMDCInterface) -> int:
        return (
            dram.device_size.value
            * dram.devices_per_rank.value
            * dram.ranks_per_channel.value
        )

    def get_size_nvm(self, nvm: NVMDCInterface) -> int:
        return (
            nvm.device_size.value
            * nvm.devices_per_rank.value
            * nvm.ranks_per_channel.value
        )

    @overrides(AbstractMemorySystem)
    def incorporate_memory(self, board: AbstractBoard) -> None:
        pass

    @overrides(AbstractMemorySystem)
    def get_mem_ports(self) -> Tuple[Sequence[AddrRange], Port]:
        return [(self._dram.range, self.mem_ctrl.port)]

    @overrides(AbstractMemorySystem)
    def get_memory_controllers(self) -> List[DcacheCtrl]:
        return [self.mem_ctrl]

    @overrides(AbstractMemorySystem)
    def get_memory_ranges(self):
        return [self._dram.range]


from .dc_interfaces.ddr3 import DDR3_1600_8x8, DDR3_2133_8x8
from .dc_interfaces.ddr4 import DDR4_2400_8x8
from .dc_interfaces.hbm import HBM_1000_4H_1x128
from .nvm_interfaces.nvm import NVM_2400_1x64

# Enumerate all of the different DDR memory systems we support
def SingleChannelDDR3_1600_NVM_2400_1x64(size_dcache: Optional[str] = None,
                size_nvm: Optional[str] = None) -> AbstractMemorySystem:
    """
    A single channel memory system using a single DDR3_1600_8x8 based DIMM
    """
    return SingleChannelDCMemory(DDR3_1600_8x8, NVM_2400_1x64, size_dcache,
                                                                    size_nvm)


def SingleChannelDDR3_2133_NVM_2400_1x64(size_dcache: Optional[str] = None,
                size_nvm: Optional[str] = None) -> AbstractMemorySystem:
    """
    A single channel memory system using a single DDR3_2133_8x8 based DIMM
    """
    return SingleChannelDCMemory(DDR3_2133_8x8, NVM_2400_1x64, size_dcache,
                                                                    size_nvm)


def SingleChannelDDR4_2400_NVM_2400_1x64(size_dcache: Optional[str] = None,
                size_nvm: Optional[str] = None) -> AbstractMemorySystem:
    """
    A single channel memory system using a single DDR4_2400_8x8 based DIMM
    """
    return SingleChannelDCMemory(DDR4_2400_8x8, NVM_2400_1x64, size_dcache,
                                                                    size_nvm)

def SingleChannelHBM_NVM_2400_1x64(size_dcache: Optional[str] = None,
                size_nvm: Optional[str] = None) -> AbstractMemorySystem:
    return SingleChannelDCMemory(HBM_1000_4H_1x128, NVM_2400_1x64, size_dcache,
                                                                    size_nvm)
