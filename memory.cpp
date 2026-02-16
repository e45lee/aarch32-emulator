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