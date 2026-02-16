/**
    memory.h

    Headers for the memory subsystem of the AArch32 emulator.
    This includes:
        - a basic Memory class that provides read/write access to a byte-addressable memory space.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <vector>

class Memory {
private:
    /**
        Memory is organized into two byte-addressable regions:
        - Lower region: 0x00000000 to lower_end - 1
        - Upper region: upper_start to 0xFFFFFFFF

        By default, the lower region is 16 MiB (0x00000000 to 0x00FFFFFF) 
        and the upper region is 16 MiB (0xFF000000 to 0xFFFFFFFF).
    */
    std::vector<uint8_t> lower_memory;
    std::vector<uint8_t> upper_memory;
    uint32_t upper_start;
    uint32_t lower_end;
public:
    Memory(uint32_t lower_end = 0x0100000, uint32_t upper_start = 0xFF000000);

    virtual uint8_t readByte(uint32_t address);
    virtual void writeByte(uint32_t address, uint8_t value);
};

#endif