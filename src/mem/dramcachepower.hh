
/**
 * @file
 * DRAMDCPower declaration
 */

#ifndef __DRAM_CACHE_POWER_HH__
#define __DRAM_CACHE_POWER_HH__

#include "libdrampower/LibDRAMPower.h"
#include "params/DRAMDCInterface.hh"

/**
 * DRAMDCPower is a standalone tool which calculates the power consumed by a
 * DRAM in the system. This class wraps the DRAMPower library.
 */
class DRAMDCPower
{

 private:

    /**
     * Transform the architechture parameters defined in
     * DRAMInterfaceParams to the memSpec of DRAMDCPower
     */
    static Data::MemArchitectureSpec getArchParams(
                                     const DRAMDCInterfaceParams &p);

    /**
     * Transforms the timing parameters defined in DRAMInterfaceParams to
     * the memSpec of DRAMPower
     */
    static Data::MemTimingSpec getTimingParams(const DRAMDCInterfaceParams &p);

    /**
     * Transforms the power and current parameters defined in
     * DRAMInterfaceParams to the memSpec of DRAMDCPower
     */
    static Data::MemPowerSpec getPowerParams(const DRAMDCInterfaceParams &p);

    /**
     * Determine data rate, either one or two.
     */
    static uint8_t getDataRate(const DRAMDCInterfaceParams &p);

    /**
     * Determine if DRAM has two voltage domains (or one)
     */
    static bool hasTwoVDD(const DRAMDCInterfaceParams &p);

    /**
     * Return an instance of MemSpec based on the DRAMDCInterfaceParams
     */
    static Data::MemorySpecification
    getMemSpec(const DRAMDCInterfaceParams &p);

 public:

    // Instance of DRAMDCPower Library
    libDRAMPower powerlib;

    DRAMDCPower(const DRAMDCInterfaceParams &p, bool include_io);

};

#endif //__DRAM_CACHE_POWER_HH__

