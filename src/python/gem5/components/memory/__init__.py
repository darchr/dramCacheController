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

from .single_channel import SingleChannelDDR3_1600
from .single_channel import SingleChannelDDR3_2133
from .single_channel import SingleChannelDDR4_2400
from .single_channel import SingleChannelHBM
from .single_channel import SingleChannelLPDDR3_1600
from .single_channel import DIMM_DDR5_4400
from .single_channel import DIMM_DDR5_6400
from .single_channel import DIMM_DDR5_8400
from .multi_channel import DualChannelDDR3_1600
from .multi_channel import DualChannelDDR3_2133
from .multi_channel import DualChannelDDR4_2400
from .multi_channel import DualChannelLPDDR3_1600
from .hbm import HBM2Stack
from .dcache import CascadeLakeCache
from .dcache import OracleCache
from .dcache import RambusCache

try:
    from .dramsys import DRAMSysMem
    from .dramsys import DRAMSysDDR4_1866
    from .dramsys import DRAMSysDDR3_1600
    from .dramsys import DRAMSysLPDDR4_3200
    from .dramsys import DRAMSysHBM2
except:
    # In the case that DRAMSys is not compiled into the gem5 binary, importing
    # DRAMSys components will fail. This try-exception statement is needed to
    # ignore these imports in this case.
    pass
