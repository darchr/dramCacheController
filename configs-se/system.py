import m5
from m5.objects import *
from optparse import OptionParser
from argparse import Namespace

MemTypes = {
    'ddr3_1600' : DDR3_1600_8x8,
    'ddr4_2400' : DDR4_2400_16x4,
    'ddr5_6800' : DDR5_6800_2x8,
    'hbm_1000': HBM_1000_4H_1x128,
    'nvm_2400' : NVM_2400_1x64,
    'nvm_300' : NVM_300_1x64
}

CPUTypes = {
    'timing' : TimingSimpleCPU,
    'minor' : MinorCPU,
    'o3' : DerivO3CPU
}

class MySystem(System):
    """
    A simple system to simulate
    """

    def __init__(self, dram, nvm, cpu):

        super(MySystem,self).__init__()
        self.clk_domain = SrcClockDomain(clock = "4GHz",
                                            voltage_domain = VoltageDomain())
        self.mem_mode = 'timing'
        #self.mem_ranges = [AddrRange('2GB')]


        self.cpu = CPUTypes[cpu]()
        self.membus = SystemXBar(width=64)
        self.mem_ranges = [AddrRange('1024MB')]

        self.mem_ctrl = DcacheCtrl()()
        #self.mem_ctrl.dram = DDR4_2400_8x8(range=self.mem_ranges[0])

        self.mem_ctrl.dram = MemTypes[dram](range=AddrRange('8GB'),
                                                in_addr_map=False)
        self.mem_ctrl.nvm = MemTypes[nvm](range=AddrRange('8GB'))
        self.mem_ctrl.dram.tREFI = "200"
        self.mem_ctrl.dram_cache_size = "256MB"
        self.mem_ctrl.orb_max_size = "128" 
        self.mem_ctrl.crb_max_size = "32"

        #self.mem_ranges = [AddrRange('1GB')]

        self.cpu.icache_port = self.membus.slave
        self.cpu.dcache_port = self.membus.slave

        self.mem_ctrl.port = self.membus.master
        self.system_port = self.membus.slave

        self.cpu.createInterruptController()
        if m5.defines.buildEnv['TARGET_ISA'] == "x86":
            #self.interrupt_xbar = SystemXBar()
            self.cpu.interrupts[0].pio = self.membus.master
            self.cpu.interrupts[0].int_master = self.membus.slave
            self.cpu.interrupts[0].int_slave = self.membus.master

    def setTestBinary(self, binary_path):
        """Set up the SE process to execute the binary at binary_path"""
        from m5 import options
        print(binary_path)

        benchmark = binary_path.split()[0]
        print(benchmark)

        arguments = binary_path.split()[1:]

        self.workload = SEWorkload.init_compatible(benchmark)
        self.cpu.workload = Process(
                                    cmd = [benchmark] + arguments)
        self.cpu.createThreads()
