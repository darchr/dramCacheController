
from gem5.components.memory import DualChannelDDR4_2400
from gem5.components.memory import SingleChannelDDR3_1600
from gem5.components.memory import SingleChannelDDR4_2400
from gem5.components.memory import SingleChannelLPDDR3_1600
from gem5.components.memory import SingleChannelHBM
from gem5.components.memory import SingleChannelHBM_DDR3
from gem5.components.memory import SingleChannelHBM_DDR3_v2
from gem5.components.memory import SingleChannelHBMPseudo
from gem5.components.memory import HBM2Stack
from gem5.components.memory import HBM2StackLegacy
from gem5.components.memory import HBM2StackPseudo
from gem5.components.memory import HBM2Stackv2
from gem5.components.memory import HBM2Stackv3

def GetMemClass(simulator, mem_type):
    return MemInfoDict[simulator][mem_type]

MemInfoDict = {
    'gem5' : {
        'ddr3' : SingleChannelDDR3_1600,
        'ddr4' : SingleChannelDDR4_2400,
        'lpddr3' : SingleChannelLPDDR3_1600,
        'hbm' : SingleChannelHBM,
        'hbm_pseudo' : SingleChannelHBMPseudo,
        'hbm_from_ddr3' : SingleChannelHBM_DDR3,
        'hbm_from_ddr3_v2' : SingleChannelHBM_DDR3_v2,
        'hbm_stack' : HBM2Stack,
        'hbm_stack_v2' : HBM2Stackv2,
        'hbm_stack_v3' : HBM2Stackv3,
        'hbm2_stack_pseudo' : HBM2StackPseudo,
        'hbm_2_stack_legacy' : HBM2StackLegacy
    }
}
