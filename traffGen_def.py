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

system.mem_ctrl = PolicyManager(range=AddrRange('1GiB'))
system.mem_ctrl.bypass_dcache = True
system.mem_ctrl.tRP = '14.16ns'
system.mem_ctrl.tRCD_RD = '14.16ns'
system.mem_ctrl.tRL = '14.16ns'

system.loc_mem_ctrl = MemCtrl()
system.loc_mem_ctrl.dram =  DDR4_2400_16x4_Alloy(range=AddrRange('1GiB'),in_addr_map=False, null=True)

system.far_mem_ctrl = MemCtrl()
system.far_mem_ctrl.dram = DDR4_2400_16x4(range=AddrRange('1GiB'),in_addr_map=False, null=True)

system.mem_ctrl.always_hit = False
system.mem_ctrl.always_dirty = True

system.mem_ctrl.dram_cache_size = "64MiB"

system.generator.port = system.mem_ctrl.port
system.loc_mem_ctrl.port = system.mem_ctrl.loc_req_port
system.far_mem_ctrl.port = system.mem_ctrl.far_req_port

def createRandomTraffic(tgen):
    yield tgen.createRandom(100000000,            # duration
                            0,                      # min_addr
                            AddrRange('1GiB').end,  # max_adr
                            64,                     # block_size
                            1000,                   # min_period
                            1000,                   # max_period
                            options.rd_prct,        # rd_perc
                            0)                      # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(2000000000,            # duration
                            0,                      # min_addr
                            AddrRange('1GiB').end,  # max_adr
                            64,                     # block_size
                            1000,                   # min_period
                            1000,                   # max_period
                            options.rd_prct,        # rd_perc
                            0)                      # data_limit
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
print(f"Exit reason {exit_event.getCause()}")

# for testing checkpointing
# exit_event = m5.simulate(1000000000)
# print(f"Exit reason {exit_event.getCause()}")

# # print("Draining")
# # m5.drain()
# # print("Done draining!")
# m5.stats.dump()
# m5.checkpoint(m5.options.outdir + '/cpt-test')
# m5.stats.reset()

# system.generator.start(createLinearTraffic(system.generator))
# exit_event = m5.simulate()
# print(f"Exit reason {exit_event.getCause()}")
