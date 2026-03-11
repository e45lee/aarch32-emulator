/**
    disasm-main.cpp

    Disassembler for AArch32 binary files.
    Reads a binary file and prints the disassembly to stdout.

    Usage: aarch32-disasm <binary_file>
*/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "Instruction.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <binary_file>" << std::endl;
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    std::cerr << "Error: Failed to open file: " << argv[1] << std::endl;
    return 1;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> buffer(size);
  if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    std::cerr << "Error: Failed to read file: " << argv[1] << std::endl;
    return 1;
  }

  // Disassemble 4 bytes at a time
  for (std::streamsize i = 0; i + 3 < size; i += 4) {
    uint32_t word = static_cast<uint32_t>(buffer[i]) |
                    (static_cast<uint32_t>(buffer[i + 1]) << 8) |
                    (static_cast<uint32_t>(buffer[i + 2]) << 16) |
                    (static_cast<uint32_t>(buffer[i + 3]) << 24);

    std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << i
              << "  " << std::setw(8) << word << "  "
              << std::dec << std::setfill(' ') << decodeInstruction(word)
              << "\n";
  }

  if (size % 4 != 0) {
    std::cerr << "Warning: file size is not a multiple of 4 bytes; "
              << (size % 4) << " trailing byte(s) ignored." << std::endl;
  }

  return 0;
}
