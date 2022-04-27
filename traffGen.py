from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.NVMInterface import *

args = argparse.ArgumentParser()

# build/NULL/gem5.opt --outdir=m5out traffGen.py
# linear 10000000000 256MiB 1000 67

args.add_argument(
    "traffic_mode",
    type = str,
    help = "Traffic type to use"
)

args.add_argument(
    "duration",
    type = int,
    help = "Duration of simulation"
)

args.add_argument(
    "max_address",
    type=str,
    help="End address of the range to be accessed",
)

args.add_argument(
    "inj_period",
    type = int,
    help = "Period to inject reqs"
)

args.add_argument(
    "rd_prct",
    type=int,
    help="Read Percentage",
)

options = args.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

#system.mem_ctrl = SimpleMemCtrl()
system.mem_ctrl = MemCtrl()

system.mem_ranges = [AddrRange('128MiB'),
                    AddrRange(Addr('128MiB'),
                    size = '128MiB')]

system.mem_ctrl.dram = DDR4_2400_16x4(range=system.mem_ranges[0])

system.mem_ctrl.nvm = NVM_2400_1x64(range=system.mem_ranges[1])
#system.mem_ctrl.dram = NVM_2400_1x64(range=system.mem_ranges[0])

system.generator.port = system.mem_ctrl.port

def createRandomTraffic(tgen):
    yield tgen.createRandom(options.duration,   # duration
                            0,              # min_addr
                            AddrRange(options.max_address).end,  # max_adr
                            64,             # block_size
                            options.inj_period,       # min_period
                            options.inj_period,       # max_period
                            options.rd_prct,          # rd_perc
                            0)              # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(options.duration,   # duration
                            0,              # min_addr
                            AddrRange(options.max_address).end,  # max_adr
                            64,             # block_size
                            options.inj_period,   # min_period
                            options.inj_period,   # max_period
                            options.rd_prct,       # rd_perc
                            0)              # data_limit
    yield tgen.createExit(0)


root = Root(full_system=False, system=system)

m5.instantiate()

if options.traffic_mode == 'linear':
    system.generator.start(createLinearTraffic(system.generator))
elif options.traffic_mode == 'random':
    system.generator.start(createRandomTraffic(system.generator))
else:
    print('Wrong traffic type! Exiting!')
    exit()

exit_event = m5.simulate()