/**
    cli-main.cpp

    Command-line interface version of the AArch32 emulator.
    Runs a binary program to completion using stdin/stdout for console I/O.

    Usage: aarch32-cli <binary_file> [--r0 VALUE] [--r1 VALUE] ... [--r12 VALUE]
*/

#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "CPU.hpp"
#include "MemoryMappedIO.hpp"

// MMIO addresses for console I/O
const uint32_t CONSOLE_OUT_ADDR = 0x9000000;
const uint32_t CONSOLE_IN_ADDR = 0x9000004;
const uint32_t CONSOLE_STATUS_ADDR = 0x9000008;

/**
 * Load a binary file into memory at the specified address
 */
bool loadBinaryFile(MemoryMappedIO *memory, const std::string &filename,
                    uint32_t load_address = 0x00000000) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return false;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  // Read file into buffer
  std::vector<char> buffer(size);
  if (!file.read(buffer.data(), size)) {
    return false;
  }

  // Write to memory
  for (size_t i = 0; i < buffer.size(); i++) {
    memory->writeByte(load_address + i, static_cast<uint8_t>(buffer[i]));
  }

  return true;
}

/**
 * Load signed 32-bit integers from a text file and write them as words into
 * memory. Integers can be decimal or hex (0x prefix), separated by whitespace
 * and/or commas.
 */
bool loadIntListFile(MemoryMappedIO *memory, const std::string &filename,
                     uint32_t start_address, size_t &loaded_count) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open int list file: " << filename
              << std::endl;
    return false;
  }

  loaded_count = 0;
  std::string line;
  while (std::getline(file, line)) {
    for (char &ch : line) {
      if (ch == ',') {
        ch = ' ';
      }
    }

    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
      try {
        long long parsed = std::stoll(token, nullptr, 0);
        if (parsed < std::numeric_limits<int32_t>::min() ||
            parsed > std::numeric_limits<int32_t>::max()) {
          std::cerr << "Error: Integer out of 32-bit signed range in int list "
                       "file: "
                    << token << std::endl;
          return false;
        }

        if (loaded_count >
            (std::numeric_limits<uint32_t>::max() - start_address) / 4) {
          std::cerr << "Error: Address overflow while loading int list file"
                    << std::endl;
          return false;
        }

        uint32_t value = static_cast<uint32_t>(static_cast<int32_t>(parsed));
        memory->writeWord(start_address + static_cast<uint32_t>(loaded_count * 4),
                          value);
        loaded_count++;
      } catch (const std::exception &) {
        std::cerr << "Error: Invalid integer token in int list file: " << token
                  << std::endl;
        return false;
      }
    }
  }

  return true;
}

/**
 * Set up MMIO handlers for console I/O using stdin/stdout
 */
void setupConsoleIO(MemoryMappedIO *memory) {
  // Console output handler - write to stdout
  memory->setWriteHandlerB(
      CONSOLE_OUT_ADDR, [](uint32_t address, uint8_t value) {
        std::cout << static_cast<char>(value) << std::flush;
      });

  // Console input handler - read from stdin
  memory->setReadHandlerW(CONSOLE_IN_ADDR, [](uint32_t address) -> uint32_t {
    if (std::cin.eof()) {
      return -1;
    }
    char c;
    if (std::cin.get(c)) {
      return static_cast<uint8_t>(c);
    }
    return -1;
  });

  // Console status handler
  // Returns 1 if EOF/Bad, 0 if input available
  memory->setReadHandlerB(CONSOLE_STATUS_ADDR, [](uint32_t address) -> uint8_t {
    // Use peek() to check if we can read without consuming
    // peek() returns EOF when at end of stream
    std::cin.peek();
    return std::cin.eof() ? 0x01 : 0x00;
  });
}

/**
 * Dump the full CPU register state to stderr
 */
void dumpRegisterState(CPU *cpu) {
  std::cerr << std::endl;
  std::cerr << "=== Register State ===" << std::endl;

  // Print R0-R12 in groups of 4
  for (int i = 0; i < 13; i += 4) {
    std::cerr << "  ";
    for (int j = 0; j < 4 && (i + j) < 13; j++) {
      int reg = i + j;
      std::cerr << "R" << std::setw(2) << std::left << std::dec << reg << ": 0x" << std::hex
                << std::setw(8) << std::setfill('0') << std::right
                << cpu->getRegister(reg) << std::setfill(' ') << "  ";
    }
    std::cerr << std::endl;
  }

  // Print special registers
  std::cerr << "  SP : 0x" << std::hex << std::setw(8) << std::setfill('0')
            << cpu->getRegister(13) << "  ";
  std::cerr << "LR : 0x" << std::hex << std::setw(8) << std::setfill('0')
            << cpu->getRegister(14) << "  ";
  std::cerr << "PC : 0x" << std::hex << std::setw(8) << std::setfill('0')
            << cpu->getRegister(15) << std::endl;

  // Print CPSR with flags
  uint32_t cpsr = cpu->getCPSR();
  bool N = (cpsr >> 31) & 1;
  bool Z = (cpsr >> 30) & 1;
  bool C = (cpsr >> 29) & 1;
  bool V = (cpsr >> 28) & 1;

  std::cerr << "  CPSR: 0x" << std::hex << std::setw(8) << std::setfill('0')
            << cpsr << "  [" << (N ? "N" : "-") << (Z ? "Z" : "-")
            << (C ? "C" : "-") << (V ? "V" : "-") << "]" << std::endl;
  std::cerr << std::dec << std::setfill(' ');
}

void printUsage(const char *program_name) {
  std::cerr << "Usage: " << program_name
        << " <binary_file> [--r0 VALUE] ... [--r12 VALUE] [--int-file "
          "FILE --int-addr ADDRESS]"
            << std::endl;
  std::cerr << std::endl;
  std::cerr << "Options:" << std::endl;
  std::cerr << "  --r0 to --r12   Set initial register values (decimal or hex "
               "with 0x prefix)"
            << std::endl;
  std::cerr << "  --int-file FILE Text file containing ints (decimal/hex, "
          "whitespace/comma-separated)"
        << std::endl;
  std::cerr << "  --int-addr ADDR Start memory address to place ints as 32-bit "
          "little-endian words"
        << std::endl;
  std::cerr << std::endl;
  std::cerr << "MMIO Addresses:" << std::endl;
  std::cerr << "  0x9000000  Console output (write a byte to print to stdout)"
            << std::endl;
  std::cerr << "  0x9000004  Console input (read a byte from stdin)"
            << std::endl;
  std::cerr
      << "  0x9000008  Console status (bit 0: has input, bit 1: can output)"
      << std::endl;
  std::cerr << std::endl;
  std::cerr << "Examples:" << std::endl;
  std::cerr << "  " << program_name << " program.bin" << std::endl;
  std::cerr << "  " << program_name << " program.bin --r0 42 --r1 0xFF"
            << std::endl;
  std::cerr << "  " << program_name
            << " program.bin --int-file input.txt --int-addr 0x1000"
            << std::endl;
  std::cerr << "  echo \"Hello\" | " << program_name << " echo.bin"
            << std::endl;
}

int main(int argc, char *argv[]) {
  // Check for minimum arguments
  if (argc < 2) {
    printUsage(argv[0]);
    return 1;
  }

  // Parse arguments
  std::string binary_filename = argv[1];
  uint32_t initial_registers[13] = {0};
  bool has_initial_registers[13] = {false};
  std::string int_list_filename;
  uint32_t int_list_start_address = 0;
  bool has_int_file = false;
  bool has_int_addr = false;

  // Parse register flags
  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];

    if (arg.substr(0, 3) == "--r" && arg.length() >= 4) {
      std::string reg_num_str = arg.substr(3);
      try {
        int reg_num = std::stoi(reg_num_str);
        if (reg_num >= 0 && reg_num <= 12) {
          // Next argument should be the value
          if (i + 1 < argc) {
            i++;
            std::string value_str = argv[i];
            uint32_t value = std::stoul(value_str, nullptr, 0);
            initial_registers[reg_num] = value;
            has_initial_registers[reg_num] = true;
          } else {
            std::cerr << "Error: " << arg << " requires a value" << std::endl;
            return 1;
          }
        } else {
          std::cerr << "Error: Invalid register number: " << arg << std::endl;
          std::cerr << "Valid registers are --r0 through --r12" << std::endl;
          return 1;
        }
      } catch (...) {
        std::cerr << "Error: Invalid register flag: " << arg << std::endl;
        return 1;
      }
    } else if (arg == "--int-file") {
      if (i + 1 >= argc) {
        std::cerr << "Error: --int-file requires a file path" << std::endl;
        return 1;
      }
      i++;
      int_list_filename = argv[i];
      has_int_file = true;
    } else if (arg == "--int-addr") {
      if (i + 1 >= argc) {
        std::cerr << "Error: --int-addr requires an address value" << std::endl;
        return 1;
      }
      i++;
      try {
        int_list_start_address = std::stoul(argv[i], nullptr, 0);
        has_int_addr = true;
      } catch (...) {
        std::cerr << "Error: Invalid address value for --int-addr: " << argv[i]
                  << std::endl;
        return 1;
      }
    } else if (arg[0] == '-') {
      std::cerr << "Error: Unknown option: " << arg << std::endl;
      printUsage(argv[0]);
      return 1;
    }
  }

  if (has_int_file != has_int_addr) {
    std::cerr << "Error: --int-file and --int-addr must be provided together"
              << std::endl;
    return 1;
  }

  // Create memory and CPU
  auto memory = std::make_shared<MemoryMappedIO>();
  auto cpu = std::make_shared<CPU>(memory.get(), 0x00000000);

  // Set up console I/O
  setupConsoleIO(memory.get());

  // Load binary file
  if (!loadBinaryFile(memory.get(), binary_filename, 0x00000000)) {
    std::cerr << "Error: Failed to load binary file: " << binary_filename
              << std::endl;
    return 1;
  }

  if (has_int_file) {
    size_t loaded_count = 0;
    if (!loadIntListFile(memory.get(), int_list_filename, int_list_start_address,
                         loaded_count)) {
      return 1;
    }
    std::cerr << "Loaded " << loaded_count << " ints from " << int_list_filename
              << " at address 0x" << std::hex << int_list_start_address
              << std::dec << std::endl;
  }

  // Apply initial register values
  for (int i = 0; i < 13; i++) {
    if (has_initial_registers[i]) {
      cpu->setRegister(i, initial_registers[i]);
    }
  }

  // Run the program until it halts
  uint64_t instruction_count = 0;
  const uint64_t MAX_INSTRUCTIONS = 0xffffffff; // Prevent infinite loops

  try {
    while (!cpu->isHalted() && instruction_count < MAX_INSTRUCTIONS) {
      cpu->step();
      instruction_count++;
    }

    if (instruction_count >= MAX_INSTRUCTIONS) {
      std::cerr << std::endl;
      std::cerr << "Error: Maximum instruction count reached ("
                << MAX_INSTRUCTIONS << ")" << std::endl;
      std::cerr << "Program may be in an infinite loop" << std::endl;
      dumpRegisterState(cpu.get());
      return 2;
    }

    // Program completed successfully
    dumpRegisterState(cpu.get());
    return 0;

  } catch (const std::exception &e) {
    std::cerr << std::endl;
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Executed " << std::dec << instruction_count << " instructions"
              << std::endl;
    dumpRegisterState(cpu.get());
    return 1;
  }
}
