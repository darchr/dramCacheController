
#include "mem/dramcachepower.hh"

#include "base/intmath.hh"
#include "sim/core.hh"

DRAMDCPower::DRAMDCPower(const DRAMDCInterfaceParams &p, bool include_io) :
    powerlib(libDRAMPower(getMemSpec(p), include_io))
{
}

Data::MemArchitectureSpec
DRAMDCPower::getArchParams(const DRAMDCInterfaceParams &p)
{
    Data::MemArchitectureSpec archSpec;
    archSpec.burstLength = p.burst_length;
    archSpec.nbrOfBanks = p.banks_per_rank;
    // One DRAMDCPower instance per rank, hence set this to 1
    archSpec.nbrOfRanks = 1;
    archSpec.dataRate = p.beats_per_clock;
    // For now we can ignore the number of columns and rows as they
    // are not used in the power calculation.
    archSpec.nbrOfColumns = 0;
    archSpec.nbrOfRows = 0;
    archSpec.width = p.device_bus_width;
    archSpec.nbrOfBankGroups = p.bank_groups_per_rank;
    archSpec.dll = p.dll;
    archSpec.twoVoltageDomains = hasTwoVDD(p);
    // Keep this disabled for now until the model is firmed up.
    archSpec.termination = false;
    return archSpec;
}

Data::MemTimingSpec
DRAMDCPower::getTimingParams(const DRAMDCInterfaceParams &p)
{
    // Set the values that are used for power calculations and ignore
    // the ones only used by the controller functionality in DRAMDCPower

    // All DRAMDCPower timings are in clock cycles
    Data::MemTimingSpec timingSpec;
    timingSpec.RC = divCeil((p.tRAS + p.tRP), p.tCK);
    timingSpec.RCD = divCeil(p.tRCD, p.tCK);
    timingSpec.RL = divCeil(p.tCL, p.tCK);
    timingSpec.RP = divCeil(p.tRP, p.tCK);
    timingSpec.RFC = divCeil(p.tRFC, p.tCK);
    timingSpec.RAS = divCeil(p.tRAS, p.tCK);
    // Write latency is read latency - 1 cycle
    // Source: B.Jacob Memory Systems Cache, DRAM, Disk
    timingSpec.WL = timingSpec.RL - 1;
    timingSpec.DQSCK = 0; // ignore for now
    timingSpec.RTP = divCeil(p.tRTP, p.tCK);
    timingSpec.WR = divCeil(p.tWR, p.tCK);
    timingSpec.XP = divCeil(p.tXP, p.tCK);
    timingSpec.XPDLL = divCeil(p.tXPDLL, p.tCK);
    timingSpec.XS = divCeil(p.tXS, p.tCK);
    timingSpec.XSDLL = divCeil(p.tXSDLL, p.tCK);

    // Clock period in ns
    timingSpec.clkPeriod = (p.tCK / (double)(SimClock::Int::ns));
    assert(timingSpec.clkPeriod != 0);
    timingSpec.clkMhz = (1 / timingSpec.clkPeriod) * 1000;
    return timingSpec;
}

Data::MemPowerSpec
DRAMDCPower::getPowerParams(const DRAMDCInterfaceParams &p)
{
    // All DRAMDCPower currents are in mA
    Data::MemPowerSpec powerSpec;
    powerSpec.idd0 = p.IDD0 * 1000;
    powerSpec.idd02 = p.IDD02 * 1000;
    powerSpec.idd2p0 = p.IDD2P0 * 1000;
    powerSpec.idd2p02 = p.IDD2P02 * 1000;
    powerSpec.idd2p1 = p.IDD2P1 * 1000;
    powerSpec.idd2p12 = p.IDD2P12 * 1000;
    powerSpec.idd2n = p.IDD2N * 1000;
    powerSpec.idd2n2 = p.IDD2N2 * 1000;
    powerSpec.idd3p0 = p.IDD3P0 * 1000;
    powerSpec.idd3p02 = p.IDD3P02 * 1000;
    powerSpec.idd3p1 = p.IDD3P1 * 1000;
    powerSpec.idd3p12 = p.IDD3P12 * 1000;
    powerSpec.idd3n = p.IDD3N * 1000;
    powerSpec.idd3n2 = p.IDD3N2 * 1000;
    powerSpec.idd4r = p.IDD4R * 1000;
    powerSpec.idd4r2 = p.IDD4R2 * 1000;
    powerSpec.idd4w = p.IDD4W * 1000;
    powerSpec.idd4w2 = p.IDD4W2 * 1000;
    powerSpec.idd5 = p.IDD5 * 1000;
    powerSpec.idd52 = p.IDD52 * 1000;
    powerSpec.idd6 = p.IDD6 * 1000;
    powerSpec.idd62 = p.IDD62 * 1000;
    powerSpec.vdd = p.VDD;
    powerSpec.vdd2 = p.VDD2;
    return powerSpec;
}

Data::MemorySpecification
DRAMDCPower::getMemSpec(const DRAMDCInterfaceParams &p)
{
    Data::MemorySpecification memSpec;
    memSpec.memArchSpec = getArchParams(p);
    memSpec.memTimingSpec = getTimingParams(p);
    memSpec.memPowerSpec = getPowerParams(p);
    return memSpec;
}

bool
DRAMDCPower::hasTwoVDD(const DRAMDCInterfaceParams &p)
{
    return p.VDD2 == 0 ? false : true;
}

uint8_t
DRAMDCPower::getDataRate(const DRAMDCInterfaceParams &p)
{
    uint32_t burst_cycles = divCeil(p.tBURST_MAX, p.tCK);
    uint8_t data_rate = p.burst_length / burst_cycles;
    // 4 for GDDR5
    if (data_rate != 1 && data_rate != 2 && data_rate != 4 && data_rate != 8)
        fatal("Got unexpected data rate %d, should be 1 or 2 or 4 or 8\n");
    return data_rate;
}
