/**
    Implementation of the Memory class.
*/
#include "memory.h"
#include <format>

Memory::Memory(uint32_t lower_end, uint32_t upper_start) : lower_end(lower_end), upper_start(upper_start) {
    // Initialize the lower and upper memory regions based on the provided parameters
    lower_memory.resize(lower_end, 0); // Fill with zeros
    upper_memory.resize(0xFFFFFFFF - upper_start + 1, 0); // Fill with zeros
}

uint8_t Memory::readByte(uint32_t address) {
    if (address < lower_memory.size()) {
        return lower_memory[address];
    } else if (address >= upper_start) {
        return upper_memory[address - upper_start];
    } else {
        // Address is out of bounds
        throw std::out_of_range(std::format("Memory read out of bounds: 0x{:08X}, lower_end=0x{:08X}, upper_start=0x{:08X}", address, lower_end, upper_start));
    }
}

void Memory::writeByte(uint32_t address, uint8_t value) {
    if (address < lower_memory.size()) {
        lower_memory[address] = value;
    } else if (address >= upper_start) {
        upper_memory[address - upper_start] = value;
    } else {
        // Address is out of bounds
        throw std::out_of_range(std::format("Memory write out of bounds: 0x{:08X}, lower_end=0x{:08X}, upper_start=0x{:08X}", address, lower_end, upper_start));
    }
}

void Memory::writeWord(uint32_t address, uint32_t value) {
    // Write a 32-bit word in little-endian format
    writeByte(address, value & 0xFF);
    writeByte(address + 1, (value >> 8) & 0xFF);
    writeByte(address + 2, (value >> 16) & 0xFF);
    writeByte(address + 3, (value >> 24) & 0xFF);
}

uint32_t Memory::readWord(uint32_t address) {
    // Read a 32-bit word in little-endian format
    uint32_t byte0 = readByte(address);
    uint32_t byte1 = readByte(address + 1);
    uint32_t byte2 = readByte(address + 2);
    uint32_t byte3 = readByte(address + 3);
    return (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
}