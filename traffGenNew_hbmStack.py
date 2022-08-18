from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.NVMInterface import *

args = argparse.ArgumentParser()


args.add_argument(
    "xbarLatency",
    type = int,
    help = "latency of crossbar front/resp"
)
args.add_argument(
    "device_loc",
    type = str,
    help = "Memory device to use as a dram cache (local memory)"
)

args.add_argument(
    "device_far",
    type = str,
    help = "Memory device to use as a backing store (far memory)"
)

args.add_argument(
    "dram_cache_size",
    type = str,
    help = "Duration of simulation"
)

args.add_argument(
    "far_mem_size",
    type = str,
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

options = args.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.membus[0] = SystemXBar()
system.membus[1] = SystemXBar()
system.membus[1].frontend_latency = options.xbarLatency
system.membus[1].response_latency  = options.xbarLatency

system.generator.port = system.membus[0].cpu_side_ports

loc_ranges = ['0', '1GB', '2GB', '3GB', '4GB', '5GB', '6GB', '7GB', '8GB']
for i in range (0,8):
    system.dcache_ctrl[i] = DCacheCtrl()
    system.dcache_ctrl[i].dram = eval(options.device_loc)(range=AddrRange(start = loc_ranges[i], end = loc_ranges[i+1]), in_addr_map=False)
    system.dcache_ctrl[i].dram_cache_size = options.dram_cache_size
    system.dcache_ctrl[i].always_hit = False
    system.dcache_ctrl[i].always_dirty = True
    system.membus[0].mem_side_ports = system.dcache_ctrl[i].port
    system.membus[1].cpu_side_ports = system.dcache_ctrl[i].req_port

far_ranges = ['0', '4GB', '8GB']
for i in range (0,2):
    system.farMem_ctrl[i] = MemCtrl()
    system.farMem_ctrl[i].dram = eval(options.device_far)(range=AddrRange(start = far_ranges[i], end = far_ranges[i+1]))
    system.farMem_ctrl[i].dram.device_size = options.far_mem_size
    system.membus[1].mem_side_ports = system.farMem_ctrl[i].port

system.mem_ranges = [AddrRange('8GB')]


def createRandomTraffic(tgen):
    yield tgen.createRandom(options.duration,         # duration
                            0,                        # min_addr
                            AddrRange(options.max_address).end,  # max_adr
                            64,                       # block_size
                            options.inj_period,       # min_period
                            options.inj_period,       # max_period
                            options.rd_prct,          # rd_perc
                            0)                        # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(options.duration,         # duration
                            0,                        # min_addr
                            AddrRange(options.max_address).end,  # max_adr
                            64,                       # block_size
                            options.inj_period,       # min_period
                            options.inj_period,       # max_period
                            options.rd_prct,          # rd_perc
                            0)             # data_limit
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