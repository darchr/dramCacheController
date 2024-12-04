from m5.objects import *
import m5

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = MemCtrl()
system.mem_ctrl.nvm= NVM_2400_1x64(range=AddrRange('8GB'))

# comment one of these policies
#system.mem_ctrl.mem_sched_policy = "frfcfs"
system.mem_ctrl.mem_sched_policy = "fcfs"

system.mem_ctrl.port = system.generator.port
system.mem_ranges = [AddrRange('8GB')]

def createRandomTraffic(tgen):
    yield tgen.createRandom(100000000000,   # duration
                            0,              # min_addr
                            AddrRange('1GB').end,              # max_adr
                            64,             # block_size
                            10000,          # min_period
                            10000,          # max_period
                            70,             # rd_perc
                            0)              # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(10000000000,   # duration
                            0,              # min_addr
                            AddrRange('8GB').end,  # max_adr
                            64,             # block_size
                            10000,          # min_period
                            10000,          # max_period
                            100,             # rd_perc
                            0)              # data_limit
    yield tgen.createExit(0)


root = Root(full_system=False, system=system)

m5.instantiate()
system.generator.start(createLinearTraffic(system.generator))
exit_event = m5.simulate()
