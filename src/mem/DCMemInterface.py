from m5.params import *
from m5.proxy import *

from m5.objects.AbstractMemory import AbstractMemory

# Enum for the address mapping. With Ch, Ra, Ba, Ro and Co denoting
# channel, rank, bank, row and column, respectively, and going from
# MSB to LSB.  Available are RoRaBaChCo and RoRaBaCoCh, that are
# suitable for an open-page policy, optimising for sequential accesses
# hitting in the open row. For a closed-page policy, RoCoRaBaCh
# maximises parallelism.
class AddrMap(Enum): vals = ['RoRaBaChCo', 'RoRaBaCoCh', 'RoCoRaBaCh']

class DCMemInterface(AbstractMemory):
    type = 'DCMemInterface'
    abstract = True
    cxx_header = "mem/dcmem_interface.hh"

    # Allow the interface to set required controller buffer sizes
    # each entry corresponds to a burst for the specific memory channel
    # configuration (e.g. x32 with burst length 8 is 32 bytes) and not
    # the cacheline size or request/packet size
    write_buffer_size = Param.Unsigned(64, "Number of write queue entries")
    read_buffer_size = Param.Unsigned(32, "Number of read queue entries")

    # scheduler, address map
    addr_mapping = Param.AddrMap('RoRaBaCoCh', "Address mapping policy")

    # size of memory device in Bytes
    device_size = Param.MemorySize("Size of memory device")
    # the physical organisation of the memory
    device_bus_width = Param.Unsigned("data bus width in bits for each "\
                                      "memory device/chip")
    burst_length = Param.Unsigned("Burst lenght (BL) in beats")
    device_rowbuffer_size = Param.MemorySize("Page (row buffer) size per "\
                                           "device/chip")
    devices_per_rank = Param.Unsigned("Number of devices/chips per rank")
    ranks_per_channel = Param.Unsigned("Number of ranks per channel")
    banks_per_rank = Param.Unsigned("Number of banks per rank")

    # timing behaviour and constraints - all in nanoseconds

    # the base clock period of the memory
    tCK = Param.Latency("Clock period")

    # time to complete a burst transfer, typically the burst length
    # divided by two due to the DDR bus, but by making it a parameter
    # it is easier to also evaluate SDR memories like WideIO and new
    # interfaces, emerging technologies.
    # This parameter has to account for burst length.
    # Read/Write requests with data size larger than one full burst are broken
    # down into multiple requests in the controller
    tBURST = Param.Latency("Burst duration "
                           "(typically burst length / 2 cycles)")

    # write-to-read, same rank turnaround penalty
    tWTR = Param.Latency("Write to read, same rank switching time")

    # read-to-write, same rank turnaround penalty
    tRTW = Param.Latency("Read to write, same rank switching time")

    # rank-to-rank bus delay penalty
    # this does not correlate to a memory timing parameter and encompasses:
    # 1) RD-to-RD, 2) WR-to-WR, 3) RD-to-WR, and 4) WR-to-RD
    # different rank bus delay
    tCS = Param.Latency("Rank to rank switching time")
