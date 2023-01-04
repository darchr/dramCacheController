# -*- coding: utf-8 -*-
# Copyright (c) 2016 Jason Lowe-Power
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
#
# Authors: Jason Lowe-Power

import m5
from m5.objects import *


class MyRubySystem(System):

    def __init__(self, mem_sys, num_cpus, opts, restore=False):
        super(MyRubySystem, self).__init__()
        self._opts = opts

        # Set up the clock domain and the voltage domain
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '5GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        self.mem_ranges = [AddrRange(Addr('32GiB'))]

        # self.intrctrl = IntrControl()
        self.createMemoryControllersDDR3()

        # Create the cache hierarchy for the system.
        if mem_sys == 'MI_example':
            from .MI_example_caches import MIExampleSystem
            self.caches = MIExampleSystem()
        elif mem_sys == 'MESI_Two_Level':
            from .MESI_Two_Level import MESITwoLevelCache
            self.caches = MESITwoLevelCache()
        elif mem_sys == 'MOESI_CMP_directory':
            from .MOESI_CMP_directory import MOESICMPDirCache
            self.caches = MOESICMPDirCache()

        self.reader = DRTraceReader(
            directory=f"/projects/google-traces/{opts.workload}/", num_players=num_cpus
        )

        self.players = [
            DRTracePlayer(
                reader=self.reader,
                send_data=True,
                compress_address_range=self.mem_ranges[0],
            )
            for _ in range(num_cpus)
        ]

        self.caches.setup(self, self.players, self.mem_ctrl)

        self.caches.access_backing_store = True
        self.caches.phys_mem = SimpleMemory(range=self.mem_ranges[0],
                                           in_addr_map=False)


    def createMemoryControllersDDR3(self):
        self._createMemoryControllers(1, DDR3_1600_8x8)

    def _createMemoryControllers(self, num, cls):

        self.mem_ctrl = PolicyManager(range=self.mem_ranges[0], kvm_map=False)

        # FOR DDR4
        # self.mem_ctrl.tRP = '14.16ns'
        # self.mem_ctrl.tRCD_RD = '14.16ns'
        # self.mem_ctrl.tRL = '14.16ns'

        # FOR HBM2
        self.mem_ctrl.tRP = '14ns'
        self.mem_ctrl.tRCD_RD = '12ns'
        self.mem_ctrl.tRL = '18ns'

        # HBM2 cache
        self.loc_mem_ctrl = HBMCtrl()
        self.loc_mem_ctrl.dram =  HBM_2000_4H_1x64(range=AddrRange(start = '0', end = '3GiB', masks = [1 << 6], intlvMatch = 0), in_addr_map=False, kvm_map=False, null=True)
        self.loc_mem_ctrl.dram_2 =  HBM_2000_4H_1x64(range=AddrRange(start = '0', end = '3GiB', masks = [1 << 6], intlvMatch = 1), in_addr_map=False, kvm_map=False, null=True)

        # DDR4 cache
        # self.loc_mem_ctrl = MemCtrl()
        # self.loc_mem_ctrl.dram =  DDR4_2400_16x4(range=self.mem_ranges[0], in_addr_map=False, kvm_map=False)

        # Alloy cache
        # self.loc_mem_ctrl = MemCtrl()
        # self.loc_mem_ctrl.dram =  DDR4_2400_16x4_Alloy(range=self.mem_ranges[0], in_addr_map=False, kvm_map=False)

        # main memory
        self.far_mem_ctrl = MemCtrl()
        self.far_mem_ctrl.dram = DDR4_2400_16x4(range=self.mem_ranges[0], in_addr_map=False, kvm_map=False)

        self.loc_mem_ctrl.port = self.mem_ctrl.loc_req_port
        self.far_mem_ctrl.port = self.mem_ctrl.far_req_port

        self.mem_ctrl.orb_max_size = 128
        self.mem_ctrl.dram_cache_size = "128MiB"

        self.loc_mem_ctrl.dram.read_buffer_size = 32
        self.loc_mem_ctrl.dram.write_buffer_size = 32
        self.loc_mem_ctrl.dram_2.read_buffer_size = 32
        self.loc_mem_ctrl.dram_2.write_buffer_size = 32

        self.far_mem_ctrl.dram.read_buffer_size = 64
        self.far_mem_ctrl.dram.write_buffer_size = 64

        # DDR4 no cache
        # self.mem_ctrl = MemCtrl()
        # self.mem_ctrl.dram =  DDR4_2400_16x4(range=self.mem_ranges[0])
        # self.mem_ctrl.dram.read_buffer_size = 64
        # self.mem_ctrl.dram.write_buffer_size = 64

        # HBM2 no cache
        # self.mem_ctrl = HBMCtrl()
        # self.mem_ctrl.dram =  HBM_2000_4H_1x64(range=AddrRange(start = '0', end = '3GiB', masks = [1 << 6], intlvMatch = 0))
        # self.mem_ctrl.dram_2 =  HBM_2000_4H_1x64(range=AddrRange(start = '0', end = '3GiB', masks = [1 << 6], intlvMatch = 1))
        # self.mem_ctrl.dram.read_buffer_size = 32
        # self.mem_ctrl.dram.write_buffer_size = 32
        # self.mem_ctrl.dram_2.read_buffer_size = 32
        # self.mem_ctrl.dram_2.write_buffer_size = 32


    