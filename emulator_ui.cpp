/**
    emulator_ui.cpp

    Implementation of UI helper functions for the emulator, including
    disassembly view, register view, stack view, and console I/O.
*/

#include "emulator_ui.h"
#include "instruction.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

std::string formatRegister(const std::string& name, uint32_t value) {
    std::ostringstream oss;
    oss << name << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return oss.str();
}

std::vector<std::string> getDisassembly(EmulatorState& state, int lines_before, int lines_after) {
    std::vector<std::string> result;
    uint32_t pc = state.cpu->getRegister(15);

    if (pc == halted_pc) {
        result.push_back(">>> HALTED <<<");
        return result;
    }

    // Calculate start address (aligned to 4 bytes)
    uint32_t start_addr = pc - (lines_before * 4);
    start_addr &= ~3; // Align to 4 bytes

    for (int i = -lines_before; i <= lines_after; i++) {
        uint32_t addr = pc + (i * 4);
        try {
            uint32_t raw_instr = state.memory->readByte(addr) |
                                (state.memory->readByte(addr + 1) << 8) |
                                (state.memory->readByte(addr + 2) << 16) |
                                (state.memory->readByte(addr + 3) << 24);

            std::ostringstream oss;
            if (i == 0) {
                oss << ">>> ";
            } else {
                oss << "    ";
            }
            oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr << ": ";
            oss << decodeInstruction(raw_instr);
            result.push_back(oss.str());
        } catch (...) {
            std::ostringstream oss;
            if (i == 0) {
                oss << ">>> ";
            } else {
                oss << "    ";
            }
            oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr << ": <invalid>";
            result.push_back(oss.str());
        }
    }

    return result;
}

std::vector<std::string> getStackView(EmulatorState& state, int num_entries) {
    std::vector<std::string> result;
    uint32_t sp = state.cpu->getRegister(13); // R13 is the stack pointer

    result.push_back("Stack (from SP):");

    for (int i = 0; i < num_entries; i++) {
        uint32_t addr = sp + (i * 4);
        try {
            uint32_t value = state.memory->readByte(addr) |
                           (state.memory->readByte(addr + 1) << 8) |
                           (state.memory->readByte(addr + 2) << 16) |
                           (state.memory->readByte(addr + 3) << 24);

            std::ostringstream oss;
            if (i == 0) {
                oss << "SP> ";
            } else {
                oss << "    ";
            }
            oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr << ": ";
            oss << "0x" << std::setw(8) << std::setfill('0') << value;
            result.push_back(oss.str());
        } catch (...) {
            std::ostringstream oss;
            if (i == 0) {
                oss << "SP> ";
            } else {
                oss << "    ";
            }
            oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr << ": <invalid>";
            result.push_back(oss.str());
        }
    }

    return result;
}

std::vector<std::string> getRegisterView(EmulatorState& state) {
    std::vector<std::string> result;

    // Display general-purpose registers in columns
    for (int i = 0; i < 13; i++) {
        std::ostringstream oss;
        oss << "R" << std::setw(2) << std::setfill(' ') << std::dec << i << ": 0x"
            << std::hex << std::setw(8) << std::setfill('0') << state.cpu->getRegister(i);
        result.push_back(oss.str());
    }

    // Stack pointer
    result.push_back(formatRegister("SP ", state.cpu->getRegister(13)));

    // Link register
    result.push_back(formatRegister("LR ", state.cpu->getRegister(14)));

    // Program counter
    result.push_back(formatRegister("PC ", state.cpu->getRegister(15)));

    // CPSR with flags
    uint32_t cpsr = state.cpu->getCPSR();
    std::ostringstream cpsr_oss;
    cpsr_oss << "CPSR: 0x" << std::hex << std::setw(8) << std::setfill('0') << cpsr;
    result.push_back(cpsr_oss.str());

    // Flags
    bool N = (cpsr >> 31) & 1;
    bool Z = (cpsr >> 30) & 1;
    bool C = (cpsr >> 29) & 1;
    bool V = (cpsr >> 28) & 1;

    std::ostringstream flags_oss;
    flags_oss << "Flags: "
              << (N ? "N" : "-") << " "
              << (Z ? "Z" : "-") << " "
              << (C ? "C" : "-") << " "
              << (V ? "V" : "-");
    result.push_back(flags_oss.str());

    return result;
}

void setupConsoleIO(EmulatorState& state) {
    // Console output handler
    state.memory->setWriteHandler(CONSOLE_OUT_ADDR, [&state](uint32_t addr, uint8_t value) {
        if (value == '\n') {
            state.console_output.push_back("");
        } else if (!state.console_output.empty()) {
            state.console_output.back() += static_cast<char>(value);
        } else {
            state.console_output.push_back(std::string(1, static_cast<char>(value)));
        }

        // Limit console output lines
        while (state.console_output.size() > static_cast<size_t>(state.max_console_lines)) {
            state.console_output.pop_front();
        }
    });

    // Console input handler
    state.memory->setReadHandler(CONSOLE_IN_ADDR, [&state](uint32_t addr) -> uint8_t {
        if (!state.console_input.empty()) {
            char c = state.console_input.front();
            state.console_input.pop_front();
            return static_cast<uint8_t>(c);
        }
        return 0;
    });

    // Console status handler
    state.memory->setReadHandler(CONSOLE_STATUS_ADDR, [&state](uint32_t addr) -> uint8_t {
        uint8_t status = 0;
        if (!state.console_input.empty()) {
            status |= 0x01; // Has input
        }
        status |= 0x02; // Can always output
        return status;
    });
}

void loadTestProgram(EmulatorState& state) {
    // Load a simple test program (just NOPs for now)
    for (int i = 0; i < 10; i++) {
        // NOP instruction (MOV R0, R0) = 0xE1A00000
        state.memory->writeByte(i * 4 + 0, 0x00);
        state.memory->writeByte(i * 4 + 1, 0x00);
        state.memory->writeByte(i * 4 + 2, 0xA0);
        state.memory->writeByte(i * 4 + 3, 0xE1);
    }

    // Add a halt at the end (MOV PC, LR where LR = halted_pc)
    // MOV PC, LR = 0xE1A0F00E
    state.memory->writeByte(40, 0x0E);
    state.memory->writeByte(41, 0xF0);
    state.memory->writeByte(42, 0xA0);
    state.memory->writeByte(43, 0xE1);
}

std::vector<std::string> getMemoryView(EmulatorState& state, uint32_t address, int bytes_before, int bytes_after) {
    std::vector<std::string> result;

    // Align address to 16-byte boundary for cleaner display
    uint32_t start_addr = (address - bytes_before) & ~0xF;
    uint32_t end_addr = address + bytes_after;

    result.push_back("Memory View (16 bytes per line):");

    /* Wrap around  */
    for (uint32_t addr = start_addr; addr != end_addr; addr += 16) {
        std::ostringstream oss;

        // Address column
        if (addr <= address && address < addr + 16) {
            oss << ">>> ";
        } else {
            oss << "    ";
        }
        oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr << ": ";

        // Hex bytes
        std::string ascii_repr = "";
        for (int i = 0; i < 16; i++) {
            uint32_t byte_addr = addr + i;
            try {
                uint8_t byte = 0;

                try {
                    byte = state.memory->readByte(byte_addr);
                } catch (std::out_of_range&) {
                    byte = 0; // Treat out-of-range as zero for display
                }

                oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";

                // Build ASCII representation
                if (byte >= 32 && byte <= 126) {
                    ascii_repr += static_cast<char>(byte);
                } else {
                    ascii_repr += '.';
                }
            } catch (...) {
                oss << "?? ";
                ascii_repr += '?';
            }
        }

        // Add ASCII representation
        oss << " | " << ascii_repr;
        result.push_back(oss.str());
    }

    return result;
}

bool loadBinaryFile(EmulatorState& state, const std::string& filename, uint32_t load_address) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return false;
    }

    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0) {
        std::cerr << "Error: File is empty or invalid: " << filename << std::endl;
        return false;
    }

    // Read file into buffer
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "Error: Failed to read file: " << filename << std::endl;
        return false;
    }

    // Load buffer into memory
    for (size_t i = 0; i < buffer.size(); i++) {
        try {
            state.memory->writeByte(load_address + i, buffer[i]);
        } catch (const std::exception& e) {
            std::cerr << "Error: Failed to write to memory at address 0x"
                      << std::hex << (load_address + i) << ": " << e.what() << std::endl;
            return false;
        }
    }

    std::cout << "Loaded " << std::dec << size << " bytes from " << filename
              << " at address 0x" << std::hex << load_address << std::endl;

    return true;
}

void applyInitialRegisters(EmulatorState& state) {
    for (int i = 0; i < 13; i++) {
        if (state.has_initial_registers[i]) {
            state.cpu->setRegister(i, state.initial_registers[i]);
        }
    }
}
