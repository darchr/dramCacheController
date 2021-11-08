
from m5.params import *
from m5.proxy import *
from m5.objects.DCMemInterface import DCMemInterface
from m5.objects.DRAMDCInterface import AddrMap

# The following interface aims to model byte-addressable NVM
# The most important system-level performance effects of a NVM
# are modeled without getting into too much detail of the media itself.
class NVMDCInterface(DCMemInterface):
    type = 'NVMDCInterface'
    cxx_header = "mem/dcmem_interface.hh"

    # NVM DIMM could have write buffer to offload writes
    # define buffer depth, which will limit the number of pending writes
    max_pending_writes = Param.Unsigned("1", "Max pending write commands")

    # NVM DIMM could have buffer to offload read commands
    # define buffer depth, which will limit the number of pending reads
    max_pending_reads = Param.Unsigned("1", "Max pending read commands")

    # timing behaviour and constraints - all in nanoseconds

    # define average latency for NVM media.  Latency defined uniquely
    # for read and writes as the media is typically not symmetric
    tREAD = Param.Latency("100ns", "Average NVM read latency")
    tWRITE = Param.Latency("200ns", "Average NVM write latency")
    tSEND = Param.Latency("15ns", "Access latency")

    two_cycle_rdwr = Param.Bool(False,
                     "Two cycles required to send read and write commands")

# NVM delays and device architecture defined to mimic PCM like memory.
# Can be configured with DDR4_2400 sharing the channel
class NVM_2400_1x64(NVMDCInterface):
    write_buffer_size = 128
    read_buffer_size = 64

    max_pending_writes = 128
    max_pending_reads = 64

    device_rowbuffer_size = '256B'

    # 8X capacity compared to DDR4 x4 DIMM with 8Gb devices
    device_size = '512GiB'
    # Mimic 64-bit media agnostic DIMM interface
    device_bus_width = 64
    devices_per_rank = 1
    ranks_per_channel = 1
    banks_per_rank = 16

    burst_length = 8

    two_cycle_rdwr = True

    # 1200 MHz
    tCK = '0.833ns'

    tREAD = '150ns'
    tWRITE = '500ns';
    tSEND = '14.16ns';
    tBURST = '3.332ns';

    # Default all bus turnaround and rank bus delay to 2 cycles
    # With DDR data bus, clock = 1200 MHz = 1.666 ns
    tWTR = '1.666ns';
    tRTW = '1.666ns';
    tCS = '1.666ns'

# NVM delays and device architecture defined to mimic PCM like memory.
# Can be configured with DDR4_2400 sharing the channel
# This is a hypothetical NVM interface (a scaled down version of NVM_2400_1x64)
# to be used with a single HBM channel (HBM_1000_4H_1x128)
# The clock frequency is reduced to 150MHz, and thus the other timing params
# as well
class NVM_300_1x64(NVMDCInterface):
    write_buffer_size = 128
    read_buffer_size = 64

    max_pending_writes = 128
    max_pending_reads = 64

    device_rowbuffer_size = '256B'

    # 8X capacity compared to DDR4 x4 DIMM with 8Gb devices
    device_size = '64GiB'
    # Mimic 64-bit media agnostic DIMM interface
    device_bus_width = 64
    devices_per_rank = 1
    ranks_per_channel = 1
    banks_per_rank = 16

    burst_length = 8

    two_cycle_rdwr = True

    # 300 MHz
    tCK = '6.664ns'

    tREAD = '1200ns'
    tWRITE = '4000ns';
    tSEND = '113.28ns';
    tBURST = '26.56ns';

    # Default all bus turnaround and rank bus delay to 2 cycles
    # With DDR data bus, clock = 1200 MHz = 1.666 ns
    tWTR = '13.328ns';
    tRTW = '13.328ns';
    tCS = '13.328ns'