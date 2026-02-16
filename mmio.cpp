/*
    mmio.h

    Headers for the MMIO subsystem of the AArch32 emulator.
     This includes:
        - a basic MMIO class that provides read/write access to a byte-addressable memory space.
            that can be overriden with custom read/write handlers for specific address ranges (e.g., for memory-mapped I/O devices).
*/
#include "memory.h"
#include "mmio.h"

MemoryMappedIO::MemoryMappedIO(uint32_t lower_end, uint32_t upper_start) : Memory(lower_end, upper_start) {}

void MemoryMappedIO::setReadHandler(uint32_t address, ReadHandler handler) {
    read_handlers[address] = handler;
}

void MemoryMappedIO::setWriteHandler(uint32_t address, WriteHandler handler) {
    write_handlers[address] = handler;
}

uint8_t MemoryMappedIO::readByte(uint32_t address) {
    if (read_handlers.find(address) != read_handlers.end()) {
        return read_handlers[address](address);
    } else {
        return Memory::readByte(address);
    }
}

void MemoryMappedIO::writeByte(uint32_t address, uint8_t value) {
    if (write_handlers.find(address) != write_handlers.end()) {
        write_handlers[address](address, value);
    } else {
        Memory::writeByte(address, value);
    }
}
