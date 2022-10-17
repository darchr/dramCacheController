from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.DRAMAlloyInterface import *
from m5.objects.NVMInterface import *

args = argparse.ArgumentParser()

args.add_argument(
    "traffic_mode",
    type = str,
    help = "Traffic type to use"
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

system.policy_manager = PolicyManager()
system.loc_mem_ctrl = HBMCtrl() #MemCtrl()
system.policy_manager.loc_mem_ctrl = system.loc_mem_ctrl
system.far_mem_ctrl = MemCtrl()
system.policy_manager.far_mem_ctrl = system.far_mem_ctrl

#system.loc_mem_ctrl.dram =  HBM_2000_4H_1x64(range=AddrRange(start = '0', end = '16GiB', masks = [1 << 6], intlvMatch = 0))
system.loc_mem_ctrl.dram =  DDR4_2400_16x4(range=AddrRange('16GiB'))
#system.loc_mem_ctrl.dram_2 =  HBM_2000_4H_1x64(range=AddrRange(start = '0', end = '16GiB', masks = [1 << 6], intlvMatch = 1))
system.policy_manager.dram_cache_size = "1GiB"
# system.loc_mem_ctrl.dram.read_buffer_size = 128
# system.loc_mem_ctrl.dram.write_buffer_size = 128
system.far_mem_ctrl.dram = DDR4_2400_16x4(range=AddrRange('16GiB'))

system.policy_manager.always_hit = True
system.policy_manager.always_dirty = False

# system.mem_ranges = [AddrRange('4GB')]

system.generator.port = system.policy_manager.resp_port

system.policy_manager.loc_mem_ctrl.port = system.policy_manager.loc_req_port
system.policy_manager.far_mem_ctrl.port = system.policy_manager.far_req_port

def createRandomTraffic(tgen):
    yield tgen.createRandom(10000000000,   # duration
                            0,                  # min_addr
                            AddrRange('16GiB').end,  # max_adr
                            64,                 # block_size
                            1000, # min_period
                            1000, # max_period
                            options.rd_prct,    # rd_perc
                            0)                  # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(100000000000,   # duration
                            0,                  # min_addr
                            AddrRange('16GiB').end,  # max_adr
                            64,                 # block_size
                            1000, # min_period
                            1000, # max_period
                            options.rd_prct,    # rd_perc
                            0)                  # data_limit
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


build/X86_MESI_Two_Level/gem5.opt --outdir=results_gapbs_small configs-gapbs-small/run_gapbs.py configs-gapbs-small/vmlinux-4.9.186 configs-gapbs-small/gapbs.img timing 8 MESI_Two_Level bc  1 22