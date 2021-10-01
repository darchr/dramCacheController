import sys
sys.path.append("gem5")
from components_library.memory.single_channel import SingleChannelDDR3_1600
from components_library.memory.single_channel import SingleChannelDDR4_2400
from components_library.memory.single_channel import SingleChannelLPDDR3_1600
from components_library.memory.single_channel import SingleChannelHBM

from components_library.memory.dramsim_3 import SingleChannelDDR3_1600 as SingleChannelDS3DDR3_1600
from components_library.memory.dramsim_3 import SingleChannelDDR4_2400 as SingleChannelDS3DDR4_2400
from components_library.memory.dramsim_3 import SingleChannelLPDDR3_1600 as SingleChannelDS3LPDDR3_1600
from components_library.memory.dramsim_3 import SingleChannelHBM as SingleChannelDS3HBM


def GetMemClass(simulator, mem_type):
    return MemInfoDict[simulator][mem_type]

MemInfoDict = {
    'gem5' : {
        'ddr3' : SingleChannelDDR3_1600,
        'ddr4' : SingleChannelDDR4_2400,
        'lpddr3' : SingleChannelLPDDR3_1600,
        'hbm' : SingleChannelHBM
    },
    'DRAMSim3': {
        'ddr3' : SingleChannelDS3DDR3_1600,
        'ddr4' : SingleChannelDS3DDR4_2400,
        'lpddr3' : SingleChannelDS3LPDDR3_1600,
        'hbm' : SingleChannelDS3HBM
    }
}