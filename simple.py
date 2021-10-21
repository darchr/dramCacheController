from m5.objects import *
import m5
import argparse

args = argparse.ArgumentParser()

# This scipt takes these arguments [traffic mode] [address range]
# [rd percentage] [duration of simulation in ticks]
# [request injection period in ticks] [device model for dram cache]

# sample cmd: gem5.opt simple.py linear 4GB 70 1000000000 10000 DDR3_1600_8x8

args.add_argument(
    "traffic_mode",
    type = str,
    help = "Traffic type to use"
)

args.add_argument(
    "address",
    type=str,
    help="End address of the range to be accessed",
)

args.add_argument(
    "rd_prct",
    type=int,
    help="Read Percentage",
)

args.add_argument(
    "duration",
    type = int,
    help = "Duration of simulation"
)

args.add_argument(
    "inj_period",
    type = int,
    help = "Period to inject reqs"
)

args.add_argument(
    "device",
    type = str,
    help = "Memory device to use as a dram cache"
)

options = args.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = DcacheCtrl()
system.mem_ctrl.dram = eval(options.device)(range=AddrRange('8GB'),
                                                in_addr_map=False)
system.mem_ctrl.nvm = NVM_2400_1x64(range=AddrRange('8GB'))

system.mem_ctrl.dram.tREFI = "200"
system.mem_ctrl.orb_max_size = "1024"
system.mem_ctrl.crb_max_size = "32"

system.mem_ranges = [AddrRange('8GB')]

system.generator.port = system.mem_ctrl.port

def createRandomTraffic(tgen):
    yield tgen.createRandom(options.duration,   # duration
                            0,              # min_addr
                            AddrRange(options.address).end,  # max_adr
                            64,             # block_size
                            options.inj_period,       # min_period
                            options.inj_period,       # max_period
                            options.rd_prct,          # rd_perc
                            0)              # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(options.duration,   # duration
                            0,              # min_addr
                            AddrRange(options.address).end,  # max_adr
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
