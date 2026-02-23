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

void MemoryMappedIO::setReadHandlerB(uint32_t address, ReadHandlerB handler) {
    read_handlersB[address] = handler;
}

void MemoryMappedIO::setWriteHandlerB(uint32_t address, WriteHandlerB handler) {
    write_handlersB[address] = handler;
}

void MemoryMappedIO::setReadHandlerW(uint32_t address, ReadHandlerW handler) {
    read_handlersW[address] = handler;
}

void MemoryMappedIO::setWriteHandlerW(uint32_t address, WriteHandlerW handler) {
    write_handlersW[address] = handler;
}

uint8_t MemoryMappedIO::readByte(uint32_t address) {
    if (read_handlersB.find(address) != read_handlersB.end()) {
        return read_handlersB[address](address);
    } else {
        return Memory::readByte(address);
    }
}

void MemoryMappedIO::writeByte(uint32_t address, uint8_t value) {
    if (write_handlersB.find(address) != write_handlersB.end()) {
        write_handlersB[address](address, value);
    } else {
        Memory::writeByte(address, value);
    }
}

void MemoryMappedIO::writeWord(uint32_t address, uint32_t value) {
    if (write_handlersW.find(address) != write_handlersW.end()) {
        write_handlersW[address](address, value);
    } else {
        Memory::writeWord(address, value);
    }
}

uint32_t MemoryMappedIO::readWord(uint32_t address) {
    if (read_handlersW.find(address) != read_handlersW.end()) {
        return read_handlersW[address](address);
    } else {
        return Memory::readWord(address);
    }
}