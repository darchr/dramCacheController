from m5.objects import *
import m5
import argparse

args = argparse.ArgumentParser()

# This scipt takes these arguments [device model for dram cache]
# [dram cache size] [maximum orb size]
# [traffic mode] [duration of simulation in ticks]
# [max address] [request injection period in ticks] [rd percentage]
# min address is 0, data limit is 0, block size is 64B.
# crb_max_size is 32 by default.

# sample cmd: gem5.opt traffGen.py DDR3_1600_8x8  16MiB
#             32 linear 100000000 128MiB 1000 100
# sample cmd: gem5.opt traffGen.py DDR4_2400_16x4 1GB
#             32 random 100000000 2GB    1000 100

args.add_argument(
    "dram",
    type = str,
    help = "Memory device to use as a dram cache"
)

args.add_argument(
    "dram_cache_size",
    type = str,
    help = "Duration of simulation"
)

args.add_argument(
    "max_orb",
    type = int,
    help = "Duration of simulation"
)

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

args.add_argument(
    "nvm",
    type = str,
    help = "NVM interface to use"
)

options = args.parse_args()

MemTypes = {
    'ddr3_1600' : DDR3_1600_8x8,
    'ddr4_2400' : DDR4_2400_16x4,
    'ddr5_6800' : DDR5_6800_2x8,
    'ddr5_8400' : DDR5_8400_2x8,
    'hbm_1000' : HBM_1000_4H_1x128,
    'hbm_ddr3' : HBM_FROM_DDR3,
    'nvm_2400' : NVM_2400_1x64,
    'nvm_300' : NVM_300_1x64
}

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = DcacheCtrl()

system.mem_ctrl.dram = MemTypes[options.dram](range=AddrRange('8GB'),
                                                in_addr_map=False)
system.mem_ctrl.nvm = MemTypes[options.nvm](range=AddrRange('8GB'))

system.mem_ctrl.dram.tREFI = "200"
system.mem_ctrl.dram_cache_size = options.dram_cache_size
system.mem_ctrl.orb_max_size = options.max_orb
system.mem_ctrl.crb_max_size = "32"

system.mem_ranges = [AddrRange('8GB')]

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
