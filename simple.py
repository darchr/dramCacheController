from m5.objects import *
import m5

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = DcacheCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8(range=AddrRange('8GB'),
#system.mem_ctrl.dram = DDR4_2400_16x4(range=AddrRange('32GB'),
#system.mem_ctrl.dram = HBM_1000_4H_1x128(range=AddrRange('128MiB'),
in_addr_map=False)
system.mem_ctrl.nvm = NVM_2400_1x64(range=AddrRange('8GB'))

system.mem_ctrl.dram.tREFI = "200"
system.mem_ctrl.orb_max_size = "1024"
system.mem_ctrl.crb_max_size = "32"

system.mem_ranges = [AddrRange('8GB')]

system.generator.port = system.mem_ctrl.port

def createRandomTraffic(tgen):
    yield tgen.createRandom(100000000000,   # duration
                            0,              # min_addr
                            AddrRange('1GB').end,              # max_adr
                            64,             # block_size
                            500,          # min_period
                            500,          # max_period
                            70,             # rd_perc
                            0)              # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(1000000000000,   # duration
                            0,              # min_addr
                            AddrRange('512MiB').end,  # max_adr
                            64,             # block_size
                            250,          # min_period
                            250,          # max_period
                            70,             # rd_perc
                            0)              # data_limit
    yield tgen.createExit(0)


root = Root(full_system=False, system=system)

m5.instantiate()
system.generator.start(createLinearTraffic(system.generator))
exit_event = m5.simulate()