from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.NVMInterface import *

args = argparse.ArgumentParser()

# build/NULL/gem5.opt --outdir=m5outHBMstack   traffGenNew_hbmStack.py  200  random  10000000000  12000   100
args.add_argument(
    "xbarLatency",
    type = int,
    help = "latency of crossbar front/resp"
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
system.clk_domain.clock = "8GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = [PyTrafficGen() for i in range(4)]

system.membus = [L2XBar(width=64) for i in range(2)]

system.membus[0] = L2XBar(width=64)
system.membus[1] = L2XBar(width=64)
system.membus[1].frontend_latency = options.xbarLatency
system.membus[1].response_latency  = options.xbarLatency

for i in range(0,4):
    system.generator[i].port = system.membus[0].cpu_side_ports

loc_ranges = ['0', '4GB', '8GB', '12GB', '16GB', '20GB', '24GB', '28GB', '32GB', '36GB', '40GB', '44GB', '48GB', '52GB', '56GB', '60GB', '64GB']

#loc_ranges2 = ['0', '8GB', '16GB', '24GB', '32GB', '40GB', '48GB', '56GB', '64GB']

system.dcache_ctrl = [DCacheCtrl() for i in range(16)]

for i in range (0,16):
    #system.dcache_ctrl[i] = DCacheCtrl()
    system.dcache_ctrl[i].dram = HBM_1000_4H_1x128(range=AddrRange(start = loc_ranges[i], end = loc_ranges[i+1]), in_addr_map=False)
    system.dcache_ctrl[i].dram_cache_size = '128MiB'
    system.dcache_ctrl[i].orb_max_size = 128
    system.dcache_ctrl[i].always_hit = False
    system.dcache_ctrl[i].always_dirty = True
    system.membus[0].mem_side_ports = system.dcache_ctrl[i].port
    system.membus[1].cpu_side_ports = system.dcache_ctrl[i].req_port

far_ranges = ['0', '32GB', '64GB']

system.farMem_ctrl = [MemCtrl() for i in range(2)]

for i in range (0,2):
    #system.farMem_ctrl[i] = MemCtrl()
    system.farMem_ctrl[i].dram = DDR4_2400_16x4(range=AddrRange(start = far_ranges[i], end = far_ranges[i+1]))
    system.membus[1].mem_side_ports = system.farMem_ctrl[i].port


def createRandomTraffic(tgen, start, end):
    yield tgen.createRandom(options.duration,         # duration
                            start,                        # min_addr
                            end,  # max_adr
                            64,                       # block_size
                            options.inj_period,       # min_period
                            options.inj_period,       # max_period
                            options.rd_prct,          # rd_perc
                            0)                        # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen, start, end):
    yield tgen.createLinear(options.duration,         # duration
                            start,                        # min_addr
                            end,  # max_adr
                            64,                       # block_size
                            options.inj_period,       # min_period
                            options.inj_period,       # max_period
                            options.rd_prct,          # rd_perc
                            0)             # data_limit
    yield tgen.createExit(0)

startList = []
endList = []


root = Root(full_system=False, system=system)

m5.instantiate()

if options.traffic_mode == 'linear':
    system.generator[0].start(createLinearTraffic(system.generator[0], 0, (17179869184-1)))
    system.generator[1].start(createLinearTraffic(system.generator[1], 17179869184, 17179869184*2-1))
    system.generator[2].start(createLinearTraffic(system.generator[2], 17179869184*2, 17179869184*3-1))
    system.generator[3].start(createLinearTraffic(system.generator[3], 17179869184*3, 17179869184*4-1))
elif options.traffic_mode == 'random':
    system.generator[0].start(createRandomTraffic(system.generator[0], 0, (17179869184-1)))
    system.generator[1].start(createRandomTraffic(system.generator[1], 17179869184, 17179869184*2-1))
    system.generator[2].start(createRandomTraffic(system.generator[2], 17179869184*2, 17179869184*3-1))
    system.generator[3].start(createRandomTraffic(system.generator[3], 17179869184*3, 17179869184*4-1))
else:
    print('Wrong traffic type! Exiting!')
    exit()

exit_event = m5.simulate()