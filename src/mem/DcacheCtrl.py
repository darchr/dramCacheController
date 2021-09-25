from m5.params import *
from m5.proxy import *
from m5.objects.QoSMemCtrl import *

# Enum for memory scheduling algorithms, currently First-Come
# First-Served and a First-Row Hit then First-Come First-Served
class MemSched(Enum): vals = ['fcfs', 'frfcfs']

# MemCtrl is a single-channel single-ported Memory controller model
# that aims to model the most important system-level performance
# effects of a memory controller, interfacing with media specific
# interfaces
class DcacheCtrl(QoSMemCtrl):
    type = 'DcacheCtrl'
    cxx_header = "mem/dcache_ctrl.hh"

    # single-ported on the system interface side, instantiate with a
    # bus in front of the controller for multiple ports
    port = ResponsePort("This port responds to memory requests")

    # Interface to volatile, DRAM media
    dram = Param.DRAMDCInterface(NULL, "DRAM interface")

    # Interface to non-volatile media
    nvm = Param.NVMDCInterface(NULL, "NVM interface")

    # JASON: let's get this size from the interface
    dram_cache_size = Param.MemorySize('1024MiB', "DRAM cache size")
    block_size = Param.Unsigned('64', "DRAM cache block size in bytes")
    orb_max_size = Param.Unsigned(256, "Outstanding Requests Buffer size")
    crb_max_size = Param.Unsigned(64, "Conflicting Requests Buffer size")

    # JASON: We need to think about this a bit
    # The dram interface is a abstract memory, but we don't need the backing
    # store. So, null should be true, in_addr_map should be false,
    # kvm_map false, and conf_table_reported false

    # read and write buffer depths are set in the interface
    # the controller will read these values when instantiated

    # threshold in percent for when to forcefully trigger writes and
    # start emptying the write buffer
    write_high_thresh_perc = Param.Percent(85, "Threshold to force writes")

    # threshold in percentage for when to start writes if the read
    # queue is empty
    write_low_thresh_perc = Param.Percent(50, "Threshold to start writes")

    # minimum write bursts to schedule before switching back to reads
    min_writes_per_switch = Param.Unsigned(16, "Minimum write bursts before "
                                           "switching to reads")

    # scheduler, address map and page policy
    mem_sched_policy = Param.MemSched('frfcfs', "Memory scheduling policy")

    # pipeline latency of the controller and PHY, split into a
    # frontend part and a backend part, with reads and writes serviced
    # by the queues only seeing the frontend contribution, and reads
    # serviced by the memory seeing the sum of the two
    static_frontend_latency = Param.Latency("10ns", "Static frontend latency")
    static_backend_latency = Param.Latency("10ns", "Static backend latency")

    command_window = Param.Latency("10ns", "Static backend latency")
