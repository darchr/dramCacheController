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

from typing import Optional

from gem5.components.processors.abstract_generator_core import (
    AbstractGeneratorCore,
)
from gem5.components.processors.abstract_core import AbstractCore
from gem5.utils.override import overrides

from m5.objects import (
    AddrRange,
    DRTracePlayer,
    DRTraceReader,
    Port,
    SrcClockDomain,
    VoltageDomain,
)


class DRTracePlayerCore(AbstractGeneratorCore):
    def __init__(
        self,
        max_ipc: int,
        max_outstanding_reqs: int,
        clk_freq: Optional[str] = None,
    ):
        super().__init__()
        self.player = DRTracePlayer(
            max_ipc=max_ipc,
            max_outstanding_reqs=max_outstanding_reqs,
            send_data=True,
        )
        if clk_freq:
            clock_domain = SrcClockDomain(
                clock=clk_freq, voltage_domain=VoltageDomain()
            )
            self.generator.clk_domain = clock_domain

    @overrides(AbstractCore)
    def connect_dcache(self, port: Port) -> None:
        self.player.port = port

    def set_reader(self, reader: DRTraceReader):
        self.player.reader = reader

    def set_memory_range(self, range: AddrRange):
        self.player.compress_address_range = range
