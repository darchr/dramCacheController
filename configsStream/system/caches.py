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

""" Caches with options for a simple gem5 configuration script

This file contains L1 I/D and L2 caches to be used in the simple
gem5 configuration script.
"""

from m5.objects import Cache, L2XBar, StridePrefetcher

# Some specific options for caches
# For all options see src/mem/cache/BaseCache.py

class PrefetchCache(Cache):

    def __init__(self, options):
        super(PrefetchCache, self).__init__()
        if not options:
            return
        self.prefetcher = StridePrefetcher()

class L1Cache(PrefetchCache):
    """Simple L1 Cache with default values"""

    assoc = 8
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 16
    tgts_per_mshr = 20
    writeback_clean = True

    def __init__(self, options=None):
        super(L1Cache, self).__init__(options)
        pass

    def connectBus(self, bus):
        """Connect this cache to a memory-side bus"""
        self.mem_side = bus.cpu_side_ports

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU-side port
           This must be defined in a subclass"""
        raise NotImplementedError

class L1ICache(L1Cache):
    """Simple L1 instruction cache with default values"""

    def __init__(self, opts=None):
        super(L1ICache, self).__init__(opts)
        if not opts:
            return

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU icache port"""
        self.cpu_side = cpu.icache_port

class L1DCache(L1Cache):
    """Simple L1 data cache with default values"""

    def __init__(self, opts=None):
        super(L1DCache, self).__init__(opts)
        if not opts:
            return

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU dcache port"""
        self.cpu_side = cpu.dcache_port

class MMUCache(Cache):
    # Default parameters
    size = '8kB'
    assoc = 4
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 20
    tgts_per_mshr = 12
    writeback_clean = True

    def __init__(self):
        super(MMUCache, self).__init__()

    def connectCPU(self, cpu):
        """Connect the CPU itb and dtb to the cache
           Note: This creates a new crossbar
        """
        self.mmubus = L2XBar()
        self.cpu_side = self.mmubus.mem_side_ports
        cpu.mmu.connectWalkerPorts(
            self.mmubus.cpu_side_ports, self.mmubus.cpu_side_ports)

    def connectBus(self, bus):
        """Connect this cache to a memory-side bus"""
        self.mem_side = bus.cpu_side_ports

class L2Cache(PrefetchCache):
    """Simple L2 Cache with default values"""

    # Default parameters
    assoc = 16
    tag_latency = 10
    data_latency = 10
    response_latency = 1
    mshrs = 20
    tgts_per_mshr = 12
    writeback_clean = True

    def __init__(self, opts=None):
        super(L2Cache, self).__init__(opts)
        if not opts:
            return

    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports

    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports

class L3Cache(Cache):
    """Simple L3 Cache bank with default values
       This assumes that the L3 is made up of multiple banks. This cannot
       be used as a standalone L3 cache.
    """

    # Default parameters
    assoc = 32
    tag_latency = 40
    data_latency = 40
    response_latency = 10
    mshrs = 256
    tgts_per_mshr = 12
    clusivity = 'mostly_excl'

    def __init__(self, opts):
        super(L3Cache, self).__init__()
        #self.size = (opts.l3_size)

    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports

    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports
